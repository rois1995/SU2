# SST Newton globalization cases

These manual integration cases use `n0012_225-65.su2` from the SU2 TestCases repository's `rans/naca0012` directory. Mesh SHA-256: `20dfc96875b1d1a8377c2e64248b6ee42a1ecf525c50d822fb5f539d06d68d15`.

Copy one configuration and the mesh into a new working directory and run the built `SU2_CFD` executable, for example with `OMP_NUM_THREADS=1 mpirun -n 2 SU2_CFD run.cfg`.

- `naca0012_sst_newton.cfg`: full controller, implicit pseudo-time merit, adaptive forcing and CFL, updating limiter and turbulence MUSCL. Budget: 1000 iterations. Inspect convergence separately from process success; reaching the iteration budget is not convergence.
- `naca0012_sst_failure.cfg`: deliberately restricts the linear solve to one iteration and a maximum true defect of 0.001, backtracking to one trial and recovery to zero retries, with steady merit and CFL fixed at 100. This exercises controlled failure and a failure restart even with `OUTPUT_FILES= NONE`. Expected outcome is nonzero exit, a `Numerical Failure` message, no MPI abort, and `restart_flow_sst_newton_failure.dat`.

The failure case tests failure handling rather than convergence. Its failed step must have zero nodal rollback error and a restored merit within tolerance. Load the resulting restart with `RESTART_SOL= YES` and a local `SOLUTION_FILENAME` to check that the checkpoint remains readable. Full option semantics and numerical limitations are documented in [SSTNewtonGlobalization.md](../../../Docs/SSTNewtonGlobalization.md).
