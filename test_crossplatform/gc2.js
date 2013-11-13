// node --expose_gc ...

var extension = require('../dtrace-provider');
var provider = extension.createTraceProvider({provider_name: 'my_provider', module_name: 'my_module', guid: '5A391F32-A079-42E1-97B1-8A45295AA1FD'});

// assign the returned probe object
var probe = provider.addProbe("gcprobe", "int");
provider.enable();

// run GC
gc();
console.log("gc 1 called");
// probe object should still be around
provider.fire("gcprobe", function() {
    return [12];
});

provider = "something else";
gc();
console.log("gc 2 called");

probe.fire(function() {
    return [13];
});

probe = "something else";

gc();
console.log("gc 3 called");
var provider = extension.createTraceProvider({provider_name: 'my_provider', module_name: 'my_module', guid: '5A391F32-A079-42E1-97B1-8A45295AA1FD'});
console.log("script end");