var sys = require("sys");
try {
    var binding = require('./DTraceProviderBindings');
    var DTraceProvider = binding.DTraceProvider;
    var dtrace_provider = true;
} catch (e) {
    sys.puts(e);
    var dtrace_provider = false;
}

exports.DTraceProvider = DTraceProvider;
exports.createDTraceProvider = function(name) {
    return new DTraceProvider(name);
};
