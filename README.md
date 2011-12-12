# dtrace-provider - Native DTrace providers for Node.js apps.

This extension allows you to create native DTrace providers for your
Node.js applications. That is, to create providers and probes which
expose information specific to your application, rather than
information about the node runtime.

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

## EXAMPLE

Here's a simple example of creating a provider:

    var d = require('dtrace-provider');

    var dtp = d.createDTraceProvider("nodeapp");
    var p1 = dtp.addProbe("probe1", "int", "int");
    var p2 = dtp.addProbe("probe2", "char *");
    dtp.enable();

At this point the provider has been created - the probes will show up
in a listing from dtrace -l.

You can fire the probes using a method on the provider:

    dtp.fire("probe1", function() { return [1, 2]; });
    dtp.fire("probe2", function() { return ["hello, dtrace"]; });

or a method directly on the probe objects:

    p1.fire(function(p) { return [1, 2]; });
    p2.fire(function(p) { return ["hello, dtrace"]; });

Firing the probes produces this output from dtrace:

    $ sudo dtrace -Z -n 'nodeapp*:::probe1{ trace(arg0); trace(arg1) }'  \
                     -n 'nodeapp*:::probe2{ trace(copyinstr(arg0));  }'
    dtrace: description 'nodeapp*:::probe1' matched 0 probes
    dtrace: description 'nodeapp*:::probe2' matched 0 probes
    CPU     ID                    FUNCTION:NAME
      1 123562                      func:probe1                 1                2
      1 123563                      func:probe2   hello, dtrace                    

Arguments are captured by a callback only executed when the probe is
enabled. This means you can do more expensive work to gather arguments.

## PLATFORM SUPPORT

The nature of this extension means that support must be added for each
platform. Right now that support is only in place for OS X, 64 bit and
Solaris, 32 bit.

## LIMITATIONS

The maximum number of probe arguments is 6. There's scope to increase
this, with some extra complexity in the platform-specific code.
 
The data types supported are "int" and "char *". There's definitely
scope to improve this, with more elaborate argument handling. 

## CAVEATS

Performance is not where it should be, most especially the
disabled-probe effect. Probes are already using the "is-enabled"
feature of DTrace to control execution of the arguments-gathering
callback, but too much work needs to be done before that's
checked. 

## CONTRIBUTING

The source is available at:

  https://github.com/chrisa/node-dtrace-provider.

For issues, please use the Github issue tracker linked to the
repository. Github pull requests are very welcome. 

## OTHER IMPLEMENTATIONS

This node extension is derived from the ruby-dtrace gem, via the Perl
module Devel::DTrace::Provider, both of which provide the same
functionality to those languages.
