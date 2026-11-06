# Native adaptation — current handoff

Updated2026-11-06(Europe/Rome). Goal ACTIVE: TWO strictly accepted ONERA M6 Euler
CFD→mesh→CFD→mesh at50k metric complexity; actual conservative admissible transfer,
finite continued adapted CFD and isolated usable adapted disk restart. Then
BL/constraint/MPI qualification and corresponding RANS500k. Actual mesh counts
are separate from metric complexity. ZERO accepted adapted M6 meshes so far.

Geometry1e-4,q>=.2,J>=.05,maxedge1.8 and requested heights stay strict. Construction
remains validity-first: positive/nondegenerate/topology/body/feature/height validity,
no fresh q/J measurements/ranking/vetoes. CAD optional; no-CAD authority sufficient.

## Current: build210 terminal; classify coupled closure before the next correction

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
A/terminal_handles.json. No owned heavy workload is live; source changes are now safe.
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
One connected cycle; classify actual conservative refusals/support and repair
limitations using existing saved witnesses before production edits. Bernstein-bound
subdivision is UNPROVEN/backlogged, not a selected implementation or saving.
No new scheduler/config, gate relaxation, time increase or construction q/J policy.
Then focused affected serial and ONE MPI milestone, immediately full M6 same seed.
[Terminal210 findings and continuation](Handbook/ADAPTATION_BUILD210_OUTCOME_20261106.md).

## Active bounded repair/support diagnostic: build214

Only the captured202fixture changed; production source remains6744638324.
Build212PASS156.166044s/1301pins, case tool48060TERMINAL0,39500assertions/1case
PASS87.688321s, runtime allcovered/unchanged, all5exact host handles ABSENT.
R/geometry212_partition_assessment.json:1/4/16piece existing bounds tighten, but
certify NO additional bad face beyond210's union refinement. Two difficult leading
faces remain above tolerance; this does not prove true distance violations.
Two depth2trace checks hit unchanged1048576work; fresh/repeated bounds bit-exact.
Do NOT add production subdivision from this evidence. Initial211failure was
preserved; corrected diagnostic establishes resource refusal, not path mismatch.

Next bounded experiment uses existing OptimizeMetricMesh with production160trial
allowance on the saved52cell union of four COMPLETE seed-vertex stars, and on
49cells after the qualified4502→181199364collapse including the retained star.
Same current nodal target, existing geometry/trace checks,30s per optimizer,
explicit memory reservation. Compare whole52region q/J/debt/geometry and boundary/
interior rejection counters. No global contact/publication or210tail replay claim.
Build213TERMINAL1/tool92521,15.107502s/1301pins unchanged: the diagnostic
called a private engine coverage-pruning helper. Fixed test-local pruning matching
that existing behavior, no new production API. Build214 LIVE tool57838;
R/build214_status.json/source pins. Sandbox PID3 is NOT a host PID. Freeze
until actual build/case handles terminal.
No production repair decisions, construction paths, gates/config/budgets changed.

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
