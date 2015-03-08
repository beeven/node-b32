#include <node.h>
#include <node_buffer.h>
#include <stdio.h>
#include <uv.h>

using namespace v8;
using namespace node;

const char* b32table = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

int base32_encode(const char* data, size_t length, char *buf, size_t bif_size) {
	if(length > (1 << 28)) {
		return -1;
	}
	unsigned int count = 0;
	if(length > 0) {
		int buffer = data[0];
		unsigned int next = 1;
		int bits_left = 8;
		while(count < bif_size && (bits_left > 0 || next < length)) {
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
			buf[count++] = b32table[index];
		}
	}
	if(count < bif_size) {
		buf[count] = '\000';
	}
	return count;
}

int base32_decode(const char* encoded, char *buf, size_t bif_size) {
	int buffer = 0;
	int bits_left = 0;
	unsigned int count = 0;
	for(const char *ptr = encoded; count < bif_size && *ptr && *ptr!='='; ++ptr) {
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
			buf[count++] = buffer >> (bits_left - 8);
			bits_left -= 8;
		}
	}
	if(count < bif_size) {
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

	size_t bif_size = (size * 8 - 1)/5+1;
	char *result = new char[bif_size];
	int result_size = 0;
	if((result_size = base32_encode(buf,size,result,bif_size))< 0) {
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

	size_t bif_size = (size * 5 )/8;
	char *result = new char[bif_size];
	int result_size = 0;
	if((result_size = base32_decode(buf,result,bif_size)) < 0) {
		isolate->ThrowException(Exception::TypeError(
			String::NewFromUtf8(isolate,"Decode error")));
		return;
	} else {
		args.GetReturnValue().Set(Buffer::Use(isolate,result,result_size));
	}

}

typedef struct {
	Persistent<Function> callback;
	char *data;
	size_t length;
	char *buf;
	size_t buf_size;
	bool decoding;
	int ret;
} b32_context_t;


void do_encode_decode(uv_work_t *req) {

	b32_context_t* context = (b32_context_t*)req->data;
	int ret = 0;
	if(context->decoding) {
		context->buf_size = (context->length * 5 )/8;
		context->buf = new char[context->buf_size];
		ret = base32_decode(context->data, context->buf, context->buf_size);
	}
	else {
		context->buf_size = (context->length * 8 - 1)/5+1;
		context->buf = new char[context->buf_size];
		ret = base32_encode(context->data, context->length, context->buf, context->buf_size);
	}
	if(ret < 0) {
		delete context->buf;
	}
}

void after_encode_decode(uv_work_t *req, int status) {
	Isolate* isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);

	b32_context_t *context = (b32_context_t*)req->data;

	const unsigned argc = 2;
	Local<Value> argv[argc];

	int ret = context->ret;

	if(ret >= 0) {
		argv[0] = Null(isolate);
		argv[1] = Buffer::Use(context->buf,context->buf_size);
	}
	else {
		argv[0] = String::NewFromUtf8(isolate,"Encode error");
		argv[1] = Null(isolate);
	}
	TryCatch try_catch;
	Local<Function> cb = Local<Function>::New(isolate,context->callback);
	cb->Call(isolate->GetCurrentContext()->Global(),argc,argv);
	if(try_catch.HasCaught()){
		node::FatalException(try_catch);
	}
	context->callback.Reset();
	delete context;
	delete req;
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

	uv_work_t* req = new uv_work_t;
	b32_context_t* context = new b32_context_t;
	context->callback.Reset(isolate,cb);
	context->data = Buffer::Data(arg->ToObject());
	context->length = Buffer::Length(arg->ToObject());
	context->decoding = decoding;
	req->data = (void*)context;

	uv_queue_work(
		uv_default_loop(),
		req,
		do_encode_decode,
		(uv_after_work_cb)after_encode_decode
	);

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
