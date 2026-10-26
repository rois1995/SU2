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

ACTIVE: build9/session76705 is running; all sources/helpers frozen. It links both
test_driver and SU2_CFD. Build8/session57428 passed (396.46s), all933
pins unchanged. Offline3/session76089 passes the saved M6 LOCAL repair:1 move,
12 trials, q=.2446273,J=.0532005,edge=1.5983623; prescribed interface/contact and
metric gates pass. Surface approximation remains provisional debt; not a full mesh.
Serial/session77808 passes23/24 cases. Only the new multi-star quality-pass fixture
hits its112-round cap before all stars finish. Direct height recovery and provisional
height admission pass. The current revision restores Coarsen's short-edge sweep
after productive quality passes and fixes that test allowance (production unchanged).
Then build both test_driver and SU2_CFD, run focused distributed checks and full M6.
All failures/receipts remain in staged57 and its linked handbook.
External-SSD reference files are copied and hash-verified locally; see below.

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

Latest pushed checkpoint36de990633 saves the unvalidated explicit construction/improvement stages;
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
Approval persists; no destination question is needed. Latest pushed36de990633 (WIP); build9/session76705 is active; prior owned handles are terminal.
TODO intentionally untracked; progress companion tracked. Leave unrelated dc11/dc15
logs and untracked planning files alone. Git status needs --ignore-submodules.
[AGENTS.md](AGENTS.md) holds permanent cadence/architecture/resource instructions.
[Adaptation reference inputs](Handbook/ADAPTATION_REFERENCE_INPUTS.md) are archived locally; current inputs no longer require the external SSD.
