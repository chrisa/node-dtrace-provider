# dtrace-provider - Native DTrace providers for Node.js apps.

This extension allows you to create native DTrace providers for your
Node.js applications. That is, to create providers and probes which
expose information specific to your application, rather than
information about the node runtime. The extension also supports Win32 
and uses ETW WinAPI to implement similar functionality provided by 
DTrace on non-win platforms. That is why using this addon under Win32
is different from the other platforms.

You could use this to expose high-level information about the inner
workings of your application, or to create a specific context in which
to look at information from other runtime or system-level providers. 

The provider is not created in the usual way, by declaring it and then
changing the build process to include it, but instead dynamically at
runtime. This is done entirely in-process, and there is no background
compiler or dtrace(1) invocation. The process creating the provider
need not run as root.

## INSTALL

    $ npm install dtrace-provider

*Windows-only:*

The ETW code uses some C++11 features and requires at least VS2012 for it.
To specify the version, set the appropriate environment variable.

    GYP_MSVS_VERSION=2012

It must be set before invoking npm install or creating a project.

Creating the Visual Studio project files is done using node-gyp. This is a 
node add-on for creating the appropriate builds for your installation.
To create a Visual Studio solution:

    node-gyp configure

The Windows implementation supports Node.js 0.11 as well as previous releases.
To build the addon for a release version below 0.11, uncomment

    #define BUILD_PRE011_COMPATIBILITY

in 

    src/etw_win/v8_compatibility.h

## PLATFORM-SPECIFIC INFORMATION

On the system API level, providers are created and controlled differently 
on Windows and the other supported platforms.
For the platforms supporting DTrace the user needs to supply the names for 
the provider and the module to create the provider, while Windows requires
a GUID which is also used to control the tracing session.

To handle this difference, there are two ways to create the provider.
Both options can be used for all of the supported platforms.

*Option 1 (compatibility way):*

    var d = require('dtrace-provider');
    var dtp = d.createDTraceProvider("provider", "module");

The createDTraceProvider function takes the name of the provider as the 
first argument; the second argument is an optional module name. Both 
arguments are strings. This is a compatibility function left to allow
the existing node apps using this extension to be able to work without
modifying their code.

*Option 2 (cross-platform way):*

    var d = require('dtrace-provider');
    var provider = d.createTraceProvider({provider_name: 'provider', module_name: 'module', guid: '5A391F32-A079-42E1-97B1-8A45295AA1FD'});

The createTraceProvider function takes an object with the above-written 
properties. The user can be specific about what information they want to 
supply to the function. In the situation when the information required 
by the platform has not been passed in, it will be generated from what 
has been given.

That is why the extension also exposes two helper functions: guidFromNames
and namesFromGuid exposed on Windows and the other platforms respectively. These
functions use the same algorithms that are used by createTraceProvider and
can provide the neccessary information to control the session.

See the following files for the sample code.

    test_crossplatform/guid-from-names.js
    test_dtrace/names-from-guid.js

## EXAMPLE

Here's a simple example of creating a provider:

    var d = require('dtrace-provider');
    var dtp = d.createTraceProvider({provider_name: 'nodeapp'});
    var p1 = dtp.addProbe("probe1", "int", "int");
    var p2 = dtp.addProbe("probe2", "char *");
    dtp.enable();	   

Probes may be fired via the provider object:

    dtp.fire("probe1", function(p) {
        return [1, 2];
    });
    dtp.fire("probe2", function(p) { 
        return ["hello, dtrace via provider", "foo"];
    });

or via the probe objects themselves:

    p1.fire(function(p) {
      return [1, 2, 3, 4, 5, 6];
    });
    p2.fire(function(p) {
      return ["hello, dtrace via probe", "foo"];
    });

The example above creates a provider called "nodeapp", and adds two
probes. It then enables the provider, at which point the provider
becomes visible to DTrace.

The probes are then fired.

## GENERAL INFORMATION

Available on all of the supported platforms argument types are "int", for 
integer numeric values, "char *" for strings, and "json" for objects rendered into 
JSON strings. Arguments typed as "json" will be created as "char *" probes in
the provider, but objects passed to these probe arguments will be automatically 
serialized to JSON before being passed forward to the system API.

## DTRACE INFORMATION

Firing the probes from the example above produces this output:

    $ sudo dtrace -Z -n 'nodeapp*:::probe1{ trace(arg0); trace(arg1) }'  \
                     -n 'nodeapp*:::probe2{ trace(copyinstr(arg0));  }'
    dtrace: description 'nodeapp*:::probe1' matched 0 probes
    dtrace: description 'nodeapp*:::probe2' matched 0 probes
    CPU     ID                    FUNCTION:NAME
      1 123562                      func:probe1                 1                2
      1 123563                      func:probe2   hello, dtrace                    

Arguments are captured by a callback only executed when the probe is
enabled. This means you can do more expensive work to gather arguments.

The maximum number of arguments supported is 32. 

The JSON serialization feature is best used in conjunction with the json() 
D subroutine, but is available whether or not the platform supports it.

    # create a json probe:

    var dtp = d.createDTraceProvider("nodeapp");
    var p1 = dtp.addProbe("j1", "json");
    dtp.enable();
    p1.fire(function() { return { "foo" => "bar" }; });

    # on a platform supporting json():

    $ sudo dtrace -Z -n 'nodeapp*:::j1{ this->j = copyinstr(arg0); \
                                        trace(json(this->j, "foo")) }'
    dtrace: description 'nodeapp$target:::j1' matched 0 probes
    CPU     ID                    FUNCTION:NAME
      0  68712                            j1:j1   bar
  

## ETW INFORMATION

**Controlling the session**

Before the probes are fired the user must start a tracing session. It
can be done with session controllers like logman. After they have been
fired, the session must be stopped. Starting/stopping the session
requires the administrator privileges.

To start tracing

    logman start mytrace -p {GUID} -o mytrace.etl -ets

To stop tracing

    logman stop mytrace -ets


The guid may not always be known. In this case, the guidFromNames
function must be used. The following example demonstrates it.

There is an application where the provider has been created 
without specifying the GUID:

    var extension = require('dtrace-provider');
    var provider = extension.createTraceProvider({provider_name: 'nodeapp'});
    //var provider = extension.createDTraceProvider('nodeapp'); //An alternative way to create the provider.

We need to call guidFromNames before running this application.
The code snippet below demonstates how it can be done:	
	
    var extension = require('dtrace-provider');
    var generated_guid = extension.guidFromNames('nodeapp');
    console.log('generated guid = ' + generated_guid);

This code will print the GUID generated from the specified name.
The output will look like:

    generated guid = FAC03859-DB22-3EC7-B1B5-4E6E99A4B546

Then the session can be started with

    logman start mytrace -p {FAC03859-DB22-3EC7-B1B5-4E6E99A4B546} -o mytrace.etl -ets

After the session has been started, the original application
can be executed and all the probes fired will be recorded by ETW.

The guidFromNames function allows the optional module name to be passed
as well. For example: 

    var extension = require('dtrace-provider');
    var generated_guid = extension.guidFromNames('nodeapp', 'my_module');
    var provider = extension.createTraceProvider({provider_name: 'nodeapp', module_name: 'my_module'});
    //var provider = extension.createDTraceProvider('nodeapp', 'my_module'); //An alternative way to create the provider.

This works for as long as there is a one-to-one match for the names.
It is guaranteed that the GUID will always be same for same strings, i.e.
the generation algorithm is deterministic.

**Consuming events**

After the session has been stopped, an etl file will appear. It can be
converted to a human-readable XML with tracerpt. The output will
contain the information about the probes and raw binary data.

To convert trace output to xml file (data will show names and values)

    tracerpt mytrace.etl

To convert trace output to csv file (data will show names and values)

    tracerpt mytrace.etl -of CSV

The other way of consuming events is using tools that allow a manifest
file to be supplied. The manifest contains information about the data
types of the probes added and allows the raw binary data to be parsed
correctly. The manifest is updated in runtime whenever a new probe is
created with the addProbe function and it remains valid after each update.

Some of these tools:

    PerfView - http://www.microsoft.com/en-us/download/details.aspx?id=28567
    LinqPad with Tx - http://tx.codeplex.com

For the example script the PerfView output will look like:

    ThreadID="6,772" FormattedMessage="Node.js probe" Argument1="1" Argument2="2"
    ThreadID="6,772" FormattedMessage="Node.js probe" Argument1="hello, dtrace via provider" 

    
The manifest file is unique for each provider. It is created in the
process working directory with its name given according to the pattern:

    provider_GUID.man

For the 'nodeapp' provider name it will look like:

    provider_FAC03859-DB22-3EC7-B1B5-4E6E99A4B546.man

**Windows-only features**

The extension provides a variety of types under Windows in addition to
the types supported on both platforms.
Those types are:
int8, int16, int32, int64, uin8, uint16, uint32, uint64 and wchar_t *

Where 'int32' is an alias for 'int'.

When the extension is used under Windows, the user can be specific
about the ETW descriptor for the events (probes).

    var provider = extension.createTraceProvider({provider_name: 'my_provider', module_name: 'my_module', guid: '5A391F32-A079-42E1-97B1-8A45295AA1FD'});
    var desc = [10, 0, 0, 0, 0, 0, 0];
    var probe = provider.addProbe('event1', desc1, 'char *', 'int16');

The array matches the EVENT_DESCRIPTOR structure.

    http://msdn.microsoft.com/en-us/library/windows/desktop/aa363754%28v=vs.85%29.aspx

When the probes are created without the descriptor array, the event id
is incremented for each new probe; the other fields are zero-initialized.

The descriptor array will be ignored on all of the supported platforms, 
except Windows. Passing the types supported only under Windows 
on the other platforms will result in their fallback to the dtrace types. 
For example,  "uint16" will be handled as "int" and "wchar_t *" will be 
handled as "char *".

## PLATFORM SUPPORT

This libusdt-based Node.JS module supports 64 and 32 bit processes on
Mac OS X and Solaris-like systems such as Illumos or SmartOS. As more
platform support is added to libusdt, those platforms will be
supported by this module. See libusdt's status at:

    https://github.com/chrisa/libusdt#readme

FreeBSD is supported in principle but is restricted to only 4 working
arguments per probe.

The Win 32/64 implementation doesn't use libusdt and relies directly 
on WinAPI.

Platforms not supporting DTrace (notably, Linux) may
install this module without building libusdt, with a stub no-op
implementation provided for compatibility. The Windows implementation
doesn't require libusdt either. This allows cross-platform npm modules to 
embed probes and include a dependency on this module.

GNU Make is required to build libusdt; the build scripts will look for
gmake in PATH first, and then for make.

## CAVEATS

There is some overhead to probes, even when disabled. Probes are
already using the "is-enabled" feature of DTrace to control execution
of the arguments-gathering callback, but some work still needs to be
done before that's checked. This overhead should not be a problem
unless probes are placed in particularly hot code paths.

## CONTRIBUTING

The source is available at:

    https://github.com/chrisa/node-dtrace-provider.

For issues, please use the Github issue tracker linked to the
repository. Github pull requests are very welcome. 

## RUNNING THE TESTS

    $ npm install
    $ sudo ./node_modules/.bin/tap --tap test_dtrace/*.test.js

## OTHER IMPLEMENTATIONS

This node extension is derived from the ruby-dtrace gem, via the Perl
module Devel::DTrace::Provider, both of which provide the same
functionality to those languages. 