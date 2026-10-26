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

ACTIVE: no owned job.53 build5012 and all-worker21166 terminal; pins verified.
53 MPI6 passes9/10 cases/rank incl one-budget duplicate-edge regression. Curved
Refine completes with4066work; Coarsen exhausts remaining30. Rejected209-cell
mesh has0 shape/size/geometry violations; transfer/reload not reached. No M6.
52 checkpoint2a214b1058; pushed through c093abefbd.53 checkpoint pending.
NEXT54: complete geometry-first ordering through existing coalescing metadata:
geometry score/cell/edge and metric length/edge before owner rank. Convert sorted
keys to reservation priority for every initial/expanded/final/contact call; avoid
another public API. Preserve existing duplicate collapse semantics, concurrency,
all gates and documented Coarsen exhaustion contract. Add shared-box regression
where global longest edge occurs off rank0; keep independent-cavity concurrency
and one-budget duplicate tests. Small all6 workflow must pass before broad/M6.

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
53 campaign terminal; local checkpoint/push next.
Local checkpoint retry after review timeout succeeded (895ab7274a); no local block.
TODO intentionally untracked; progress companion tracked. Leave unrelated dc11/dc15
logs and untracked planning files alone. Git status needs --ignore-submodules.
[AGENTS.md](AGENTS.md) holds permanent cadence/architecture/resource instructions.
