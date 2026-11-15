# Reviewer P6c — completed Euler regression repair, 2026-11-15

**P6c checks1–6 and isolated final-state CFD restart PASS. Curved BL/RANS objective remains open.**
R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`; D=`R/purpose_query_fix_20261114/p6c_20261115`.
Source05501a11d7; CFD SHA9c06aec181c662c8714d7e142e1802ba39468980eb6d8d008dd406a3e53d70f0; build498570.142s/1539pins unchanged.
Implements reviewer1.7 only: complete deferred sample distance once, retain queue accounting, add completion counter/bounded guard records.
No numerical source change after that build; no budget, tolerance, quality, height/progression or metric-complexity gate relaxed.

| Check | Result |
|---|---|
| Baseline scan on P6 |31gaps0.85–1.00tol; zero refinements,61.483s. Did not reproduce M6; no new fixture. |
| 1 affine/backend/entity + extended scan | PASS77cases/8710assertions210.677s. Scan zero refinements/deferred bounds; M6 tests exercise correction. |
| 2 six-rank collapse | PASS90809869work<=108459426, exactlyP6;480/473/346/263/469/464; zero refinements. |
| 3 same run to construction | PASS634.158s, zero refinements all6; deferred1at rank5/gen1 and rank2/gen4 as prescribed. |
| 4 exact-input C492 BL repeat | PASS336splits,304663positive private tets, zero refinements; API390.280s, configured388.8s, outer566.603s. |
| 5 build481/current target + native2D | PASS56cases5940assertions149.628s; saved611527assertions175.501s;2D39cases/rank1/2/4in6.503/6.486/6.518s. |
| 6 full no-BL | PASSboth accepted meshes, strict independent mesh/restart/transfer/continued-CFD gates; all12rank exits present. |
| Standing isolated restart | PASSfinal event2 copied to fresh directory; MPI6/ADAP_LOOP=NO/one further CFD iteration,106.211s. |

Full workflow (stage clocks are nested, not additive with wholeNative):

| Event | Collapse→construction→quality entry→engine exit s | WholeNative s | Actual points/cells |
|---|---|---:|---:|
| 1 |265.358→637.180→674.853→2839.815|2941.587|146676/726442|
| 2 |300.343→346.921→391.515→2070.980|2128.892|168790/855760|

WholeNative sum5070.478s; process wall5493.181s includes CFD/metric/I/O. The unchanged configured Native limit is5400s per event.
Both final q/J/size/geometry bad counts0; complete owned-cell census and shape/geometry certificates on every rank.
One-sided refinements0/0; deferred completions20/0. Source1.7cause confirmed at earlier failure sites.
Conservative transfer true residuals<=1e-10, projected inadmissible states0; both continued CFD/restart states finite/admissible.
Isolated restart coordinates match exactly; minimumrho.626054690, internal-energy density103922.2183; conservative fields change.
Its six actual executing inodes and mapped ELF files are pinned; all10owned identities verified terminal after wrapper exit.
50k is nominal Euler metric complexity, not point count. Geometry/size constraints stay strict; evolved integrated complexity unmeasured.
Full host load1m4.04–10.62; reported per-rank modeled peak7880819872B<12GiB. Sampled concurrent solverRSS13069084KiB;
that sum is not a per-rank bound or physical whole-job peak. Overlapping update/proof/collective timers are not summed as exclusive CPU.
Runtime/source/input/binary pins unchanged; solver exited0. Frozen controller exited1 solely on a publication-log format mismatch:
CSinglezoneDriver.cpp:1815writes "cycle 1/2", checker expected "cycle 00001/00002". Companion read-only correction changes only this label;
filenames remain padded and every physical/resource/transfer gate stays. Raw receipt/source preserved; no numerical rerun.
BL first edit differs at index59 (oldedge24251/24259, new24259/24433); P6explicitly permits changed output. Surface readiness remains false.
Unit scan compile initially missed<fstream>/<iomanip> (10.145s); includes corrected, build30.137s, scan run once. Compiler receipt retained.
Legacy496case labels execute build498; current-target481port retained without editing pins. F1count reference moved only after full success.
D/euler_completion.json seals receipts; D/full_workflow/timing_resource_assessment.json retains all rank/phase/purpose counters.
Accepted mesh/ref/restart payloads remain local with SHA256 manifests; exact target/scheduler continuation is not claimed.
Next connected cheap group needs corrected B4/B6 proposals: see ADAPTATION_M6_CHEAP_GROUP_SOURCE_GAPS_20261115.md.
B1/B2/B3drafts remain set aside; do not independently replace contradicted proposals or start another full run.
