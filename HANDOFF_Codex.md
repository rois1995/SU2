# Native adaptation — current handoff

Updated2026-11-06(Europe/Rome). Goal ACTIVE: TWO strictly accepted ONERA M6 Euler
CFD→mesh→CFD→mesh at50k metric complexity; actual conservative admissible transfer,
finite continued adapted CFD and isolated usable adapted disk restart. Then
BL/constraint/MPI qualification and corresponding RANS500k. Actual mesh counts
are separate from metric complexity. ZERO accepted adapted M6 meshes so far.

Geometry1e-4,q>=.2,J>=.05,maxedge1.8 and requested heights stay strict. Construction
remains validity-first: positive/nondegenerate/topology/body/feature/height validity,
no fresh q/J measurements/ranking/vetoes. CAD optional; no-CAD authority sufficient.

## LIVE: selected full M6 build217 mesh/metric capture

User explicitly requests the actual latest-checkpoint mesh even if refused, WITH
metric. Selected full two-event M6 is LIVE from pushed sourcebb61db8c0d; production
source/binary remain6744638324/EXACT210binary, hidden diagnostic fixture only changed.
R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
A217=`R/m6_runs/euler_build217_real_cfd50k_inspection_metric`.
Host tool27489 LIVE. Helper552625/start1488380; launcher552832/start1489019;
time552830/timeout552831/start1489019; ranks552835/start1489022 and
552836–552840/start1489023. A217/owned_startup.json is exact authority.
1518pins unchanged;6test ranks,84observed ELFs all covered by runtime193pins.
R/inspection217_launch.json records4TBscratch, nice19/MPI6/libs1/pt2pt,46.029GB
available RAM and32.868GB free disk at launch. One owned heavy workload ONLY.
CFD seed SHA1eaee788...a880 verified; config BYTE-IDENTICAL210(SHA74b2e9f...2bfe8d),
50k/5400native budget/two events/all strict gates unchanged. Initial CFD completed;
original-grid00000restart is not adapted-transfer/restart proof.

Freeze ALL executing/pinned code/helpers/config/inputs until owned handles terminal
or preserved natural failure permits authorized stop. Do not restart from log age.
Existing --inspect-rejected workflow captures the FIRST rejected actual exit;
no trial/routine mesh dumps. Before any stop preserve EXIT/DETAIL evidence AND all6
capture manifests/raw shards; stop only verified current owned launcher ONCE.
After owned run terminal, use existing inspect_native_failure3d.py on all6manifests
with --output A217/inspection. It produces full rejected_mesh.su2/rejected_mesh.vtu
plus bad_cells/cavities/reference views; metric tensor/availability included in
whole/bad/cavity VTUs. Verify actual counts, completeness, full nodal metric and
file hashes before giving user links. No217mesh file is ready yet. Keep watching.
210in-memory mesh was not captured;202inspection remains historical, not latest.
Latest preserved complete construction census:688657cells/3530geometry-bad/
11355size-bad at406.957711native seconds. q/J intentionally UNMEASURED, not zero
accepted shape. A217/stage_progress_snapshot.json (live observation, not terminal).
Read-only source review found existing publication/support invalidation covers
changed neighbor stars and migration retains history; no demonstrated gap selected
for a fix. A217/repair_invalidation_source_review.json. Await actual217residuals.

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
A/terminal_handles.json. Those historical handles are absent; LIVE217 above now
prevents pinned source changes.
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
that historical now-repairable witness. Use LIVE217capture for CURRENT residuals.
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

Actual selected inspection remains202: R/m6_runs/euler_build202_real_cfd50k_inspection/
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
