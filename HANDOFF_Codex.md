# Native adaptation — current handoff

Updated2026-11-13 Europe/Rome. Goal ACTIVE. One connected curved-BL cycle.
R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
Read AGENTS.md, [trajectory and stalls](Handbook/ADAPTATION_CONNECTED_PATCH_TRAJECTORY_20261113.md),
[ordered stages](Handbook/ADAPTATION_CURVED_BL_STAGE_INTEGRATION_20261111.md),
[timings](Handbook/ADAPTATION_TIMINGS.md), [next action](HowToProceed.md),
[bounded cavities](Handbook/ADAPTATION_M6_BOUNDED_CAVITY_COMPARISON_20261112.md),
[continuation](Handbook/ADAPTATION_FIRST_HEIGHT_CONTINUATION_20261112.md),
[backlog](Handbook/ADAPTATION_BACKLOG.md).
[Prior handoff/history](Handbook/ADAPTATION_HANDOFF_HISTORY_PRE_470_OUTCOME_20261113.md)
retains all earlier findings/handles; it is NOT live workload state.

## Live state, source and build

NO owned heavy workload live.470build, focused, actual trajectory, MPI milestone and
actual full surface stage are TERMINAL; actual PID/starttick identities independently
verified before reporting. No stop or timeout was needed. Preserve unrelated changes.
Source checkpoint0adea12bff is pushed to origin/codex/native-3d-bl. Later evidence-only
checkpoint may supersede it; consult HEAD/origin. Current production source and470
binaries match frozen pins.469experimental wall-placement fallback was rejected and
restored before470; it is NOT integrated. Exact archive/patch/disposition remain in R.

Build470PASS580.205s/all155actualidentities terminal/pins/newbinaries/cleanNinja.
TestSHA bcd378a9b412ca60e0fb803b3525f65c913e6f5ff67c0874dd00e964f1e45e62;
CFDSHA a0eea83a5a2f957ff5ed33e617d9ef2330edda86f7cd502a49e0c0c22c4bfd81.
R/build470_status/source_pins/terminal_handles.json records exact provenance.

470corrects phase-inconsistent ReconstructSplitPatch: under existing ValidityOnly,
a complete positive cone may defer first-row carving ONLY with exact requested
heights, fully measured provisional admission, certified reference clearance and
passing-row preservation. Restore/Finish/final height and passing-row checks remain.
Absolute callers and nonadmissible cones retain existing carving. No empty heights,
changed target, config, scheduler, geometry authority, direction policy or gate.
ValidityOnly covers the entire Adapt call, not just PrepareSurface; strict final
height closure outside surface preparation remains mandatory. This is provisional
construction, never a first-row certificate.

## Qualified scope and actual complete trajectory

Focused21cases/4077assertions PASS7.713swrapper/1711pins/allactualhandles terminal.
Actual R/cases/curved470_surface_transaction_serial_20261113T012249_1303657
(session54309terminal): PASS251.852swrapper/1770pins/303622originalOwned byte-identical451.
Exact16step replay reproduces2cell inherited inversion; full rediscovery/reservation
of3cells then publishes10positivecells with all metric/parameters/contact/passing-
height/provisional/reservation/publication checks. Geometrybad3→3, excess
5.169003664825083e-7→4.913117750796659e-7, q/J3→10. No strict height/shape claim.
24calls/22commits: observation36→110(+11previouslyunobserved oldcells), geometry16→24,
localdelta+2geometry/+63q/+63J. Geometry notclosed, tangential readiness unproved,
firstrow UNREACHED.74.886533wall/74.866628CPU/1590930geometrywork. Different observation
scope prevents like-for-like bad-count/timing claims. R/surface_transaction470_assessment.json.

MPI32cases on EVERYrank PASS155.969swrapper/3118–123670assertions/rank, actualhandles
and pins verified. R/surface_phase470_qualification.json. Covers changed local strict/
provisional admission, firstrow/corner engine behavior, curves, empty ranks, constraints,
resource expiry and actual saved-M6 cross-owner growth/reservation/conflict/stale/cache
control. The MPIactual control uses existing coarsening correction; the exactnew3→10
cavity has SERIAL publication evidence. Do not claim it was independently replayed
through MPI publication. Full distributed stage below tests production integration;
no per-operator deferral counter is exported, so its exact contribution is unknown.

## Latest full same-target surface stage — NOT READY

R/cases/curved470_phase_consistent_saved_target_surface_mpi6_20261113T013323_1333069
(session91510terminal). Wrapper498.917s/native389.138s; all10actualidentities terminal,
1740source/runtime/input pins verified. R/surface_stage470_assessment.json and
surface_phase470_outcome.json. All303757tets positive,61494nodes;1153534exact shared
nodal coordinate/tensor checks. Complete six Owned/donor/parameter shards retained.
They are failure/diagnostic captures, NOT a qualified reusable successful stage.

164commits/3676globalwork.30500/39128requested-wall cells measured(77.95%), versus
45113779/39014(35.32%) at the same388.8s/233220work stage limits. Ranks0–3 complete
requested-wall measurement; ranks4/5 incomplete. Zero complete global surface passes.
Known1458geometry/37sizebad,273257cellsunmeasured; q/J ALL UNKNOWN. These are different
partial measured sets: neither rawcounts nor committotals prove closure or churn.
Surface_prepared FALSE; no tangential/firstrow/growth/acceptedBL/RANS completion.

Existing counters: ranks4/5geometrywork11.751M/14.219M dominate total30.089M; rank5
geometry-guidance1124refusals/1157trials. Final rank4edge18152–18153 fails geometry
preservation; rank5feature2:8010 retains uncertified curved clearance at facet10649,
cell5930309,lower/h0=-1.404292. Negative LOWER is not proof of penetration. R2has522
known geometry-bad cells despite completed wall measurement. Source/counters do not
yet distinguish every refusal's geometric violation from conservative certification.
Last-rejection text is not a saved complete candidate: feature2:8010 prints only
the first vertex. Rediscover CURRENT full support and verify reproduction; do not
assume its private cell5930309 or original cavity still exists after other commits.

CPU sum2296.593s includes MPIpolling. Seed~90.5s/rank and reconstruction~164s/rank
are synchronized wrappers, not independent useful-work or wait totals. Rank5election
192.886s/rank0~339.328s overlap wrappers; trackedcollectives~.8–.9s exclude that work.
Do not sum nested timers or call election/join time total MPIwait. Sampled treeRSS
4.432GB/individual.925GB are not true peaks. No accepted-mesh speedup is established.

## Next active action

Use latest saved470remaining actual geometry cavities and a successful control.
Reproduce complete current support/transported metrics/associations; classify whole-
patch geometry preservation versus uncertain clearance, topology/visibility,
association/features/heights, missing support and resource refusal. Test a connected
surface+adjacent-volume trajectory with existing operators, not another isolated
normal offset or split variant. Choose ONE correction only from demonstrated useful
complete replacements. No newbaseline/perf/GDB campaign, scheduler, representation,
larger bounds or waived clearance. Keep measured height debt provisional and protect
passing rows. Then continue ordered stages when a credible surface output exists.

## Saved inputs, physical constraints and full remaining goal

Original RANS+SA CFD/mesh/authority: R/m6_bl_inputs/rans_metric_entry364_v1.
Mach-onlyL1; ninefinite/admissible fields; CL.15573/CD.05027 do not prove convergence
or shock resolution. Exact hashes in producer provenance and prior handoff.
Complete441target: R/cases/curved441_ramp5_metric_stage_20261112T124340_1300487;
seed R/cases/curved441_ramp5_seed_stage_20261112T123956_1290282.
12payloads303788608B,61330nodaltensors/chord reload BITIDENTICAL qualified in
R/target_checkpoint441_qualification.json. Policy+authority+current transported nodal
state required together; sensor-only export is insufficient. OriginalP1 is bootstrap.

Requested complexity500k NOMINAL, initialestimate486914.722 ±empirical41525.003
(NOT certified bound); evolved-mesh complexity UNMEASURED. Actual61494nodes/303757tets
are separate. Core.0054996988679568166,tangent.37775074746138892 unchanged.
Finalh0=2e-6,growth1.2,thickness.01,tolerance.1. CurrentRAMP5stage0height32h0=6.4e-5.
h(k)=2^(N-k)h0,k=0..N. Preserve exact stage/profile/policy on resume. Factor10 comparison
queued only after a usable patch:32→3.2→1 versus32→16→8→4→2→1; no ratio/config change.

Euler293COMPLETED two strictEuler50k meshes149051/736323 then172001/870422points/tets,
conservative admissible transfer, finitecontinuedCFD(ONEiteration) and isolated adapted
restart.22payloads384685699B freshly hash-preserved in
R/euler293_preservation_during_bl470.json. No new Euler numerical qualification.
Remaining: credible surface, constrained/blended directions/firstrow, actual physical
height/alignment/progression/coverage/junctions/collisions/outertransition, strict BL
closure, dedicated native-MPI constraints, TWOaccepted FINAL-height RANS adaptations,
conservative admissible flow+SAtransfer, finite continuedCFD, actual isolated diskrestart.
Known Growth12stale-front guard and queued Coulaud/MAdLib target/delivery investigation
remain open; never bypass stale dependencies or freeze columns. Strictgeometry1e-4,
positivity/bodyseparation,q>=.2,J>=.05,maxedge1.8,heights/progression/resources unchanged.

Oneheavyjob,nice19,j3,libs1,4TBcompile-tmp. Native5400s/outer14400s/12GiBperrank;
surface388.8s/233220work; boundedtrajectory24calls/300s/600souter/64old/128new.
Keep source freezing, resumable provenance, meaningful MPI milestones and pushed
reviewable checkpoints. Goal ACTIVE; no final capability from passing fixtures.
