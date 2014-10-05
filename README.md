node-b32
========

Implementation of RFC-3548 Base32 encoding/decoding for node using C (suppose to be faster than pure javascript).

Features
=========
. Implemented in C
. Sync & Async methods support
. Options to add '=' paddings
. Promises chaining support (Thanks to q)

Installation
=========
```
npm install node-b32
```

Test
=========
```
npm test
```

Usage
=========
```
var b32 = require("node-b32");
// Encode a string and use callback to pick up the result 
b32.encode('foo',function(err,result){
    console.log(result.toString());
});

// Encode a buffer and use promises
b32.encode(new Buffer('foo\x00'),{padding:true})
    .then(function(encoded_result){
        console.log(encoded_result.toString());
        return b32.decode(encoded_result);
    })
    .then(function(decoded_result){
        console.log(decoded_result);
    });

// Decode a string in synchronize mode
var decoded = b32.decodeSync('MZXW6===');

// Decode a buffer with async function
b32.decode(new Buffer('MZXW6')).
    .then(function(result){
        console.log(result);
    });

```


Functions
=========
# encodeSync(Buffer[,options])
options:
 - padding: Boolean (default: false) add '=' padding to the end
Returns:
 Encoded base32 string in buffer

# decodeSync(Buffer)
Returns:
 Decoded binary buffer

# encode(Buffer,[options],[callback])
options:
  - same as sync function
callback: a function with signature function(err,result)
Returns:
    A promise which will resolve with the result

# decode(Buffer,[callback])
callback: a function with signature function(err,result)
Returns:
    A promise which will resolve with the result