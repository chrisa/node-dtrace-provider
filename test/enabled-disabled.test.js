var test = require('tap').test;
var format = require('util').format;
var dtest = require('./dtrace-test').dtraceTest;

test(
    'enabling and disabling a provider',
    dtest(
        function() { },
        [
            'dtrace', '-Zqn', 
            'nodeapp*:::{ printf("%d\\n", arg0); }',
            '-c', format('node %s/enabled-disabled_fire.js', __dirname)
        ],
        function(t, traces) {
            t.equal(traces.length, 12);
            for (var i = 0; i < 10; i++) {
                t.equal(traces[i], [i].toString());
            }
            t.equal(traces[11], '42');
        }
    )
);
