{
    'conditions': [
        ['OS=="mac" or OS=="solaris"', {

            # If we are on the Mac, or a Solaris derivative, attempt
            # to build the DTrace provider extension.

            'targets': [
                {
                    'target_name': 'TraceProviderBindings',
                    'sources': [
                        'src/dtrace/dtrace_provider.cc',
                        'src/dtrace/dtrace_probe.cc',
                        'src/dtrace/dtrace_argument.cc'
                    ],
                    'include_dirs': [
                        'libusdt'
                    ],
                    'dependencies': [
                        'libusdt'
                    ],
                    'libraries': [
                        '-L<(module_root_dir)/libusdt -l usdt'
                    ]
                },
                {
                    'target_name': 'libusdt',
                    'type': 'none',
                    'actions': [{
                        'inputs': [''],
                        'outputs': [''],
                        'action_name': 'build_libusdt',
                        'action': [
                            'sh', 'libusdt-build.sh'
                        ]
                    }]
                }
            ]
        }],
        ['OS=="win"', {
        
            # If we are on Windows, attempt
            # to build the ETW provider extension.
            
            'targets': [
                {
                    'target_name': 'TraceProviderBindings',
                    'sources': [ 
                        'src/etw_win/etw_provider.cc', 
                        'src/etw_win/v8_provider.cc', 
                        'src/etw_win/v8_probe.cc', 
                        'src/etw_win/manifest_builder.cc'
                    ],
                    'link_settings': {
                      'libraries': [ "rpcrt4.lib" ]
                    }
                }
            ]
        },

        # If we are not on the Mac, Solaris or Windows, tracing is unavailable. 
        # This target is necessary because GYP requires at least one
        # target to exist.

        {
            'targets': [ 
                {
                    'target_name': 'TraceProviderStub',
                    'type': 'none'
                }
            ]
        }]
    ]
}