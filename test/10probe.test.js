var test = require('tap').test;
var format = require('util').format;
var dtest = require('./dtrace-test').dtraceTest;

if (process.platform == 'darwin') {
  test(
      '10-arg probe',
      dtest(
          function() {},
          ['dtrace', '-Zqn',
           'testlibusdt$target:::10probe{ printf("%d %d %d %d %d %d %d %d %d %d\\n", arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9) }',
          '-c', format('node %s/10probe_fire.js', __dirname)
          ],
          function(t, exit_code, traces) {
            console.log(traces)
              t.notOk(exit_code, 'dtrace exited cleanly');
              t.equal(traces.length, 10, 'got 10 traces');

              var args = [];
              for (var i = 1; i <= 10; i++) {
                  args.push(i);
                  var traced = traces[i - 1].split(' ');
                  args.forEach(function(n) {
                      t.equal(traced[n - 1], [n].toString(),
                              format('arg%d of a %d-arg probe firing should be %d', n - 1, i, n));
                  });
              }
          }
      )
  );
}
