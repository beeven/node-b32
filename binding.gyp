{
	"targets": [
		{
			'target_name' : 'b32',
			'sources' : ['b32.cc'],
			'include_dirs':[
			],
			'cflags' : ['-fexceptions', '-Wall', '-D_FILE_OFFSET_BITS=64',
						'-D_LARGEFILE_SOURCE','-O2'],
			'cflags_cc' : ['-fexceptions', '-Wall', '-D_FILE_OFFSET_BITS=64',
						'-D_LARGEFILE_SOURCE','-O2'],
			'conditions': [
				['OS=="mac"',{
					'xcode_settings':{'GCC_ENABLE_CPP_EXCEPTIONS':'YES'}
				}]
			]
		}
	]
}
