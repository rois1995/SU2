# Native adaptation — current handoff

Updated2026-11-06(Europe/Rome). Goal ACTIVE: TWO strictly accepted ONERA M6 Euler
CFD→mesh→CFD→mesh at50k metric complexity; actual conservative admissible transfer,
finite continued adapted CFD and isolated usable adapted disk restart. Then
BL/constraint/MPI qualification and corresponding RANS500k. Actual mesh counts
are separate from metric complexity. ZERO accepted adapted M6 meshes so far.

Geometry1e-4,q>=.2,J>=.05,maxedge1.8 and requested heights stay strict. Construction
remains validity-first: positive/nondegenerate/topology/body/feature/height validity,
no fresh q/J measurements/ranking/vetoes. CAD optional; no-CAD authority sufficient.

## Current cycle: reviewed local metric correction, selected L1

The shared2D/3DComputeMetric patch is REVIEWED and integrated for the next M6.
CSolver.cpp local Hessian relative regularization/logarithmic normalization/flat
handling and AdaptationMPI_tests.cpp regressions are the production change.
Build219TERMINAL0, all1301source pins and source/binary/check hashes reverified.
Related serial40cases/207605assertions; MPI6all6ranks4cases PASS. Reuse that evidence;
no new AD/single-precision or geometry-empty-rank qualification claim. No gate,
operator, budget or construction-policy change. Preserve unrelated working edits.

Actual production L1/L2assessment COMPLETE, no remeshing: same saved CFD seed,
one continuation iteration, MPI6sequential, actual binary restart tensors. Seven
conserved/primitive fields BITWISE identical to build217and between norms.
R/metric_target219_production/comparison.json and selection.json are current authority.
Both50kand all778330directed-edge checks PASS. CorrectedL1old-plateau share33.02%,
L2share3.45%; shock-section medians4.37–6.54%chord and3.62–6.03%respectively.
L2coarsens4of11samples and raises radius.05tip budget6.44%→13.59%; finest tip
normal request4.46e-5→6.64e-6. Select **L1**, isolating the proven correction without
an unproven stronger corner objective. Existing default ADAP_NORM=1is verified.
Production M6 is now LIVE as recorded below.
All18metric-assessment owned handles ABSENT; frozen assessment pins unchanged.
[Review and measured target choice](Handbook/ADAPTATION_METRIC_FLOOR_20261106.md).

## LIVE219: corrected-target complete two-event M6

Pushed implementation/target checkpoint6209e172e3. Tool96966 LIVE.
A219=`R/m6_runs/euler_build219_local_metricL1_real_cfd50k`.
Host helper987033/start2727950; time987275/timeout987276/mpirun987277allstart2728523;
6ranks987280–987285allstart2728526. A219/owned_startup.json is exact authority.
1518pins unchanged;84actual mapped ELFs covered by pins. R/metric219_m6_launch.json
records resources/environment. One owned heavy workload, nice19/MPI6/libs1/4TBscratch.
Freeze all executing/pinned sources/helpers/config/inputs. Never infer terminal
state from elapsed time or log age; poll tool/verify exact owned handles.

Same saved real CFD seed,50k/defaultL1, unchanged5400native/14400outer budgets and
two events. Selected first-rejected snapshot capture enabled because this is a
substantial changed-target checkpoint; no trial dumps. Config identical217.
Actual initial original-grid restart and all6metric components plus7conserved/
primitive fields are BITWISE IDENTICAL to assessed correctedL1restart(SHA5132ee2a...
5d692). A219/initial_target_match.json. This is not adapted transfer/restart proof.
Initial native partition has executed; await outcome. No source improvement speedup
claim against217because target changed. All strict gates and construction policy retained.

If native event naturally rejects, preserve all6first-event EXIT/DETAIL rows AND
complete6-rank capture shards/hashes before any authorized single owned-launcher
stop. Final census completeness must be checked; never treat partial zero counts
as closure. Do not repeat historical217or210stops. If accepted, continue event2,
conservative admissible transfer, finite adapted CFD and isolated actual restart.

User literature steering is recorded in
[repair/support diagnostic](Handbook/ADAPTATION_REPAIR_SUPPORT_20261106.md): AFTER
this corrected-target outcome, choose representative actual boundary cavities,
distinguish placement/constraints/connectivity/certification, and test ONE combined
surface-volume hypothesis with existing operators and complete neighboring-patch
q/J/geometry/metric/height checks. Papers authorize no new scheduler/larger cavity/
geometry representation or quality-aware construction policy. CAD remains optional.
218diagnostic built but was not executed before user pause; keep its unrelated local
fixture/helper edits deferred while219drives the sole active cycle.

## TERMINAL217: latest requested mesh and metric saved

R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
A217=`R/m6_runs/euler_build217_real_cfd50k_inspection_metric`.
Selected full two-event M6 used pushed sourcebb61db8c0d; production source/binary
remain6744638324/EXACT210binary. Same saved real CFD seed, byte-identical210config
SHA74b2e9f476cf6ba1fdc5ebcdcb1c34a2b3c45b25e58d651e75417d89402bfe8d,
50k/two events/5400native/all gates unchanged. First event NATURALLY failed
`time_budget`, max5346.271730native seconds, round25473,790537working tetrahedra.
ZERO accepted meshes. All6EXIT/DETAIL rows preserved in event1_natural_failure.json.
Final full census is INCOMPLETE on5ranks: q0/J0 and geometry42 in EXIT are NOT
whole-field closure counts. Final working mesh and nodal tensor capture ARE complete.

After preserving natural failure and hashing all30capture files, sent SIGTERM ONCE
to verified launcher552832/start1489019. NEVER repeat or signal historical PIDs.
M6tool27489TERMINAL1; all10exact owned handles ABSENT,1518pins unchanged:
A217/terminal_handles.json. No live solver/build. Exporttool31700TERMINAL0.

Actual requested files: A217/inspection/rejected_mesh.vtu (WITH9component
captured_nodal_metric and availability), rejected_mesh.su2, bad_cells.vtu,
cavities.vtu, reference.vtu. Full mesh159281points/790537tets; all159281nodal
metrics present. Engine current_nodal/full_target1 makes this the transported nodal
target, not an immutable original-background reconstruction. Surface extraction
inherits that3Dfield; tangential restriction is used for surface motion.

User reported VTK ASCII NaN read failure. Array lengths were correct; corrected
floating arrays to inline binary IEEE encoding, and moved FieldData from Piece to
UnstructuredGrid. Original ASCII files retained locally as *.vtu.ascii-original.
Exporter12focused checks pass; actual corrected files independently read with
installed meshio. A217/inspection_binary_repair.json and inspection_artifacts.json
record checks/hashes. No geometry/connectivity/tensor values changed. Final q/J/
geometry cache unavailable for589332cells: bad_cells42/cavities685are PARTIAL views,
not all remaining failures. Full mesh/tensor are complete; mesh remains UNAPPROVED.
[Inspection details](Handbook/ADAPTATION_M6_INSPECTION_20261101.md).

Complete quality entry721643cells/49380q/48218J/2713geometry/0size at759.619664s
matches210whole-field entry counts. Shape tail reached49q/16J/2505geometry at
4885.723s and then stalled despite coordinate/optimizer commits and hundreds of
unsuccessful topology trials per window. Geometry construction reopened shape:
complete next entry789125cells/441q/527J/2259geometry/0size at5085.871720s.
quality_trend_assessment.json retains113complete groups. No source timing speedup
claim (same production binary). Final incomplete census must not supersede these
last complete measurements. Existing neighbor invalidation review found no concrete
gap; geometry subdivision and larger-star controls did not justify a new operator.

Next: inspect current saved residual cavities. Reuse current OwnedCell sensors via
existing EvaluateCavitySample at represented Centroid then Measure for cheap
independent q/J diagnosis, checked against known rank4rows. Do NOT average tensors
or treat diagnostic recensus as acceptance; geometry/edges still need strict final
certification. Select one demonstrated connected repair/geometry-reclosure fix,
focused affected checks and one MPI milestone, then immediate full two-event M6.
No new scheduler/config, gate relaxation, budget increase or construction q/J policy.

## Latest terminal reference: build210; coupled closure still missing

R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
A=`R/m6_runs/euler_build210_real_cfd50k_bound_refinement`.
Executed source6744638324 was unchanged throughout. Build210fresh GCC13.3,
OpenMPI4.1.6/runtime-v2(193files); active binaries at
`integration_evidence/build-integrated-v3-currenthost/`.

First native event NATURALLY rejected at unchanged time_budget on all6ranks:
**790388cells/64q-bad/39J-bad/2283geometry-bad/0size-bad**, max engine5336.156753s.
All6 EXIT census/cache/shape/geometry flags1; DETAILcomplete0 means failed capability.
A/event1_natural_failure.json SHA7877d7ff7b6fe2322236097eee02f35efd6a1714f05c4ee5cbbe157fc72cf63b.
Only after preservation, SIGTERM was sent ONCE to verified launcher195411/start480761.
NEVER repeat that stop or signal historical PIDs. A/event1_failure_owned_stop.json.
Tool54752TERMINAL1, all10exact handles ABSENT,1517pins unchanged:
A/terminal_handles.json. Those historical handles and217handles are absent.
Overall helper5704.300732s includes post-rejection original-grid work; those files
are not adapted transfer/continuation/restart proof.

First quality entry721643cells/49380q/48218J/2713geometry. First quality exit:
26q/11J/2535geometry at5164.024206s; all26shape-union cells blocked, queue0.
Geometry construction reopens356q/488J, next entry382q/499J/2286geometry at5184.036991s.
Final80shape-union cells include10blocked, queue770; final search not exhausted.
Published45192repairs remove49672q/48667J; subtract356q/488J construction reopenings
for net49316q/48179J, exactly entry-minus-EXIT. Interior can be constrained. All80
short<.5cells do not prove construction causation; saved clean controls are also short.
16fixed metric samples remain supported, max drift6.128517%; no immutable original
continuous-target fidelity proof. Private smoothing merit is not published gain.

Next target: repair power and geometry/shape reclosure, with geometry cost secondary.
Dedicated shape-preserving geometry repair was never reached in210(all6sealed
prefixes): it needs zero q/J/height/size debt first. Its strict geometry descent
then precedes residual construction/reclosure; final two-sided gates independent.
A/geometry_phase_readiness_assessment.json.
One connected cycle; classify actual conservative refusals/support and repair
limitations using existing saved witnesses before production edits. Bernstein-bound
subdivision is UNPROVEN/backlogged, not a selected implementation or saving.
No new scheduler/config, gate relaxation, time increase or construction q/J policy.
Then focused affected serial and ONE MPI milestone, immediately full M6 same seed.
[Terminal210 findings and continuation](Handbook/ADAPTATION_BUILD210_OUTCOME_20261106.md).

## Completed217 diagnosis before the selected full run

217buildPASS174.143064s/1301pins; case tool57630TERMINAL0,39853assertions/1case
PASS89.622024s, all5case handles absent/runtime unchanged. Existing exact partition
bounds admitted no additional bad face; no production subdivision selected.
Mirrored engine size/quality gates: full49postcollapse optimizer closes the old202
witness, BUT3of4SMALLER star controls also close (9active cells/1trial for one).
Larger cavities are NOT justified. One control raises q-bad1→2despite lower debt.
R/repair217_support_assessment.json. No new production correction selected from
that historical now-repairable witness. Use saved217capture for CURRENT residuals.
[Detailed findings and limits](Handbook/ADAPTATION_REPAIR_SUPPORT_20261106.md).

## Preserved qualification, runtime and evidence

The current shared ParameterError refinement tries EXISTING physical-union bounds
for failed budgeted direct correspondence bounds, retains the tighter upper and
independent affine maximum; successful cheap direct paths stay unchanged. Admitted
scratch and sticky resource errors preserved; final gates unchanged. Actual saved
face152256924:direct1.234657724e-4 vs rigorous union9.465791007e-5 under same support.
Other saved bounds can worsen, so retaining the better bound matters.

Build210PASS45.113142s/1301unchangedpins after209fresh build exposed missing direct
<optional> include; include-only fix. Scoped qualification PASS: actual witness
38938assertions/1case, geometry2666assertions/34cases, MPI6all6ranks23cases each.
R/geometry210_qualification.json. Runtime193pins cover observed ELFs; historical
175pins remain preserved. MPI requires host execution (sandbox sockets denied),
OMPI_MCA_osc=pt2pt and explicit4TB TMPDIR. Initial setup failures preserved; the
owned MPI fixture launcher181079/start444653 was already stopped after TMPDIR
failure. NEVER repeat it. All qualification/build handles terminal/absent.

Reuse build_checkpoint.py/run_cases.py/run_m6.py. Check unused build/attempt names
and actual handles before launch; nice19,2–4build workers(3current), libraries1,
one heavy job,4TBcompile-tmp. Freeze sources/helpers/binary/config/inputs through
build/run; inspect terminal/pins before edits. Prefer exact HOST PID/start/argv/cwd;
namespace PID4/10 from sandbox builds is not a host PID. Tool session is authoritative.

Saved real CFD seed: R/m6_inputs/euler_original_force_seed_v1,
SHA1eaee788bd62bd92a92ec539577dcbcfc0f8042dcbcec23adb04e825bb01a880;
61330points/303359cells, CL~.269535/CD~.013536, finite/admissible and force-stable,
not a fully residual-converged aerodynamic reference. Reuse before first remesh;
second event must use first ACCEPTED mesh and its continued adapted solution.

A/current receipts: event1_natural_failure.json, terminal_counter_assessment.json,
phase_progress_assessment.json, terminal_resource_assessment.json,
quality_trend_assessment.json, quality_entry_comparison.json. Sealed first-native
prefixes exclude later original-grid work. Peak sampled descendant RSS14736031744B,
largest process2649538560B; not physical RAM/true peak. No pressure-driven stop.

Historical selected inspection202: R/m6_runs/euler_build202_real_cfd50k_inspection/
inspection/rejected_mesh.su2 and rejected_mesh.vtu (164526points/810163tets).
Do not call it210. Save no per-trial/routine-attempt meshes; select useful major
milestones. Next selected rejected VTU includes9-component captured nodal metric
and availability (12offline tests PASS). Current M6 full_target1; this is current
transported nodal target, not original background or other-mode query composition.
Surface extraction inherits3Dfield; surface motion uses its tangent restriction.

## Remaining complete-workflow obligations

A/workflow_acceptance_readiness_current.json revalidates transfer/projection/kernel
source pins; driver differs only by inactive diagnostic hook. Require actual true
mass residual<=1e-10, full admissibility and scaled target-integral gate1e-12 before
publication. Donor-vs-new integral change/domain correction is separate. Post-CFD
admissibility alone does not prove immediate transfer conservation.
After TWO accepted meshes and finite adapted CFD, verify matching mesh/native_ref/
solution point count/order/finite/rho/internal-energy with existing state audit.
Then fresh isolated ADAP_LOOP=NO restart, distinct saved input/output prefixes,
actual load and finite admissible continued CFD. Original-grid00000 or rejected
00001files are not proof. General adaptation-resume cycle0/output collision remains
open; isolated solver restart does not claim it fixed. Then dedicated BL curved/
junction/interface/subsequent-operation/redistribution/height MPI checks and RANS500k.

## Linked design and preserved history

- [How to proceed](HowToProceed.md)
- [Integrated specification](Handbook/ADAPTATION_INTEGRATED_NEXT_TASK.md)
- [Current geometry review](Handbook/ADAPTATION_GEOMETRY_CERTIFICATION_20261101.md)
- [Repair/support diagnostic and required controls](Handbook/ADAPTATION_REPAIR_SUPPORT_20261106.md)
- [Actual inspection](Handbook/ADAPTATION_M6_INSPECTION_20261101.md)
- [Assessment review](Handbook/ADAPTATION_TECHNICAL_ASSESSMENT_REVIEW_20261101.md)
- [Backlog](Handbook/ADAPTATION_BACKLOG.md)
- [Archived detailed handoff](Handbook/History/HANDOFF_Codex_build210_terminal_archive_20261106.md)

Git origin https://github.com/rois1995/SU2.git, branch codex/native-3d-bl. Push exact
reviewed whitelists only; Git metadata is outside workspace, requires escalation.
Use gc.auto=0/submodule.recurse=false and ignore-submodules=all. Leave unrelated
historical logs/analysis, .orig, submodules and user untracked steering untouched.
Do not broadly add/check dirty history. No binaries/meshes/restarts/raw solver logs
committed; exact metadata receipts under ignored R need force-add individually.
