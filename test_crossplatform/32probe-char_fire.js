// see 32probe-char.test.js

var extension = require('../dtrace-provider');

var provider = extension.createTraceProvider({provider_name: 'my_provider', module_name: 'my_module', guid: '5A391F32-A079-42E1-97B1-8A45295AA1FD'});
var probe = provider.addProbe(
    "probe32",
    "char *", "char *", "char *", "char *", "char *", "char *", "char *", "char *",
    "char *", "char *", "char *", "char *", "char *", "char *", "char *", "char *",
    "char *", "char *", "char *", "char *", "char *", "char *", "char *", "char *",
    "char *", "char *", "char *", "char *", "char *", "char *", "char *", "char *");
provider.enable();

var letters = ['a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
               'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
               'q', 'r', 's', 't', 'u', 'v', 'w', 'x',
               'y', 'z', 'A', 'B', 'C', 'D', 'E', 'F'];

probe.fire(function(p) {
    return letters;
});