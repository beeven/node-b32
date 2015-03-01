#include <node.h>
#include <node_buffer.h>
#include <stdio.h>
#include <uv.h>

using namespace v8;
using namespace node;

const char* b32table = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

int base32_encode(const char* data, size_t length, char *buf, size_t bufSize) {
	if(length < 0 || length > (1 << 28)) {
		return -1;
	}
	int count = 0;
	if(length > 0) {
		int buffer = data[0];
		int next = 1;
		int bitsLeft = 8;
		while(count < bufSize && (bitsLeft > 0 || next < length)) {
			if(bitsLeft < 5) {
				if(next < length) {
					buffer <<= 8;
					buffer |= data[next++] & 0xff;
					bitsLeft += 8;
				} else {
					int pad = 5 - bitsLeft;
					buffer <<= pad;
					bitsLeft += pad;
				}
			}
			int index = 0x1f & (buffer >> (bitsLeft - 5));
			bitsLeft -= 5;
			buf[count++] = b32table[index];
		}
	}
	if(count < bufSize) {
		buf[count] = '\000';
	}
	return count;
}

int base32_decode(const char* encoded, char *buf, size_t bufSize) {
	int buffer = 0;
	int bitsLeft = 0;
	int count = 0;
	for(const char *ptr = encoded; count < bufSize && *ptr && *ptr!='='; ++ptr) {
		char ch = *ptr;
		if(ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n' || ch == '-') {
			continue;
		}
		
		buffer <<= 5;

		if(ch == '0') {
			ch = 'O';
		} else if (ch == '1') {
			ch = 'L';
		} else if (ch == '8') {
			ch = 'B';
		}

		if((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z')) { // 0x4? 0x6?
			ch = (ch & 0x1F) - 1;
		} else if ( ch >= '2' && ch <= '7') { // 0x3?
			ch -= '2' - 26; // ch = ch - '2' + 26;
		} else {
			return -1;
		}

		buffer |= ch;
		bitsLeft += 5;
		if(bitsLeft >= 8) {
			buf[count++] = buffer >> (bitsLeft - 8);
			bitsLeft -= 8;
		}
	}
	if(count < bufSize) {
		buf[count] = '\000';
	}
	return count;
}

class B32Async: public NanAsyncWorker {
public:
	int method;
	const char* input;
	const size_t input_size;
	char* result;
	size_t result_size;

	B32Async(NanCallback *callback, int method,const char* input, const size_t input_size):NanAsyncWorker(callback),
		method(method),input(input), input_size(input_size) {
	}

	~B32Async() {
		delete result;
	}

	void Execute() {
		size_t bufSize;
		if(method == 1) { // encode
			bufSize  = (input_size * 8 - 1)/5+1;
			result = new char[input_size];
			if((result_size=base32_encode(input,input_size,result,bufSize)) == -1) {
				SetErrorMessage("encode error");
			}
			
		} else { // decode
			size_t size = strlen(input);
			bufSize = (size * 5 )/8;
			result = new char[bufSize];
			if((result_size = base32_decode(input,result,bufSize)) == -1) {
				SetErrorMessage("decode error");
			}
		}
		//result_size = bufSize;
	}

	void HandleOKCallback() {
		NanScope();
		Handle<Value> argv[] = {
				NanNull(),
				NanBufferUse(result,result_size)
			};
		callback->Call(2,argv);
	}
};

void encodeSync(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);

	if(args.Length() < 1) {
		isolate->ThrowException(Exception::TypeError(
			String::NewFromUtf8(isolate,"Wrong number of arguments")));
		return;
	}

	Local<Value> arg = args[0];
	if(!Buffer::HasInstance(arg)) {
		isolate->ThrowException(Exception::TypeError(
			String::NewFromUtf8(isolate,"Argument 1 must be a buffer")));
		return;
	}
	size_t size = Buffer::Length(arg->ToObject());
	char* buf = Buffer::Data(arg->ToObject());

	size_t bufSize = (size * 8 - 1)/5+1;
	char *result = new char[bufSize];
	int result_size = 0;
	if((result_size = base32_encode(buf,size,result,bufSize))< 0) {
		isolate->ThrowException(Exception::TypeError(
			String::NewFromUtf8(isolate,"Encode error")));
		return;
	} else {
		args.GetRetrunValue().Set(Buffer::Use(isolate,result,result_size));
	}

}

void decodeSync(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);

	if(args.Length() < 1) {
		isolate->ThrowException(Exception::TypeError(
			String::NewFromUtf8(isolate,"Wrong number of arguments")));
		return;
	}

	Local<Value> arg = args[0];
	if(!Buffer::HasInstance(arg)) {
		isolate->ThrowException(Exception::TypeError(
			String::NewFromUtf8(isolate,"Argument 1 must be a buffer")));
		return;
	}
	
	size_t size = Buffer::Length(arg->ToObject());
	char* buf = Buffer::Data(arg->ToObject());

	size_t bufSize = (size * 5 )/8;
	char *result = new char[bufSize];
	int result_size = 0;
	if((result_size = base32_decode(buf,result,bufSize)) < 0) {
		isolate->ThrowException(Exception::TypeError(
			String::NewFromUtf8(isolate,"Decode error")));
		return;
	} else {
		args.GetRetrunValue().Set(Buffer::Use(isolate,result,result_size));
	}

}

typedef struct {
	Local<Function>& callback,
	const char* data,
	size_t length,
	char* buf,
	size_t buf_size,
	bool decoding
} b32_context_t;


void do_encode_decode(uv_work_t *req) {
	b32_context_t* context = (b32_context_t*)req->data;
	if(context->decoding) {
		base32_decode(context->data,context->buf,context->buf_size);
	}
	else {
		base32_encode(context->data, context->length,context->buf, context->buf_size);
	}
}

void after_encode_decode(uv_work_t *req) {
	b32_context_t* context = (b32_context_t*)req->data;

}

void encode(const FunctionCallbackInfo<Value>& args) {
	Isolate isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);

	if(args.Length() < 2) {
		isolate->ThrowException(Exception::TypeError(
			String::NewFromUtf8(isolate,"Wrong number of arguments")));
		return;
	}
	Local<Value> arg = args[0];
	if(!Buffer::HasInstance(arg)) {
		isolate->ThrowException(Exception::TypeError(
			String::NewFromUtf8(isolate,"Argument 1 must be a buffer")));
		return;
	}
	if(!args[1]->IsFunction()){
		isolate->ThrowException(Exception::TypeError(
			String::NewFromUtf8(isolate,"Argument 2 must be a function")));
		return;
	}
	Local<Function> cb = args[1].As<Function>();

	uv_work_t work_req;
	b32_context_t context;
	context.callback = cb;
	context.data = Buffer::Data(arg->ToObject());
	context.length = Buffer::Length(arg->ToObject());
	context.buf_size  = (context.length * 8 - 1)/5+1;
	context.buf = new char[context.buf_size];
	context.decoding = false;
	work_req.data = (void *) &context;

	uv_queue_work(uv_default_loop(),&work_req,do_encode_decode,after_encode_decode);

	return;

	/*
	NanCallback *callback = new NanCallback(cb);
	size_t size = Buffer::Length(arg->ToObject());
	char* buf = Buffer::Data(arg->ToObject());
	

	B32Async* job = new B32Async(callback,1,buf,size);
	NanAsyncQueueWorker(job);
	NanReturnUndefined();
	*/
}

NAN_METHOD(decode) {
	NanScope();
	if(args.Length() < 1) {
		NanThrowTypeError("Wrong number of arguments");
		NanReturnUndefined();
	}
	Local<Value> arg = args[0];
	if(!Buffer::HasInstance(arg)) {
		NanThrowTypeError("argument 1 must be a buffer");
		NanReturnUndefined();
	}
	if(!args[1]->IsFunction()){
		NanThrowTypeError("argument 2 must be a function");
		NanReturnUndefined();
	}
	Local<Function> cb = args[1].As<Function>();
	NanCallback *callback = new NanCallback(cb);
	size_t size = Buffer::Length(arg->ToObject());
	char* buf = Buffer::Data(arg->ToObject());

	B32Async* job = new B32Async(callback,2,buf,size);
	NanAsyncQueueWorker(job);
	NanReturnUndefined();
}


void init(Handle<Object> exports, Handle<Object> module) {
	exports->Set(NanNew("encodeSync"), NanNew<FunctionTemplate>(encodeSync)->GetFunction());
	exports->Set(NanNew("decodeSync"),NanNew<FunctionTemplate>(decodeSync)->GetFunction());
	exports->Set(NanNew("encode"),NanNew<FunctionTemplate>(encode)->GetFunction());
	exports->Set(NanNew("decode"),NanNew<FunctionTemplate>(decode)->GetFunction());
}

NODE_MODULE(b32, init);
