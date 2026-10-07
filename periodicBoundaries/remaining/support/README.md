# Reject unsupported periodic solver combinations

Develop control: `6db10127d1`, production sources unchanged; testcase additions are in `../validation/develop-reproducer.patch`. Fixed source: the branch listed in `../README.md`. Complete runnable unit source is under `../validation/tests/wt_support/UnitTests/`. BOX fixtures generate their meshes.

After building SU2 with tests, run `OMP_NUM_THREADS=1 timeout 180 build/UnitTests/test_driver "[Periodic]"`, then `OMP_NUM_THREADS=1 timeout 180 mpirun -np 2 build/UnitTests/test_driver "[Periodic]"`. Use `OMP_NUM_THREADS=2` with an OpenMP build for the hybrid check. Support cases use `UnitTests/Common/check_periodic_support.py`.

The shared pass logs cover the combined integration checkpoint, not separate builds of every delivery branch. The wall-distance branch has separate pass logs when available and is excluded from that combined checkpoint. See the PR testcase comment for exact before/after numbers and remaining limits.
