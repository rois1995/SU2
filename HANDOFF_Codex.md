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

No new production placement fallback has been implemented. Only the lost-rejection
message fix is changed production behavior (MPI2 passes5 tests/rank). Test-only
probes/replay establish the diagnosis. All26 original calls/11 shape refusals match.
Details: [transaction replay](Handbook/M6_TRANSACTION_REPLAY.md). Latest pushed
checkpoint1a5745fe3e; accepted witness and revised direction being saved next.
Large303,428-cell capture stays local with hashes. No new M6 prefix or1800s rerun
until the connected staged implementation is ready. Keep C++ pinned during jobs.

54 full M6 Euler6 session64848 terminal (driver0, native REJECTED).
Native1805.12s, whole1980.41s, largest process736,780KiB (not whole-job peak).
24,972 private edits/31,133 attempts;427,852 cells;235,717 cached invalid;
235,227 queued edges;76,739 shape refusals;2113 optimizer calls/115 validated;
max6 simultaneous commits. Original CFD mesh/solution retained, no transfer accepted.
Source990/runtime19,81 loaded MPI/library pins, inputs and connected receipts verify.
54 connected qualification remains passed: serial127/1,082,802; MPI6 118/rank;
all-worker11/rank incl cubic Euler/RANS BDF2+SU2/CGNS reload; focused4/31.
Implementation e9263060cf and docs68621318f5 pushed to approved fork/branch.
Independent review of f5e9456f06 reconciled:
[findings and implementation qualifications](Handbook/M6_REVIEW_RECONCILIATION.md).
Witness table now separates failed-cell q/J from patch minima; MPI1.428 is only a
hypothetical accounting quotient. That review itself established no production numerical defect.
Do not add local fallbacks or raise limits blindly. M6 restart_adap_00000.dat
contains exact original 61,330-node sensor metric (Metric_XX..ZZ,23 fields), usable
with existing hidden NativeFrozenVolume3D replay. Verify coordinate/ID correspondence.
Investigate fixed cavity/surface constraints and bad boundary triangles, using
actual frozen M6 field and bounded witnesses; do not substitute toy-case success.
Profile13781 terminal:491 user-CPU samples, about74% inclusive in MPI_Allreduce,
active reconstruction/donor measurement. Decoder91717 interrupted after DWARF errors;
symbol-only reports pass. Raw54/m6_euler/live_profile.data remains LOCAL ONLY.
No raw sampled stacks or large restart binaries added to git; pin their hashes.

Historical checkpoint895ab7274a: empty initial FVM ranks fix; four-point sphere
import passes on6 ranks.4933586627 fixed optimizer work accounting;47 serial125
cases/1,082,524 assertions pass.48 MPI6 passes114/116 cases/rank: cubic runtime
work4096 and collapse-filter counter fail. All-worker48 affine stagnates with1 bad
cell; cubic exhausts4096.49 diagnostic build95165/probe6364 terminal, pins verified:
whole-mesh topology cannot fix affine last cell; one insertion/flip can. Cubic45bad
->34 topology, ->88 full-interior/boundary at8-sweep cap, normalizedminimum0.0394.
No diagnostic output is an accepted mesh. These are historical validations47–49;
the current full-M6 failure is validation54 above.

50 connected implementation checkpoint7113fd984d (WIP;51 correction checkpoint b214ff210b): Runtime Absolute shape policy;
bounded optimizer repair of private candidates before publication, both resident and
streamed paths. Swaps/insertion; no smoothing/boundary operations. Construction
preserves old vertices; repair retains collapse. Candidate parameters/interface,
original-field metrics, height, geometry and progress checks retained.160 extra node
IDs separate from original construction range; actual trial/node guards. Reserve
work only for optimizer phase, refund unused immediately; cumulative160/proposal and
global limits unchanged. Collapse-filter fixture retains its two-owner local witness
layout while all ranks participate; every original counter/mesh assertion retained.
49 adds test-only complete rejected-cell dumps and optional bounded probe using
SU2_NATIVE_MAPPED_REPAIR_PROBE with SU2_NATIVE_MAPPED_ALL_WORKERS=1. Preserve its
failure evidence. Do not introduce further late fallbacks or raise limits blindly.

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

User explicitly approved push to rois1995/SU2 branch codex/native-3d-bl on2026-10-26.
Prior destination approval block is resolved; push21357 terminal/pass through c093abefbd. Remote updated ca6e3e81bb..c093abefbd.
53 pushed through4ffe4e507d; approval persists for this fork/branch.
Local checkpoint retry after review timeout succeeded (895ab7274a); no local block.
TODO intentionally untracked; progress companion tracked. Leave unrelated dc11/dc15
logs and untracked planning files alone. Git status needs --ignore-submodules.
[AGENTS.md](AGENTS.md) holds permanent cadence/architecture/resource instructions.
