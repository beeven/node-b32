#include <node.h>
#include <node_buffer.h>
#include <stdio.h>
#include <uv.h>

using namespace v8;
using namespace node;

const char* b32table = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

int base32_encode(const char* data, size_t length, char *buf, size_t bufSize) {
	if(length > (1 << 28)) {
		return -1;
	}
	unsigned int count = 0;
	if(length > 0) {
		int buffer = data[0];
		unsigned int next = 1;
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
	unsigned int count = 0;
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
		args.GetReturnValue().Set(Buffer::Use(isolate,result,result_size));
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
		args.GetReturnValue().Set(Buffer::Use(isolate,result,result_size));
	}

}

typedef struct {
	Local<Function> *callback;
	char *data;
	size_t length;
	bool decoding;
} b32_context_t;


void do_encode_decode(uv_work_t *req) {
	fprintf(stderr,"Working...\n");

	Isolate* isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);

	b32_context_t* context = (b32_context_t*)req->data;

	fprintf(stderr, "Working with: %s",context->data);
	size_t buf_size;
	char *buf;
	int ret;
	if(context->decoding) {
		buf_size = (context->length * 5 )/8;
		buf = new char[buf_size];
		ret = base32_decode(context->data, buf, buf_size);
	}
	else {
		buf_size = (context->length * 8 - 1)/5+1;
		buf = new char[buf_size];
		ret = base32_encode(context->data, context->length, buf, buf_size);
	}

	fprintf(stderr,"Done calculating...\n");

	const unsigned argc = 2;
	Local<Value> argv[argc];

	if(ret >= 0) {
		argv[0] = Null(isolate);
		argv[1] = Buffer::Use(buf,buf_size);
	}
	else {
		argv[0] = String::NewFromUtf8(isolate,"Encode error");
		argv[1] = Null(isolate);
	}
	(* context->callback)->Call(isolate->GetCurrentContext()->Global(),argc,argv);

}

void after_encode_decode(uv_work_t *req, int status) {
	fprintf(stderr,"Finish working...\n");
}

void encode_decode(const FunctionCallbackInfo<Value>& args, bool decoding = false) {
	Isolate* isolate = Isolate::GetCurrent();
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
	Local<Function> cb = Local<Function>::Cast(args[1]);

	uv_work_t work_req;
	b32_context_t context;
	context.callback = &cb;
	context.data = Buffer::Data(arg->ToObject());
	context.length = Buffer::Length(arg->ToObject());
	context.decoding = decoding;
	work_req.data = (void *) &context;

	uv_queue_work(uv_default_loop(),&work_req, do_encode_decode, after_encode_decode);

	return;
}

void encode(const FunctionCallbackInfo<Value>& args) {
	encode_decode(args, false);
}

void decode(const FunctionCallbackInfo<Value>& args){
	encode_decode(args, true);
}


void init(Handle<Object> exports, Handle<Object> module) {
	NODE_SET_METHOD(exports, "encodeSync", encodeSync);
	NODE_SET_METHOD(exports, "decodeSync", decodeSync);
	NODE_SET_METHOD(exports, "encode", encode);
	NODE_SET_METHOD(exports, "decode", decode);
}

NODE_MODULE(b32, init);
