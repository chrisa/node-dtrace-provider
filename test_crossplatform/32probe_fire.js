// see 32probe.test.js

var extension = require('../dtrace-provider');

var provider = extension.createTraceProvider({provider_name: 'my_provider', module_name: 'my_module', guid: '5A391F32-A079-42E1-97B1-8A45295AA1FD'});
var probe = provider.addProbe(
    "probe32",
    "int", "int", "int", "int", "int", "int", "int", "int",
    "int", "int", "int", "int", "int", "int", "int", "int",
    "int", "int", "int", "int", "int", "int", "int", "int",
    "int", "int", "int", "int", "int", "int", "int", "int");

provider.enable();

var args = [];
for (var n = 1; n <= 32; n++) {
    args.push(n);
    probe.fire(function(p) {
        return args;
    });
}
