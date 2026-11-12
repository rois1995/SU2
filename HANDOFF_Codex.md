# Native adaptation — current handoff

Updated2026-11-12 Europe/Rome. Goal ACTIVE. ONE curved-BL cycle/one owned heavy job.
R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
Read AGENTS.md, [ordered stages](Handbook/ADAPTATION_CURVED_BL_STAGE_INTEGRATION_20261111.md),
[current cost/scope evidence](Handbook/ADAPTATION_SURFACE_COST_AUDIT_20261112.md),
[bounded cavities](Handbook/ADAPTATION_M6_BOUNDED_CAVITY_COMPARISON_20261112.md),
[first-height continuation](Handbook/ADAPTATION_FIRST_HEIGHT_CONTINUATION_20261112.md),
[timing ledger](Handbook/ADAPTATION_TIMINGS.md), [HowToProceed](HowToProceed.md),
[backlog](Handbook/ADAPTATION_BACKLOG.md).
[History through447 qualification](Handbook/ADAPTATION_HANDOFF_HISTORY_PRE_447_OUTCOME_20261112.md)
preserves earlier evidence/handles, NOT live workload state.

## Live state and provenance

No owned heavy workload is live. Build451 and every validation/replay wrapper are
terminal; actual PID/start-tick receipts were rechecked before recording outcomes.
Source checkpoint **dd4614949f** is pushed to origin/codex/native-3d-bl; consult git
log for a later evidence-only checkpoint. Preserve unrelated working changes.

Build451 PASS407.947s, all36observed actual identities terminal, source pins unchanged,
new test binary and clean Ninja dry-run. R/build451_status.json/source_pins.json/
terminal_handles.json. Current test SHA:
3d68c30ce14a4afe1e3b2eabd94e5e8a73b899ce5fabb0e381a556d3dc4fe03d;
CFD SHA f15bc306e1558ed5d9e8ee804ecc03155f4aecc55b754634ee2398f79fa5509a.
Serial22cases/11679assertions PASS4.783s; MPI6 same22cases on EVERYrank,
1146–11258assertions PASS4.658s,1684pins/case and actual handles terminal.
R/boundary_strip451_qualification.json. This is not generated-BL qualification.

Latest actual stage3 output:
R/cases/curved451_strip_reuse_saved_target_surface_mpi6_20261112T185700_120424
(session10685terminal). Wrapper498.966s/native389.125s; all10actual identities terminal,
1712source/runtime/input pins verified. R/surface_stage451_assessment.json and
boundary_strip451_outcome.json. All303622tets positive,61437nodes,1153051exact shared
nodal coordinate/tensor checks.107commits/4180proposalwork;13779/39014requested-wall
rows measured,0completepasses, NOT surface-ready. Partial486geometry/23sizebad;
q/J ALL UNKNOWN,289843unmeasuredcells. Failure shards retained, not a credible
successful surface checkpoint or usable BL. Same savedCFD/complete441target/original
six partitions/32h0,388.8s/233220work/12GiB limits; no CFDadvance, metric recomputation,
sensor/norm/height/normalization/gate change. Requested nominal500k, INITIAL estimate
486914.722±empirical41525.003(notbound); achieved new-mesh complexity UNMEASURED.
Sampled treeRSS4.336GB/individual.899GB are not true peaks; rankCPU2284.099s includes
possible MPIpolling. Euler293 and original396target remain intact.

## Connected correction and demonstrated remaining blocker

450actual diagnostic rejected the StorageBytes accounting-cache hypothesis:
.0000122–.0000217s per82–84calls versus.117–.141s complete guidance per14nodes.
R/planning_cost450_assessment.json:608989assertions,1736pins, all303493Owned records
byte-identical449, all actual handles terminal. Do not reopen without contrary evidence.

451reuses position-only OptionA thin-strip results from the first side ALREADY in
the result vector, only within ONE BoundaryGuidance call. Fresh side-specific metric/
normal evaluations, exact output order/duplicates and256sample cap remain. No extra
allocation, persistent cache, configuration, target, scheduler or publication change.
Actual449 star comparison PASS136.695s/610055assertions,1738pins/all actual handles
terminal/all303493Owned records byte-identical. Exact ordered tensors/normals/rules
and geometrywork agree with a frozen pre-change implementation.39140strip pairs;
paired local guidance3.70–5.79x faster,10,880,920derived avoided predicate calls.
Old runs first; timing/order/contention limits explicit. R/strip_reuse451_assessment.json.

At matched389s stage budgets,451vs449 requested-wall coverage13779vs5890(2.34x),
commits107vs54, summedCPU2284.099vs2296.573s. No completed pass in either run. Different
partial bad sets(486/23vs179/8) cannot establish whole-field closure or churn.
R4/5 metric wrappers98.360/97.582s versus163.599/171.997s; nested guidance23.592/25.884s
versus12.028/14.306s. More useful stage work, not accepted-mesh time or totalMPIwait.
Geometrywork16.268Mvs7.547M reflects more trials; do not call it traversal savings.
Rank3 now measures all1670local requested rows; global readiness remains false.

Next investigate actual required-surface replacement feasibility, not another
speculative performance cycle. Final R5 rejection is edge7907,7908/facet10515,
privatecell2077984, clearance/h0=-3.742569; R4 feature2:23829/facet47732,
privatecell2083141, clearance/h0=-4.970417. Positive smooth-reference clearance was
not certified; these numbers do NOT independently prove physical penetration.
Reconstruct complete final saved contexts with current associations/nodal metrics,
including a successful control. Separate actual geometry/height violation, conservative
certificate refusal, placement/connectivity obstruction and incomplete support before
selecting a correction. Existing four vertex-star query samples are NOT exact failed
split-cavity reproductions. No new baseline/perf/GDB campaign or raised budget.

Qualified447wall-only published measurement,448required-debt handoff and449bounded
idle census remain. Busy cursors are not drained; all ranks keep aligned admission/
vote/measurement; repick precedes the same active iteration. No upfront full census.
Full private cavity support/validity/transport/publication remain mandatory. Earlier
counter attribution/handle details are in the linked cost audit and449outcome receipts.

## Reuse conclusions and bounded cavity capability

446actual fivecontext guidance comparisonPASS167.118s/602428assertions,300958Owned
recordsbyte-identical441, allhandles terminal. Existing GeometryGuidanceMemo reused
ZERO faces in every sampled removed-vertexstar; reject its coarsening integration.
Warm replayworkdrops withzerohits reflect existingcachewarming/order, not newmemo benefit.
Cachedplanning edge scalars include sidedguidance but lack association/policy keys;
never pass them blindly as core-only reuse. Published CavityMetricSource already reads
localcurrent sensors. Existing MeasureSurfacePlanning deduplicates nodes withinbatch.

445qualified bounded visibility correction remains integrated. Actual11990direct
collapse fails positivity; bounded grown16→14reconnect-collapse is private feasible,
qbad14→10/J13→10/geometry0→0; afterpublishedtransport q/J10/10 remain. Discovery precedes
reservation, retains full removedstars/version identities, fullvertex conflicts and
atomic publication. MPI6 migration/emptyrank5/stale/resource restoration/constraints
qualified; strictheightFALSE. No accepted BL or pyAMG superiority/equivalence claim.
See detailed handbook/archived receipts, including preserved442/443/444fixture failures.

## Upstream state and remaining workflow

Saved original RANS+SA CFD/mesh/reference: R/m6_bl_inputs/rans_metric_entry364_v1.
Mach-onlyL1, nominal500k, truefinalh0=2e-6,g1.2,T.01,heighttol.1. Ninefinite/admissible
fields; CL.15573/CD.05027 doesNOTprove convergence/shock resolution.
MeshSHA8c839170eb5e359f7de6a41434aa6a9c2d620053f37de6d9360bd04d2a63088b;
ref2e99fb7870e2c718f110da8509e4c0f7e746ce138e7f27ab57aa0dedbb6c42de;
fieldaf7ade468dc40cd39dbe9238248e50057cdd151a64fef92fda957677aa3f6cae;
originalcfg0a7de022729f69b170deaee2eaabbd5347550da8134cbdaa39587f7173e78742.
DerivedRAMP5stage0seed: R/cases/curved441_ramp5_seed_stage_20261112T123956_1290282.
Complete441target: R/cases/curved441_ramp5_metric_stage_20261112T124340_1300487.
Initialestimate486914.722±empirical41525.003, core.0054996988679568166,
tangent.37775074746138892;12payloads303788608B retained. Freshreload61330tensors/chord
9.6480553402102753 BITIDENTICAL: R/target_checkpoint441_qualification.json.
Policy+geometry+currentOwned sensors required together; sensor-onlyexport insufficient.
Current nodaltransport authority stays; originalP1donors bootstrap only.

Stage3surface readiness stillOPEN. Then constrained/blended directions/firstrow,
actual physical firstheight/alignment/coverage/progression, junctions/collisions,
outertransition and strict closure. Known Growth12 stalefront/dependencyguard OPEN;
do not bypass/freezecolumns. Queued Coulaud/MAdLib curvature/target/delivery work remains.
RAMP=N uses h(k)=2^(N-k)h0,k0..N; stage0currently32h0=6.4e-5. Persist/resumeexactstage
and complete target. Factor10 experiment queued ONLYon usablegeneratedpatch:
compare32→16→8→4→2→1 against32→3.2→1(finalcap), not100→10→1. No ratio/config change.
Arbitraryheightrebuild support doesNOTprove fullshrink-band progression.

Euler293COMPLETED two strict50k meshes149051/736323 then172001/870422points/tets,
conservative admissible transfer, finitecontinuedCFD(ONEiteration) and isolated adapted
restart. Preserve22payloads384685699B and original396target. Remaining: usableSTRICT
curvedBL, dedicated nativeMPI BLconstraintqualification, TWOacceptedFINAL-heightRANS
adaptations, conservative admissible flow+SAtransfer, finitecontinuedCFD, actualisolated
adapteddiskrestart.500k NOMINALcomplexity, not nodecount/rejectiongate. Strictgeometry
1e-4,positivity/bodyseparation,q>=.2,J>=.05,maxedge1.8,heights/progression/resources unchanged.
Oneheavyjob,nice19,j3,libs1,4TBcompile-tmp; native5400s/outer14400s/12GiBperrank,
surface388.8s,patch300s/outer600s. Intermediate qualitydebt provisional; no premature
finalshape gates. Keep stagecadence, regular pushed checkpoints and full goalACTIVE.
