# Native adaptation — current handoff

Updated2026-11-07(Europe/Rome). Goal ACTIVE: TWO strictly accepted ONERA M6 Euler
CFD→mesh→CFD→mesh at50k metric complexity, conservative admissible transfer, finite
continued adapted CFD and isolated ACTUAL adapted disk restart. Then dedicated
BL/constraint/MPI qualification and corresponding RANS500k. ZERO accepted meshes.
Actual point/cell counts are separate from metric complexity. Fixtures are not completion.

Keep geometry1e-4,q>=.2,J>=.05,max metric edge1.8,requested heights strict. Construction
remains validity-first: exact positive/nondegenerate/topology/body/feature/height
validity, NO fresh q/J measurement/ranking/veto. CAD optional; no-CAD sufficient.
One connected cycle/ONE owned heavy job. Freeze C++/fixtures/helpers/binary/config/
inputs through builds/validation. Verify exact HOST handles before edits/restarts.
Preserve evidence before an authorized stop/fix/retry. Do not repeat historical stops.

R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
A237=`R/m6_runs/euler_build237_signed_bounds_real_cfd50k`.
A235=`R/m6_runs/euler_build235_geometry_coupled_real_cfd50k`.

## LIVE237 — full same-seed two-event M6, source FROZEN

Production source checkpoint4d1e2ec5b0, pushed. Latest measured-entry/first-window
checkpoint179f299dfd pushed; earlier startup32d4873423/construction4a3376d903.
Full tool session2688 is ACTIVE. Native5400/outer14400, unchanged gates/target/seed.
R/m6_237_launch.json records helper2861865/start7510148, runner2861866/start7510152,
qualification SHA/resources/scratch. A237/owned_startup.json records exact live tree:
GNUtime2862080/start7510715,timeout2862081/start7510715,launcher2862082/start7510715;
workers2862112/7510779,2862113/7510779,2862114/7510780,2862115/7510780,
2862116/7510780,2862117/7510781. Verify argv/cwd/start ticks on actual host, not sandbox
/proc. All11owned handles reverified LIVE at quality entry and subsequent observations.
NO source, fixture, pinned helper, binary, config or input edits during this attempt.
Documentation/Git checkpoints and read-only counter analysis are permitted.

Current237ONLY production delta since235: CNativeAffineReference.cpp skips a signed
simplex residual solve if fresh outward-rounded intervals prove it nonpositive;
absolute bound already includes zero. Mixed-sign intervals retain both solves.
No new cache/operator/scheduler/config, support enlargement or policy change.
[Proof, actual residual/query controls and qualification](Handbook/ADAPTATION_GEOMETRY_SCREEN236_20261107.md).
Broader236coarse-plane screening saved no counted work: REJECTED and fully reverted
before237. R/screen236_{source.patch,assessment.json} preserves it. No broad/MPI/M6on236.
Five geometry-clean two-face cores already have useful original-star controls;
do NOT broaden optional four-star preflight merely from eligibility. Both sampled
geometry-bad controls refuse tested alternatives, often for increased geometry excess.
No slack cache selected: none of six expensive actual nearby pairs has enough margin.

Build237tool64986TERMINAL0,33.141690s,1302pins unchanged/both targets up to date;
all observed compiler handles absent. Same12query replay237tool73632TERMINAL0,
163assertions, ALL bounds bit-identical235, traversal575272→540961(-5.96%), pins unchanged.
Serial237tool51486TERMINAL0,372.369733s:116cases/82379assertions (actual7cavities,
quality/heights,atlas/geometry,failure/census). All168actual-cavity outcome records
match235 excluding time/work. ONE MPI6tool19231TERMINAL0,6.322127s,26cases EVERY rank.
All qualification pins unchanged/owned handles terminal. R/signed237_qualification.json.
Adjacent MPI/height protocols pass; this is NOT accepted meshes or full BL qualification.

## Measured live progress — update this section in place

All76construction reports match235counts/operations/cache events/preparation limits.
Construction completes871607cells/1997geometry-bad/0size, q/J UNMEASURED there.
Traversal272143468→258429382(-5.04%). Native construction902.062421→659.237313s is
DESCRIPTIVE; contention/ownership/cache effects prevent isolated wall-speed attribution.

Quality-entry native705.179721s (235969.422254s), round34594: COMPLETE census,
102058q-bad/101860J-bad/1997geometry-bad/0size. Rank counts AND q/J-debt distribution
IDENTICAL235. All1519source/runtime/input pins rechecked unchanged in
A237/quality_entry_assessment.json. Source remains frozen.

Second complete quality window34688/native945.000761s:880804cells,96237q/96066J/
1969geometry/0size. Cumulative net5821q/5794J/28g removed; counts/churn/3439commits,
rank debt distribution and smoothing decisions match235at the same round.
Quality-only traversal ratio.94547(-5.45%). Observed netq24.27/s versus21.20/s over
239.82s versus274.53s quality ages; not an isolated causal timing proof. Cumulative
q-retired83817/q-introduced77996, not83817net fixes. Join-call wall share45.58% vs46.15%
is not CPU idle; counters/timers overlap. Boundary19775candidates/869moves, interior
13305/1646 equal235. Geometry-only repair is not yet exercised.
A237/quality_comparison235.json verifies churn and states unequal nearest census times.
No acceptance, geometry-only closure, transfer, adapted CFD or disk restart yet.

Resources at quality entry: summed tree RSS10.11GB (shared pages included, not physical
peak),MemAvailable~27.5GiB. Subsequent observation~10.15GB/RAM~27.9GiB, all owned
VmSwap0 and memory PSI avg10/60/300=0. Global swap use includes unrelated cold pages.
A237/{quality_entry_owned_receipt,quality_entry_memory_receipt}.json and local
owned_observations.jsonl retain receipts. No demonstrated resource-stop condition.

Read-only refresh (existing first-event prefix parser, exact rank/stage/round joins):
`python3 R/analyze_m6237_stages.py`, then `python3 R/analyze_m6237_quality.py`.
Expand R to the path above. Quality helper requires a complete entry/window; now present.
These reuse existing facilities, do not change repair choices, and stop parsing at
FIRST native EXIT. Complete prefix hashes are checked. Partial EXIT zero counts are
UNKNOWN unless cache/census/shape/geometry flags prove measurements complete.
Judge net whole-field q/J-bad removal/s, geometry counts/churn and counted work,
not commits, private debt or lower RSS. Timers overlap; join-call share is not CPU idle.

## Next actions and acceptance obligations

Keep237frozen until terminal or preserved demonstrated failure/bottleneck/resource
pressure justifies the existing authorized stop workflow. Never stop on age/stale logs.
Before any stop verify exact owned launcher/start/argv/cwd, preserve first-event
summaries/census/source pins and seal selected capture files with hashes/counts.
A first refused event makes two accepted meshes impossible in this two-event attempt;
a sealed-evidence one-time owned stop is authorized then. Do not repeat any prior stop.
Existing selected capture preserves evidence; no routine per-trial VTU exports.

Establish shape closure, then existing shape-preserving residual geometry/size mode;
any validity-only reconstruction requires fresh shape/size/height reclosure within
SAME budgets. Geometry-only repair was not reached235; no rate/closure claim follows
from shape-phase geometry retirements or these local fixtures. All final gates remain.

Require TWO accepted complete all-rank native events AND actual adapted mesh/native_ref
and restart outputs. BEFORE publication require true CV mass-solve residual<=1e-10,
all projected states/timelevels admissible, scaled target-integral defect<=1e-12.
Production CConservativeTransfer.cpp final collective checks precede setting new arrays;
per-field summary prints true residuals. Old/new-domain integral change is SEPARATE.
Finite CFD or run_m6's finite-integral helper alone is insufficient conservation proof.

Verify adapted_mesh_adap_00001/00002.su2 + native_ref and matching restart_flow_adap files,
exact point order/coordinates1e-12, finite conserved states, positive rho/internal energy.
Then fresh isolated ADAP_LOOP=NO/RESTART_SOL=YES/READ_BINARY_RESTART=YES, distinct input/
output prefixes, ACTUAL disk load and finite admissible continuation. Refused-event
restarts on ORIGINAL grid are not adapted proof. Resume-cycle0overwrite/rejected-cycle
consumption remain OPEN; isolated solver restart does not claim those issues fixed.
Then dedicated BL curved/junction/MPI-interface/subsequent-operation/ownership/requested-
height qualification, then full RANS500k. Do not mark goal complete from fixtures.

## Fixed input provenance and saved inspection

Saved REAL original-grid CFD seed R/m6_inputs/euler_original_force_seed_v1:
61330points/303359cells,CL.269535/CD.013536,stable force window, NOT full residual convergence.
SeedSHA1eaee788bd62bd92a92ec539577dcbcfc0f8042dcbcec23adb04e825bb01a880.
Corrected shared metric219qualified; same-seed L1/L2at50k compared and L1selected
(shock/corner/farfield/gradation evidence R/metric_target219_production/selection.json).
ConfigSHA74b2e9f476cf6ba1fdc5ebcdcb1c34a2b3c45b25e58d651e75417d89402bfe8d;
initialrestartSHA5132ee2a83635b457943c2679a3a50c5e0b733427e620cc748da53865ce5d692
BYTE-IDENTICAL219/225/227/235/237. A237/initial_metric_input_provenance.json.
First adapted metric always uses this saved real solution; event2requires accepted
mesh plus continued CFD. Do not recompute/reselect seed or assume L2better.

Full235naturally refused at native5334.959466s:1017408cells/3840q/2068J/1686g/0size,
COMPLETE final caches. All30files sealed; one authorized stop after seal, all handles
terminal. NEVER repeat it or historical227/225/219/217/210stops. Its independently read
A235/inspection/rejected_mesh.vtu WITH9-component transported nodal metric has
198318points/1017408tets; bad_cells.vtu ALL7090cores, cavities.vtu ALL220573neighbors,
reference/rejections/ID maps. Exact SU2/VTU coordinates/tets agree, metrics finite/
symmetric/SPD; no ParaView GUI check, diagnostic SU2 marker-name fallbacks. Current
saved mesh remains235; no per-trial files.227final geometry caches were UNKNOWN;
its separate centroid q/J diagnostic is not an equivalent production final census.

## Operations and records

One owned heavy job,nice19,libs1,build3workers. Check actual host RAM/load/disk,
use4TB compile-tmp. Cache-accounted bytes, summed RSS and physical RAM differ.
Fork origin https://github.com/rois1995/SU2.git, branch codex/native-3d-bl; pushes
already authorized. Git metadata outside workspace requires escalation. Use
`gc.auto=0`, `submodule.recurse=false`, exact reviewed whitelists/ignore-submodules=all.
Preserve unrelated logs/analysis/.orig/broken Eigen submodule. No raw meshes/binaries/
restarts/logs committed. Literature is design context, not a competing implementation.

- [How to proceed](HowToProceed.md)
- [Integrated task](Handbook/ADAPTATION_INTEGRATED_NEXT_TASK.md)
- [237proof/control/qualification](Handbook/ADAPTATION_GEOMETRY_SCREEN236_20261107.md)
- [235outcome](Handbook/ADAPTATION_BUILD235_OUTCOME_20261107.md)
- [Connected coupled repair](Handbook/ADAPTATION_GEOMETRY_CONFORMING_REPAIR_20261107.md)
- [Geometry witness qualification](Handbook/ADAPTATION_GEOMETRY_WITNESS_20261106.md)
- [Repair support and literature](Handbook/ADAPTATION_REPAIR_SUPPORT_20261106.md)
- [Metric correction/target choice](Handbook/ADAPTATION_METRIC_FLOOR_20261106.md)
- [Backlog](Handbook/ADAPTATION_BACKLOG.md)
- [Detailed pre-consolidation handoff](Handbook/History/HANDOFF_Codex_before_quality237_20261107.md)
