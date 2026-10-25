# Codex handoff — current state, 2026-10-25

Current request: review and fix the native adaptation implementations, maintain
[ADAPTATION_TODO.md](ADAPTATION_TODO.md), then proceed through its ordered roadmap.
Standing target: accepted M6 Euler followed by RANS adaptation, general geometry
and metric driven operators, unchanged final acceptance and transfer gates.

HEAD at review start: `95058150af` (DC-17 c9). The latest full M6 chain has
terminated: event 0 published, event 1 rejected with three first-height failures.
The harness exited zero despite complete=0/written=0. C++ completion assertions
and receipt/output/provenance checks now prevent that false success. DC-18 is a
standalone header and tests, not a production authority. Contract fixes pass all
ten standalone groups in `integration_evidence/native_3d_bl_v1/dc18_review_v1/standalone_final/`.
[Review and remaining integration gates](integration_evidence/native_3d_bl_v1/dc18_review_v1/review.txt).
The existing native analytic metric/refinement/collapse/region paths now use the
geometric tolerance independently of h0. Final build and source/runtime pins verified.
Validation: all ten standalone groups reused with verified provenance; metric,
unit2d, unit3d and FIRST_ROW pipeline stages pass; selected two-rank MPI checks
pass (12 cases per rank). Connected curved checks pass23/24: the baseline c9
hidden-sphere quadrature work limit remains open. mesh3d retains the six already
listed distributed-repair assertions (100/101 cases pass), not a clean suite.
Sphere oracle integer division and stale expected reference policies are fixed.
Quadrature experiments failed unchanged limits and were reverted; see the review.
[Final receipts](integration_evidence/native_3d_bl_v1/dc18_review_v1/final_validation/summary.json).
No owned build/test job remains live. Checkpoint d11d05d9cc and the follow-up are
local; automatic approval review blocked pushing to origin and destination approval
remains pending. Unrelated dc11/dc15 campaign logs were left out of the checkpoints.

User curvature decision: configured geometric error ADAP_HAUSD, independent of h0
and ramp. Use mesh units (1e-4 means 0.1 mm only for metre coordinates). Preserve
sharp features, body restrictions, wall height and all final mesh/transfer gates.
No accepted curved M6 or full coupled adaptation result exists yet.

Host check at start: about 38 GiB RAM available, 89 GiB free on the4TB disk;
no active compiler/test driver/solver. Leave unrelated historical shell waiters
and other worktrees/jobs untouched. One heavy job, nice19, threads1, build-j1,
compile scratch in integration_evidence/build-integrated-v2/compile-tmp.
Git status needs --ignore-submodules because an existing Eigen submodule gitdir
points into a removed scratch worktree; dependencies have not been repaired.

The [October 20 handoff](Handbook/HANDOFF_2026_10_20_ARCHIVE.md) preserves older
Goal3 work, failures and evidence. Its paused status and binary claims describe
that historical campaign, not the current DC-17/DC-18 work. See [AGENTS.md](AGENTS.md)
for cadence, source pinning, resource limits and checkpoint rules.
