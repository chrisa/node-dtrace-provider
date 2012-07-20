{
  'targets': [
    {
      'target_name': 'DTraceProviderBindings',
      'sources': [
	'dtrace_provider.cc',
	'dtrace_probe.cc',
      ],
      'conditions': [
	[ 'OS=="mac"', { 'sources': ['darwin-x86_64/dtrace_probe.cc'],
			 'defines': ['cflags', '-D_HAVE_DTRACE',],
			 'actions': [{'action_name': 'build_libusdt',
				     'action': ['cd ../libusdtdir', 
						'make clean all',
						'cd -',
					       ],
				    }],
		       }
	], 
	[ 'OS=="solaris"', {'sources': ['solaris-i386/dtrace_probe.cc'], 
			    'defines': ['cflags', '-D_HAVE_DTRACE',],
			    'actions': [{'action_name': 'build_libusdt',
					'action': ['cd ../libusdtdir', 
						    'make clean all',
						    'cd -',
					       ],
				    }],
			   }],
      ],
      'defines': [
	'compiler_cxx',
      ],
    }
  ]
}

