# Native adaptation — current handoff

Updated2026-11-08(Europe/Rome). Goal ACTIVE;ZERO accepted meshes.
Complete TWO strictly accepted ONERA M6 Euler CFD→mesh→CFD→mesh events at50k
METRIC COMPLEXITY,conservative admissible transfer,finite continued ADAPTED CFD
and ACTUAL isolated ADAPTED disk restart. Then dedicated BL/constraint/MPI
qualification and corresponding RANS500k. Actual points/cells are separate.
Fixtures,private candidates,balanced weights and lower work/memory are not completion.

Strict geometry1e-4,q>=.2,J>=.05,maxedge1.8,requested heights. Construction stays
validity-first:exact positive/nondegenerate cells,topology,geometry/body/feature/
requested-height validity;NO unnecessary fresh q/J/ranking/veto. Boundary movement
on prescribed geometry is allowed. CAD optional;no-CAD authority sufficient.
ONE active cycle/ONE owned heavy job;all pinned sources/fixtures/helpers/binary/
config/inputs FROZEN while running. Preserve authorized evidence/stop/fix/retry.

R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
A254=`R/m6_runs/euler_build254_conditioned_mach_only_real_cfd50k`(TERMINAL REFUSED).
A253=`R/m6_runs/euler_build253_witness_cutoff_mach_only_real_cfd50k`(TERMINAL REFUSED;partial terminal census).
A251=`R/m6_runs/euler_build251_upper_cutoff_mach_only_real_cfd50k`(terminal REFUSED).
A247=`R/m6_runs/euler_build247_canonical_mach_only_real_cfd50k`(REFUSED baseline).

## CURRENT — canonical geometry-debt256 building;254terminal refused

Implementation8a2c4e1d48and inspection checkpointfc87c40994PUSHED. 254first event
REFUSED at native5332.870105/round42620:private repair round elapsed-time budget.
COMPLETE terminal741236cells/251q/205J/95geometry/0size,all measured/flags complete.
ZERO accepted meshes. Tool70856TERMINAL1/5533.745599s. ALL30raw files sealed and
741236owned/metric rows complete:A254/capture_ready.json. ALL1520pins verified
AFTER sealing BEFORE ONE pidfdSIGTERM to exact MPI1274730/start13602153.
owned_stop_once.json attempts1/signal_sent;ALL11solver/wrapper handles absent in
terminal_handles.json. NEVER repeat that stop. Later original-grid CFD and adap-
named files are not adapted proof. No owned solver alive.

Meaningful saved mesh NOW A254/inspection/rejected_mesh.vtu and.su2:
150080points/741236tets. ALL260bad cells and2400neighbor cells plus reference saved.
PointData captured_nodal_metric9row-major;CellData q/J/geometry_excess_l2/edges.
Exporter tool55016TERMINAL0/329.411249s,ALL1554pins unchanged,all3handles absent.
Independent reader tool22015TERMINAL0/2.681038s:correct array lengths/associations,
VTU/SU2 coords+tets identical,all150080metrics SPD(minEig.00312949064),all tets
positive(minVolume5.298e-15),bad counts251/205/95 exact;all input hashes unchanged.
A254/inspection_{export_status,export_terminal_handles,reader_status,reader_terminal_handles}.json.
This is refused actual geometry,not acceptance. Large artifacts remain local;
metadata/hashes pushed. Previous meaningful247 remains historical,not latest mesh.

ONE focused serial256 milestone RUNNING tool63150:
R/cases/current_compound_20261108T010915_1724241,status pid1724529.
Host wrapper1724241/start14459481,time1724529/timeout1724530/test1724531
all start14459983;orted1724532/start14459984. owned_handles_start.json.
ALL source/runtime/input/helper pins FROZEN;nice19/libs1,existing600s case cap.
Build256TERMINAL0/480.354111s,all1303pins unchanged,both targets up to date,
all11recorded build startup/compiler handles absent;build256_terminal_handles.json.
[Current correction/evidence](Handbook/ADAPTATION_GEOMETRY_DEBT256_20261108.md)
and R/geometry_debt256_plan.json:shared GeometryGuidanceExcess now matches
canonical complete-census squared certificate excess,removing face-area weights.
Actual raw collapse:q28→18/J25→17,failed PHYSICAL faces8→7,certified debt−15%,
max error unchanged,one fewer point;old area weighting rises6% and vetoes it.
255diagnostic41955assertions/103.936634s,allpins unchanged,all5handles terminal;
all6baseline strict certificate sums match sealed census to rounding. No metric,
operator,scheduler,support/gate/budget change. Production selection/retained-star
orchestration remain limitations,NOT guaranteed by this scalar correction.

NEXT:verify current serial terminal/owned handles;then ONE MPI6 milestone via
R/geometry_debt256_validate.py mpi6 (serial already running). It reuses current254
six-context serializer for serial and qualified244/placement246_inputs_v1 forMPI.
Validate affected curves/seams/holes/bodies/features/heights/caches/bounds/budgets,
conflicts/empty-rank publication and actual complete-context net q/J/geometry.
Then IMMEDIATELY full same-seed two-eventM6. No baseline/perf/GDB campaign.
All acceptance/transfer/adaptedCFD/disk-restart/BL/RANS obligations remain open.
Current exact capture:117bad cores own wing faces,143interior cores touch CURRENT
wing vertices,ZERO free-interior. Original-reference-node provisional classification
was wrong and is preserved;use bounded_residual_selection.json v2 ONLY.
Latest exactLE edges remain short(.13–.19),8–15xoriginal stationtriangles;
64geometry-bad wing cells in explicit LE band(vs795old247),not spacing-necessity proof.
255build/test and254test/export/reader ALLTERMINAL;no M6 solver alive.
Source freeze may be released ONLY after current256owned handles are terminal.
Prepared launcher:R/launch_m6256.py requires geometry_debt256_qualification.json
from qualify_geometry_debt256.py SERIAL_CASE MPI_CASE (after both terminal).
Preserve literal placement input directory:its run.cfg is a SYMLINK and resolving
it loses companion files;validation wrapper correctly uses absolute(),not resolve().

Same saved CFDseed,MACH-only L1/50k,same configSHA5ad2dc1d9a1cd8bff87f633617b191b50defbc261ab3d0b2ed0987a34722b89e.
Actual full-run initial22fields/coords match qualified target exactly. Initial
quality entry690715cells/27016q/27453J/1205g versus253878443/106454/106204/1381.
Construction110s slower despite fewer commits;no same-workload speedup claim.
First shape exit3274.720:49q/29J/974g. Guarded geometry construction/reclosure did
execute;best complete shape census34q/20J/728g,best geometry census133q/122J/60g.
Late cycles worsened/churned to terminal251/205/95. No full closure. THREE ownership
publications preserve aggregate flags/counts,not individual dependency proof.
Terminal shape255bad union:111qsurface/140interior,95Jsurface/110interior;all255
have edge<.5,none marked near genuine features;stuck97,missing0,private collectives0.
Window10 resource proof:all11identities/1520pins unchanged,treeRSS9.188GB,available
~32.84GB,no observed memory stalls;CPU contention. RSS not whole physical peak.

NEXT target from this outcome:repair power/admission and coupled surface/volume
closure,using actual254 residuals. Do NOT start new scheduler,larger-cavity campaign,
geometry representation,mandatory curvature metric,quality-aware construction or
norm/sensor/complexity sweep. No blanket minimum-edge gate. Review existing
operators/support/certificates;test ONE connected hypothesis before production
change. Source-freezing/one-heavy-job/evidence/stop-fix-retry rules persist.
All transfer,continued ADAPTED CFD,actual isolated ADAPTED disk restart remain open.

A254/terminal_assessment.json and phase_recovery_progress.json retain ALL first-
event transitions. quality_progress/comparison253 cover ONLY FIRST uninterrupted
shape pass(23windows),not terminal. quality_counter_assessment covers actual complete
WINDOW/EXIT;use matching round. Partial construction q/J never imply passes.
Read-only refresh:R/analyze_m6254_frozen.py. Source/raw receipts authoritative.

## Preserved253 outcome and active diagnosis

253 first event REFUSED native5346.554979/round34761,time budget. Final census
UNKNOWN on693003/1032510cells;partial30q/20J are NOT whole-field counts. Last
complete34752:1641q/695J/1044geometry. ALL30raw files closed/hashed in
A253/capture_preserved_partial_census.json (NOT capture_ready qualification).
All1520pins verified BEFORE ONE pidfdSIGTERM to MPI871976/start12808268;
all11handles gone. Never repeat that stop. Driver's later original-grid CFD and
adap-named files do not prove adaptation. A253 terminal receipts authoritative.
At matching34752,253 counts/decisions exactly251 but quality37.66s SLOWER despite
7.32%less upper work. No causal timing/closure gain. Full partial EXIT retained.

[Current diagnosis](Handbook/ADAPTATION_TARGET_DELIVERY253_20261107.md) records
same-seed flow/target stages,exact LE edges/reference faces,cavity controls and
paper assessment. Paper criteria differ from ours;final iterated result is not a
first50kbenchmark;recovery L2projection differs from Lp error objective.
Confirmed:coarse target in sampled compression strips;very short actual LE edges;
71ill-conditioned first-ring fits corrected below. Farfield allocation/necessary
geometric resolution/certification scopes remain unresolved;no second correction.

## Qualified conditioning254 — full M6 refused; integrated fix preserved

Existing-output trace verified raw Hessians;bounded actual smooth-LE compound
checks passed35836assertions/103.969246s,all pins unchanged and all handles gone.
First setup omitted OMPI_MCA_osc=pt2pt and failed MPI_Win_create before tests;
receipts preserve failure and corrected retry. Two actual contexts(51/40cells),
24directed collapses and original-star controls;some combined repairs help,others
worsen q/J/geometry. Generic fixture boundary=false STILL allows geometry-constrained smoothing:
302boundary candidates/7moves observed. Earlier no-smoothing inference was wrong;
Smooth dispatch6169–6178 ignores that flag with geometry. No rerun/source change.
Its geometry-guidance score differs from terminal certification;do not conflate.
Worst geographical LE candidate115cells/star90 exceeds64;not enlarged.

ONE connected correction selected in R/condition254_plan.json:apply existing
QR Frobenius condition<=1000 admission to BASE quadratic fits too,not just richer
wall polynomials. Actual trailing-edge extreme H matches first-ring designs with
condition~2.9e6..1.45e8;many residuals below.05 prevented growth. Independent
well-conditioned two rings give far smaller H. Controlled smooth cubic perturbation
reproduces Hxx10.60 instead of2 at residual2.55e-6;two-ring Hxx1.99999968.
Unsupported base first ring now uses existing complete-owner two-ring retry;
unsupported final fit retains caller fallback. No sensor filter/norm/config/gate/
construction/scheduler change. Shared2D/3D path plus one focused serial synthetic
regression. Existing MPI owner/recovery tests must qualify changed behavior.

Build254TERMINAL0/75.129806s,ALL1303pins unchanged,both targets up-to-date,
all build handles absent. Focused serial31cases/168175assertions PASSED6.659140s;
ONE MPI6 milestone17cases on EACH rank PASSED6.839107s;all source/runtime pins
unchanged,all owned handles terminal. R/condition254_qualification.json.
Same-seed production target replayTERMINAL0/57.147841s,pins unchanged,all handles
absent. ALLflow fields/coords bit-identical;71Hessians changed;maxrawHnorm
1.3025e13→8.5484e7,minQRpivotratio1.1528e-8→.00290566,zero WLSfallbacks.
Finalcomplexity49999.999976,SPD/bounds/AR and independent directed-gradation audit
passed. Sampled shock target5–12%finer;farfield share34.872→40.398% (not solved).
R/target_delivery254_v1/comparison.json. No norm/sensor/complexity change.
Full same-seed two-event254M6 refused;unchanged gates/native5400/outer14400 preserved.
Zero accepted meshes;no transfer/adaptedCFD/restart claim. No competing cycle.

## Preserved geometry improvements and inspection

[Witness253 qualification](Handbook/ADAPTATION_WITNESS_CUTOFF253_20261107.md):
losing convex-triangle witness cutoff and seed reuse preserved unchanged in254.
5241ordered query identities/bounds/cache outcomes and166result rows match251;
7.709%less counted work. Serial38/2044 and MPI6(90cases EACHrank) passed;not closure.
[252controls](Handbook/ADAPTATION_COMPLETE_CONTEXT252_20261107.md) did not justify
larger supports/operators. [Recovery guards](Handbook/ADAPTATION_RECOVERY_GUARDS253_20261107.md):
dedicated geometry repair requires clean q/J;separate construction return can run
with q/Jbad after queue exhaustion/progress/strict census-size-height conditions.
Do not invent a hard shape-zero barrier or change policy while frozen.

[251outcome](Handbook/ADAPTATION_BUILD251_OUTCOME_20261107.md):REFUSED1487q/639J/
1044geometry at native5348.32;complete capture,one owned stop,all handles gone.
[247outcome](Handbook/ADAPTATION_BUILD247_OUTCOME_20261107.md):REFUSED1148q/484J/
1044geometry. Latest meaningful saved mesh A247/inspection/rejected_mesh.vtu
and.su2:201203points/1032602tets. Metric9row-major PointData;q/J/geometry/edges
CellData. Complete bad/cavity/reference exports;independent reader/SPD/positive
volume checks passed. Surface uses SAME volume nodal tensor;no fabricated field.
No per-trial VTU export. Failed250footprint memo worsened work10.97%;do not revive.

## Seed and completion obligations — all open

R/m6_inputs/euler_original_force_seed_v1:61330points/303359cells,M.8395/AOA3.06,
CL.269535/CD.013536 stable force window accepted by user,NOT full residual convergence.
ITER1continues real saved field,not startupCL.059. SeedSHA
1eaee788bd62bd92a92ec539577dcbcfc0f8042dcbcec23adb04e825bb01a880;
initial continued original-grid restartSHA
c99d057882238eb75a768da717e939ab426fa60a5cc0a27ec484eb6cb758a147.
This corrects a stale hash in earlier handoffs. Actual253/251restart bytes match
exactly and match251quality_entry_comparison247.json;A253/
initial_cfd_restart_comparison251.json preserves the correction.
MACH ONLY in generator/future M6configs(including RANS);pressure physics/output stay.
219local metric floor/log normalization/flatsensors qualified/integrated. L1chosen
from production same-seed target comparison;Mach-only complexity49999.999626,SPD,
zero gradation violations and bit-identical CFDfields/coords. No universal optimum.

Need shape closure→strict residual geometry/size construction→shape REclosure,
TWO accepted WHOLE meshes on all ranks within unchanged budgets. True CVmass-solve
residual<=1e-10 EVERYfield/timelevel,all transferred states admissible,and scaled
TARGET integral defect<=1e-12 BEFORE publication. Printed old/new DOMAINintegral
changes and run_m6finite helper alone are insufficient. CConservativeTransfer.cpp
997–1039/PrintSummary1420+ enforce/report relevant checks. A253/
transfer_precision_provenance.json verifies CURRENT executing test_driver AND CFD
hashes against pins,double/mixedOFF build metadata/compilecommand/transfer source;
read-only provenance,NOT a transfer result.
Need actual adapted_mesh_adap_00001/2.su2+.native_ref and matching
restart_flow_adap_00001/2.dat,exact point order/coords1e-12,finite conserved states,
positive density/internal energy. Fresh ISOLATED ADAP_LOOP=NO,RESTART_SOL=YES,
READ_BINARY_RESTART=YES actual ADAPTED disk load,distinct input/output prefixes,
finite admissible continuation. Existing seed reader can validate with matching
mesh. Resume-cycle0overwrite/rejected-event consumption remain OPEN;isolation does
not fix them. Then dedicated BL/curved/junction/interface/subsequent-op/ownership/
heights MPI,and RANS500k. Generic MPI90does NOT establish dedicated BL qualification.
Historical broadserial8/344failures include unattributed RANS wallpolicy4096work;
do not rerun generic broad suite or weaken assertions.

## Working rules and references

HOST /proc differs from sandbox;identity/resource reads require escalation. One
heavy job,nice19,libs1,build3workers,4TBcompile-tmp;check load/RAM/disk before heavy
work. Source freezing includes fixtures/helpers. Never kill unrelated processes.
Origin github.com/rois1995/SU2.git,branch codex/native-3d-bl;fork pushes authorized
with EXACT whitelists. Git metadata outside workspace at SU2_AdapNoExt/.git needs
escalation;gc.auto=0,submodule.recurse=false,ignore-submodules=all. Preserve unrelated
dirty logs/.orig/untracked work;never broad-stage raw meshes/binaries/restarts/logs.
Existing parsers:analyze_m6244_mach_only.py transforms BOTH literal attempt paths;
retain original __file__,verify output entry/window sets and raw complete six-rank
stage groups. compare_quality227.py joins ROUND+STAGE+RANK to preserve EXITdelta.
Use existing source/counters/corpus,not another baseline/perf/GDB campaign. One
connected production fix,affected serial/ONE MPI6 milestone,then immediately M6.
[HowToProceed](HowToProceed.md),[AGENTS](AGENTS.md),[integrated task](Handbook/ADAPTATION_INTEGRATED_NEXT_TASK.md)
(historical155numbers superseded),[backlog](Handbook/ADAPTATION_BACKLOG.md),
[repair references](Handbook/ADAPTATION_REPAIR_SUPPORT_20261106.md).
[Detailed pre-compaction state](Handbook/History/HANDOFF_Codex_before_concise254_quality16_20261108.md)
is HISTORICAL,not live;preserves earlier detailed timings/evidence/source maps.
