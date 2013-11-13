var TraceProviderCreatorFunction;
var NamesFromGuidFunction;
var GuidFromNamesFunction;
var platform = process.platform;
var is_platform_supported = false;

function TraceProviderStub() { }
TraceProviderStub.prototype.addProbe = function() {
    return {
        'fire': function() { }
    };
};

TraceProviderStub.prototype.removeProbe = function () { };
TraceProviderStub.prototype.enable = function () { };
TraceProviderStub.prototype.disable = function () { };
TraceProviderStub.prototype.fire = function () { };
NamesFromGuidFunction = function () { return ['not implemented on this platform', 'not implemented on this platform'] };
GuidFromNamesFunction = function () { return 'not implemented on this platform' };
TraceProviderCreatorFunction = TraceProviderStub;

if (process.platform == 'darwin' ||
	process.platform == 'solaris' ||
	process.platform == 'freebsd' || 
	process.platform == 'win32') {
	is_platform_supported = true;
}

var builds = ['Release', 'default', 'Debug'];

for (var i in builds) {
	try {
		var binding = require('./build/' + builds[i] + '/TraceProviderBindings');
		TraceProviderCreatorFunction = binding.createTraceProvider;
		
		//If we are on win32 - expose the guidFromNames function to see what guid will be generated for the ETW provider if the user didn't specify it.
		if(platform == 'win32') {
			GuidFromNamesFunction = binding.guidFromNames;
		} else {
			NamesFromGuidFunction = binding.namesFromGuid;
		}
		
		break;
	} catch (e) {
		//If the platform looks like it _should_ support the extension,
		//log a failure to load the bindings.
		if (is_platform_supported) {
			console.log(e);
		}
	}
}

//Expose the universal function for provider creation.
exports.TraceProvider = TraceProviderCreatorFunction;
exports.namesFromGuid = NamesFromGuidFunction;
exports.guidFromNames = GuidFromNamesFunction;
exports.createTraceProvider = function (object) {
	return new TraceProviderCreatorFunction(object);
};

//We must also expose the dtrace-only creator function to support existing code.
exports.createDTraceProvider = function (name, module) {
	if (arguments.length == 2) {
		return (new TraceProviderCreatorFunction(name, module)); 
	}
	else {
		return (new TraceProviderCreatorFunction(name));
	}
};

