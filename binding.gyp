{
	"targets": [
		{
			'target_name' : 'b32',
			'sources' : ['b32.cc'],
			"type":"shared_library",
			"product_extension":"node",
			'include_dirs':[
				"<!(node -e \"require('nan')\")"
			],

			'conditions': [
				['OS=="mac"',{
					'xcode_settings': {
						'OTHER_CPLUSPLUSFLAGS': ['-std=c++11', '-stdlib=libc++'],
						# node-gyp 2.x doesn't add this any more
						# https://github.com/TooTallNate/node-gyp/pull/612
						'OTHER_LDFLAGS':['-undefined dynamic_lookup'],
						'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
						'MACOSX_DEPLOYMENT_TARGET': '10.8'
					}
				}]
			]
		}
	]
}
