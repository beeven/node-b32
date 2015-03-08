#include <nan.h>
#include <node.h>
#include <node_buffer.h>
#include <stdio.h>

using namespace v8;
using namespace node;

const char *b32table = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

int base32_encode(const char *data, size_t length, char *result, size_t buf_size) {
	if(length < 0 || length > (1 << 28)) {
		return -1;
	}
	int count = 0;
	if(length > 0) {
		int buffer = data[0];
		int next = 1;
		int bits_left = 8;
		while(count < buf_size && (bits_left > 0 || next < length)) {
			if(bits_left < 5) {
				if(next < length) {
					buffer <<= 8;
					buffer |= data[next++] & 0xff;
					bits_left += 8;
				} else {
					int pad = 5 - bits_left;
					buffer <<= pad;
					bits_left += pad;
				}
			}
			int index = 0x1f & (buffer >> (bits_left - 5));
			bits_left -= 5;
			result[count++] = b32table[index];
		}
	}
	if(count < buf_size) {
		result[count] = '\000';
	}
	return count;
}

int base32_decode(const char *encoded, char *result, size_t buf_size) {
	int buffer = 0;
	int bits_left = 0;
	int count = 0;
	for(const char *ptr = encoded; count < buf_size && *ptr && *ptr!='='; ++ptr) {
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
		bits_left += 5;
		if(bits_left >= 8) {
			result[count++] = buffer >> (bits_left - 8);
			bits_left -= 8;
		}
	}
	if(count < buf_size) {
		result[count] = '\000';
	}
	return count;
}

class B32Async: public NanAsyncWorker {
public:
	int method;
	const char* input;
	const size_t input_size;
	char* result;
	int result_size;

	B32Async(NanCallback *callback, int method,const char* input, const size_t input_size):NanAsyncWorker(callback),
		method(method),input(input), input_size(input_size) {
	}

	~B32Async() {
		//delete result;
	}

	void Execute() {
		size_t buf_size;
		if(method == 1) { // encode
			buf_size  = (input_size * 8 - 1)/5+1;
			result = new char[input_size];
			if((result_size=base32_encode(input,input_size,result,buf_size)) == -1) {
				SetErrorMessage("encode error");
			}
			
		} else { // decode
			size_t size = strlen(input);
			buf_size = (size * 5 )/8;
			result = new char[buf_size];
			if((result_size = base32_decode(input,result,buf_size)) == -1) {
				SetErrorMessage("decode error");
			}
		}
		//result_size = buf_size;
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

NAN_METHOD(encodeSync) {
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
	size_t size = Buffer::Length(arg->ToObject());
	char* buf = Buffer::Data(arg->ToObject());

	size_t buf_size = (size * 8 - 1)/5+1;
	char *result = new char[buf_size];
	int result_size = 0;
	if((result_size = base32_encode(buf,size,result,buf_size))< 0) {
		NanThrowError("encode error");
		NanReturnUndefined();
	} else {
		NanReturnValue(NanBufferUse(result,result_size));
	}

}

NAN_METHOD(decodeSync) {
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
	
	size_t size = Buffer::Length(arg->ToObject());
	char* buf = Buffer::Data(arg->ToObject());

	size_t buf_size = (size * 5 )/8;
	char *result = new char[buf_size];
	int result_size = 0;
	if((result_size = base32_decode(buf,result,buf_size)) < 0) {
		NanThrowError("decode error");
		NanReturnUndefined();
	} else {
		NanReturnValue(NanBufferUse(result,result_size));
	}

}

NAN_METHOD(encode) {
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

	B32Async* job = new B32Async(callback,1,buf,size);
	NanAsyncQueueWorker(job);
	NanReturnUndefined();
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