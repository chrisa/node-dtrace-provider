// expected output:
// 
// $ sudo dtrace -Zn 'nodeapp*:::gcprobe{ trace(arg0); }' -c 'node --expose_gc test/gc.js'
// Dtrace: description 'nodeapp*:::gcprobe' matched 0 probes
// dtrace: pid 66257 has exited
// CPU     ID                    FUNCTION:NAME
//   1   1778                  gcprobe:gcprobe        4320227343

var extension = require('../dtrace-provider');
var provider = extension.createTraceProvider({provider_name: 'my_provider', module_name: 'my_module', guid: '5A391F32-A079-42E1-97B1-8A45295AA1FD'});

// don't assign the returned probe object anywhere
provider.addProbe("gcprobe", "int");
provider.enable();

// run GC
gc();

// probe object should still be around
provider.fire("gcprobe", function() {
    return [];
});
