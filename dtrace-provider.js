var util = require('util');

var DTraceProvider;

function DTraceProviderStub() {}
DTraceProviderStub.prototype.addProbe = function() {
    return {
        'fire': function() { }
    };
};
DTraceProviderStub.prototype.enable = function() {};
DTraceProviderStub.prototype.fire = function() {};

try {
    var binding = require('./DTraceProviderBindings');
    DTraceProvider = binding.DTraceProvider;
} catch (e) {
    util.debug(e);
    DTraceProvider = DTraceProviderStub;
}


exports.DTraceProvider = DTraceProvider;
exports.createDTraceProvider = function(name) {
    return new DTraceProvider(name);
};
