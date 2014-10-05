{
	"targets": [
		{
			'target_name' : 'b32',
			'sources' : ['b32.cc'],
			'include_dirs':[
				"<!(node -e \"require('nan')\")"
			],
			'cflags' : ['-fexceptoins', '-Wall', '-D_FILE_OFFSET_BITS=64',
						'-D_LARGEFILE_SOURCE','-O2'],
			'cflags_cc' : ['-fexceptoins', '-Wall', '-D_FILE_OFFSET_BITS=64',
						'-D_LARGEFILE_SOURCE','-O2'],
			'conditoins': [
				['OS=="mac"',{
					'xcode_settings':{'GCC_ENABLE_CPP_EXCEPTOINS':'YES'}
				}]
			]
		}
	]
}