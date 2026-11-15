# Reviewer P6c — 2026-11-15

**Checks1–5 pass; full workflow pending. No accepted mesh claimed.**
R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`; D=`R/purpose_query_fix_20261114/p6c_20261115`.
Implements section1.7 only: shared complete-distance body, one completion per queued node, accounted queue storage,
upper_deferred_bounds and first8per-process one-sided guard records. No consumer/budget/quota/gate changes.
Build498570.142s, all1539source/build inputs unchanged; CFD SHA9c06aec181c662c8714d7e142e1802ba39468980eb6d8d008dd406a3e53d70f0.

| Check | Result |
|---|---|
| Baseline scan on unchanged P6 |31gaps0.85–1.00tol, zero refinements;61.483s. Does not reproduce M6; no new fixture invented. |
| 1 affine/backend/entity units + same scan | PASS:77cases/8710assertions,210.677s. Scan zero refinements/deferred bounds; does not exercise new path. |
| 2 first collapse | PASS:90809869work exactly equals P6;480/473/346/263/469/464 exactly; zero refinements all6ranks. |
| 3 same run to construction_entry | PASS:all6rows; zero refinements, required deferred completions observed. |
| 4 C492 BL repeat | PASS:336splits,304663positive private tets, zero refinements; outer566.603s. |
| 5 build481 + native2D1/2/4 | PASS:56cases/5940assertions149.628s; saved-current1case/611527assertions175.501s;2D39cases/rank6.503/6.486/6.518s. |
| 6 full two-adaptation no-BL | PENDING. |

At the first old failure sites: rank5/gen1 deferred1,refinements0 at297.596s; rank2/gen4 deferred1,refinements0 at413.747s.
This meets the reviewer’s M6 cause-confirmation criterion; the unit scan alone did not. No guard record appeared.
Construction-entry cumulative counters (C/M/L = classification/magnitude/limit work):

| Rank | Elapsed s | Update s | Deferred | Facet work/misses | C/M/L work |
|---|---:|---:|---:|---:|---:|
| 0 | 634.158 | 266.762 | 0 | 339060/13328 | 39925241/25684764/0 |
| 1 | 634.158 | 266.738 | 1 | 247266/13423 | 41926087/25892953/0 |
| 2 | 634.157 | 266.821 | 4 | 221826/7807 | 19333393/13484902/0 |
| 3 | 634.158 | 266.753 | 0 | 243198/8367 | 33477592/25155137/0 |
| 4 | 634.157 | 266.782 | 0 | 256604/12446 | 30822178/22888530/0 |
| 5 | 634.158 | 266.817 | 8 | 338284/14616 | 53731815/33807641/0 |

Collapse max267.270s → construction max634.158s: phase366.887s. Prior P5 phase1952s; build293 phase348s.
Behavior/workloads differ after collapse; these are milestone seconds with contention, not an equal-work speed claim.
Rank0facet339060/13328=25.439work/miss (build293339492/13332=25.465); no refined-model cascade observed.
Update/geometry/collective timers overlap; no exclusive attribution claimed. Full per-purpose calls/work and phase deltas:
D/nobl_to_construction/construction_assessment.json; stage rows and rank logs remain raw.
Prescribed stop at all6construction-entry rows: intentional -15, outer750.796s; all9owned identities terminal.
Inputs/source/binary/runtime pins unchanged. Load1m3.96–10.37; sampled concurrent solver RSS max7066300KiB, not whole-job peak.
Baseline harness compile first failed10.145s on missing <fstream>/<iomanip>; corrected test-only includes, build30.137s.
No numerical repair was attempted; baseline scan and each P6c check were run once. Failed compiler receipt retained.
Cheap-group B1/B2/B3 drafts remain set aside. F1 build494 count reference stays until check6 passes.
BL first differing edit from RunA: index59, oldedge24251/24259 → newedge24259/24433 (full points in p6_bl_receipt.json).
BL remains surface_prepared=false; same388.8sconfigured budget/target/input, output identity not required by P6.
Legacy496 case labels for the481checks execute build498 (binary/source pins authoritative); prior current-target port reused, no pin edited.
Next: check6 once; stop on a rejected adaptation or any one-sided refinement guard. No further fix.
