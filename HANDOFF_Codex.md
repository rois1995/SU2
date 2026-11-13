# Native adaptation — current handoff

Updated 2026-11-13 Europe/Rome. Goal ACTIVE. One connected curved-BL cycle.
R = `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
Read AGENTS.md, [trajectory and stalls](Handbook/ADAPTATION_CONNECTED_PATCH_TRAJECTORY_20261113.md),
[ordered stages](Handbook/ADAPTATION_CURVED_BL_STAGE_INTEGRATION_20261111.md),
[timings](Handbook/ADAPTATION_TIMINGS.md), [next action](HowToProceed.md),
[bounded cavities](Handbook/ADAPTATION_M6_BOUNDED_CAVITY_COMPARISON_20261112.md),
[continuation](Handbook/ADAPTATION_FIRST_HEIGHT_CONTINUATION_20261112.md),
[backlog](Handbook/ADAPTATION_BACKLOG.md). [Historical handoff](Handbook/ADAPTATION_HANDOFF_HISTORY_PRE_473_OUTCOME_20261113.md)
preserves earlier details; it is NOT live state.

## Live state and provenance

NO owned heavy workload live. Build473, focused checks, current-residual replay,
coupled regression, MPI qualification and actual full surface integration are TERMINAL.
Actual host PID/starttick identities and pinned source/runtime/inputs independently
verified before reporting or starting the next job. No stop/restart was needed.
Full integration exec92661 is terminal; all10actual identities and1743pins verified.
Source checkpoint **35e173b043** is pushed to origin/codex/native-3d-bl; later evidence
checkpoints may supersede HEAD. C++ remains unchanged since build473. Preserve
unrelated working changes. Consult actual handles before any restart or compiled edit.

Build473 PASS305.914s; all26actual identities terminal, cleanNinja, source/binary pins.
Test SHA `fe107f3e4dcd290b2de2f06896e6647b85b079f5308aed48579f15c508f9309f`.
CFD SHA `70c1974cf3f56b7ee6c11a19703f2d6c8d17206a40f223860623228f7429c2f7`.
R/build473_status/source_pins/terminal_handles.json records exact provenance.
Do not use historical470/472 binary hashes as the current build.

## Implemented and qualified scope

473 addresses the demonstrated classification-bound/fitting-merit mismatch in shared
BoundaryParameterState::EntityFaceError. Retain the strict query; only if its valid
upper exceeds2*GeometryTolerance, make ONE additional query at that certificate
ceiling on the EXACT SAME support/physical triangle, taking minvalidbounds and
preserving the maximum floor. This is a fixed shared old/fresh/direct/cross/census
policy. Acceptance remains1e-4; no target, API/config, scheduler, authority, ownership,
construction validity, height or resource change. Extra-query failure stays failclosed.
Exact support/coordinates/tolerance distinguish memo entries; certificates are not
serialized. R/geometry_merit473_design/review.json. Uncached primitive interruption
is tested separately; the new second-query exhaustion check uses a cached query.

470's phase-consistent provisional reconstruction is retained: valid positive cones
may defer first-row carving only under existing ValidityOnly with exact requested
heights, certified clearance and passing-row preservation. Absolute callers retain
strict carving. The rejected469physical-normal fallback is NOT active.
Diagnostic mapped control now avoids planar ConstrainMovePoint before MoveVertex;
production SmoothBoundary already uses mapped projection, and was not changed.

Focused52cases/5688assertions PASS62.658s. Actual current470 residuals: PASS148.615s,
24calls/608092assertions/1772pins/5actual terminal handles; all303757original Owned
records byte-identical. Four previously refused private candidates now pass all
original guards: R4Split2→4/Reconstruct2→8, geometry excess1.277010e-7→8.650461e-8;
R5same2→4/8,2.563259e-7→1.785057e-7. Geometrybad stays2→2; q/J2→4or8,
strictheightFALSE/provisionalTRUE. No publication/closure from those preflights.
Controlcollapse10→8 remains feasible. MappedcontrolMove constructs but improves no
geometry/tangential demand, so admission refuses. R/current_residual473_assessment.json.

Saved451 coupled regression PASS247.696s/1773pins/5actual handles/all303622Owned
unchanged. Exact16step checkpoint and3→10publication preserved;24calls/22pubs,
observation36→110(+11old),geometry16→24,localgeometrydelta+2/qJ+63/+63.
Firstrow UNREACHED.72.709656wall/72.696812CPU/1650973geometrywork. Same mesh/progress
as470; no speedup or new current473-residual publication claim.

MPI milestone PASS185.914s:41cases EVERYrank,3642–124194assertions/rank. Constraints,
empty ranks, stale dependencies, conflicts, cache/recomputed agreement and resource
refusal pass. Actual cross-owner publication control uses EXISTINGcoarsening; the
four exact new private candidates were not separately published in MPI. Full stage
below exercises production MPI integration. R/surface_phase473_qualification.json.
No strictBL qualification from these fixtures or provisional outputs.

## Latest actual full surface stage — complete wall census, NOT READY

R/cases/curved473_geometry_merit_saved_target_surface_mpi6_20261113T041459_1770102.
Terminal PASS497.751s wrapper/389.166s native;1743pins/all10actual identities verified.
R/surface_stage473_assessment.json and surface_phase473_outcome.json are authoritative.
**61811nodes/304508positive tets**,1156221bit-exact shared nodal coordinate/tensor
checks. Six complete Owned/donor/parameter shards preserved with payload hashes.
Failure/diagnostic capture, NOT a successful surface checkpoint or usable CFD restart.

**39760/39760requested-wall cells measured,100%**, vs47030500/39128(77.95%) at the
SAME388.8s/233220work stage budget. Requested-wall geometry measurement complete;
full geometry census is NOT complete (264748unmeasured cells).3102cells fail the
current conservative geometry screen,34sizebad; q/J and heights UNMEASURED globally.
These are NOT3102independently proved physical violations. Surface_preparedFALSE;
no tangential readiness, constrained firstrow, growth or acceptedBL/RANS mesh.

481commits/4804proposalwork, vs470164/3676. Geometrywork51,309,637 vs30,088,848;
fieldrequests171947 vs188521. No computational-efficiency or rawbad-count/churn claim:
coverage and evolving cells differ. R4/R5hold1527+1087geometrybad; R2holds488.
Rank5 guidance refusals481/622 vs4701124/1157, but bad-cell retirement remains unknown.
Geometry upper timers are nested in guidance/other wrappers; CPU sum2296.785 includes
MPIpolling. Election/join time is NOT totalMPIwait. One rank's exit counters are
interleaved/discarded. Sampled treeRSS4.642GB/individual1.006GB are not true peaks.

All ranks stop safely at elapsed-time budget. Last R4edge24152–24423 and R5edge67–68
fail geometry-preservation guidance; R2last edge is deferred to next construction
pass. One last message does not classify every rejection or prove a lifecycle defect.
One split pass/no collapse passes; no relaxed gate or increased budget is justified.

## Next active action

Move from incomplete measurement to **coupled geometry-closure capability** on actual
saved residuals. Keep ONEcycle. Do not start another speculative timing, ceiling,
scheduler, representation or placement variant. Use current complete wall census.
R/residual473_terminal_inventory.json and inventory_residual473_terminal.py scan all
304508Owned rows: current edge24152–24423 has3cells[225587,234324,242569], endpoint
stars12/8; edge67–68 has1cell[266816], endpointstars6/5; control6070has10cells.
All six5118736Bparameter payloads are byte-identical. This is inventory, NOT native
association/feature/geometry certification. Last private candidates are not assumed
published. Rediscover exact current support, versions, metrics and associations.

First resolve the test coverage gap: the existing connected controller is entered
through legacy451fourcuts/4218–4219seeds. Do not just point it at473and claim a current
trajectory. Reuse it with explicit CURRENTsupport and one shared24call budget, without
that unrelated precursor, while preserving its old regression. Then test COMPLETEpatch
plus neighbors through geometry closure, tangential delivery and firstrow prerequisites
with existing operators/Directory/reservations/publication.
Separate genuine distance violation, conservative refusal, visibility/positivity,
missing support, association/feature/body/height restrictions and pass deferral.
Use bounded enlargement only for demonstrated necessary neighbors, reacquiring the
complete reservation BEFORE constructing/publishing. Keep q/J observational during
construction, every final gate strict. If neighbor debt/crowding or unavailable
coupled replacements prevents readiness, reconsider construction—not another isolated
variant, more rounds or raised limits. No new baseline/perf/GDB campaign.

## Inputs, physical request and unchanged completion

Saved reasonable RANS+SA seed/authority: R/m6_bl_inputs/rans_metric_entry364_v1.
Ninefinite/admissible fields; CL.15573/CD.05027 do not prove convergence/shock resolution.
Mach-onlyL1. Complete441target R/cases/curved441_ramp5_metric_stage_20261112T124340_1300487;
seed R/cases/curved441_ramp5_seed_stage_20261112T123956_1290282.
12payloads303788608B,61330nodaltensors/chord9.6480553402102753 reload bit-identically.
Current transported nodal field plus profile/policy/authority/associations is required;
originalP1 is bootstrap only. Sensor-only export is not a composed target checkpoint.

Requested500k is NOMINAL. Saved initialestimate486914.722±empirical41525.003 (not a
certified bound); evolved-mesh complexity UNMEASURED. Actual nodes/tets are separate.
Core.0054996988679568166,tangent.37775074746138892 unchanged.
Finalh0=2e-6,growth1.2,thickness.01,tolerance.1. RAMP5stage0height32h0=6.4e-5,
h(k)=2^(N-k)h0,k=0..N. Preserve exact stage on resume; config postprocess applies
SetAdap_BL_RampEvent(0). Factor10 comparison remains queued after usable output.

Euler293 COMPLETED two strictEuler50k meshes149051/736323 then172001/870422points/tets,
conservative admissible transfer, finitecontinuedCFD(ONEiteration), isolated adapted
restart.22payloads384685699B freshly hash-preserved after473 in
R/euler293_preservation_during_bl473.json. No new Euler numerical qualification.
Remaining: strictly accepted curvedBL, dedicated nativeMPI BLconstraints, TWOaccepted
FINAL-height RANS adaptations, conservative admissible flow+SAtransfer, finitecontinued
CFD and isolated adapted diskrestart. Growth12stale-front guard and queued Coulaud/
MAdLib target/delivery investigation remain OPEN. Never bypass stale dependencies.

Strictgeometry1e-4,positivity/bodyseparation,q>=.2,J>=.05,maxedge1.8,heights/progression
and budgets unchanged. Oneheavyjob,nice19,j3,libs1,4TBcompile-tmp. Native5400s/
outer14400s/12GiBperrank; surface388.8s/233220work; boundedtrajectory24calls/300s/
600souter/64old/128new. Keep source freezing and pushed reviewable checkpoints.
