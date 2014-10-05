var should = require("should");

var encode_testcases = {
	inputs: ["f","fo","foo","foob","fooba","foobar","f\x00","f\x00\x00","f\x00\x00\x00"],
	expected:["MY","MZXQ","MZXW6","MZXW6YQ","MZXW6YTB","MZXW6YTBOI","MYAA","MYAAA","MYAAAAA"]
}

var decode_testcases = {
	inputs: encode_testcases.expected,
	expected: encode_testcases.inputs
}

describe("b32node",function(){
	var b32node = require("bindings")("b32");

	it("should have a method named encodeSync",function(){
		b32node.encodeSync.should.be.a.Function;
	});

	describe("#encodeSync",function(){
		it("should pass all testcases",function(){
			var testcases = encode_testcases.inputs;
			var expected = encode_testcases.expected;
			for(var i=0;i<testcases.length;i++) {
				var result = b32node.encodeSync(new Buffer(testcases[i])).toString().trim();
				result.should.be.exactly(expected[i]);
			}
		});
	});

	describe("#decodeSync",function(){
		it("should pass all testcases",function(){
			var testcases = decode_testcases.inputs;
			var expected = decode_testcases.expected;
			for(var i=0;i<testcases.length;i++) {
				var result = b32node.decodeSync(new Buffer(testcases[i])).toString('utf8');
				result.should.be.exactly(expected[i]);
			}
		})
	});

	describe("#encode",function(){
		it("should pass the testcases: 'foo' ", function(done){
			b32node.encode(new Buffer('foo'),function(err,result){
				if(err) {
					done(err);
					throw err;
				}
				result.toString().should.be.exactly("MZXW6");
				done();
			})
		});
	});

	describe("#decode",function(){
		it("should pass the testcases: 'foo' ", function(done){
			b32node.decode(new Buffer('MZXW6'),function(err,result){
				if(err) {
					done(err);
					throw err;
				}
				result.toString().should.be.exactly("foo");
				done();
			})
		});
	});
});


describe("b32",function(){
	var b32 = require("./b32");
	describe("#encode",function(){
		it("should return 'MZXW6' if input is 'foo' ", function(done){
			b32.encode(new Buffer('foo'),function(err,result){
				if(err) {
					done(err);
					throw err;
				}
				result.toString().should.be.exactly("MZXW6");
				done();
			})
		});
		it("should return 'MZXW6===' if input is 'foo' and options contain 'padding'", function(done){
			b32.encode('foo',{padding:1}).then(function(result){
				result.toString().should.be.exactly('MZXW6===');
				done();
			})
			.fail(function(err){
				done(err);
			})
		});
		it("should return 'MYAA' if input is 'f\x00'",function(done){
			b32.encode('f\x00',function(err,result){
				if(err) {
					done(err);
					throw err;
				}
				result.toString().should.be.exactly("MYAA");
				done();
			})
		}) 
	});

	describe("#decode",function(done){
		it("should pass the testcases: 'foo' ", function(done){
			b32.decode(new Buffer('MZXW6'),function(err,result){
				if(err) {
					done(err);
					throw err;
				}
				result.toString().should.be.exactly("foo");
				done();
			})
		});
	});

	describe("#encodeSync",function(){
		it("should pass all testcases",function(){
			var testcases = encode_testcases.inputs;
			var expected = encode_testcases.expected;
			for(var i=0;i<testcases.length;i++) {
				var result = b32.encodeSync(new Buffer(testcases[i])).toString().trim();
				result.should.be.exactly(expected[i]);
			}
		});
	});

	describe("#decodeSync",function(){
		it("should pass all testcases",function(){
			var testcases = decode_testcases.inputs;
			var expected = decode_testcases.expected;
			for(var i=0;i<testcases.length;i++) {
				var result = b32.decodeSync(new Buffer(testcases[i])).toString('utf8');
				result.should.be.exactly(expected[i]);
			}
		})
	});
});