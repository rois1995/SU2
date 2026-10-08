# Native adaptation partition and execution goal

Activated by the user on 2026-10-08; replaces the previous frozen-target MPI performance goal as the working objective. Previous findings, binaries and evidence remain preserved.

Worktree: `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`.
Branch: `codex/native-unsteady-performance`. Leave AdapNoExt and completed branches untouched.

Make native MPI adaptation use an efficient partition independently of the CFD partition, and establish when fewer adaptation ranks reduce total cost.

1. Resolve old/current fixed-mesh CFD timing differences with reproducible, contention-aware controls.
2. Accelerate immutable donor discovery by reusing a local spatial index, preserving exact coverage checks, canonical ordering and interpolation semantics.
3. Introduce adaptation-cost weighted working-mesh partitioning, initially with M=N. Measure partitioning and migration costs as well as imbalance.
4. Support explicit M<=N through a separate adaptation communicator. Keep accepted CFD mesh, solution and time histories on N ranks. Return an accepted mesh to N ranks and transfer directly from the original CFD donor. Preserve collective failure handling and atomic rejection. Defer automatic rank selection until measurements justify it.
5. Validate frozen and actual unsteady RAE Euler-to-BL and BL-to-Euler, existing small lifecycle controls, MPI consistency, restart, conservation, positivity and histories. Investigate transaction scheduling only if profiling establishes it as the remaining dominant cost.

Preserve original-P1 sensor-only donor interpolation, geometric BL at every actual query including private/remote candidates, finer sensor demands, current fade and ADAP_HESSIAN_NOISE=0. Preserve quality, length, reference geometry and first-height gates. Do not smooth or replace the frozen target to make remeshing easier.

Completion requires validated M=N and M<N paths; reproducible complete-cost comparisons including partition/migration/remesh/validation/return/transfer; demonstrated total-cost improvement for the recommended mode; practical scaling/memory limits; passing rejection/restart checks; updated handoff/reports and inspectable grids/solutions. Different partitions may produce different meshes/work: distinguish workload comparisons from strict fixed-work speedups. Retain regressions and rejected experiments. No native 3D or general affordability claim.

Machine policy: one heavy job, MPI ranks<=4, numerical libraries one thread, builds -j2. Inspect and record contention before/during runs; defer launches while competing heavy jobs are active. Do not terminate unrelated processes. Do not hash or reorganize large binary archives during timing measurements.

Tracker ACTIVE: the user cleared the superseded goal on 2026-10-08; create_goal then successfully activated this objective. The old evidence is preserved. No budget was requested.
