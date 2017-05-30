// see 32probe-char.test.js

var d = require('../dtrace-provider');

var provider = d.createDTraceProvider("testlibusdt");
var probe = provider.addProbe(
    "10probe",
    "char *", "char *", "char *", "char *", "char *",
    "char *", "char *", "char *", "char *", "char *");
provider.enable();

var letters = ['a', 'b', 'c', 'd', 'e',
               'f', 'g', 'h', 'i', 'j']

probe.fire(function(p) {
    return letters;
});

