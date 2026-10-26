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

ACTIVE: no owned job.51 build10434/focused14145/all-worker24928 terminal; pins verify.
Focused16 cases/2532 assertions pass,23.09s; serial cubic0 invalid,work1504/4096.
All-worker6 flat Euler/RANS and collapse-filter cases pass. Cubic rejects at4093work:
145cells,all q/J pass,17oversized-edge cells,12geometry cells,7both. No M6 started.
NEXT52: geometric demands currently lose to lower-rank metric demands in dependency
reservation and peer clearance, unlike serial geometry-first selection. Propagate
geometry-first conflict priority through initial, expanded/final reservations and
peer clearance; preserve disjoint concurrency, version/ownership checks and all
current-mesh/exact-contact vetoes. Reverse-priority tests for dependency expansion
and peer overlap/obstacles. Qualify all6 curved runtime, then broader suites/M6.
Preserve51 evidence and code checkpoint before modifying pinned sources.

Latest production checkpoint895ab7274a: empty initial FVM ranks fix; four-point sphere
import passes on6 ranks.4933586627 fixed optimizer work accounting;47 serial125
cases/1,082,524 assertions pass.48 MPI6 passes114/116 cases/rank: cubic runtime
work4096 and collapse-filter counter fail. All-worker48 affine stagnates with1 bad
cell; cubic exhausts4096.49 diagnostic build95165/probe6364 terminal, pins verified:
whole-mesh topology cannot fix affine last cell; one insertion/flip can. Cubic45bad
->34 topology, ->88 full-interior/boundary at8-sweep cap, normalizedminimum0.0394.
No diagnostic output is an accepted mesh. No full M6 run since43 (rejected1800s).

50 connected implementation checkpoint7113fd984d (WIP;51 correction uncommitted): Runtime Absolute shape policy;
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

Remote push was rejected by automatic approval review; destination approval remains
pending. Do not retry remote export without approval. Local checkpoints authorized.
Local checkpoint retry after review timeout succeeded (895ab7274a); no local block.
TODO intentionally untracked; progress companion tracked. Leave unrelated dc11/dc15
logs and untracked planning files alone. Git status needs --ignore-submodules.
[AGENTS.md](AGENTS.md) holds permanent cadence/architecture/resource instructions.
