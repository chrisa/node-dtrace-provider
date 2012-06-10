{
  'targets': [
    {
      'target_name': 'DTraceProviderBindings',
      'sources': [
	'dtrace_provider.cc',
	'dtrace_dof.cc',
	'./libusdt/*',
      ],
      'conditions': [
	[ 'OS=="mac"', { 'sources': ['darwin-x86_64/drtace_probe.cc'],
			 'defines': ['CXXFLAGS', '-D_HAVE_DTRACE',],
		       }
	], 
	[ 'OS=="solaris"', {'sources': ['solaris-i386/dtrace_probe.cc'], 
			    'defines': ['CXXFLAGS', '-D_HAVE_DTRACE',],
			   }],
      ],
      'defines': [
	'compiler_cxx',
      ],
    }
  ]
}

