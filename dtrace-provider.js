var DTraceProvider;

function DTraceProviderStub() {}
DTraceProviderStub.prototype.addProbe = function() {};
DTraceProviderStub.prototype.enable = function() {};
DTraceProviderStub.prototype.fire = function() {};

try {
    var binding = require('./DTraceProviderBindings');
    DTraceProvider = binding.DTraceProvider;
} catch (e) {
    DTraceProvider = DTraceProviderStub;
    console.log(e);
}


exports.DTraceProvider = DTraceProvider;
exports.createDTraceProvider = function(name) {
    return new DTraceProvider(name);
};
