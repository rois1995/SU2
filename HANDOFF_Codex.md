# Codex handoff — current state, 2026-10-26

User requests persistent implementation until DC-17/DC-18 and ensuing defects are
verified closed. Do not end with WIP while independent work remains. Keep the local
[TODO](ADAPTATION_TODO.md) and tracked [progress companion](ADAPTATION_PROGRESS.md)
current. Detailed evidence/history: [curved source/construction](Handbook/DC18_CURVED_SOURCE.md)
and [completion handbook](Handbook/DC17_DC18_COMPLETION_2026_10_25.md).
Latest user-requested [MPI audit/paper assessment](Handbook/MPI_AUDIT_AND_LOSEILLE_ASSESSMENT.md)
corrects the regeneration premise: separate weighted tetrahedral partitioning is
already implemented/enabled. Paper was read previously; mismatch-based work and
hierarchical interface recovery need evaluation, not blind adoption. No new run/code.

Target: accepted full M6 Euler then RANS adaptation, general metric/geometry-driven
operators, q>=.20,J>=.05,edge<=1.8,h0 within10%, configured500k metric-complexity
(not tet-count), geometry, conservative transfer and restart. Curvature uses
ADAP_HAUSD in mesh units independently ofh0. No accepted full M6 exists.

ACTIVE: no owned build/run remains live. Build96356 and offline7/session83140 are
terminal/pass, pins verified. The captured M6 cavity now has an accepted LOCAL
replacement: move only the authoritative split parameter to.8125; q=.24412,J=.05570,
max edge1.59836; curvature, prescribed interface and full-current-mesh contact pass.
Independent q/J/source/orientation check agrees. No full M6 acceptance.

LATEST USER STEERING: validity-first construction then metric/BL-aware optimization
was ALREADY an agreed ledger decision. User reaffirmed it and explicitly permits
physical boundary-node motion; do not continue adding strict-construction fallbacks.
Follow [connected scope](Handbook/STAGED_NATIVE_ADAPTATION.md). Reuse distributed
Adapt/Repair; it already supports validity-only interior repair and strict completion.
RefineOptimize is serial; RegionFieldService is rank0-driven. Production still
forces Absolute. Boundary geometry currently protects original mesh-chart edges/
vertices beyond genuine features; smooth cross-chart mobility and its geometry/source
certificates are part of the gap, not license to weaken the authority.

Active implementation is unvalidated after a diagnosed M6 quality-stage failure.
Build2 passed; offline1 made0 moves in24 trials. The debugger traced this to a
monotonic surface-error veto, not a budget: quality-improving points increased the
already nonzero approximation-error deficit. The connected revision now allows
that deficit to return to refinement, scores first-height/footprint deficits with
quality, proposes requested heights, and rejects partial geometry measurements or
non-positive curved clearance. Production uses validity-only shape admission;
strict final census/height/geometry/publication checks remain. Feature-keyed
concurrent repair and boundary parameter propagation are implemented but unvalidated.
Next: build, saved M6 quality-stage check, connected serial/MPI/transfer checks,
then complete difficult-target qualification. No claim of DC closure.
Details and receipts: [staged implementation](Handbook/STAGED_NATIVE_ADAPTATION.md).

Latest pushed checkpoint d9e840ba35 preserves the accepted LOCAL boundary-placement
witness and staged direction. All26 original calls/11 shape refusals replay exactly;
this is diagnosis, not full-mesh acceptance. Details:
[transaction replay](Handbook/M6_TRANSACTION_REPLAY.md) and
[staged implementation](Handbook/STAGED_NATIVE_ADAPTATION.md).
Large303,428-cell capture stays local with hashes. Keep C++ pinned during jobs.

Last complete baseline qualification was validation54: connected serial127 cases,
MPI6 118 cases/rank and all-worker11 cases/rank (flat/cubic Euler/RANS BDF2 transfer
and SU2/CGNS reload). Full M6 Euler6 still REJECTED at1805s native time:
427,852 private cells,235,717 invalid; original CFD state retained. These receipts
predate the active changes and do not validate them. Preserve the detailed
[validation47–54 history](Handbook/VALIDATION47_54_HANDOFF_HISTORY.md) and
[independent-review qualifications](Handbook/M6_REVIEW_RECONCILIATION.md).

Other open scope:21/9,393 changing-frame gradation violations (worst1.406); M6
normal-only budget profiles exceed500k (best20 profile0 821,265+/-53,688; normal-cap21
rejected). Tangential growth outside protected band remains an unanswered question;
normal-only contract still binds.46 hidden-sphere integration passes positivity and
second moments under unchanged caps; whole-field normalization remains open.
Curved BL support/chord/quadrature consumers, FIRST_ROW surface-error demands,
fitted-plane seams and automatic curved initialization remain. Keep separate BL
refusal until its consumers are complete.32–34 full M6 original-source/field coverage
passes450,221 source cells/all48,954facets/293,724directedchords, serial/MPI2 identity.

Resource rules: user authorizes up to6 MPI ranks. Check actual host RAM/load and live
handles; one owned heavy job,nice19,single-threaded libraries,ninja-j1. Ninja is
/home/rausa/Software/su2-develop/ninja; TMPDIR must be4TB
integration_evidence/build-integrated-v2/compile-tmp. Preserve terminal receipts and
source/runtime/input hashes. Build initial50 log/pins preserved separately.

User explicitly approved pushes to rois1995/SU2 branch codex/native-3d-bl.
Approval persists; no destination question is needed. Latest pushed d9e840ba35.
TODO intentionally untracked; progress companion tracked. Leave unrelated dc11/dc15
logs and untracked planning files alone. Git status needs --ignore-submodules.
[AGENTS.md](AGENTS.md) holds permanent cadence/architecture/resource instructions.
