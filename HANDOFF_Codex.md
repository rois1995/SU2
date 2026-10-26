# Codex handoff — current state, 2026-10-26

User requests persistent implementation until DC-17/DC-18 and ensuing defects are
verified closed. Do not end with WIP while independent work remains. Keep the local
[TODO](ADAPTATION_TODO.md) and tracked [progress companion](ADAPTATION_PROGRESS.md)
current. Detailed evidence/history: [curved source/construction](Handbook/DC18_CURVED_SOURCE.md)
and [completion handbook](Handbook/DC17_DC18_COMPLETION_2026_10_25.md).

Target: accepted full M6 Euler then RANS adaptation, general metric/geometry-driven
operators, q>=.20,J>=.05,edge<=1.8,h0 within10%, configured500k metric-complexity
(not tet-count), geometry, conservative transfer and restart. Curvature uses
ADAP_HAUSD in mesh units independently ofh0. No accepted full M6 exists.

ACTIVE: no owned job.54 connected MPI56906 terminal/pass118 cases/rank; serial34415
passes127/1,082,802; all-worker89244 passes11/rank; focused4/31 and build64078 pass.
Source990/runtime19 pins and independently frozen stage receipts verified.
Curved Refine1750+Coarsen225=1975/4096; flat82+23=105. Euler/RANS BDF2 transfer
and SU2/CGNS reload pass.54 implementation checkpoint/push next, then full M6.
Full M6 Euler inputs prepared under54/m6_euler (same verified47 inputs), NOT started:
6ranks,500k metric-complexity,HAUSD1e-4,1800native seconds,outer2400,12GiB/rank.
Run low-priority with one owned job; monitor actual RAM/CPU. Keep C++/binaries pinned.
53 checkpoint4ffe4e507d pushed; fork approval persists. Initial54 build23366 failure
retained (wire priority type); search priority uses supported double, never certifies.
54 carries geometry benefit/cell/edge and metric length/edge before rank using
existing proposal metadata/unsigned reservation priorities at all initial/expanded/
final/contact calls. Duplicate election and independent concurrency remain.

Latest production checkpoint895ab7274a: empty initial FVM ranks fix; four-point sphere
import passes on6 ranks.4933586627 fixed optimizer work accounting;47 serial125
cases/1,082,524 assertions pass.48 MPI6 passes114/116 cases/rank: cubic runtime
work4096 and collapse-filter counter fail. All-worker48 affine stagnates with1 bad
cell; cubic exhausts4096.49 diagnostic build95165/probe6364 terminal, pins verified:
whole-mesh topology cannot fix affine last cell; one insertion/flip can. Cubic45bad
->34 topology, ->88 full-interior/boundary at8-sweep cap, normalizedminimum0.0394.
No diagnostic output is an accepted mesh. No full M6 run since43 (rejected1800s).

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
