var extension = require('../dtrace-provider');

var provider1 = extension.createTraceProvider({provider_name: 'my_provider1', module_name: 'my_module', guid: '5A391F32-A079-42E1-97B1-8A45295AA1FD'});
provider1.addProbe('probe1', 'int', 'int');
provider1.addProbe('probe2', 'int', 'int');
provider1.enable();

var provider2 = extension.createTraceProvider({provider_name: 'my_provider2', module_name: 'my_module', guid: '6A391F32-A079-42E1-97B1-8A45295AA1FD'});
provider2.addProbe('probe3', 'int', 'int');
provider2.addProbe('probe1', 'int', 'int');
provider2.enable();

var provider3 = extension.createTraceProvider({provider_name: 'my_provider3', module_name: 'my_module', guid: '7A391F32-A079-42E1-97B1-8A45295AA1FD'});
provider3.addProbe('probe1', 'int', 'int');
provider3.addProbe('probe2', 'int', 'int');
provider3.enable();

var provider4 = extension.createTraceProvider({provider_name: 'my_provider4', module_name: 'my_module', guid: '8A391F32-A079-42E1-97B1-8A45295AA1FD'});
provider4.addProbe('probe1', 'int', 'int');
provider4.addProbe('probe3', 'int', 'int');
provider4.enable();

provider1.fire('probe1', function () {
    return ([12, 3]);
});

provider2.fire('probe1', function () {
    return ([12, 73]);
});

provider3.fire('probe1', function () {
    return ([12, 3]);
});

provider4.fire('probe1', function () {
    return ([12, 73]);
});
