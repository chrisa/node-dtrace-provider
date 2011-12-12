# dtrace-provider - TODO

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

Even with the probe objects available from 0.0.4, there's still quite
a lot going on. 
