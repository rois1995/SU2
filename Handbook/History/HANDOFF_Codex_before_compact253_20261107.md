# Native adaptation — current handoff

Updated2026-11-07(Europe/Rome). Goal ACTIVE; ZERO accepted meshes.
Complete TWO strictly accepted ONERA M6 Euler CFD→mesh→CFD→mesh events at50k
METRIC COMPLEXITY,conservative admissible transfer,finite continued adapted CFD
and ACTUAL isolated adapted disk restart. Then dedicated BL/constraint/MPI
qualification and corresponding RANS500k. Actual points/cells are separate.
Fixtures,private candidates,balanced weights and lower memory are not completion.

Strict geometry1e-4,q>=.2,J>=.05,maxedge1.8,requested heights. Construction stays
validity-first:exact positivity/nondegeneracy/topology/body/feature/height validity;
NO fresh q/J/ranking/veto. CAD optional;no-CAD authority sufficient.
ONE active cycle/ONE owned heavy job;all pinned sources/fixtures/helpers/binary/
config/inputs FROZEN while running. Verify actual HOST identities before any action.

R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
A251=`R/m6_runs/euler_build251_upper_cutoff_mach_only_real_cfd50k`(terminal REFUSED).
A247=`R/m6_runs/euler_build247_canonical_mach_only_real_cfd50k`(REFUSED baseline).

## CURRENT — terminal251 refusal; diagnosis active

Tool5327TERMINAL1/5537.759614s,pins verified unchanged. First native event REFUSED
at5348.321852/round34773:1032521cells/1487q/639J/1044geometry/0size,
ALL measured/complete flags. Reason:private repair round exhausts elapsed-time
budget. Still first shape pass;no accepted mesh or geometry closure.
A251/capture_ready.json seals ALL30files and1032521owned/metric rows. All1520pins
verified before ONE pidfdSIGTERM to HOST MPI475670. A251/owned_stop_once.json
records exact identity;NEVER repeat. All10sampled owned handles PLUS wrapper absent
in A251/terminal_handles.json. Production C++unchanged;diagnostic fixture changed below.
Terminal analysis complete;[251outcome](Handbook/ADAPTATION_BUILD251_OUTCOME_20261107.md)
and A251/terminal_comparison247.json preserve full first-event scope.
Actual candidate201192points/1032521tets. Versus247terminal:339MOREq/155MOREJ,
same1044geometry. Quality4670.668245vs4658.117179,net q/s22.473658vs22.606988.
Last matching34752counts/85055commits/query counts identical;quality upper work
−0.9735%,but no runtime/closure gain. EXIT34773NONZERO219commits/net154q/56J;
baseline EXIT34816shares final window and has zero delta. Do not drop current EXIT.
Queue24031,stuck_shape46of1999,missing0,private tracked collectives0. Latest q-bad
1236interior/251surface,J495interior/144surface. All1999shape-union cells haveedge<.9;
1889<.5is correlation,not a construction-policy justification.
Immediate blocker remains SPEED of useful closure;retained geometry phase untested.

ONE next bounded diagnosis:existing whole-context optimizer versus existing single-
star and collapsed-retained-star candidates at SAME160work/64cell cap. All SEVEN
prior247actual contexts are EXACTLY unchanged in251(full stars,raw geometry/nodal
metric/physical facets,q/Joracles,forward bindings,dependency versions). The2.212690s
identity check is terminal/host absent. All1563pins of the qualified251same-corpus
case and all30current capture hashes verified;A251/qualified_fixture_reuse251.json.
No new full VTU/baseline campaign. Source review confirms existing cheap merit
screening and old-side guidance reuse;no duplicate filter needed.
Production optimizer sees original feature star,then raw topology is immediately
validated. Existing pre-reservation full-star union is limited to geometry-bad
shape-bad cores with2physical faces;reuse point for required support closure.
Current surface core212134150has raw1q/0J→3q/4J(no progress,refused),then existing
retained-star optimization→0/0with geometry0 and valid metric/interface/replacement
checks. Assess whole-context optimizer FIRST before adding a composite branch.
Build252DIAGNOSTIC-ONLY extension TERMINAL0/180.178644s,1303pins unchanged,
both targets current,all11host handles absent. ONE current_residual case
current_residual_20261107T201453_813263 TERMINAL0/96.082380s,39213assertions;
all case pins unchanged,all6recorded HOST handles/wrapper absent. Tool97581terminal.
R/diagnostic252_{assessment,terminal_handles,owned_startup}.json.
Diagnostic252 production unchanged;fixture extension preserved.
BUILD253 TERMINAL0/33.104877s,1303pins unchanged,both targets current,
all recorded host handles absent:R/build253_terminal_handles.json.
Shared TriangleUnion::Upper now stops a losing convex-triangle witness once
its partial max reaches current best and skips the already measured seed.
No state/config/gate/construction policy change. Actual current_compound253
TERMINAL0/104.023934s,pins unchanged,all6HOST handles/wrapper absent. EXACT5241
orderedquery identities/cache outcomes/bounds and166candidate rows match251.
Counted query work4983517→4599314(−7.709475%);no whole-M6 speedup claim.
R/witness253_actual_comparison.json;[proof/current validation](Handbook/ADAPTATION_WITNESS_CUTOFF253_20261107.md).
Focused canonical_serial253 PASSED38cases/2044assertions71.699312s,pins unchanged,
all5case handles absent. The initial sequential wrapper tool84208 TERMINAL1:
MPI harness lacked SU2_NATIVE_FROZEN_CONFIG and failed BEFORE launching MPI;
preserved placement_mpi6_20261107T202808_856307,status2;wrapper/runner absent.
No C++ failure or MPI test occurred there. Required MPI config is
A244/placement246_inputs_v1/run.cfg (NOT the seven-context247 config).
ONE actual MPI6 milestone PASSED90cases on EACH rank187.187539s,pins unchanged;
placement_mpi6_20261107T202852_858268,tool53521TERMINAL0. All10case handles
PLUS wrapper absent:R/witness253_validation_terminal_handles.json.
R/witness253_qualification.json seals affected serial/MPI evidence.
Checkpoint d56b5588a2 PUSHED to origin/codex/native-3d-bl.
FULL TWO-event M6 witness253 is LIVE/FROZEN tool54771:
A253=R/m6_runs/euler_build253_witness_cutoff_mach_only_real_cfd50k.
R/m6_253_witness_launch.json and A253/owned_startup.json are authoritative HOST
identities;wrapper871749/start12807749. Startup verified ALL1520source/runtime/
input pins unchanged;configSHA5ad2dc1d9a1cd8bff87f633617b191b50defbc261ab3d0b2ed0987a34722b89e
EXACT251. R/m6_253_startup_checkpoint.json;immutable launch snapshot
R/m6_253_launch_checkpoint.json. HOST runner871751,time871974,
timeout871975,MPI871976,workers871979–871984 live at startup (exactticks/argv/cwd
in receipt);never infer terminal from stale logs or observation timeouts. Same saved real CFDseed,MACH-only L1/50k,
strict gates,native5400/outer14400,trace UNSET,pt2pt/private4TBscratch/nice19/libs1.
ONE heavy workload;keep ALL pinned source/fixtures/helpers/binary/config/inputs
FROZEN until terminal or preserved evidence authorizes owned stop. No competing
implementation. Compare useful whole-field q/J retirement/s and geometry counts
at matching rounds/quality times;counted diagnostic savings are not runtime proof.
Initial/latest per-handle scheduler counters are included in owned_startup.json;
CPU includes MPI spinning,not all useful compute. No new profiler/GDB campaign.
On refusal:seal complete six-rank owned/metric/reference/rejection captures and
verify all pins BEFORE one exact verified owned stop;verify terminal handles.
No routine VTU export. On acceptance:actual conservative/admissible transfer,
continued adapted CFD and isolated ADAPTED disk restart remain required below.
[Complete result/next decision](Handbook/ADAPTATION_COMPLETE_CONTEXT252_20261107.md).
All four useful complete-context cases already have useful original stars.
Feature-adjacent complete context worsens1q→3q despite debt admission;geometry
controls unchanged. DO NOT generalize support or add a composite operator from
these results. Existing useful severe/surface-clean stars have all14features
UNBLOCKED at terminal251;A251/diagnostic252_residual_frontier.json. Queued work
remains;speed of net useful closure is still immediate target,not missing support.
The measured witness253 fix above is the ONE active production change. No further broad baseline/larger-cavity campaign/new
scheduler/config/cap/pass/construction quality policy. Then focused serial/ONE
MPI6 and IMMEDIATELY same-seed Mach-only L1/50k full two-event M6. Geometry
residuals require targeted construction/size closure then shape REclosure;not
established by these shape controls. All full-goal obligations below remain OPEN.
The driver continued CFD on its ORIGINAL grid after refusal;its adap-named restart
is NOT adapted transfer/continued-CFD/restart proof. Full goal remains OPEN.

## Frozen251 startup and measured prefixes — historical

Frozen runtime source checkpoint1943eb3199pushed;tool5327is now terminal above. Same saved realCFDseed,MACH-only
L1/50k,strict gates,native5400/outer14400,pt2pt/private4TBscratch/nice19/libs1;
geometry trace UNSET. ConfigSHA5ad2dc1d9a1cd8bff87f633617b191b50defbc261ab3d0b2ed0987a34722b89e
matches247exactly. Initial continued CFD CL.269535/CD.013536;61330point-sensor fits.
HOST identities:runner475457,time475668,timeout475669,MPI475670,
workers475673–475678. Exact starts/argv/cwd and wrapper identity in
A251/owned_startup.json,R/m6_251_startup_checkpoint.json and live
R/m6_251_upper_cutoff_launch.json. Status/elapsed alone NEVER prove termination.
Latest HOST verification at4494.3s:all10sampled handles live,ALL1520pins unchanged,
sampled process-tree RSS10.752GB,PSI averages0.00. A251/quality_window31_live_observation.json.
RSS is a sample,not physical or whole-job peak. Owned VmSwap=0at prior check;
full host swap had0swap-out/9swap-in pages over108.93s,PSI0.00. No memory pressure.
Resource snapshot AFTERwindow13had load8.03/8.10on CPUs0–7,CPU PSI some4.85/4.55/4.71.
Workers cumulative scheduler waits27–29s over~2710s;CPU includes MPI spinning.
No comparable baseline scheduler sample exists,so this does NOT establish the
cause of timing differences. A251/scheduler_observation_after_window13.json.

THIRTY-ONE-window snapshot through33856/census45,quality age3604.256898s:
1020076cells/13978q/10156J/1090geometry/0size,ALL measured/complete flags.
Exactly matching247counts/65704commits/churn;net92476q/96048J/291geometry removed.
Net q/s25.65744vs25.88444;baseline age3572.648732s,offset+31.608166s.
The EARLY timing advantage has NOT held. Whole-native4281.910505vs4262.231331.
Timings observational;no causal speedup/regression or closure established.
Quality upper1555478086→1539568095(−1.0228%),same1081831upper queries.
Boundary kernels597991candidates/24612moves in both,3039.287829→3020.864266summed-rank
seconds;interior383308/48715,215.914263→219.723396. Timers OVERLAP geometry/other work;
kernel moves/debt are not published whole-field gain. smooth_boundary_reused=4is
recorded without diagnosing a new blocker before terminal.

A251/quality_comparison247.json verifies complete raw prefixes,matching rounds,
explicit nearest-age pairs and whole-field churn identities. quality_counter_assessment.json
has current location/ladder/rejections/height/metric-drift counters. Latest q-bad
location:12915interior/1063surface;stuck_shape29,missing features0,tracked
private collectives0. Interior means no physical face,not necessarily free vertices.
SIX actual ownership publications preserve whole-field measured counts/flags:
A251/ownership_census_integrity.json. Balanced debt is not mesh acceptance.
First quality entry EXACTLY247:878443cells/106454q/106204J/1381geometry/0size,
all complete. Same cfg,CFDseed AND initial continued original-grid restart bytes;
A251/quality_entry_comparison247.json. Construction120785commits/52092upper calls
in both;upper274601233→271243211(−1.2229%),native689.582599→677.653607observational.

For future attempts,keep the full run FROZEN until its terminal outcome or preserved evidence warrants
an authorized stop for demonstrated failure,clear bottleneck or actual pressure.
No profiler/GDB/baseline campaign/new configuration. At refusal,seal all30capture
files/complete flags and verify source pins BEFORE one exact verified owned stop;
then verify tool/host handles terminal before any C++/helper/binary edit/rebuild.
Never repeat a stop or infer termination from an observation timeout. Raw refusal
capture is selected;do not routinely export every trial to VTU.

## Qualified implementation and preserved evidence

ONE active cycle is shared upper-certification efficiency. Failed query-local
footprint memo250preserved exact5241query results/166result rows but ADDED10.97%
counted query work. REMOVED,with exact patch/failure receipts in
R/footprint250_rejected.patch and footprint250_*.json. Do not reintroduce from hits.
Current251passes the caller's best upper witness to PlaneUnion and abandons its
monotone maximum once it cannot improve that witness. No added state/config/
representation/scheduler/construction policy;all fresh geometry/association/body/
feature/height/final checks retained. [Design/proof](Handbook/ADAPTATION_UPPER_CUTOFF251_20261107.md).

Build251TERMINAL0/33.100301s,1303pins unchanged,both targets current,host handles
absent. Actual seven-terminal247-context comparison passed102.925903s:all5241
ordered identities/cache outcomes/bounds and166complete result rows EXACT;
counted query work5096073→4983517(−2.2087%). Final serial38cases/2044assertions
passed61.301636s;ONE MPI6milestone90cases on EACH rank passed163.894983s.
All qualification handles terminal;R/upper251_qualification.json seals scope.
Build248first serial failed ONLY an unprepared-scope low-memory fixture assumption;
fixture warms exact scope without changing16384ceiling. Failure retained;249passed.
Source251adds no memo and preserves useful hole/body/lift/resource assertions.

Analysis uses existing parsers unchanged. Replace ALL TWO literal directory
references in analyze_m6244_mach_only.py,retain original __file__,assert output
entry/window sets/counts against CURRENT complete stage groups. A251/
live_quality251_analysis_provenance.json records transformations and corrected
post-generation dict-inspection wrapper. compare_quality227.py joins round+stage+
rank to preserve same-round window/exit deltas;do not double count final EXIT.
Earlier247wrong-scope wrapper correction is explicitly preserved;never use its
invalid records. [Detailed pre-compaction handoff](Handbook/History/HANDOFF_Codex_before_compact251_20261107.md)
is HISTORICAL,not live. It links full source/query/export and failure history.

## Baseline,seed and inspectable mesh

Full247first event REFUSED native5347.699778/round34816:1032602cells/1148q/484J/
1044geometry/0size,ALL measured/flags complete. Phase ownership exceeds deadline;
prior ownership correctly retained. Still first shape pass;no geometry closure.
All30capture files sealed,1520pins verified before ONE pidfdSIGTERM to verified
MPI59208/start10786446. Tool14272TERMINAL1/5546.863332s;all owned handles absent.
Never repeat that stop. [Outcome](Handbook/ADAPTATION_BUILD247_OUTCOME_20261107.md).
Actual247diagnostic confirmed speed as immediate target with useful queued work;
geometry repair power remains unmet. No construction-policy change justified.

Latest meaningful saved mesh:A247/inspection/rejected_mesh.vtu and .su2:
201203points/1032602tets,complete nine-component captured_nodal_metric in PointData;
q/J/geometry/edge arrays in CellData. ALL2525bad cores/90430neighboring cavity
cells/reference/rejections retained. Independent meshio readback passed exact
array lengths/coords/connectivity,SPD(min eigen.00270493803760962),all tetspositive.
No ParaView GUI or acceptance claim. Surface extraction uses same volume nodal
tensor. Export only meaningful terminal meshes,not each trial.

Seed R/m6_inputs/euler_original_force_seed_v1:61330points/303359cells,M.8395/AOA3.06,
CL.269535/CD.013536 stable force window,user accepted reasonable forces,NOT full
residual convergence. ITER1continues saved real CFD,not startupCL.059.
SeedSHA1eaee788bd62bd92a92ec539577dcbcfc0f8042dcbcec23adb04e825bb01a880;
initialrestartSHA5132ee2a83635b457943c2679a3a50c5e0b733427e620cc748da53865ce5d692.
MACH ONLY in generator/future M6configs(including RANS);pressure physics/output stay.
219local metric floor/log normalization/flat-sensor fix integrated/qualified.
Combined L1/L2 comparison selected L1;Mach-only production target check:
complexity49999.999626,SPD,zero gradation violations,CFD fields/coords bit-identical.
R/metric_target244_mach_only. No universal norm-optimum claim or new baseline.

## Completion obligations — all open

Need shape closure,strict residual geometry/size closure and shape REclosure,
then TWO accepted WHOLE meshes on all ranks within unchanged budgets.
True CV mass-solve residual<=1e-10 EVERY field/time level,all transferred states
admissible,scaled TARGET integral defect<=1e-12 BEFORE publication. Printed old/new
DOMAIN integral changes and run_m6finite helper are insufficient. Relevant source:
CConservativeTransfer.cpp997–1039/PrintSummary1420+. A247/transfer_precision_provenance.json
is predecessor metadata/binary proof,not current transfer;refresh provenance if251
reaches transfer. Build double/mixed OFF remains required for strict double limits.
Actual adapted_mesh_adap_00001/2.su2+.native_ref and matching
restart_flow_adap_00001/2.dat,exact point order/coords1e-12,finite conserved states,
positive density/internal energy. Refused driver may write adap-named ORIGINAL-grid
restart;that is not adapted proof. Fresh ISOLATED ADAP_LOOP=NO,RESTART_SOL=YES,
READ_BINARY_RESTART=YES actual adapted disk load,distinct input/output prefixes,
finite admissible continuation. Existing seed reader can validate with matching mesh.
Resume-cycle0overwrite/rejected-event consumption remain OPEN,not fixed by isolation.
Then dedicated BL/curved/junction/interfaces/subsequent-ops/ownership/heights MPI,
then RANS500k. Historical broad serial8/344failures include an unattributed RANS
wall-policy4096work refusal;MPI90does NOT qualify dedicated BL. Do not rerun generic
broad suite or weaken assertions. Raw-collapse+retained-star optimizer,motion reuse,
quality-aware construction and new geometry representations remain DEFERRED in
[backlog](Handbook/ADAPTATION_BACKLOG.md). Literature does not establish gate equivalence.

## Working rules

HOST /proc differs from sandbox;identity/resource reads need escalation. One heavy
job,nice19,libs1,build3workers,4TBcompile-tmp;check RAM/load/disk before heavy work.
Origin github.com/rois1995/SU2.git,branch codex/native-3d-bl;fork pushes authorized
with EXACT whitelists. Git metadata outside workspace at SU2_AdapNoExt/.git needs
escalation;gc.auto=0,submodule.recurse=false,ignore-submodules=all. Preserve unrelated
dirty logs/.orig/untracked work;never broad-stage raw meshes/binaries/restarts/logs.
[HowToProceed](HowToProceed.md),[integrated task](Handbook/ADAPTATION_INTEGRATED_NEXT_TASK.md)
(historical155numbers superseded),[placement](Handbook/ADAPTATION_VISIBILITY_PLACEMENT243_20261107.md),
[repair references](Handbook/ADAPTATION_REPAIR_SUPPORT_20261106.md),[AGENTS](AGENTS.md).
