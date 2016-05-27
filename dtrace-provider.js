var DTraceProvider;

function DTraceProviderStub() {}
DTraceProviderStub.prototype.addProbe = function(name) {
    var p = { 'fire': function () {} };
    this[name] = p;
    return (p);
};
DTraceProviderStub.prototype.enable = function() {};
DTraceProviderStub.prototype.fire = function() {};
DTraceProviderStub.prototype.disable = function() {};

var builds = ['Release', 'default', 'Debug'];

for (var i in builds) {
    try {
        var binding = require('./build/' + builds[i] + '/DTraceProviderBindings');
        DTraceProvider = binding.DTraceProvider;
        break;
    } catch (e) {
        // if the platform looks like it _should_ have DTrace
        // available, log a failure to load the bindings.
        if (e && e.code === 'MODULE_NOT_FOUND' &&
            (process.platform === 'darwin' ||
             process.platform === 'solaris' ||
             process.platform === 'freebsd')) {

            console.warn('Warning. dtrace provider binding not found in  %s/build/%s folder on %s', __dirname, builds[i], process.platform);
        } else {
            console.error('Error. dtrace provider: ' + e);
        }
    }
}

if (!DTraceProvider) {
    DTraceProvider = DTraceProviderStub;
    console.warn('Warning. dtrace uses fallback provider.');
}

exports.DTraceProvider = DTraceProvider;
exports.createDTraceProvider = function(name, module) {
    if (arguments.length == 2)
        return (new exports.DTraceProvider(name, module));
    return (new exports.DTraceProvider(name));
};
