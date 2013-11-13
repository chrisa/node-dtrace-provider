var extension = require('../dtrace-provider');
var provider = extension.createTraceProvider({provider_name: 'my_provider', module_name: 'my_module', guid: '5A391F32-A079-42E1-97B1-8A45295AA1FD'});
var probe = provider.addProbe("json1", "json");
provider.enable();

var obj = new Object;
obj.foo = 42;
obj.bar = 'forty-two';

probe.fire(function(p) {
    return [obj];
});
