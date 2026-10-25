# Codex handoff — current state, 2026-10-25

Current request: implement the missing BL-to-outer metric transition and finish
DC-17 and DC-18. Keep [ADAPTATION_TODO.md](ADAPTATION_TODO.md) current.
[Active completion work](Handbook/DC17_DC18_COMPLETION_2026_10_25.md) records scope,
evidence, acceptance and current work; the reviewed baseline below remains valid
only for its pinned source. No new capability is yet declared complete.
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
Current checkpoint: `fe9bb2d30e` (local). Shared Option A normalization,
construction, reassessment and original-family correction now use the same field;
logarithmic blending preserves finer sensor directions. Registered-column cavities
resolve the last two M6 first-height failures (validation11), but that diagnostic
mesh still fails final quality/edge/budget gates. No accepted full M6 exists.

The native memory ceiling, final publication-budget check and lost query-error
diagnostic are fixed. The six historical distributed-repair assertions above are
resolved. Validation19 passes1,658 assertions/12 serial cases,11 MPI2 cases/rank
and7 invalid-configuration checks, with source/runtime provenance.
Normalized M6 event1 passes12GiB admission, then rejects all nine profiles before
construction (814k at0 increasing to2.37M at1, budget500k). Session26009 is terminal.

Validation20 isolates the uniform-sensor field cost at32,768 samples: profile0
821,265 +/-53,688; profile1 2,298,938 +/-198,670. Profile1 contributes1,876,743
beyond10T. All15 completed estimates reconstruct from query receipts and direct/
cached values agree. This diagnoses excessive continuation from a coarse normal
with fine tangents; it does not prove infeasibility. Validation21 rejects a diagnostic normal-spacing cap: best sampled1,182,909
+/-67,994, still over budget. The normal-only field contract remains in force; an
asynchronous tangential-policy clarification is pending. Validation22 is terminal: curved mapped controls, strict bounds, v7 persistence
and source/child error plumbing pass959 assertions/11 cases in serial and on each
of2 MPI ranks, plus all10 standalone groups. Full M6 admits48,954 patches with
zero fallbacks and a bit-identical46.6MB sidecar round trip. No volume mesh was
constructed. Next: curved field queries, clearance, refinement and initialization
as one connected change; no automatic curved switch is enabled.
Full-field gradation, final accepted mesh, curved integration and transfer/restart
remain open; see the active handbook for scope, live handles and detailed receipts.

Automatic approval review blocked pushing to origin; destination approval remains
pending. Do not retry remote export without it. Unrelated dc11/dc15 campaign logs
remain outside our checkpoints.

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
