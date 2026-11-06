# Native adaptation — current handoff

Updated2026-11-06(Europe/Rome). Goal ACTIVE: TWO strictly accepted ONERA M6 Euler
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

## LIVE225: complete two-event M6, FROZEN

R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
A225=`R/m6_runs/euler_build225_entity_shell_real_cfd50k`.
Build225TERMINAL0(33.114415s),1302pins unchanged, both targets up to date.
Final serial exec34204TERMINAL0:116cases/75941assertions PASS across geometry88,
quality13,failure/census14,actual-current1. MPI6exec35336TERMINAL0:26cases on EACH
of6ranks PASS. All validation handles terminal; full M6exec32146 LIVE is the sole heavy workload.
Source integration checkpoint60b32aaf74 is PUSHED. Helper1530921; launcher1531290/
start4040862; ranks1531301/2/3/4/5/10. Exact all10argv/cwd/startticks and1519pins
are in A225/owned_startup.json. All owned handles verified live; NO stop sent.
Freeze C++/fixtures/helpers/binary/config/inputs until terminal or preserved evidence
justifies the authorized owned stop. NEVER signal historical219/217/210handles.
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

LIVE run uses same qualified real CFD seed, correctedL1/50k,6ranks/nice19/libs1,
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

Latest COMPLETE window35328 at4145.132923s(2560.973205s since entry):
941827cells/53183q/52357J/1869geometry/0size; allflags1. Twelfth window net3282q/
3336J/1geo removed in169.328883s(19.38q/19.70J/s). Matching219repair round has
51659q/50688J/1850geometry at2094.318080quality seconds. Near current quality
time219has39067q/37007J/1821geometry at2575.566893s,14.593688s later. These are
observations, not bounds/interpolated counts. Previous near-exact time comparison:
22560563q at2213.197066s versus21948084q at2210.501935s. Useful progress materially
trails219; no demonstrated whole-field benefit. Contention and changed geometry
work are not isolated. quality_net_progress.json contains first quality episode
only; matched_quality_comparison219.json records BOTH matching rounds and observed
neighboring censuses at matching quality time, explicit time differences and no
interpolation/forecast/count bounds. Keep frozen to its outcome.
quality_location35072_assessment.json: qsurface6629/interior62575, Jsurface10112/
interior58806; badunion90207,90183with edge<.9,87801with edge<.5. Interior means
no physical face, not necessarily unconstrained vertices. Short-edge association
is not proof of construction causality. Stuckshape0/queue585178. All16fixed metric
samples supported/0lost, maxrelativeFrobenius3.92349%; not a whole-field drift bound.
All three quality redistributions35136/35008/34752preserved counts/flags. Latest
q/J debt min/max1386.07/2291.25→1589.87/2063.47; window-to-committed16.7911s includes
transition work. These observations do not independently prove every history field.

Detailed first/fourth/fifth_quality_comparison219.json preserve per-rank service,
churn and geometry work. First window16.2916q/s vs16.1765, both1170commits;
qretired31647/qintroduced29625 shows substantial churn. Fifth window368.8705s
vs191.6450s, traversal126.922Mvs92.479M; uppercalls37683vs31230; boundary
candidates15565vs14487/moves628vs662/private debt63.192vs66.511/time297.77vs140.49
rank-s. Extra work and lower boundary yield accompany contention. Timers overlap;
join/collective call seconds are not pure wait/CPU; private debt is not whole-field
benefit. OS process_cpu_observations.jsonl includes possible MPI spin, not useful
work proof. No profiler/new baseline/competing implementation is selected.

All10exact owned handles remain LIVE/no stop: owned_observations.jsonl.
Sampled summed RSS briefly12.635GB during migration, then10.436GB; available
RAM26.252GB(decimal),load10.73 at the latest eleventh-window observation. Not a whole-job physical-memory
peak. Keep sources/fixtures/helpers/binary/config/inputs FROZEN to its outcome.
Judge complete whole-field q/J-bad net removal/s and geometry-bad at matching
quality times, alongside work/certification/busy/wait/rank spread. Commits/debt
alone do not establish closure. Construction remains unchanged.

## Corrected target and latest TERMINAL M6:219

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
