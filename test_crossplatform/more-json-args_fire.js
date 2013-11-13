var extension = require('../dtrace-provider');

var provider = extension.createTraceProvider({provider_name: 'my_provider', module_name: 'my_module', guid: '5A391F32-A079-42E1-97B1-8A45295AA1FD'});
var probe = provider.addProbe("p1", "json", "json");
provider.enable();

var obj1 = new Object;
obj1.foo = 21;
obj1.bar = 'twenty-one';

var obj2 = new Object;
obj2.foo = 42;
obj2.bar = 'forty-two';

probe.fire(function(p) {
	return [obj1, obj2];
});