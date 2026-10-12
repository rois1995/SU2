# Joint curved reconstruction — work in progress

Repository: `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`.
Branch: `codex/native-3d-curved-local`. Goal 2 remains active and incomplete.

The new native proposal combines a projected physical-edge split with one new
interior point and reconstruction of its complete volume patch. It preserves the
original oriented artificial interface and changes only the declared physical
surface children. The shared split-point and surface-child helpers retain the
existing finite-feature, marker-junction, projection and identity checks.

Point placement follows the bounded coordinate-search pattern already used in
native 2D, with a three-dimensional metric frame. A tensor sampled from the
unchanged frozen field at an actual old-cell centroid guides placement only.
Every final new cell centroid and edge is then imported and checked against the
actual frozen field; the proxy never becomes an exported tensor or a new target.
The search has at most 9 levels, 3 sweeps and 26 directions per level/sweep.

When a retained interface obstructs reconstruction, the engine imports the
complete neighbouring face dependencies, checks ownership/version consistency,
merges the patch and reserves it again. Growth is bounded by 12 trials, 64 old
cells, 128 replacement cells and the existing admitted memory/global work/time
ceilings. Every additional joint trial counts against global work. Curved commits
remain serialized; no concurrent displaced-cavity reservation claim is made.

Fresh shape/sliver/metric-edge and geometric-progress gates remain unchanged.
Exact oriented-interface/topology checks and current-mesh contact/overlap checks
precede publication. Two node identities are reserved for curved proposals;
planar identity spacing is unchanged. Existing conservative CFD replacement,
new/lost-domain accounting, current/BDF transfer and writer/restart paths are reused.
Recovery now retries reopened curvature candidates as well as oversized metric
edges; previously a geometry-only target could remain in repair mode indefinitely.

## Evidence

`joint_replay_v1` uses the saved primitive_runtime_v7 rejected tilted-cylinder
working mesh. A combined proposal grows from two to six old cells, creates twelve
new cells and reduces its local geometric deficit by about 48%. Both an installed
scientific optimizer and a bounded native-compatible coordinate search find a
proposal. The latter gives minimum q 0.224286/J 0.065229 and maximum metric edge
0.790569. This is a local floating-point proposal, not a complete mesh certificate.

The native regression in CNativeBoundary3D_tests.cpp reproduces that exact saved
six-cell geometry, validates the constant 0.5I actual-query metric, geometric
progress, pairwise exact conformity/no-overlap and rejection rollback. Its small
fixture retains the source replay hash. It passes in joint_runtime_v2 and v3.
The constant diagnostic field is intentionally not a claim about arbitrary P1
fields; full driver cases retain their original/declared-extension source.

`joint_runtime_v1` retains a new-test missing-header compile failure; production
files compiled. `joint_runtime_v2` builds and passes four of five serial cases,
but the tilted-cylinder runtime variant stagnates. `joint_runtime_v3` also passes
four of five serial cases, after enabling reopened curvature-candidate retries;
the tilted cylinder still stagnates after 77 private edits. No accepted CFD state
is published after these failures. MPI2/4 were not launched by those controllers.

`joint_replay_v2` then searches the latest v3 rejected working geometry and finds
no admissible one-interior-point proposal in 792 bounded trials. Its recorded
single-core affinity migration was a contention adjustment, not a timing study.
This failure separates a demonstrated local operator from complete path robustness.

`joint_runtime_v4` tests growth by the limiting normalized quality/sliver ratio,
matching the successful replay selection rather than its summed placement penalty.
Build and four of five serial cases pass; the tilted cylinder still operator-stagnates after 86 private edits. The change improves local progress but does not complete the runtime path. All frozen fixture
tolerances remain sphere 0.22R/cylinder 0.04R, q 0.20, J 0.05, metric edge 1.8 and
work 4096. No source target or acceptance tolerance was relaxed.

## Remaining work

Full tilted-cylinder completion, current-source MPI1/2/4 with M=N/M<N, feature/
component negative controls, independent accepted-state audits and broader native
regression remain unproven. If the v4 trajectory still stalls, use coordinated
surface retriangulation/more than one interior point on the saved obstruction;
an additional single-point growth heuristic is not a demonstrated complete fix.
The actual solved Euler/bump evidence is still at its earlier published source;
refresh it for a qualified integration milestone and add the locally resolved
nonuniform/shock control. Preserve every full Goal 2 requirement.

References inspected locally: Loseille, Alauzet and Menier (2017), Computer-Aided
Design 85, DOI 10.1016/j.cad.2016.09.008, sections 2.3–2.5; native 2D MoveStar;
MeshAdaptation/Scripts/FunScripts/LocalRepair3D_Fun.py and MetricPatchRepair3D_Fun.py.
Their mechanisms inform combined reconstruction and joined validation; their
source/results do not qualify this native implementation. No new production
dependency, general CAD, production 3D BL, performance or cluster claim.


`joint_mpi_audit_v1` runs the unchanged v4 source on MPI2/4, continuing to retain
nonzero matrix exits instead of stopping observation at the known serial failure.
Controller session 75064 is active; inspect its process/receipt before launching
anything else. Neither a launched MPI job nor a passing subset proves the full
matrix. Further full-path work should change coordinated reconstruction freedom,
not increase caps or repeat small candidate-order adjustments.


MPI observation is terminal FAIL_MATRIX (MPI2 exit2/MPI4 exit4), with no timeout
or source drift. Four of five selected cases pass per rank; every tilted-cylinder
worker-count variant still rejects. Four N2/eight N4 other primitive lifecycles
complete; their independent current-source accepted-state audits remain required.
Captured owned-rank stacks show different in-flight phases; the run then completed,
so no deadlock was established. No extra simultaneous correctness job was launched.
The two-apex experiment is now active under handle43705 in two_apex_replay_v2.
Its initial nonmanifold intermediate-skin assertion failure is retained in v1.


The corrected two-apex experiment ended normally with no admissible proposal in
3074 bounded candidates. It tests a limited single-face/adjacent-pair partition
family, not all possible multi-point reconstructions and not global infeasibility.
Its initial nonmanifold intermediate-patch failure and corrected result are
preserved. The next major step is coordinated surface diagonal/fan reconnection
and volume reconstruction, retaining all immutable feature/target/atomic guards.
All jobs in this continuation are now terminal; production code unchanged.
