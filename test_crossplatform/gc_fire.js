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
