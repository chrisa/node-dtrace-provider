var extension = require('../dtrace-provider');

var provider = extension.createTraceProvider({provider_name: 'my_provider', module_name: 'my_module', guid: '5A391F32-A079-42E1-97B1-8A45295AA1FD'});
var probe = provider.addProbe("p1", "int", "char *");
provider.enable();

probe.fire(function(p) {
    return [42, 'forty-two'];
});

provider.addProbe("p1", "int", "char *");
provider.enable();

probe.fire(function(p) {
    return [42, 'forty-two'];
});