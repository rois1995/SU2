## G5 controller startup recovery (R3)

The first controller exited before launching the native workflow. Its deferred
build-verifier import resolved to FP3-E because the inherited collector had
added that directory to Python's search path. The journal records the missing
FP3-E build681 receipt. The service had MainPID 0 and there was no DEF1 S1 case.

The DEF1 controller now imports the local verifier before loading the inherited
helpers. The offline launch self-check authenticates build681 as well as the
generated workflow and restart controllers. Solver inputs, pins and gates were
untouched.

The section-3 seal initially expected an empty list of hash differences. The
inherited comparator emits a map keyed by payload name and returned an empty
map for the successful comparison. Corrected the seal reader and reused the
complete G5 and restart evidence.
