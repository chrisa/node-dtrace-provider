// node --expose_gc ...

var extension = require('../dtrace-provider');

for (var i = 0; i < 1000000; i++) {
    console.log("i: " + i);
    var provider = extension.createTraceProvider({provider_name: 'my_provider' + i});
    var probe = provider.addProbe("gcprobe");
    provider.enable();
    provider.disable();
}
gc();
