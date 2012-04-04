// expected output:
//
// $ sudo dtrace -Z -n 'nodeapp*:::probe1{ printf("%i %i %i %i %i %i", arg0, arg1, arg2, arg3, arg4, arg5); }' \
//                  -n 'nodeapp*:::probe2{ printf("%s %s %s %s %s %s", copyinstr(arg0), copyinstr(arg1), copyinstr(arg2), copyinstr(arg3), copyinstr(arg4), copyinstr(arg5)) }' \
//                  -c 'node test/dtp.js'
//
// dtrace: description 'nodeapp*:::probe1' matched 0 probes
// dtrace: description 'nodeapp*:::probe2' matched 0 probes
// CPU     ID                    FUNCTION:NAME
//  1   3421                      func:probe1 1 2 3 4 5 6
//  1   3422                      func:probe2 a b c d e f

var d = require('../dtrace-provider');
var dtp = d.createDTraceProvider("nodeapp");
dtp.addProbe("probe1", "int", "int", "int", "int", "int", "int");
dtp.addProbe("probe2", "char *", "char *", "char *", "char *", "char *", "char *");
dtp.enable();
dtp.fire("probe1", function(p) { return [1, 2, 3, 4, 5, 6]; });
dtp.fire("probe2", function(p) { return ["a", "b", "c", "d", "e", "f"]; });

setTimeout(function() { }, 1000);
