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
var probe1 = provider.addProbe('event1', desc1, 'char *', 'int16');
var probe2 = provider.addProbe('event2', desc2, 'wchar_t *');
// use auto created descriptor
var probe3 = provider.addProbe('event3', 'int', 'uint64', 'char *');
var probe4 = provider.addProbe('event4', 'json');
console.log('added probes');

provider.enable();
console.log('enabled probes');

var obj = new Object;
obj.foo = 42;
obj.bar = 'forty-two';

probe1.fire(function () { return ['abc', 77]; });
probe1.fire(function () { return ['MMMMMMMMM', 513]; });

//provider.removeProbe("event2");
probe2.fire(function () { return ['MMMMMMMMM']; });

probe3.fire(function () { return [500, 999, 'xxxxxx yyyyyy']; });
probe4.fire(function (p) { return [obj]; });

console.log('fired probes');

process.exit(0);

