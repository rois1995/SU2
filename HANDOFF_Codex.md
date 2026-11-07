# Native adaptation — current handoff

Updated2026-11-07(Europe/Rome). Goal ACTIVE: TWO strictly accepted ONERA M6 Euler
CFD→mesh→CFD→mesh at50k metric complexity, conservative admissible transfer, finite
continued adapted CFD and isolated actual adapted disk restart. Then BL/constraint/
MPI qualification and corresponding RANS500k. Actual mesh counts are separate.
ZERO accepted adapted M6 meshes so far. Passing fixtures are not completion.

Geometry1e-4,q>=.2,J>=.05,maxedge1.8,requested heights remain strict. Construction
stays validity-first: exact positive/nondegenerate/topology/body/feature/height
validity, no fresh q/J measurement/ranking/veto. CAD optional; no-CAD sufficient.
One connected cycle/one owned heavy workload. Freeze compiled/executing/pinned
sources, fixtures, helpers, binary/config/inputs through each run. Verify actual
owned handle terminal before edits/restarts. Preserve failures; authorized stop/
fix/retry for demonstrated failures, clear bottlenecks or actual resource pressure.

## TERMINAL227; exported mesh+metric verified, ACTIVE residual shape recovery

Full goal remains ACTIVE/zero accepted adapted meshes. Source bd653aa58f and
late-run evidence0e9b07bf61 PUSHED.227geometry witness screening passed build,
serial102cases/35396assertions and MPI6each26cases; diagnostic12faces had exact
bounds/production guidance and traversal-45.7%. Whole construction had-17.6%
traversal with all76tested stage/count/operation vectors equal225. No gate/config/
construction-policy change. [Detailed evidence](Handbook/ADAPTATION_GEOMETRY_WITNESS_20261106.md).
[Terminal227 outcome](Handbook/ADAPTATION_BUILD227_OUTCOME_20261107.md).

A227=R/m6_runs/euler_build227_witness_real_cfd50k. Full M6exec50998TERMINAL1,
wrapper5824.165090s,pins_verifiedtrue/changed[]. First native event1 naturally
rejected at5335.996158native seconds,round37022: metric update exhausts time budget.
Final1017523cells, but ALL final census flags0/measurements unknown: printed zero
q/J/geometry counts are NOT passing counts. Last COMPLETE census36992 at5303.549478s:
1017224cells/2691q-bad/1295J-bad/1700geometry-bad/0size-bad, all flags1. No transfer
or adapted CFD/restart proof. A227/capture_ready.json preserves both states.

All30capture files COMPLETE for owned/reference/rejection data, stable/hashed;
metrics_completefalse on all6ranks. Six export receipts complete1,416-byte framing
and1017523owned rows checked. Ready validator initially assumed a root-only rejection
message on every rank; corrected, prior rank0prefix preserved, no raw data changed.
A227/capture_readiness_attempt0.json records checker-only failure.
After evidence sealed, SIGTERM sent ONCE via verified pidfd to launcher1929998/
start4973666. NEVER REPEAT. A227/owned_stop_once.json. All10exact host solver handles
ABSENT;30capture files/1519pins reverified unchanged after stop. A227/terminal_handles.json
and terminal_source_receipt.json. Historical225/219/217/210stops also never repeat.
No C++/runtime helper has changed after this run yet.

Selected export exec24795TERMINAL0,362.480822s; all3host handles absent and
helper/30raw input hashes unchanged. Independent reader4exec83790TERMINAL0,
2.755319s. A227/inspection_artifacts.json: full rejected_mesh.vtu and .su2 have
198364points/1017523tetrahedra, exact matching coordinates, finite symmetric SPD
9-component captured nodal metric on every point. Full mesh+metric COMPLETE.
Final q/J/geometry caches ALL unknown/NaN with cache_valid0. bad_cells.vtu and
cavities.vtu are EMPTY because classification is unavailable, NOT because gates
pass. Reference complete. No ParaView GUI qualification. Earlier reader attempts
are preserved: meshio rejects empty ASCII FieldData; structural retry initially
mistook zero-length binary payload headers for data. Corrected checker validates
both empty ASCII and zero-byte binary arrays. Full mesh reads passed throughout.

Diagnostic build228exec13608TERMINAL0,165.163135s,1302pins unchanged/both targets
up to date. Native captured_shape_census exec79887TERMINAL0,11.224238s,
3052623assertions/1case, all pins unchanged. Attempt R/cases/
captured_shape_census_20261107T010656_2223729. Recovered1017523centroid q/J rows:
2368q-bad/1101J-bad. This is final-cell shape DIAGNOSTIC with actual captured nodal
tensors, not geometry/metric-edge/final acceptance certification. All recorded
build/diagnostic host handles verified absent. No production source changed yet.
Actual shape_inspection/{bad_cells,cavities}.vtu saved with explicit
q_centroid_diagnostic/J_centroid_diagnostic and the captured metric.3258bad-union
cells and141392complete one-node-neighbor cells; existing q/J/geometry/cache fields
remain unknown. Exact ID/version/native-record join, array/tensor readback and all
input hashes verified. inspect_shape227.py exec30027TERMINAL0,13.020296s.
select_shape227.py exec43451TERMINAL0,3.710035s: actual residual classes include
1414interior cells touching wing vertices versus1341free interior,452wing-face,
39other physical-face,12interior touching other boundary. Thus most residuals
classified interior by face census are not all unconstrained volume stars.

Preparation exec20376TERMINAL0,37.840728s: all1017523paired records checked,
32665boundary bindings/65326physical faces, all input hashes unchanged. Four actual
residual contexts sealed in A227/current_witnesses_v1/witness_preparation.json.
Existing current219fixture generalized via optional diagnostic case TSV (legacy
defaults retained), no larger active cavities/supports. Recovered centroid q/J is
oracle; captured edge caches explicitly unavailable. Fresh full boundary/feature
reconstruction, not historical private publication replay.

SOLE heavy workload: build229exec43967LIVE, host ninja2241504, nice19/libs1/3workers,
existing build_checkpoint.py. All C++/runtime helpers FROZEN until build and actual
current serial diagnostic terminal. Next run SU2_NATIVE_FROZEN_CONFIG pointing to
A227/current_witnesses_v1/run.cfg with run_cases.py build229 current_residual,
OMPI_MCA_osc=pt2pt and4TBscratch. Then choose connected closure hypothesis and ONE
MPI6 qualification; full same-seed M6 follows. Do not reuse old build227/228pin
assertions as current after intentional source changes.

Production optional-ownership guard locally changed, UNBUILT/UNVALIDATED: collective
AND of local admission decisions using existing world.maximum prevents split
participation before repartition. No scheduler/config/gate/budget change. All old
227/228source pins now historical after intentional Engine/fixture edits. After build229terminal, run actual-current serial diagnostic,
select ONE connected closure hypothesis from those results, qualify affected
serial behavior plus ONE MPI6 milestone, then immediately same-seed full M6.

Deadline invalidated final measurement cache. Existing local218test tag
NativeCapturedShapeCensus3D was reviewed/generalized: historical790537row assertion
replaced with positive total while retaining exact per-shard counts/end checks.
Uses production EvaluateCavitySample and Measure on actual captured nodal tensors. Do not treat centroid diagnostics as geometry/edge/transfer/final
acceptance certification, or fabricate cache-valid geometry. Existing inspector
is generic; prepare_geometry225_queries.py is225-specific and stays unchanged.

Timing/comparison: nineteen quality windows matched225through35776. At35776old225
skips migration (<=443s remaining vs621.8s safety hurdle),227admits (~1997s vs403.7s).
Next35840newhas251fewerq/333fewerJ/+5geometry/+221cells. Later paths differ; no pure
kernel timing attribution. A227/first_quality_divergence225.json and counter-only
R/compare_quality227.py. Its matched baseline tail stops35968; use actual current
progress/last-complete state, not that tail, for later227windows. Timers overlap;
join-call wall share is not CPU idle, private smoothing debt is not global gain.

REQUIRED postterminal source correction within this SAME cycle (implemented locally,
not yet built/qualified): optional ownership
admission uses rank-local wall/ownership timers without a collective proceed vote
(CNativeEngine3D.cpp2671-2689). False ranks return while true ranks enter collective
repartition. No actual split was observed in sealed28group six-rank audit;
A227/ownership_admission_alignment.json. Correct/qualify with existing collective
facilities after diagnostic export; no new scheduler/config. [Backlog](Handbook/ADAPTATION_BACKLOG.md).
Then use actual terminal counters and representative residual boundary/volume
cavities to choose ONE connected closure hypothesis; test existing construction/
admission facilities before adding operators or larger supports. New ideas stay
in backlog. Source-freeze each build/validation and one MPI6 milestone, then
immediately full same-seed correctedL1/50000two-event M6, native5400 unchanged.

Input proof retained: configSHA74b2e9f476cf6ba1fdc5ebcdcb1c34a2b3c45b25e58d651e75417d89402bfe8d EXACT225;
initial restartSHA5132ee2a83635b457943c2679a3a50c5e0b733427e620cc748da53865ce5d692
BYTE-IDENTICAL225/219(coords,6tensor/7flow fields), CL.269535/CD.013536. Actual mesh
counts are separate from complexity50000. Solver retained the original accepted
CFD state after rejection; this is not continued CFD on an adapted mesh.

After shape closes, residual geometry/size repair must preserve shape/heights;
validity-only geometry construction requires shape remeasurement/reclosure. Require
two accepted meshes, trueCV residual<=1e-10/full admissibility/scaled target-integral
1e-12 BEFOREpublication, finite continued CFD and fresh isolated adapted disk restart.
Workflow helper finite-integral flags do not establish these proofs. Then BL/MPI
qualification and RANS500k. Old227source-pin assertions become historical after
intentional next-cycle source edits; preserve this sealed evidence, do not overwrite.

## TERMINAL225: rejected; full mesh+metric saved and independently read

R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
A225=`R/m6_runs/euler_build225_entity_shell_real_cfd50k`.
Build225TERMINAL0(33.114415s),1302pins unchanged, both targets up to date.
Final serial exec34204TERMINAL0:116cases/75941assertions PASS across geometry88,
quality13,failure/census14,actual-current1. MPI6exec35336TERMINAL0:26cases on EACH
of6ranks PASS. All validation handles terminal. Full M6exec32146TERMINAL1:
first event naturally rejected time budget; wrapper stopped ONCE only after complete
capture. All10owned solver handles ABSENT/1519pins unchanged: A225/terminal_handles.json.
SIGTERM ALREADY SENT ONCE to launcher1531290/start4040862; NEVER repeat. Historical
219/217/210launchers also must never be signaled. Source checkpoint60b32aaf74 PUSHED.
Selected mesh+metric export exec13387TERMINAL0(428.700427s); all3owned export
handles absent, helper/30raw input hashes unchanged. Reader exec45220TERMINAL0
(3.378982s). No owned heavy workload. Exact receipts:
A225/inspection_export_terminal_handles.json and inspection_artifacts.json.
Actual full A225/inspection/rejected_mesh.vtu WITH9component captured_nodal_metric;
also rejected_mesh.su2.193855points/987865tets. Independent meshio5.3.5 reads all4VTU
and SU2 PASS; point tensors all finite/symmetric/SPD/available, q/J/geometry counts
match terminal exactly. Full mesh/classifications COMPLETE. Bounded bad_cells.vtu
omits4585of37353all-gate bad cells; cavities.vtu omits177089neighbors. Reference
complete. No ParaView GUI qualification. Surface extraction inherits the same3Dtensor.
R/shell225_qualification.json is the complete focused receipt, not workflow proof.

Connected production correction:
- ShellBoundaryCell uses exact existing EntityKey(body,sheet,marker) equivalence
  across released original subfacet seams; legacy raw-facet rule retained.
- Validity-only optimizer shell may fall back from a failed strict new-face bound
  to COMPLETE non-increasing whole-ring geometry guidance, matching smoothing.
  Absolute/final gates and existing height/metric/interface/feature checks retained.
- Common ImportGeometry now compares complete geometry declaration bytes before
  metric/source-envelope branching. Global maximum serialized bound admitted before
  broadcast. This fixes demonstrated MPI nonempty-version mismatch, without
  restoring global source envelopes or adding slot collectives/configuration.

Actual current219witness validation enables2of4previously stalled root stars:
best context q5→3/J3→2/geometry8→6. Raw midspan shell now constructs, but its
geometry excess INCREASES despite fewer bad cells; optimizer correctly refuses it.
All24core-edge collapses still refuse exact construction; do not add that proposed
postcollapse optimizer as a remedy. Full feature publication remains authoritative;
private candidate construction alone does not prove nonlocal/feature acceptance.
[Diagnosis, controls and design](Handbook/ADAPTATION_REPAIR_SUPPORT_20261106.md).

Two old envelope-import tests were corrected to current nodal semantics introduced
in159a5fe538: no auxiliary shell, exact owner tensors/row attributes and original
CFD donor/history preservation through N/M migration. Initial broader failure and
subsequent real MPI declaration bug are preserved in R/shell223_failed_atlas_
assessment.json and shell224_failed_mpi_assessment.json. Missing TMPDIR caused a
separate preserved setup failure; helper now defaults to existing4TBscratch and
records it. All affected checks subsequently PASS; no production import-mode rollback.

The completed225run used the same qualified real CFD seed, correctedL1/50k,6ranks/nice19/libs1,
5400native/14400outer/two events. Generated config BYTEIDENTICAL219:
SHA74b2e9f476cf6ba1fdc5ebcdcb1c34a2b3c45b25e58d651e75417d89402bfe8d.
Selected --inspect-rejected for this first coupled-boundary attempt after corrected
metric219: actual leading-edge before/after mesh+metric is useful. No per-trial
mesh exports. Actual initial restart coordinates,6metric components and7flow fields
BITWISE match219and its assessed production L1target: A225/initial_target_match.json,
restartSHA5132ee2a83635b457943c2679a3a50c5e0b733427e620cc748da53865ce5d692.
Restarted CL.269535/CD.013536; curved mobility181938600setup work.
All76matching complete construction stage/round pairs have identical tested
per-rank work vectors, totals and flags versus219: construction_comparison219.json.
Coarsening2520collapses/10313geometry trials/106031874traversal work are identical,
but953.638858s vs301.972880s(~3.158xwall): coarsening_comparison219.json.
Final validity-only construction1527.934458s vs753.370630s; q/J unmeasured there.
First COMPLETE quality entry round34594 at1584.159718native seconds:
871607cells/102058q/101860J/1997geometry/0size, allflags1, counts/flags match219.
Counts alone do not prove identical connectivity. quality_entry_assessment.json.

COMPLETE first native EXIT round35976 at5326.087064s/3741.927346quality seconds:
987865cells/24200q/21065J/1784geometry/0size, all completeness flags1.
All30capture files complete/hashed; binary row counts and1519pins verified unchanged.
A225/capture_ready.json,first_failure_prefix.json,terminal_counter_assessment.json.
Net quality removal77858q/80795J/213geometry across49649commits, with substantial
churn532222qretired/454364introduced. Seven stuckshape/queue324400. Terminal
qsurface1731/interior22469; Jsurface3014/interior18051.16metric samples supported/
0lost/max5.00896%, sample-only. No accepted mesh/transfer/continued-CFD/restart proof.
Next target SPEED: geometry certification/boundary trial cost within this SAME cycle;
shape still progresses with a large nonempty queue. Boundary repair power remains
a residual risk. Review existing reuse/bounds first; no speedup from slack envelopes
is established. [Outcome and next analysis](Handbook/ADAPTATION_BUILD225_OUTCOME_20261106.md).
quality_net_progress.json seals first episode windows; actual EXIT is in terminal
receipt. matched_quality_comparison219.json preserves rounds AND observed neighboring
quality-time censuses, explicit differences/no interpolation/forecast/count bounds.
sixteenth_quality_comparison219.json: traversal48.413Mvs29.752M, upperwork55.693Mvs
38.220M, uppercalls37206vs32545; boundary candidates24339vs19080/moves1024vs734/
private debt30.413vs19.972/time100.846vs82.216rank-s. Boundary private yield is
better here, but whole-field q/commit1.297vs1.310 and q/s19.73vs26.46 are worse;
geometry removal23vs3 is better. This mixed result does not establish closure.
quality_location35072_assessment.json: qsurface6629/interior62575, Jsurface10112/
interior58806; badunion90207,90183with edge<.9,87801with edge<.5. Interior means
no physical face, not necessarily unconstrained vertices. Short-edge association
is not proof of construction causality. Stuckshape0/queue585178. All16fixed metric
samples supported/0lost, maxrelativeFrobenius3.92349%; not a whole-field drift bound.
All six quality redistributions35648/35520/35456/35136/35008/34752preserved counts/flags.
Earlier35520q/J debt min/max413.94/814.15→500.04/704.25; q-bad range4069–9586→5271–9601.
Previous transition narrowed debt but widened q-bad range6548–9214→4778–10532;
this is not a blanket balancing/quality win. 35520window-to-committed14.4908s
includes transition work. These observations do not independently prove every
history field.

Detailed first/fourth/fifth_quality_comparison219.json preserve per-rank service,
churn and geometry work. First window16.2916q/s vs16.1765, both1170commits;
qretired31647/qintroduced29625 shows substantial churn. Fifth window368.8705s
vs191.6450s, traversal126.922Mvs92.479M; uppercalls37683vs31230; boundary
candidates15565vs14487/moves628vs662/private debt63.192vs66.511/time297.77vs140.49
rank-s. Extra work and lower boundary yield accompany contention. Timers overlap;
join/collective call seconds are not pure wait/CPU; private debt is not whole-field
benefit. OS process_cpu_observations.jsonl includes possible MPI spin, not useful
work proof. No profiler/new baseline/competing implementation is selected.

Solver and export owned handles are all ABSENT; owned_stop_once.json proves the
one solver postfailure stop. Never repeat it. No export stop was sent. Functions
store m6_225_owned_observe_cmd verifies terminal solver handles/pins;
m6_225_capture_readiness_cmd reads the sealed receipt and never signals.
Full mesh+metric export and independent reader checks are complete. Original raw
capture remains complete; bounded bad-cell/cavity view omissions are explicit.
Next ACTIVE work is analysis first of cheaper geometry certification/boundary trial
cost using existing facilities and actual evidence, then ONE justified connected
change/affected validation/full M6. No configuration/gate/budget/construction-policy
change selected; no certificate-slack/envelope speedup established. See225outcome.
Judge complete whole-field q/J-bad net removal/s and geometry-bad at matching
quality times, alongside actual work/busy/wait/rank spread. Timers overlap;
commits/debt alone do not establish closure. Construction remains validity-first.

## Corrected target and previous terminal M6:219

Shared2D/3DComputeMetric local regularization/log normalization/flat handling was
reviewed/integrated6209e172e3; related serial40cases/207605assertions and MPI6each
rank4cases PASS. Actual same-seed production L1/L2at50k assessed before remeshing:
R/metric_target219_production/{comparison,selection}.json. Both hit50k/gradation;
L2coarsens4of11shock samples and raises tip budget6.44%→13.59%. Select corrected
L1; no assumed L2benefit. [Metric review](Handbook/ADAPTATION_METRIC_FLOOR_20261106.md).

Saved real CFD seed: R/m6_inputs/euler_original_force_seed_v1,61330points/303359cells,
M.8395/AOA3.06,CLmean.2695353537/CD.0135358565(force-stable last200, not full residual
convergence). SeedSHA1eaee788bd62bd92a92ec539577dcbcfc0f8042dcbcec23adb04e825bb01a880.
Each first remesh loads this seed and continues1iteration; event2must use an
ACCEPTED first mesh and continued CFD. Never count post-reject original-grid work.

A219=`R/m6_runs/euler_build219_local_metricL1_real_cfd50k`.
First native event naturally rejected time_budget at max5337.855034s/round36525.
COMPLETE6rank census:1011596cells/10286q/6872J/1753geometry/0size-bad, allflags1.
Entry at798.559652s:871607cells/102058q/101860J/1997geometry/0size. No geometry
repair phase reached. TerminalCounter/selection/capture receipts are in A219.

After all30complete capture files were hashed, SIGTERM ALREADY SENT ONCE to verified
launcher987277/start2728523; NEVER repeat or signal historical handles. Tool96966
TERMINAL1,all10owned handles ABSENT,1518pins unchanged: terminal_handles.json.
Historical217and210launchers were also already stopped; never reuse their IDs.

Actual selected219inspection: A219/inspection/rejected_mesh.vtu WITH9component
captured_nodal_metric and availability; also rejected_mesh.su2.197550points/
1011596tets; independent meshio reads/counts/tensor SPD checks PASS. Full mesh and
16721bad-cell view COMPLETE. Only cavities.vtu partial:262144cells exported,
136409neighbor cells omitted. Reference complete. No ParaView GUI qualification.
A219/inspection_artifacts.json contains hashes. Surface extraction inherits the
same3Dnodal target; tangential restriction is used for surface motion. Final current
nodal-density integral50985.79(+1.97%) is diagnostic: quadrature/transport effects
not separated, NOT an immutable-background drift bound.16fixed samples max4.3994%,
all supported; sample-only. No target renormalization/transport change selected.

## Closure and downstream obligations

Close shape/size/requested heights, then existing EnableGeometryRepairs must preserve
strict q/J/size/heights while reducing complete guidance. If bounded geometry
construction is needed, subsequently reclose shape/size/heights. Final whole-mesh
two-sided geometry/association/body/feature gates remain independent.

A225/workflow_acceptance_readiness_current.json (copied from R) revalidates all4transfer/projection/kernel/
driver source paths against reviewed219and225pins. Require actual true-CV mass
residual<=1e-10, full admissibility and scaled target-integral1e-12 BEFOREpublication.
Donor-vs-new domain integral change is separate; finite post-CFD does not prove
immediate conservative transfer. After TWO accepted meshes, audit matching actual
adapted_mesh_adap_00001/2.su2/native_ref and restart fields, exact point order/coords,
finite conserved states/rho/internal energy. Then fresh isolated ADAP_LOOP=NO disk
restart with distinct saved input/output prefixes, actual load and finite admissible
continuation. General remesh resume cycle0/output collision remains open; isolated
solver restart does not claim it fixed. Then dedicated BL curved/junction/interface/
subsequent-operation/redistribution/height MPI qualification and RANS500k.

## Operations and links

Check actual host load/RAM; one owned heavy job,nice19,libs1,build3workers. Scratch:
integration_evidence/build-integrated-v2/compile-tmp on4TB. Account RSS separately
from cache/physical memory. Push exact reviewed whitelists regularly. Fork origin
https://github.com/rois1995/SU2.git,branch codex/native-3d-bl; Git metadata outside
workspace requires escalation. gc.auto=0/submodule.recurse=false/ignore-submodules=all.
No broad staging/checks. No mesh/binary/restart/raw solver logs committed. Preserve
unrelated logs/analysis,.orig,broken Eigen submodule,user steering, and local218
captured_shape_census edits in native_failure202_witness.inc/run_cases.py(unexecuted).

- [How to proceed](HowToProceed.md)
- [Integrated task](Handbook/ADAPTATION_INTEGRATED_NEXT_TASK.md)
- [Geometry review](Handbook/ADAPTATION_GEOMETRY_CERTIFICATION_20261101.md)
- [Repair/support and literature](Handbook/ADAPTATION_REPAIR_SUPPORT_20261106.md)
- [Actual inspection](Handbook/ADAPTATION_M6_INSPECTION_20261101.md)
- [Assessment review](Handbook/ADAPTATION_TECHNICAL_ASSESSMENT_REVIEW_20261101.md)
- [Backlog](Handbook/ADAPTATION_BACKLOG.md)
- [Detailed pre225handoff archive](Handbook/History/HANDOFF_Codex_before_shell225_launch_20261106.md)
