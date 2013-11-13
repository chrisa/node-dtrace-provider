var extension = require('../dtrace-provider');
console.log('required');

try {
  var provider = extension.createTraceProvider({provider_name: 'my_provider', module_name: 'my_module', guid: '5A391F32-A079-42E1-97B1-8A45295AA1FD'});
} catch (ex) {
  console.log(ex);
  process.exit(1);
}
console.log('created provider');

var desc1 = [10, 0, 0, 0, 0, 0, 0];
var desc2 = [12, 0, 0, 0, 0, 0, 0];
// add probes with descriptor
provider.addProbe('event1', desc1, 'char *', 'int16');
provider.addProbe('event2', desc2, 'wchar_t *');
// use auto created descriptor
provider.addProbe('event3', 'int', 'uint64', 'char *');
provider.addProbe('event4', 'json');
console.log('added probes');

provider.enable();
console.log('enabled probes');

var obj = new Object;
obj.foo = 42;
obj.bar = 'forty-two';

provider.fire('event1', function () { return ['abc', 77]; });
provider.fire('event1', function () { return ['MMMMMMMMM', 513]; });

provider.disable();
provider.enable();

//provider.removeProbe('event2');
provider.fire('event2', function () { return ['MMMMMMMMM']; });

provider.fire('event3', function () { return [500, 999, 'xxxxxx yyyyyy']; });
provider.fire('event4', function (p) { return [obj]; });


console.log('fired probes');

process.exit(0);



