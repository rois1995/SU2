# Initialized RANS/SA production workflow checks

The current mesh-generation engine has independent stronger-field MPI1/2/4 evidence.
This folder separately checks actual initialized RANS/SA current+BDF transfer,
strict bounds, sensor-only transport, SU2/CGNS/reference reload and restart. It does
not run CFD iterations or establish viscous accuracy/performance.

The preceding complete one/two-rank results remain preserved. The four-rank combined
run was interrupted with wrapper143 before a completion receipt; it is not PASS.

The collector now accepts an optional test-only worker count. Each combination has
its own folder and native/audit receipts, so a completed combination need not repeat:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 integration_evidence/native_3d_bl_v1/production_policy_v1/run.py 1
PYTHONDONTWRITEBYTECODE=1 python3 integration_evidence/native_3d_bl_v1/production_policy_v1/audit.py 1
PYTHONDONTWRITEBYTECODE=1 python3 integration_evidence/native_3d_bl_v1/production_policy_v1/run.py 4 1
PYTHONDONTWRITEBYTECODE=1 python3 integration_evidence/native_3d_bl_v1/production_policy_v1/audit.py 4 1
PYTHONDONTWRITEBYTECODE=1 python3 integration_evidence/native_3d_bl_v1/production_policy_v1/run.py 4 2
PYTHONDONTWRITEBYTECODE=1 python3 integration_evidence/native_3d_bl_v1/production_policy_v1/audit.py 4 2
PYTHONDONTWRITEBYTECODE=1 python3 integration_evidence/native_3d_bl_v1/production_policy_v1/run.py 4 4
PYTHONDONTWRITEBYTECODE=1 python3 integration_evidence/native_3d_bl_v1/production_policy_v1/audit.py 4 4
```

Run sequentially after the fresh test build. `mpi4_m1`, `mpi4_m2`, `mpi4_m4` each
require both adaptation frames and the actual restart. The complete four-rank
matrix requires all three independent audits, not just one selected case. Without
the second argument the original combined stage still runs. The selector affects
only the test fixture, not production options or remeshing.

Exact PASS reuse requires source/build/binary/object/command/environment/artifact
pins. RUNNING records save execution intent before launch; verify actual live
process/session handles before restarting. Sources/binary must remain unchanged
during each stage. The new selector's focused build/runtime qualification is pending;
existing receipts describe their actually executed prior test binaries.
