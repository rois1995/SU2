# Native adaptation — current handoff

Updated2026-11-07(Europe/Rome). Goal ACTIVE; ZERO accepted meshes.
Complete TWO strictly accepted ONERA M6 Euler CFD→mesh→CFD→mesh events at50k
METRIC COMPLEXITY, conservative admissible transfer, finite continued adapted CFD
and ACTUAL isolated adapted disk restart. Then dedicated BL/constraint/MPI
qualification and corresponding RANS500k. Actual points/cells are separate.
Fixtures, private candidates, balanced weights and lower memory are not completion.

Strict geometry1e-4,q>=.2,J>=.05,maxedge1.8,requested heights. Construction stays
validity-first: exact positivity/nondegeneracy/topology/body/feature/height validity;
NO fresh q/J/ranking/veto. CAD optional; no-CAD authority sufficient.
ONE connected cycle/ONE owned heavy job; freeze all pinned C++/fixtures/helpers/
binary/config/inputs. Actual HOST process identity overrides stale metadata.

R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
A247=`R/m6_runs/euler_build247_canonical_mach_only_real_cfd50k` (LIVE).
A_MACH=`R/m6_runs/euler_build244_mach_only_real_cfd50k` (terminal REFUSED baseline).

## LIVE — full build247 two-event M6

Tool session14272 remains LIVE. Runner58992/start10785929;
launcher59206/start10786445,MPI59208/start10786446;
ranks59229–59234/start10786457–58. Exact ten identities in A247/owned_startup.json.
Wrapper58990/start10785923 is also recorded in R/m6_247_canonical_launch.json.
Keep these processes/sources frozen until terminal or preserved evidence justifies
an authorized owned stop. Never stop/restart solely on timeout or stale logs.

Runtime pt2pt/private4TBscratch/nice19/libs1/geometry trace UNSET.
Same saved REAL CFD seed,MACH ONLY,L1/50k,native5400/outer14400.
run.cfg byte-identical to Mach-only244:
sha5ad2dc1d9a1cd8bff87f633617b191b50defbc261ab3d0b2ed0987a34722b89e.
Selected bounded raw refusal capture uses the production driver with an observer;
no routine VTU export. A247/command.json,pins.json,run.cfg,owned_startup.json,
resources.jsonl and owned_observations.jsonl preserve runtime/host scope.

Latest complete quality window32448,all-rank census19:
936877cells/68289q/67119J/1215geometry/0size,all measured/flags complete.
Quality1358.564680s;net38165q/39085J/166geometry removed.
Net q/s28.092148 versus25.844990 in baseline at matching round.
Current field differs from baseline by +172q,+88J,−4geometry;do not claim exact
progress equivalence after round32320. First seven windows had identical field
counts/commits/churn;first small divergence32384 was +12q,+17J,+1geometry.
Its cause is UNATTRIBUTED;no certification failure or gate change is established.

A247/quality_comparison244.json covers nine COMPLETE matching windows,exact field
differences,churn,rank debt spread,memo hits/misses and BOTH cumulative-native and
quality-entry-subtracted geometry/smoothing work. First divergence round is explicit.
Cumulative32384upper/traversal work883.831M/834.958M versus987.267M/932.164M;
these include construction. Union hits145593versus120610,misses155235versus180038.
Boundary/interior candidate counts nearly unchanged;rank-summed smoothing timers
OVERLAP geometry timers and are not additive wall savings. Wall rates remain
contention-sensitive;join-call share is not CPU idle. No full-run causal speedup.

Quality entry was EXACTLY identical:878443cells/106454q/106204J/1381geometry/0size,
all measured/flags complete;native689.582599 versus701.671352.
Construction upper_work282283657→274601233,traversal263600528→256967738;
120785local accepted operations in both,not accepted meshes.
A247/quality_entry_comparison244.json and construction_progress_assessment.json.
latest_matched_quality_census.json is an EARLIER32256snapshot with actual baseline
age bracket (no interpolation),not the latest window. Complete prefix hashes are
preserved;unmeasured construction q/J zeros mean UNKNOWN.

## Active implementation and qualified checkpoint

ONE active cycle:canonical SINGLE-triangle numeric certification queries in shared
BoundPhysicalSurface. BOTH memo key and full computation sort exact corner bits;
exact authority/support/tolerance/coordinates and all fresh association/feature/
body/interface/height checks remain. Multi-triangle path unchanged. Nine work units;
admit canonical scratch/key overlap,restore caller bytes after temporaries die;
tight admission uses full-query fallback. No new scheduler/config/operator or
quality-aware construction. No competing motion cache/collapse coupling.
[Design,proof and detailed history](Handbook/ADAPTATION_QUERY_CANONICALIZATION246_20261107.md).

Latest compiled build247TERMINAL0/42.102124s,1303pins unchanged,both targets current,
all five sampled build handles absent. Build246serial failure preserved:two fixture
contracts (unsorted support throws;exact work includes nine canonical units).
Production budgets/gates were not enlarged. Build247serial38cases/1678assertions
passed62.274139s;MPI6milestone90cases on EVERY rank passed165.014869s,pins unchanged,
all ten owned handles absent. R/canonical247_qualification.json and linked cases.
Actual three-context247probe passed92.804422s,pins unchanged/handles absent:
681926→631308counted query work (−7.4228%),same complete candidate outcomes.
This bounded trace is not a whole-M6 performance estimate.

Implementation/qualification pushed f7855ed7ec;startup4a6ea8e44e;
quality-entry0f50053d1a;three-window evidence860a5b6330. Runtime source commit is
f7855ed7ec;later documentation/evidence commits do not change its pinned runtime.

Two diagnostic provenance corrections are explicit,not hidden:
- run_cases now preserves requested config directory with absolute(),matching C++
  companion lookup when run.cfg is a symlink. Earlier three subset probes have
  supplemental actual PRELAUNCH hashes in A_MACH/actual_companion_input_provenance.json;
  their original harness did not pin the subset TSV. Raw shared payloads were pinned.
- An initial ad-hoc247analysis replaced only one hardcoded directory and produced
  old244quality records. Invalid derived hashes/correction are in A247/
  analysis247_scope_correction.json;invalid records were never used for a performance
  claim or pushed.
  Replace ALL two literal directory occurrences in analyze_m6244_mach_only.py
  before executing for247;assert output window set/counts against CURRENT complete
  stage groups. Existing helpers remain frozen. Corrected results are saved.

## Seed, baseline and latest inspectable mesh

MACH is the sole adaptation sensor in generator and future M6 configurations,
including RANS;pressure physics/output remain. Same seed in
R/m6_inputs/euler_original_force_seed_v1:61330points/303359cells,M.8395/AOA3.06,
CL.269535/CD.013536 stable force window,user accepted reasonable forces,not full
residual convergence. ITER1continues saved real CFD,not startupCL.059.
SeedSHA1eaee788bd62bd92a92ec539577dcbcfc0f8042dcbcec23adb04e825bb01a880;
initialrestartSHA5132ee2a83635b457943c2679a3a50c5e0b733427e620cc748da53865ce5d692.
Corrected219local metric floor/log normalization/flat sensors qualified40cases/
207605assertions,MPI6every4cases. Combined-sensor L1/L2 comparison selected L1;
L2coarsened4/11shock points,tip allocation6.44→13.59%,maxAR38→199. L1 held fixed
for Mach-only comparison;no universal norm-optimum claim.
Mach-only production check:complexity49999.999626,SPD,zero gradation violations,
CFD fields/coordinates bit-identical. R/metric_target244_mach_only.

A_MACH terminal first event REFUSED native5347.578358/round34425:
1032169cells/2035q/967J/1046geometry/0size,ALL measured/flags complete.
Still first shape pass;no subsequent geometry-construction closure established.
ALL30capture files sealed;1520pins unchanged before ONE verified owned stop;
tool73370TERMINAL1/5590.219278s,all ten owned handles absent. Historical stop only.
[Mach-only baseline outcome](Handbook/ADAPTATION_MACH_ONLY244_OUTCOME_20261107.md).

Latest meaningful saved mesh:A_MACH/inspection/rejected_mesh.vtu and .su2,
201113points/1032169tets. captured_nodal_metric has nine row-major components in
PointData;q/J/geometry/edge arrays in CellData,complete measured counts above.
3841bad cores/142889neighboring cavity cells,reference/rejections retained.
Independent reader passed exact array lengths/connectivity/coordinates and SPD
(min eigen.002704938038);no ParaView GUI or mesh acceptance claim. Surface extraction
retains the volume nodal tensor,not a fabricated separate metric. Export meaningful
terminal meshes only,never every trial. A247has no terminal inspection export yet.

## Next action and downstream obligations — all open

Continue frozen247to its terminal outcome;compare net whole-field q/J-bad removal,
geometry counts and counted work at complete stages/explicit quality ages.
At refusal preserve/seal bounded evidence and exact source pins before any authorized
owned stop. Use its demonstrated blocker to select ONE next connected cycle.
Measured raw-collapse+retained-star optimizer gap,motion-certificate reuse and
quality-aware construction are DEFERRED hypotheses,not competing implementations.
[Backlog](Handbook/ADAPTATION_BACKLOG.md). Literature does not establish gate equivalence.
Preserve visibility-placement243/244,signed-bound/cache/ownership improvements.

Completion requires shape closure,strict residual geometry/size closure and shape
reclosure within unchanged budgets,TWO accepted WHOLE meshes on all ranks.
True CV mass-solve residual<=1e-10 EVERY field/time level;all transferred states
admissible;scaled TARGET integral defect<=1e-12 BEFORE publication. Printed old/new
DOMAIN integral change is separate. SU2_CFD/src/adaptation/CConservativeTransfer.cpp
997–1039enforces final admissibility/target totals;PrintSummary1420+true residuals.
run_m6finite-integral helper alone is insufficient.

Actual adapted_mesh_adap_00001/2.su2+.native_ref and matching
restart_flow_adap_00001/2.dat,exact point order/coordinates1e-12,finite conserved
states,positive density/internal energy. Refused driver can write an adap-named
restart on ORIGINAL mesh;not adapted proof. Fresh ISOLATED ADAP_LOOP=NO,
RESTART_SOL=YES,READ_BINARY_RESTART=YES load,distinct input/output prefixes,
finite admissible continuation. Existing seed reader can check adapted files with
the matching mesh;historical BDF2audit does not prove this Euler restart.
Resume-cycle0overwrite/rejected-event consumption remain OPEN,not fixed by isolation.
Then dedicated BL/curved/junction/interfaces/subsequent-ops/ownership/heights MPI,
then corresponding RANS500k. Historical broad serial8/344failures include an
unattributed RANS wall-policy4096work refusal;MPI90does NOT qualify dedicated BL.
Do not repeat generic broad suite or weaken assertions to hide it.

## Working rules and links

Actual HOST /proc differs from sandbox;identity/resource reads need escalation.
Check RAM/load/disk before heavy work;one heavy job,nice19,libs1,build3workers,
4TBcompile-tmp. RSS samples are not physical/whole-job peak. No perf/GDB or old-code
baseline campaign. Verify owned handle terminal before source edits/restarts.
Origin github.com/rois1995/SU2.git,branch codex/native-3d-bl;fork pushes authorized
with EXACT whitelists. Git metadata outside workspace at SU2_AdapNoExt/.git requires
escalation;gc.auto=0,submodule.recurse=false,ignore-submodules=all. Preserve unrelated
dirty logs/.orig/untracked work;never broad-stage raw meshes/binaries/restarts/logs.

[HowToProceed](HowToProceed.md),[integrated task](Handbook/ADAPTATION_INTEGRATED_NEXT_TASK.md)
(historical155numbers superseded),[placement design](Handbook/ADAPTATION_VISIBILITY_PLACEMENT243_20261107.md),
[repair-support references](Handbook/ADAPTATION_REPAIR_SUPPORT_20261106.md).
[Detailed prior handoff](Handbook/History/HANDOFF_Codex_before_compact247_20261107.md)
is historical,NOT LIVE;earlier linked topic records preserve accumulated evidence.
