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
