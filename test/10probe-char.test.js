var test = require('tap').test;
var format = require('util').format;
var dtest = require('./dtrace-test').dtraceTest;

var dscript = 'testlibusdt$target:::10probe{ printf("%s %s %s %s %s %s %s %s %s %s\\n", copyinstr(arg0), copyinstr(arg1), copyinstr(arg2), copyinstr(arg3), copyinstr(arg4), copyinstr(arg5), copyinstr(arg6), copyinstr(arg7), copyinstr(arg8), copyinstr(arg9)); }'

if (process.platform == 'darwin') {
  test(
      '10-arg probe',
      dtest(
          function() {},
          ['dtrace', '-Zqn',
           dscript,
           '-c', format('node %s/10probe-char_fire.js', __dirname)
          ],
          function(t, exit_code, traces) {
              t.notOk(exit_code, 'dtrace exited cleanly');
              t.equal(traces.length, 1);

              var letters = ['a', 'b', 'c', 'd', 'e',
                             'f', 'g', 'h', 'i', 'j']

              var traced = traces[0].split(' ');
              for (var i = 0; i < 10; i++) {
                  t.equal(traced[i], letters[i],
                          format('arg%d of a 10-arg probe firing should be %s', i, letters[i]));
              }
          }
      )
  );
}
