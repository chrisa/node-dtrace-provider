var spawn = require('child_process').spawn;

exports.dtraceTest = function(setup, dtargv, test) {
    return function(t) {
        setup();

        var dtrace = spawn('/usr/sbin/dtrace', dtargv.slice(1));
        
        var traces = [];
        dtrace.stdout.on('data', function (data) {
            //console.error("DTRACE STDOUT:", data.toString());
            traces.push(data.toString());
        });
        dtrace.stderr.on('data', function (data) {
            console.error("DTRACE STDERR:", data.toString());
        });
        dtrace.on('exit', function (code) {
            t.notOk(code, 'dtrace exited cleanly');
        });
        dtrace.on('close', function () {
            traces = traces.join('').split('\n')
                .filter(function (t) { return t.trim().length });
            
            test(t, traces);
            t.end();
        });
    };
}
        
