var extension = require('../dtrace-provider');

var provider = extension.createTraceProvider({provider_name: 'my_provider', module_name: 'my_module', guid: '5A391F32-A079-42E1-97B1-8A45295AA1FD'});
provider.addProbe("probe1", "int");
provider.enable();
provider.fire("probe1", function(p) { return [1]; });
provider.disable();

provider.disable();