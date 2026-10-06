# Codex handoff — native integration

Updated 2026-10-06. Goal active; see NATIVE_INTEGRATION_GOAL.md.
Branch codex/native-integrated. Worktree /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
AdapNoExt remains feat_adap_noExt at 88828474db; its .gitignore change is preserved.
Do not write new artifacts into that checkout or alter other workers' branches.

Pinned branches have been reconciled; see BRANCH_RECONCILIATION.md. Stage G
uncommitted fixes were copied from an assessed, hashed snapshot, without
modifying the Stage G worktree. B0 remains standalone comparison research.

Previous completed native baseline: 57f0550db4; original evidence remains
read-only in AdapNoExt/BL_NATIVE_INTEGRATION_WORK. Do not reuse those passes
as proof of this integrated build. New evidence/builds live in this worktree.

Next: fresh primal MPI/MMG/CGNS build, sequential focused matrix, independent
mesh audits; then AD Stage G controls and controlled robustness/scaling sweeps.
One heavy job at a time; -j2, MPI <=4, OMP/OPENBLAS threads1. Never stop foreign
jobs. Check live process before restarting a supervisor; a tool observation
timeout is not job termination. /tmp has about7.4GB free, 4TB disk67GB at start.

Dependency symlinks externals/{codi,eigen,medi,mel,meson} point read-only to
installed main-checkout sources; leave them unstaged. Ninja is in the prior
scratchpad/ninjabin directory. Static MMG needs mmg_scotch_root=/home/rausa/Software/scotch.

Current job: integration_evidence/run_primal_chain_v2.py; live state in
integration_evidence/primal_chain_v2.json. Version1 stopped at configuration
because the fresh worktree lacked the preconfigure stamp; dependencies were
then verified/materialized locally and v2 uses a fresh build directory.
Never restart without checking terminal state and the actual live PID.
The chain builds primal MPI/MMG/CGNS at -j2, runs focused native/output and
memory/default-MMG controls sequentially at1/2/4, independently audits
outputs, then runs a bounded12-job engine scaling pilot (16–1024 input cells).

Current validation state: fresh integrated build underway. No scaling limits
have yet been established for the reconciled branch.

Scaling probe: hidden [NativeScaling2D], with SU2_NATIVE_SCALING_TILES (1..256),
LAYOUT (1 cyclic,2 contiguous), AR (1..1000) and MATCHED (1 default: geometry
and h0 scale consistently with the tensor,2: fixed geometry/h0 to expose
incompatible requests). The isotropic-coordinate scaling maintains equal
metric-space difficulty across ARs. For the constant diagonal target of this
probe, metric_height >1.8 proves an altitude/edge contract incompatibility;
record this separately from a feasible construction failure. Pilot AR10 has
metric_height1. Timings/traffic stop before the audit gather. VmHWM includes
process startup and replicated synthetic input; it is not production RSS or
a proof of the2MiB transaction-work bound. Future sweeps need repetitions and
contiguous ownership controls before claiming speedup or an envelope.
Fixture flags/matched-anisotropy controls were completed while its object did
not yet exist; no production source changed during the active build. Initial
chain source_revision is25c6ccb420; runtime runner captures the final tested
revision and full relevant source hashes. Preserve both source manifests.

Queued follow-up: integration_evidence/run_extended_chain_v1.py (host PID3988784),
state extended_chain_v1.json. It waits for successful terminal primal state,
then independently audits the engine pilot, validates custom sensors/two-pass
and adjoint transfer, and builds AD separately at-j2 before GoalSwap1/2/4.
No heavy work runs concurrently. Failure of either chain stops subsequent work.
The full goal still requires repeated/contiguous-ownership and matched-AR
sweeps, fresh real-airfoil audits/transfer/cost limits, actual goal-loop and
interruption checks, and the final quantitative report. See the predeclared
ROBUSTNESS_SCALING_PROTOCOL.md. Do not mark the goal complete after a pilot.

Update: fresh primal build passed. Initial focused matrix passed atMPI1
(7611 assertions/49cases), then failed five inherited output tests atMPI2:
the old ReadMesh helper compared each rank's local owned-point count with
the global input count (22/23 versus45). All44 other selected cases passed.
Both v2 primal and v1 extended supervisors are terminal failed; no AD/scaling
jobs ran. Fixed the output test harness in6ce2cf34d4: gather/deduplicate global
point/element/physical face records, validate halo agreement, synchronize
file creation/deletion, and enable previously serial-only mixed/order tests.
New sole chain: run_primal_chain_v3.py (incremental test rebuild, full fresh
matrix, memory/default-MMG/audits, scaling_pilot_v3). Follow-up
run_extended_chain_v2.py waits for it and also includes GoalMetric controls.
Preserve failed v1 matrix and original build logs; do not call them passing.

Update: v3 incremental rebuild stopped on a test-only private-member access
and Catch assertion syntax. Neither v3 nor extended v2 ran runtime jobs.
Corrected ReadMesh to use the public file-reader connectivity for boundary
order and existing CPhysicalGeometry reader constructor; no production API
change. Latest sole chain: run_primal_chain_v4.py; dependent extended v3.
Both preserve old failed evidence; build-integrated-v2 is reused incrementally.

Update: v4 runtime identified a second test-harness error: SU2 readers
replicate boundary rows across ranks, unlike CGNS. Gathering every copy
doubled the expected boundary rows at MPI2. Use only the master's file rows
for the independent boundary-order comparison. Native tests otherwise pass.
v4/extended v3 stopped. Sole chain v5 first runs output-only1/2/4, then
the full matrix and pilot; extended v4 waits for successful primal v5.

Update: output-only v5 passes1/2. Four ranks exposed the tiny mixed fixture's
empty CFD sparse pattern (six input vertices/three cells). Expanded the same
triangle/quad/triangle ordering motif to45vertices/48cells rather than
changing solver empty-partition handling. Record the too-small-partition
abort as an existing CFD constraint, not a mesher scaling result. v5 and
extended v4 terminal. Sole latest chain v6; dependent extended v5.

Prepared robustness_chain_v1 (waiting for successful extended v5): three
repetitions for both ownership layouts at the largest common complete pilot
size, matched AR10/100/1000, proved-incompatible AR25, independent raw audits,
real-airfoil baseline1/2/4 plus one-axis demand escalations. Independent
opposing-wall audits reuse the fresh integrated matrix outputs. This is
queued work, not passed evidence; check JSON state before restarting.
