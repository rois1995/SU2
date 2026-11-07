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
A251=`R/m6_runs/euler_build251_upper_cutoff_mach_only_real_cfd50k`(LIVE).
A247=`R/m6_runs/euler_build247_canonical_mach_only_real_cfd50k`(REFUSED baseline).

## CURRENT — frozen251 full TWO-event M6 LIVE

Tool5327LIVE;source checkpoint1943eb3199pushed. Same saved realCFDseed,MACH-only
L1/50k,strict gates,native5400/outer14400,pt2pt/private4TBscratch/nice19/libs1;
geometry trace UNSET. ConfigSHA5ad2dc1d9a1cd8bff87f633617b191b50defbc261ab3d0b2ed0987a34722b89e
matches247exactly. Initial continued CFD CL.269535/CD.013536;61330point-sensor fits.
HOST identities:runner475457,time475668,timeout475669,MPI475670,
workers475673–475678. Exact starts/argv/cwd and wrapper identity in
A251/owned_startup.json,R/m6_251_startup_checkpoint.json and live
R/m6_251_upper_cutoff_launch.json. Status/elapsed alone NEVER prove termination.
Latest HOST verification at3524.7s:all10sampled handles live,ALL1520pins unchanged,
sampled process-tree RSS10.708GB,PSI averages0.00. A251/quality_window20_live_observation.json.
RSS is a sample,not physical or whole-job peak. Owned VmSwap=0at prior check;
full host swap had0swap-out/9swap-in pages over108.93s,PSI0.00. No memory pressure.
Resource snapshot AFTERwindow13had load8.03/8.10on CPUs0–7,CPU PSI some4.85/4.55/4.71.
Workers cumulative scheduler waits27–29s over~2710s;CPU includes MPI spinning.
No comparable baseline scheduler sample exists,so this does NOT establish the
cause of timing differences. A251/scheduler_observation_after_window13.json.

TWENTY-window comparison snapshot through33152/census34,quality age2579.014368s:
990530cells/32031q/28869J/1156geometry/0size,ALL measured/complete flags.
Exactly matching247counts/45458commits/churn;net74423q/77335J/225geometry removed.
Net q/s28.85715vs29.26471;baseline age2543.096958s,offset+35.917411s.
The EARLY timing advantage has NOT held. Whole-native3256.667975vs3232.679556.
Timings observational;no causal speedup/regression or closure established.
Quality upper1123249596→1111190322(−1.0736%),same691887upper queries.
Boundary kernels377367candidates/16145moves in both,2112.458520→2105.805401summed-rank
seconds;interior266921/34488,150.503772→153.993901. Timers OVERLAP geometry/other work;
kernel moves/debt are not published whole-field gain. smooth_boundary_reused=3is
recorded without diagnosing a new blocker before terminal.

A251/quality_comparison247.json verifies complete raw prefixes,matching rounds,
explicit nearest-age pairs and whole-field churn identities. quality_counter_assessment.json
has current location/ladder/rejections/height/metric-drift counters. Latest q-bad
location:29238interior/2793surface;stuck_shape6,missing features0,tracked
private collectives0. Interior means no physical face,not necessarily free vertices.
SIX actual ownership publications preserve whole-field measured counts/flags:
A251/ownership_census_integrity.json. Balanced debt is not mesh acceptance.
First quality entry EXACTLY247:878443cells/106454q/106204J/1381geometry/0size,
all complete. Same cfg,CFDseed AND initial continued original-grid restart bytes;
A251/quality_entry_comparison247.json. Construction120785commits/52092upper calls
in both;upper274601233→271243211(−1.2229%),native689.582599→677.653607observational.

Keep the full run FROZEN until its terminal outcome or preserved evidence warrants
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
