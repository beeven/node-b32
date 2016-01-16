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
					'xcode_settings':{'GCC_ENABLE_CPP_EXCEPTIONS':'YES'}
				}]
			]
		}
	]
}
