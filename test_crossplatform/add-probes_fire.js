var extension = require('../dtrace-provider');

var provider = extension.createTraceProvider({provider_name: 'my_provider', module_name: 'my_module', guid: '5A391F32-A079-42E1-97B1-8A45295AA1FD'});
provider.addProbe("probe0", "int");
provider.enable();
provider.fire("probe0", function(p) { return [0]; });

for (var i = 1; i < 10; i++) {
    provider.addProbe("probe" + i, "int");
    provider.disable();
    provider.enable();
    provider.fire("probe" + i, function(p) { return [i]; });
}