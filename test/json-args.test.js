var test = require('tap').test;
var format = require('util').format;
var dtest = require('./dtrace-test').dtraceTest;

test(
    'probes with json type',
    dtest(
        function() { 
        },
        [
            'dtrace', '-Zqn',
            'nodeapp$target:::json1{ this->j = copyinstr(arg0); printf("%s\\n%d\\n%s\\n", this->j, strtoll(json(this->j, "foo")), json(this->j, "bar")) }',
            '-c', format('node %s/json-args_fire.js', __dirname)
        ],
        function(t, traces) {
            t.equal(traces[0], '{"foo":42,"bar":"forty-two"}');
            t.equal(traces[1], '42');
            t.equal(traces[2], 'forty-two');
        }
    )
);
