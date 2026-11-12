# Native adaptation — current handoff

Updated2026-11-12 Europe/Rome. Goal ACTIVE, one connected curved-BL cycle/one heavy job.
R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
Read AGENTS.md, [ordered stages](Handbook/ADAPTATION_CURVED_BL_STAGE_INTEGRATION_20261111.md),
[target checkpoint](Handbook/ADAPTATION_BL_TARGET_CHECKPOINT_20261111.md),
[actual assessment](Handbook/ADAPTATION_BL_TARGET_ASSESSMENT_20261111.md),
[timings](Handbook/ADAPTATION_TIMINGS.md), [HowToProceed](HowToProceed.md),
[backlog](Handbook/ADAPTATION_BACKLOG.md). [History through425](Handbook/ADAPTATION_HANDOFF_HISTORY_PRE_426_20261112.md)
retains all failures, prior handles, normalization and rejected alternatives. This file owns live state.

## Current frozen workload and provenance

NO owned heavy workload.431 nativeMPI6 surface stage TERMINAL/PASS490.279s diagnostic,
surface_prepared=false. Saved396target → existingPrepareSurface, no production change.
Case R/cases/curved431_saved_target_surface_mpi6_20261112T080701_534595;
launcher R/check_saved_surface431.py; execsession82847 TERMINAL; allactualworkers and
wrapper verifiedterminal, allcasepins verified. Build431PASS212.186s,
allpins/actualhandles terminal; production CFD unchanged428, test
1f96caa91a56dc0322b380c7ec24f59dbed13634fa43e71ee8a8f5b84d9366c7.
Original6partitions, same233220work/388.8s/12GiB,600s wrapper; noCFD/metric recompute.
Input/output/acceptance in residualcavityhandbook's nativeMPI431section.
R/surface_stage431_assessment.json:60546points/301183positive tets/corepresent,
globalIDunique/sharednodalcoords+tensorsbitexact. Stage389.056s,784collapses,
5104work,15980cached/285203unknown,0split/0passes.134geometrybad/3sizebad PARTIAL,
allq/J/heightsunmeasured. Targetload42.048s/import.472s; summedrankCPU2321.204s
includespolling, collective/electiontimers overlap and are nottotalwait. Largest
sampled treeRSS4.879GB/individual.941GB, nottruephysicalpeak; no resourcepressure.
MPI gave1.212x commits versusserial428, notsurface readiness. Geometryworkmax/min13.23.
Sourceaudit confirms fullcensus+drainedfrontier required beforecoarsen→refine;
geometryqueue unavailablewhilecoarsening. Seed<=16IDs/rank on global!any; immediate
changed-cell updates are notwholemeshrescans. Seed22.543s/rank does NOTprove interior
measurement dominated389s. Next localize seed/update work by wall/otherboundary/
interior and queue-refill/remaininginitialIDs before changing census/eligibility.
Do not addoperators/scheduler, raisebudgets or relaxgates from partialcounts.
430 actual cavity diagnostic TERMINAL/PASS146.812s/608082assertions.
Case R/cases/curved430_cavity_planning_20261112T074752_490678;
launcher R/check_cavity_planning430.py; execsession20185 TERMINAL. Allactualhandles
includingwrapper and allcasepins verified. Build430PASS236.191s,
pinsintact/actualhandles terminal. CFD34901d2b7c0ad64653c1d54b8d9b61c8c9db3683b8f1079d511cccd66ac769b8
UNCHANGED428; test6b42b98bd055e0c516ac025632a7b9c4bc544e92cf2b03f2b13af9f36b4d7863.
R/build430_status.json/terminal_handles and case status/resources ownreceipts.
429failed20.111s on test assertion treating geometry_failure
string as bool; failedsource archived in source_snapshots430, allhandles terminal.
430also corrects review-found contact candidate flag and production repair-progress
preflight. No production change. Input/output/acceptance:
[residual cavities](Handbook/ADAPTATION_SURFACE_RESIDUAL_CAVITIES_20261112.md).
Actual run uses saved428Owned/current parameters and complete396policy;
no new CFD/normalization/fullsurface attempt.
R/cavity_planning430_assessment.json: allfourcurrentfieldABBAcontexts exactlyagree.
Smoothcollapse and LE split pass selectedprivatepreflights, not published/acceptedBL.
Recordedcollapse's geometry regression reproduced. LE split reducesgeometrydebt but
q/J-bad3→6; laterqualityclosure remains. LEcollapse reducesdebt but currentCoarsen
zero-debtgate refuses. Moveproposalconstrainedtozero is INCONCLUSIVE, not a general
movementfailure. No newoperator/admissionchange justified.431tested nativeMPI surface
capacity; its incomplete census/phase progression is the active blocker above.
428fullW TERMINAL/PASS491.507s as diagnostic, NOT readiness.
All pins and actual PID/start identities terminal. Case
R/cases/curved428_checkpoint_surface_20261112T064413_336601;
R/surface_stage428_assessment.json. Native389.594s at388.8limit,3650work/647collapses,
0splits/0completedpasses.60683points/301546positive tets/transportedcore present.
Census12352measured/289194unknown/cacheconsistent;98knowngeometrybad/1sizebad are
PARTIAL, allq/Junknown. No firstrow/growth. Prior421349commits/5680cached atsametime;
progress improved but no complete requiredsurface output. Metric107.622s,reconstruct
105.631s,seed48.536s,other71.488s; upper77.127s/guidance68.535s overlap. Collision6s/
9builds notdominant. SampledindividualRSS3.687GB notwholepeak; admission11.257GB<12GiB.
Next blocker is initialcoarsening/census before refinement, not proven missingoperators.
Do not raisebudgets, skipconstraints or launch another microoptimization.430aligned
the probe to currentnodal authority and demonstrated existing operator preflights;
431tested nativeMPI capacity and remains blocked before refinement; see currentaudit.

ONE latest inspection artifact saved: case/inspection/rejected_surface428_nodal_witness.vtu
(37.693MB/60683points/301546tets), script/README/manifest alongside. ALLcoords/cores match
396pointwise; relevantpolicy/source/profileprovenance verified. ComposedNODALWITNESS is
visualization, not arbitrarypoint/current-field certificate. q/J/geometry NaN+known0.
Serializedarrays/topology/SPDtensors checked; VTK/pvpython unavailable, no reader/render
validation. VTU local/hashtracked; do not generate pertrial VTUs. Complete failed native
owned/donor/parameter payloads retained, not a successful stagecheckpoint.
428batch planning QUALIFIED and PUSHED55a3ec818d: allbuild/check/prefix pins and
actual handles terminal. Production CFD34901d2b7c0ad64653c1d54b8d9b61c8c9db3683b8f1079d511cccd66ac769b8,
test79ca00a01391811b2594b579ef307c769c9d1081df1e81e2ebf0ff27bcb2c4c5.
Build66.129s; serial35cases9053assertions PASS9.409s; ONE MPI6PASS7.589s everyrank.
Actual64prefix owned/donor/parameters BYTEIDENTICAL423/same23commits. Requests3438→1752,
metric11.401→4.093s, nativewall29.159→22.711s/CPU28.778→22.306s. Substantial−49%work/
−22%stagecost, notwholeworkflow speedup. R/surface_batch428_comparison.json.
[Design/acceptance](Handbook/ADAPTATION_SURFACE_PLANNING_BATCH_20261112.md).
Complete same396target W integration is TERMINAL as recorded above, under unchanged
388.8s/native5400s/12GiB; launcher R/check_checkpoint_surface428.py.
No newcache/import/scheduler/config, freshcoordinates/associations,
strict finalgates, genericfulltarget paths and empty-rank votes unchanged.
427build PASS216.195s; actualdiagnosticPASS136.228s/allpins/actualhandles terminal.
Allfouractualcontexts exactsample/edgeagreement; repeatedwallguidance40→10, LE48→11,
coreedges60→27/72→31. ABBAwarm LE.304→.077s, wall.340→.108s; interiorunchanged.
R/cavity_planning427_assessment.json. Fullglobaldonorfield timings are not production
performance. No meshapplication/acceptance. Source426control-selector failure preserved;
427uses mobilityfeatures/corners, not originalchartbinding.dimension.
428qualification and failed fullsurface integration above are complete. Genericfulltarget/
CFD paths and finalstrictgates unchanged.
Prior qualified423baseline pushed8a2b8779ea; prior evidence8abe01c943 pushed.
Branch codex/native-3d-bl, authorized fork origin github.com/rois1995/SU2.git.
425build PASS216.191s; CFD
`9d34b427cde6fced1af51be947044086bf56f4c449b66539bf6e61a72d9c8f82`;
test `7dfbef22bb51d198924e0380a59d69570c406645bc6e2c0cf8e52c43f3bb77c4`.
All pre426 build/check/actual handles terminal and pins verified.

## Active stage3 investigation

Latest requested edge-accuracy follow-up reuses existing399 investigation:
R/edge_measurement428_review.json and Handbook/ADAPTATION_BL_EDGE_ACCURACY_20261112.md.
All1453 currentbuild source pins/both binaries/raw394+399inputs verified. No new
production precision change or campaign. Main scalar consumers need ranking/merit/
insertion/final lengths; two boolean-only firstrow/sector checks are downstream,
not demonstrated bottlenecks. CurrentW fullcompositions0 rules out assigning this
surfacefailure to expensive composed quadrature. Keep existing current-cavity
diagnosis active; conservative bounds remain unproved, not a selected implementation.

Full421existing PrepareSurface failed deadline:389.644s at388.8limit,1390work,
349collapses/0splits.60981points/302382positive tets with transportedcore. Only5680
geometrycached/296702unknown; allq/Junknown.39knowngeometrybad is PARTIAL census.
Not usableBL/readiness. R/surface_stage421_assessment.json and complete local~235MB
owned/donor/parameter capture in cases/curved421_checkpoint_surface_20261112T051236_121725.
It is failed-progress evidence, not a successful fresh-reload-qualified stage checkpoint.

423removes old-patch measurement after an already refused surfacecollapse with no later
repair route. GenericEuler, viable/repair/split measurement, empty-rank participation,
sticky failure votes and all gates retained. Serial32cases8456assertions PASS9.473s;
ONE MPI6PASS6.394s/28cases everyrank1681–6391assertions. Not nativeMPI BL generation.
Actual same64work prefix keeps owned/donor/parameters BYTEIDENTICAL422,23commits:
requests3830→3438(−10.2%), metric13.299→11.401s, stage31.188→29.159s(−6.5%).
R/rejected_collapse423_comparison.json. Modest single-prefix saving, no fullsurface
retry solely on it. Geometryguidance3.941s overlapsupper4.333s; not all11.401s metric.
Source audit: FilterCollapse uses cachedlocalstar, not fullmeshscan; selection/publication/
collision small in prefix. Dedicated nativeMPI BL construction remains unqualified.

425accounting hypothesis REJECTED:2048Storagecalls full2.840ms/base.041ms, workspace
.029/.028ms. Intrinsicguidance usesbase; no production storagecache added. Actual94.248s/
28assertions PASS; all pins/actual handles terminal. R/storage_accounting425_assessment.json.
424shortcut omitted PrepareReference mobility/height and correctly failed checkpoint
source-authority check; exact fixture archived.425uses existing geometryfactory +
PrepareReference, no CFD solve/guard relaxation.3DM6 d.segments is empty (2D-only).

IMPORTANT scope correction while428fullW frozen:427isolated probe uses original-P1
CONTROL field, not production currentfield. CavityMetricSource::ImportMetricSamples
uses transported current nodal rows/topology; originaldonors bootstrap only.428actual
64prefix uses that real production path and remains byte-identical, authoritative
validation. FullW is now terminal; align isolated probe with current-cavity imports and loaded
Owned.sensor, not old-donor rebootstrap. No production authority change. Do not freeze originalP1 production authority.

427diagnostic loads complete421currentOwned/originalDonors/parameter payload against
prepared immutable authority plus complete396policy. Original P1 in this probe is a CONTROL only; production uses transported current
nodal field. Compare cellwise vs wholepatch
planning in ABBAorder, exact edge/tensor/normal/strip values and work/timing. Contexts:
complete10cellstar remove21292 (recorded exactorientation refusal retain21543, witness
102519), unchanged wallcontrol, interiorcontrol. No application or mesh acceptance.
Public MeasureSurfacePlanning contract says edgeguidance independent of measuring cell;
seedcaller already batches. Privatecaller repeats percell. Source review supports
measuring SAME admitted <=10cell donor batch once, extracting six edges/cell, keeping
batch-only physicalfaces/full currentparameters/coverage and final fullcavitysurface_merit.
Preserve imports/retries/empty votes and existing scratch bounds. Actual exact agreement
and measured useful work saving required before selecting production change. No newcache,
scheduler, larger cavity, configuration, geometry representation or gate relaxation.

## Preserved source/target

Same303 RANS+SA entry: R/m6_bl_inputs/rans_metric_entry364_v1; Mach-onlyL1,
nominal500k, h0=2e-6,g1.2,T=.01,heighttol.1. No CFD/normalization recompute without reason.
MeshSHA8c839170eb5e359f7de6a41434aa6a9c2d620053f37de6d9360bd04d2a63088b;
reference2e99fb7870e2c718f110da8509e4c0f7e746ce138e7f27ab57aa0dedbb6c42de;
entryfieldaf7ade468dc40cd39dbe9238248e50057cdd151a64fef92fda957677aa3f6cae;
config0a7de022729f69b170deaee2eaabbd5347550da8134cbdaa39587f7173e78742.
Ninefields finite/admissible; CL.15573/CD.05027 not convergence/shock-resolution proof.

396complete target checkpoint QUALIFIED: producer733.225s/freshreload546.920s,
61330owned composed tensors/chord42.06659984296543/4635evals BITEXACT.
R/target_checkpoint396_qualification.json and target_checkpoint396_producer.json.
Producer cases/curved396_saved_cfd_metric_stage_20261111T214613_3294100;12localpayloads
303787984B, complete originalfamily/full+base histories/OptionA/authority/layouts.
selected_core_native.bin is CORE ONLY; witness tensors never authority. Complete policy
plus originalsource and current stage geometry required. Estimate461495.991 with empirical
indicator47864.008 NOTbound; tangent.25/core.00460725. Original61330points/303359tets.
123186composednodal violations/core0; physicaltarget suitability still incomplete.
383whole-edge exclusion qualifiedboth factories;394prepared/projectionfix retained.
GL10underestimates6.82%; no approximate classification/ranking/insertion/final scalar.

Keep404passingheight,411strictchild,412physicalapex,416sharedsurfacefeedback and417–419
complete-edge binding/fatalpublication guards.416smooth actual16edits/21descendants/
14sampledfeasible/5columns25q/Jbad, repairnet0; not fullR1/R2coverage.419LE0edits, strict
wholepatchgeometry refusal starts ORIGINAL mesh, skippingW; not postprepared failure.
Known Growth12failure OPEN: boundary dependencies change during growth; do not ignore
stale-direction/front guard. Remain at stage3 until credible surface output, then stage4
constrained/blended directions/firstrow→5growth→6strictclosure→7workflow. Keep queued
physicalcurvature/target/delivery study and CADoptional noCAD authority; no policy from proxies.

## Goal and resources (unchanged)

Euler293 COMPLETE: two strict Euler50k meshes149051/736323 and172001/870422points/tets,
conservative admissible transfer, finite continuedCFD and isolated adapted restart.
ContinuedCFD ONE iteration, not convergence. Preserve22payloads/384685699B/checkpoints.
Remaining: usable strict curvedBL, dedicated nativeMPI BL constraint qualification,
TWO accepted RANS adaptations with admissible conservative flow+SA transfer, finite
continued adaptedCFD and actual isolated adapted disk restart.500k nominal, not nodecount
or rejectiongate. Strictgeometry1e-4, positivity/bodyseparation,q>=.2,J>=.05,maxedge1.8,
requestedheight/progression and resourcegates unchanged. Intermediate qualitydebt provisional.
One heavy job,nice19,j3,OMP/BLAS/MKL1,4TBcompile-tmp. Native5400s/outer14400s/12GiBperrank;
Wexisting388.8s; patchdiagnostic300s/outer600s. Verify host resources and exact ownedhandles.
Reuse evidence, bounded ledgers, sourcefreezing, credible complete stage checkpoints and
reviewable pushes. Unrelated P1/FEM/.orig/logs untouched; explicitgitstaging/ignore submodules.
