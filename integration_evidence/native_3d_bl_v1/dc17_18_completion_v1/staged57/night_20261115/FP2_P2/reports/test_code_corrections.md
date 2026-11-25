# FP2-P2 test-code corrections

- Build671 failed while compiling the hidden fixture because the point aggregate initializers had an extra brace level. A final fixture edit also overlapped source pinning for that build. The logs and original manifest remain in build671. The fixture point initializers were corrected and the complete fixture was frozen before build672. Build672 compiled with zero warnings and the direct fixture passed 3,464 assertions.
