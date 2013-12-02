var extension = require('../dtrace-provider');
var provider = extension.createTraceProvider({provider_name: 'my_provider', module_name: 'my_module', guid: '5A391F32-A079-42E1-97B1-8A45295AA1FD'});
var probe = provider.addProbe("json1", "json", "json");
provider.enable();

var obj1 = { "value": "abc" };
var obj2 = { "value": "def" };

probe.fire(function(p) {
    return [obj1, obj2];
});
