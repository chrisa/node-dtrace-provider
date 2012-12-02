var test = require('tap').test;
var format = require('util').format;
var dtest = require('./dtrace-test').dtraceTest;
var skip = false;

test(
    'probes with json type',
    dtest(
        function() {
            var execSync = require('execSync');
            var cmd = ['/usr/sbin/dtrace', '-qn', '\'BEGIN { json("", ""); exit(0); }\''].join(' ');
            var code = execSync.code(cmd);
            skip = (code == 0) ? false : true;
        },
        [
            'dtrace', '-Zqn',
            'nodeapp$target:::json1{ this->j = copyinstr(arg0); printf("%s\\n%d\\n%s\\n", this->j, strtoll(json(this->j, "foo")), json(this->j, "bar")) }',
            '-c', format('node %s/json-args_fire.js', __dirname)
        ],
        function(t, exit_code, traces) {
            t.test("json tests, need json() subroutine", {"skip": skip}, function (t) {
                t.notOk(exit_code, 'dtrace exited cleanly');
                t.equal(traces[0], '{"foo":42,"bar":"forty-two"}');
                t.equal(traces[1], '42');
                t.equal(traces[2], 'forty-two');
                t.end();
            });
        }
    )
);
