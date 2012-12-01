var test = require('tap').test;
var format = require('util').format;
var dtest = require('./dtrace-test').dtraceTest;

test(
    'probes with multiple json types',
    dtest(
        function() { 
        },
        [
            'dtrace', '-Zqn',
            'nodeapp$target:::json1{ printf("%s %s", json(copyinstr(arg0), "value"), json(copyinstr(arg1), "value")) }',
            '-c', format('node %s/multiple-json-args_fire.js', __dirname)
        ],
        function(t, traces) {
            t.equal(traces[0], 'abc def');
        }
    )
);
