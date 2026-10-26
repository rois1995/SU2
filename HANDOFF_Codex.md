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

ACTIVE: build11/session30951 is running; all sources/helpers frozen. Build10 passed
(171.24s,933 pins unchanged).
Broad serial_full/session42727 is terminal:133/138 cases and1,083,966/1,083,996
assertions pass;85.84s. Five failing cases split into metric/accounting/obsolete
strict-construction path assertions and a genuine cubic runtime construction stall.
Cubic Euler/RANS fixture spends4096 work in2348 rounds/613 accepts, leaving572
geometry-demand cells before quality starts. Strict GeometryRefine metric-excess
admission conflicts with independently certified geometric improvement. The connected
fix permits provisional size debt for that intent only under ValidityOnly; final
size/q/J/geometry gates and the4096 workflow budget remain unchanged. Agents own
Metric.cpp/tests and Engine.cpp/Distributed tests respectively. Build11 is the only owned heavy job.
Build9 focused serial25/25 passed, including repair→coarsen completion. Saved M6
offline3 passes LOCAL q/J/interface/contact repair, with surface approximation debt;
no full M6 acceptance. Next build11 then affected runtime+broader qualification.
External-SSD references are copied and hash-verified locally; see below.

LATEST USER STEERING: construction must not measure/rank q/J or require shape
progress. J>=.001 is also a quality gate, not geometric validity. Construction
retains exact positive cells, topology, geometry/body separation and metric size
guidance. Quality/height improvement follows, with all final gates unchanged.
Physical nodes may move on prescribed geometry, retaining genuine features.
Use efficient models for bounded tasks; primary agent owns design/review.

Current work removes centroid shape sampling from construction, makes unavailable
shape explicit, and connects the serial FIRST_ROW constructor to general distributed
Refine/Coarsen before publication. A stalled valid construction can enter quality
improvement with remaining size/geometry deficits; productive repair returns those
deficits to construction. No-progress phase bouncing must reject, not loop.
The saved M6 midpoint test previously made0 moves in24 trials because of a monotonic
surface-error veto, not a budget. The revision permits provisional approximation
error debt while retaining geometry membership/contact and strict final acceptance.
Useful quality passes now amortize censuses; shared internal height admissions
allow measured provisional deficits while retaining geometric clearance.
Curved cross-chart movement and full M6 acceptance
remain open. Follow [connected scope](Handbook/STAGED_NATIVE_ADAPTATION.md).

Latest pushed checkpoint519a4813a2 saves useful quality passes, provisional-height
admission, resumed coarsening and LOCAL saved-M6 evidence; build9 focused serial passes; broad qualification remains pending.
Checkpoint36de990633 saves the explicit construction/improvement stages;
checkpoint6cf0dcedfd saves the earlier connected implementation
and the diagnosed M6 quality-stage refusal; d9e840ba35 holds the earlier LOCAL witness. All26 original calls/11 shape refusals replay exactly;
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
Approval persists; no destination question is needed. Latest pushed519a4813a2 (WIP); build11/session30951 is active; prior owned handles are terminal.
TODO intentionally untracked; progress companion tracked. Leave unrelated dc11/dc15
logs and untracked planning files alone. Git status needs --ignore-submodules.
[AGENTS.md](AGENTS.md) holds permanent cadence/architecture/resource instructions.
[Adaptation reference inputs](Handbook/ADAPTATION_REFERENCE_INPUTS.md) are archived locally; current inputs no longer require the external SSD.
