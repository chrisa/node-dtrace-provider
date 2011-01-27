# dtrace-provider - Native DTrace providers for Node.js apps.

This extension allows you to create native DTrace providers for your
Node.js applications. That is, to create providers and probes which
expose information specific to your application, rather than
information about the node runtime.

You could use this to expose high-level information about the inner
workings of your application, or to create a specific context in which
to look at information from other runtime or system-level providers. 

The provider is not created in the usual way, by declaring it, and
then changing the build process to include it, but dynamically at
runtime. This is done entirely in-process, and there is no background
compiler invocation. The process creating the provider need not run as
root.

## EXAMPLE

Here's a simple example of creating a provider:

  var d = require('dtrace-provider');

  var dtp = d.createDTraceProvider("nodeapp");
  dtp.addProbe("probe1", "int", "int");
  dtp.addProbe("probe2", "char *");
  dtp.enable();
  dtp.fire("probe1", function(p) { return [1, 2]; });
  dtp.fire("probe2", function(p) { return ["hello, dtrace"]; });

This example creates a provider called "nodeapp", and adds two
probes. It then enables the provider, at which point the provider
becomes visible to DTrace.

The probes are then fired, which produces this output:

  $ sudo dtrace -Z -n 'nodeapp*:::probe1{ trace(arg0); trace(arg1) }'  \
                   -n 'nodeapp*:::probe2{ trace(copyinstr(arg0));  }'
  dtrace: description 'nodeapp*:::probe1' matched 0 probes
  dtrace: description 'nodeapp*:::probe2' matched 0 probes
  CPU     ID                    FUNCTION:NAME
    1 123562                      func:probe1                 1                2
    1 123563                      func:probe2   hello, dtrace                    

## PLATFORM SUPPORT

The nature of this extension means that support must be added for each
platform. Right now that support is only in place for OS X, 64 bit,
but Solaris x86 and 32 bit support is on the way. 

## LIMITATIONS

The maximum number of probe arguments is 6. There's scope to increase
this, with some extra complexity in the platform-specific code.
 
The data types supported are "int" and "char *". There's definitely
scope to improve this, with more elaborate argument handling. 

## CAVEATS

This is the first working version, and there are a number of important
cleanups to be made. Creating a provider will leak memory, although
firing a probe should not. 

Performance is not where it should be, most especially the
disabled-probe effect. Probes are already using the "is-enabled"
feature of DTrace, but too much work needs to be done to get
there. Improving this may imply changes to the API shown in the
example above.
