# Native adaptation — current handoff

Updated2026-11-12 Europe/Rome. Goal ACTIVE; ONE connected curved-BL cycle/one heavy job.
R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
Read AGENTS.md, [ordered stages](Handbook/ADAPTATION_CURVED_BL_STAGE_INTEGRATION_20261111.md),
[first-height implementation/evidence](Handbook/ADAPTATION_FIRST_HEIGHT_CONTINUATION_20261112.md),
[actual cavity comparison/MPI design](Handbook/ADAPTATION_M6_BOUNDED_CAVITY_COMPARISON_20261112.md),
[timings](Handbook/ADAPTATION_TIMINGS.md), [HowToProceed](HowToProceed.md),
[backlog](Handbook/ADAPTATION_BACKLOG.md). [History through440](Handbook/ADAPTATION_HANDOFF_HISTORY_PRE_441_20261112.md)
retains compiler failures, previous handles,434 surface results and older qualified improvements.

## Live workload: preserve frozen source

No owned heavy job currently.441 producer/reload BOTH TERMINALPASS, allactualhandles verified.
Freshreload R/cases/curved441_ramp5_target_reload_20261112T125829_1338941 exec14080terminal:
435.543s; all61330tensors BITIDENTICAL and chord9.6480553402102753 BITIDENTICAL.
R/target_checkpoint441_qualification.json / target_checkpoint441_producer.json;
complete303788608bytes/12payloads locally retained. Fidelityqualified, not physicalBL/mesh acceptance.
Source pins R/build441_source_pins.json. Build441PASS288.295s, all17actualhandles terminal.
TestSHAb806f7d8195e1c2018170ff340a39f1bc2305e5f2f9feb15e911c2e6ecfbff32;
CFDSHAefd55973195fa6751e5261ffbf6d01ffbac2263a243d5213bc7693fc52cd5495.
Next one bounded stage3 saved-newtarget surface integration (check_saved_surface441.py),
only after host/owned-handle verification; no fullCFD workflow retry.441source bytes unchanged.
Qualified task source/evidence is being checkpointed; use git HEAD for commit identity.
Preserve unrelated P1/FEM/.orig/log/submodule working changes.

## Current stage2: common starting field, not a chosen shrink ratio

Latest USER asks factor10 instead ofhalving. Answered: test, do not switch blindly.
Compare SAME start32h0:32→16→8→4→2→1 versus32→3.2→1(finalcap).100→10→1 changes initial
height too. No production ratio change yet. Default production dc17::RebuildFirstRow
accepts arbitrary requestedh (no exacthalf assumption); Halve+ColumnInsert branch is
test-only. CompleteFirstRow does NOT establish full shrink-band g/T progression/coverage.
Use actual generated patches after credible surface; fewer events alone is insufficient.
The32h0 initial target is common upstream work while this choice is evaluated.

Explicit proposed stage0 seed saved/qualified metadata:
R/cases/curved441_ramp5_seed_stage_20261112T123956_1290282,
R/ramp5_seed_stage441.json. Original mesh/ref/CFD bytes unchanged; original364 inputs intact.
Derived cfg ONLY adds existing ADAP_BL_H0_RAMP=5 and SUBITER7; finalh0=2e-6,g1.2,T.01,
stage0h0=6.4e-5. Seven remesh events would include five intermediate and TWO final-height
events; this is baseline metadata, not proof that this schedule is preferable/constructible.
Stage0 initializer uses existing sidecar format and refuses overwriting progress; no CFD advance.
Seed helperPASS5.661s, JSON-export regressionPASS3assertions/1case4.845s, allactualhandles terminal.

Stage0 metric producer TERMINALPASS624.400s; allpins/actual9handles terminal:
R/cases/curved441_ramp5_metric_stage_20261112T124340_1300487 (exec31528terminal).
SourceCFD unchanged/no flow step, Mach error4.44e-16; original61330nodes/303359tets.
Requested500000; estimated486914.7222, empirical indicator41525.0034 NOT a bound,
bracketed1; core.0054996988679568166,tangent.37775074746138892.
Metric max529.708s, construction462.297s, Hessian~.5s, setup~66.9s. Old396 metric531.651s,
construction480.951s/setup50.305s: walltotal similar; different target/contention, no speed claim.
Complete target payloads and fresh reload now independently fidelity-qualified; see above.
Checkpoint chord9.6480553402102753 (different field from396's42.0666, not an accuracy comparison).
Producer sampled individualRSS.852GB, notwhole-job peak; metric reserved10.67GB is admission,
notresidentmemory. Geometry/gradation/collective timers overlap; CPU can include MPI polling.

## Connected correction implemented and focused qualification

Existing ADAP_BL_H0_RAMP=N already implements h(k)=2^(N-k)h0,k=0..N; Nhalvings/N+1heights.
Saved364cfg omitted it(default0), so396/434/437 imposed final2e-6 immediately.
Existing resume reset event0 and stored no stage. Fixed using existing .native_rows envelope:
NEXT absolute event + N/true-request/schedule signature + final saved mesh fingerprint/checksum.
Row topology digest remains independent and may invalidate after optimization. Pending metadata
promotes only on accepted replacement. Driver elects rank errors/agreement before reference setup,
restores effective height/flow controls and absolute cycles, checks only FUTURE output names.
Target checkpointv2 stores CURRENT event; explicit restoring read rolls config back on refusal.
Legacy396v1 still reads rampOFF, refuses ambiguous rampON. Seed restart requires explicit event0
stamp, not case-name/mesh-class inference. No new config/scheduler/gate/budget policy.
Existing Json(string)=std::quoted produced invalid replay JSON for C0 characters;441 fixes actual
shared serializer (native failure-capture escaping pattern) and tests nested replay/newline/NUL.

438dproduction qualified serial11cases367assertions6.731s. MPI439numeric alias =exact438d:
60/61cases passed/rank; ninefailures/rank ONLY obsolete count==3 assertion despite valid6-cell
repair/full independent strict AuditPASS.440 removes only fixedcount assumption, keeps bounded
count/topology/geometry/q/J/metric gates. RetryONLY failedcasePASS10.117s,1512–2996assertions/rank;
production CFD binary identical438d. Combined milestone complete by reuse; original failure retained.
R/height_continuation440_qualification.json. Scope: codecs/persistence and existing constraints/
ownership guards. NOT actual generated-BL continuation, new MPI cavities or adapted CFD restart.

## Actual bounded evidence: do not overclaim

437 fixed diagnostic prescribed-interface ordering and stale/retired sample selection.
R/cavity_correction437_assessment.json:301171Owned records unchanged434;9private-feasible
alternatives, zero oriented-interface refusals.11990→11886 direct8star fails positivity;
apex27313 coupled reconnect/collapse after growth to17 gives16cells, privateq/Jbad16→13.
Another10→8 alternative;13785 retained cone grows8→11 then9cells (all9q/Jbad; smaller alone
is not shape closure). Geometry/height-preservation/tangent-metric/parameter/clearance pass;
no publication/final certificate. Fulltarget3edges/zero complete patches before work/time refusal.
Leading-edge contractions still refuse geometry guidance; no outside-tolerance witness established.
Reference decompiled production has broader proposal/reduction/surface/restart logic; this native
bounded composition is NOT equivalent, and no pyAMG runtime/superiority comparison was performed.

440 matching-cavity height diagnosticPASS284.555s, allpins/actualhandles terminal:
R/cases/curved440_cavity_height_continuation_20261112T120931_1211699.
R/height_continuation440_assessment.json:461records,301171Owned records unchanged.
Finalk5centroid tensors BITIDENTICAL396. Tangent edge maxrelativechange1.506e-6/no sampled
.5/.9/1.6/1.8classification change. All60strictheightchecks fail: positions were held fixed,
NO layers generated. Stage0 exact6032–6072length.9943139649742233 in10.346s (geometry9.987nested,
45integrand evaluations); stages1–5each20s censored in PREPARATION before integrand/0evaluations.
Normal severity affects preparation, but no fine-stage cost/full-run speedup or row progression proven.
Raw malformed JSON retained; normalized copy has same values. No approximate final measurements.

No production cavity growth implemented/MPI-qualified. Read-only seam audit: preserve contraction,
feature/body/height filters; direct orientation prefilter can retire salvageable choices before
operator. Discover full support collectively between Lookup/Fetch and FIRST reservation on the
unbatched curved/composed SurfacePlanning coarsening route. No Directory calls in private repair/
MeshOptimizer. Whole final old rows/vertices must be reserved/versioned, mapped params validated,
clearance checked and atomically published. Current coarsening parent-owner mapping assumes retained
endpoint star and must be generalized for expanded reconnect outputs. Reuse PublishMetrics and
complete changed-vertex notification/front invalidation; never publish against the seed footprint.

## Next actions in order

1.Freshreload/actualhandles/fidelity qualification DONE. Preserve complete stage0 files. Existing
check_metric_stage441.py supports producer/reload; no CFD step. Source remains441.
2.New coreexport DONE with assess_metric_stage441.py; producer/core_allocation_assessment.json
and selected_core_native.bin are CORE ONLY, never fulltarget replacement. Flow/core median-dual
complexity192875.685 (old396119826.626); corefraction r>20 .52066 (old.49525), NOT composed fraction.
V2directionalinspector assess_target_directions441.py crosschecked61330JSON/tensors;42local
samples retain stage normaldensity~15625, tangent densities~11–23%higher than396 because
normalization reallocates freed normal budget. Curvature proxies NOT certificates. Complete
checkpoint and original/current geometry remain necessary; no396core with new target.
3.Once target is credible, coupled surface preparation under unchanged budgets/gates.434 was NOT
ready (789points removed,4016/37227requiredwallrows,0completepasses); no new firstrow/full retry
from a speed counter. Use demonstrated cavity correction only if active residuals justify it,
with serial+ONE meaningful cross-rank growth/conflict/stale/empty/capacity/publication milestone.
4.Then constrained/blended directions/firstrow, actual half-vs10 shrink comparison, successive
fronts/fullphysical progression+coverage, outer transition/strict closure. Known Growth12 stale
boundary/front dependency failure remainsOPEN; do not bypass guard/freeze columns. Then transfer/
continuedCFD/second FINAL-height adaptation/isolated adapted disk restart.
Queued Coulaud/MAdLib physicalcurvature/target/delivery investigation remains; CAD optional.
Latest inspection VTU remains428 known0/NaN q/J/geometry, not accepted BL; no pertrial VTUs.

## Preserved input, objective and resource gates

Original same303 RANS+SA entry R/m6_bl_inputs/rans_metric_entry364_v1: Mach-onlyL1,
nominal500k,h0=2e-6,g1.2,T.01,heighttol.1. Ninefields finite/admissible; CL.15573/CD.05027
not convergence/shock-resolution proof. MeshSHA8c839170eb5e359f7de6a41434aa6a9c2d620053f37de6d9360bd04d2a63088b;
ref2e99fb7870e2c718f110da8509e4c0f7e746ce138e7f27ab57aa0dedbb6c42de;
fieldaf7ade468dc40cd39dbe9238248e50057cdd151a64fef92fda957677aa3f6cae;
originalcfg0a7de022729f69b170deaee2eaabbd5347550da8134cbdaa39587f7173e78742.
Original396 fulltarget independentlyqualified/reloadbitexact preserved (304MB/12payloads);
estimate461495.991±empirical47864.008,core.00460725,tangent.25. Current nodal transport authority
must be preserved after topology changes; original donors bootstrap only, not a pointwise frozen field.

Euler293 COMPLETE: two strictEuler50k meshes149051/736323 and172001/870422points/tets,
conservative admissible transfer, finite continuedCFD(ONEiteration) and isolated adaptedrestart.
Preserve22payloads/384685699B. Remaining: usable STRICTcurvedBL, dedicated nativeMPI BL constraints,
TWOaccepted FINAL-height RANS adaptations with conservative admissible flow+SA transfer, finite
continuedadaptedCFD and ACTUALisolated adapteddiskrestart.500k nominal, not nodecount/rejectiongate.
Strictgeometry1e-4, positivity/bodyseparation,q>=.2,J>=.05,maxedge1.8,requestedheight/progression
and resources unchanged. Intermediate quality debt provisional. One heavy job,nice19,j3,libs1,
4TBcompile-tmp; native5400s/outer14400s/12GiBperrank; surface388.8s; patch300s/outer600s.
Reuse passing provenance/counters, bounded work/memory/time, exactsourcefreezing, checkpoints/pushes.
