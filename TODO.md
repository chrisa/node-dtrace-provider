# dtrace-provider - TODO

## ARGUMENT TYPES

### Structured Arguments

The current support for argument types is limited to "char *" and
"int", i.e. strings and integer types. Native string types in
Javascript are converted to raw C strings for use with DTrace. 

With support for dynamic types and translators from the host DTrace
implementation, it'd be possible to provide a mapping from Javascript
objects to C structs, which could then be inspected in D scripts.

## PERFORMANCE

### Disabled Probe Effect

The main problem here is how much work needs to be done to get to the
point we can use the is-enabled check.

While the is-enabled check allows us to avoid invoking the probe-args
callback with its potentially expensive args-gathering work, we still
do the following when a disabled probe is fired:

 * Dispatch Fire method from JS
 * Obtain DTraceProvider instance from V8 context
 * Args checking (must get a probe name and a callback)
 * Linear search through provider's probes for probe name
 * Invoke the is-enabled stub

The fix I'd like to make here is to allow probes to exist as JS
functions themselves, so that the calling code can directly invoke the
function to fire the probe, avoiding the search by name. In this
situation we could also defer checking the arguments until the probe
is actually fired.

With these changes, the disabled-probe overhead should be reduced to a
JS method call, and then the invocation of the is-enabled stub.

The API change to support this would be to return these probe
functions from provider.addProbe(), rather than calling through
provider.fire(), though that could easily remain for backwards
compatibility.
