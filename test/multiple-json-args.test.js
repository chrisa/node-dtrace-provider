var test = require('tap').test;
var format = require('util').format;
var dtest = require('./dtrace-test').dtraceTest;
var skip = false;

test(
    'probes with multiple json types',
    dtest(
        function() { 
            var execSync = require('execSync');
            var cmd = ['/usr/sbin/dtrace', '-qn', '\'BEGIN { json("", ""); exit(0); }\''].join(' ');
            var code = execSync.code(cmd);
            skip = (code == 0) ? false : true;
        },
        [
            'dtrace', '-Zqn',
            'nodeapp$target:::json1{ printf("%s %s", json(copyinstr(arg0), "value"), json(copyinstr(arg1), "value")) }',
            '-c', format('node %s/multiple-json-args_fire.js', __dirname)
        ],
        function(t, exit_code, traces) {
            t.test("json tests, need json() subroutine", {"skip": skip}, function (t) {
                t.notOk(exit_code, 'dtrace exited cleanly');
                t.equal(traces[0], 'abc def');
                t.end();
            });
        }
    )
);
