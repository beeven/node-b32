var addon = require("bindings")("b32");


function async_call(method, input, options, callback) {
	if(typeof(options) === 'function') {
		callback = options;
	}

	if(typeof(input) === 'string') {
		input = new Buffer(input);
	} 
	if(!Buffer.isBuffer(input)) {
		throw new Error("input should be string or buffer");
	}

	var promise;
	if(method == 'encode') {
		promise = new Promise(function(resolve, reject){
			addon.encode(input,function(err,data){
				if(err) {
					reject(err);
				}
				resolve(data.toString());
			});
		});
		if(!!options && !!options.padding) {
			promise = promise.then(function(result){
				var length = result.length;
				var padding_count = (length % 8) ? 8 - (length % 8) : 0;
				return result + Array(padding_count+1).join('=');
			});
		}
	} else if (method == 'decode') {
		var index = input.toString().indexOf('=');
		if(index == -1) index = input.length;
		input = input.slice(0,index);
		promise = new Promise(function(resolve,reject){
			addon.decode(input,function(err,data){
				if(err) {
					reject(err);
				}
				resolve(data);
			});
		});
	}

	if(typeof(callback) === 'function') {
		promise = promise.then(function(data){
			callback(null,data);
		},function(err){
			callback(err,null);
		});
	}
	return promise;
}

function sync_call(method, input, options) {
	if(typeof(input) === 'string') {
		input = new Buffer(input);
	} 
	if(!Buffer.isBuffer(input)) {
		throw new Error("input should be string or buffer");
	}
	if(method == 'encode') {
		var result = addon.encodeSync(input).toString();
		if(!!options && options.padding) {	
			var length = result.length;
			var padding_count = (length % 8) ? 8 - (length % 8) : 0;
			result += Array(padding_count+1).join('=');
		}
		return result;
	} else if (method == 'decode') {
		var index = input.toString().indexOf('=')
		if(index == -1) index = input.length;
		return addon.decodeSync(input.slice(0,index));
	}
}

module.exports.encode = function(input, options, callback) {
	return async_call('encode',input,options,callback);
}

module.exports.decode = function(input, callback) {
	return async_call('decode', input,callback);
}

module.exports.encodeSync = function(input,options) {
	return sync_call('encode',input,options);
}

module.exports.decodeSync = function(input) {
	return sync_call('decode',input);
}
