true && echo and-runs
false && echo should-not-print
false || echo or-runs
true || echo should-not-print
false && echo no || echo fallback
true && echo first && echo second
