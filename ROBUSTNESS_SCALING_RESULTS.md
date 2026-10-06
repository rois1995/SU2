# Integrated robustness and scaling results

Updated 2026-10-06. Validation campaign complete; final requirement audit is in
INTEGRATION_COMPLETION_AUDIT.md. This report consolidates current evidence; earlier
chronological reports and failures remain in Git and integration_evidence.

Worktree `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`, branch
`codex/native-integrated`. AdapNoExt is untouched. BRANCH_RECONCILIATION.md pins
source decisions; HANDOFF_Codex.md identifies current jobs and remaining work.
ROBUSTNESS_SCALING_PROTOCOL.md retains the predeclared thresholds/stopping rules.

## What is demonstrated

The integrated experimental native2D backend performs distributed boundary/BL
adaptation, solution transfer and resumed CFD on1/2/4 ranks. MMG stays default;
SU2 and CGNS mesh writing preserve in-memory boundaries and reference state.
Robustness is an observed envelope, not a universal mesh-generation guarantee.
A baseline curved NACA0012 request passes; larger/thinner requests encounter
whole-job cost limits. Four-rank strip capacity exceeds the demonstrated serial
and two-rank capacity under the same budget. These conclusions do not imply
native3D, CAD conformance, converged aerodynamics or general large-rank scaling.

## Integrated regression and output gates

| Gate | Ranks | Executed evidence / result |
| --- | --- | --- |
| Fresh assertions-enabled primal MPI/MMG/CGNS build | — | build-integrated-v2, PASS |
| SU2/CGNS output, mixed order, marker names | 1/2/4 | integrated_output_controls_v6,7cases/rank PASS |
| Native geometry/field/BL/import/output controls | 1/2/4 | integrated_primal_controls_v6,49cases/rank PASS |
| Native memory and unchanged default MMG | 1/2/4 | integrated_primal_memory_v6, integrated_mmg_default_v6 PASS |
| Independent output/mesh/CGNS audits | 1/2/4 | independent_outputs_v6,84PASS |
| Sensors, goal metric, two-pass/reference, adjoint transfer | 1/2/4 | integrated_auxiliary_v9,58cases/rank PASS |
| Fresh AD build and corrected mesh-swap controls | 1/2/4 | ad_repair_controls_v14,3cases/26assertions per rank PASS |
| Actual main-loop native CFD, mesh/restart/flow/metric | 1/2/4 | integrated_native_production_v2,12cycle outputs PASS |
| Actual production frozen-P1/original-reference targets | 1/2/4 | all9PASS, native_production_complete_v1.json |
| Fresh smooth and abrupt nodal-step BL controls | 1/2/4 |192full independent snapshots PASS |
| Exact continuous-adjoint/missing-reference rejection | 1/2/4 | native_step_support_complete_v1.json PASS |
| Instrumented NACA runtime, targets and immediate flow exports | 1/2/4 |9target/phase/field checks PASS |
| General MPI/distributed transfer regressions | 1/2/4 | integrated_general_transfer_v1,40cases/rank PASS |

Native geometry/engine controls include sharp feature retention, nesting and
holes, open/crossing/contact rejection, remote physical collision veto, exact
accepted records after rejected staging, oversized star admission before payload,
and fixed-boundary obstructions repaired by adaptive sampling. These are focused
controls, not exhaustive sharp-corner/high-demand CFD validation.

Production cases live in `integration_evidence/integrated_native_production_v2/np{1,2,4}`.
input.su2 pairs with flow_adap_00000.vtu/solution_adap_00000.dat before adaptation;
later mesh/flow/restart indices agree. All12 double restart/grid pairs are exact,
finite and positive in density/internal energy. Every plotted scalar and coordinate
matches the double restart roundedFloat32; vector fields are outside that reader.
All9 audited target tensors exactly equal the corresponding actual restart fields.

Mach0.3,Re10000,AOA1.25degrees,h0=.0002chord,levels4000/6000/3000; four short
three-iteration solves and three adaptations. Whole-job production1/2/4 times
430.517/155.435/99.552s include CFD/output/checks and are NOT isolated remesh timings.
Final triangles19502/19575/19487; rank-dependent output/work prevents strict
strong-scaling claims. GRID_GUIDE.md links inspectable grids and actual solutions.

## Native BL and target robustness

Fresh smooth and abrupt moving nodal-step fixtures each pass96 independent
snapshots: eight changing-height/refinement-center cycles × single/opposing
walls × barycentric/conservative transfer ×1/2/4 ranks. The nodal step requests
.004left/.014right tangential lengths. Frozen P1 interpolation stays continuous
inside donor cells; cell-discontinuous tensors at a shared vertex are not tested.

| Target | Minimum metric quality | Maximum metric length | Maximum relative h0 error |
| --- | ---: | ---: | ---: |
| Smooth | .2408091374 |1.6490232778 |4.4408921e-16 |
| Nodal step | .3912636549 |1.6950870183 |4.4408921e-16 |

All pass topology/orientation, actual frozen-P1 target and immutable flat-reference
checks; all576 donor/metric/candidate pins were rechecked. Contract: q>=.18,
metric length<=1.8,wall-height relative error<=1e-8. Source fixtures preserve exact
accepted flow/reference/count on incomplete remeshing; incomplete candidates
never replace accepted state. Evidence native_step_support_complete_v1.json.
Earlier smooth96 in integrated_primal_controls_v6 also independently pass.

Matched small straight-wall AR10/100/1000 tests on1/2/4 all9PASS. Geometry,h0 and
tensor scale together, keeping metric demand fixed; these are affine/precision
controls, not curvedAR1000 or increased-demand construction proof. The separate
constant diagonal AR25 request has necessary h0_metric2.5>edgecap1.8; all3
outputs are structurally valid but incomplete, a demonstrated incompatible
request rather than a feasible-construction failure.

## Engine size and partition measurements

Pilot16/64/256/1024 input triangles,cyclic ownership,AR10: all12 independently
PASS. At1024 single-sample adapt1/2/4 times3.443/3.515/3.326s show little benefit;
4ranks has2898cross-owner/3079commits and1277conflicts. No reliable speedup
estimate follows from this small adversarial single-sample group.

Low-cut vertical strips all independently PASS:

| Input triangles | Adapt1/2/4(s) | Output triangles1/2/4 |
| ---: | --- | --- |
|2048 |9.453/6.403/4.797 |1532/1548/1512 |
|4096 |28.019/15.195/9.205 |2948/2883/2957 |
|8192 |98.936/52.893/28.138 |5808/5834/5766 |

These are single samples with differing work/output. Largest complete common
size8192 was repeated as predeclared below. Raw pilot: scaling_pilot_v6;
size/anisotropy/repeats: robustness_campaign_v8, all with independent_audit.json.

### Repeated largest-common-size measurements

All27 runs pass the final independent structure and full frozen-target audit:
`integration_evidence/robustness_campaign_v8/repeated/independent_audit.json`.
Three samples per layout/rank at8192 input triangles:

| Initial ownership | 1 rank min / median / max (s) | 2 ranks (s) | 4 ranks (s) |
|---|---:|---:|---:|
| Cyclic | 97.988 / 99.516 / 101.773 | 94.756 / 95.533 / 115.950 | 52.242 / 52.513 / 53.654 |
| Horizontal | 104.948 / 106.234 / 108.123 | 52.268 / 52.416 / 52.755 | 53.843 / 53.982 / 54.318 |
| Vertical low-cut | 97.181 / 97.598 / 100.287 | 51.790 / 51.900 / 52.225 | 28.060 / 40.365 / 50.656 |

[Plot](integration_evidence/partition_comparison_v1/partition_times.png), raw CSV
and provenance are in `partition_comparison_v1`; plotting source is
`integration_evidence/plot_partition_comparison.py`. Vertical four-rank snapshots
and work counts are identical across repetitions (16012 commits,163 cross-owner,
11 conflicts), while timings vary widely on this shared machine. The median
is40.365s;28.060s is the fastest observation, not a reliable typical time.
Rank/layout-dependent meshes and work prevent an identical-work speedup claim.
Engine timing excludes CFD, transfer and audit. Capacity follow-through is complete;
real-airfoil phase verification is complete.



### Memory, ownership and encoded traffic

All27 audited raw hashes were rechecked before aggregation. Owned-cell imbalance
is P times the largest rank-owned output-cell count divided by global output
cells;1 is balanced. Within each layout/rank group this value is identical across
three repetitions. RSS entries are the distribution of each run's maximum rank
Linux VmHWM, captured before the final probe audit gather. They include startup,
replicated synthetic input and earlier operations, not CFD or a transaction
allocation ceiling. Encoded payload is summed across ranks including self buckets
and engine initialization; it is not measured network traffic.

| Ownership | Ranks | Owned-cell imbalance | Rank-max RSS min / median / max (MiB) | Encoded payload median (MiB) |
|---|---:|---:|---:|---:|
| Cyclic | 1 | 1.0000 | 31.234 / 31.406 / 31.832 | 413.386 |
| Cyclic | 2 | 1.0407 | 26.477 / 26.523 / 26.836 | 484.646 |
| Cyclic | 4 | 1.0720 | 24.953 / 24.965 / 25.082 | 533.728 |
| Horizontal | 1 | 1.0000 | 31.445 / 31.520 / 31.539 | 413.386 |
| Horizontal | 2 | 1.0028 | 26.973 / 27.020 / 27.055 | 459.870 |
| Horizontal | 4 | 1.2640 | 24.934 / 25.090 / 25.168 | 544.537 |
| Vertical low-cut | 1 | 1.0000 | 31.285 / 31.461 / 31.473 | 413.386 |
| Vertical low-cut | 2 | 1.0130 | 26.547 / 26.750 / 26.852 | 451.679 |
| Vertical low-cut | 4 | 1.0635 | 24.109 / 24.781 / 25.211 | 520.791 |

Horizontal four-rank ownership ends with26.4% more cells on its busiest rank than
the average, compared with6.35% for vertical and7.2% for cyclic. This imbalance
is observed alongside the horizontal four-rank timing plateau; these measurements
do not isolate its contribution or prove a sole cause. All27 report zero memory
admission rejections. Initial-directory exchange work reaches3.625MiB at one rank;
that transport is outside the transaction ceiling, so it does not demonstrate a
2MiB admitted-transaction breach. Actual CFD rank memory/durations are independently verified below. Exact inputs/aggregation:
`integration_evidence/partition_work_memory_summary_v1.json`.

## Actual NACA phases and rank memory

The rebuilt instrumented1/2/4 testcase passes all9actual frozen-P1/original-
reference target audits. check_airfoil_phase_timing.py validates each MPI MAX
against local rank rows, owned-point totals against saved geometry, monotonically
cumulative VmHWM, and all18donor/immediate-transferred flow CSVs against exact
global point IDs/coordinates with positive density/internal energy.
Evidence: integrated_airfoil_phase_timing_v1/{independent_v1,timing_audit_v1.json}.

| Ranks | Cycle | Remesh MAX(s) | Replace MAX(s) | Transfer MAX(s) | Process VmHWM MAX(MiB) | Owned-point imbalance |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
|1 |0 |56.619795 |0.241228 |0.124765 |110.062 |1.0000 |
|1 |1 |116.257270 |0.382893 |0.255120 |156.207 |1.0000 |
|1 |2 |93.977124 |0.333444 |0.239420 |159.414 |1.0000 |
|2 |0 |40.936907 |0.153065 |0.075155 |72.387 |1.0204 |
|2 |1 |60.875604 |0.238221 |0.147984 |100.121 |1.0180 |
|2 |2 |49.954152 |0.196692 |0.129335 |100.121 |1.0081 |
|4 |0 |26.186099 |0.146994 |0.061310 |53.383 |1.0177 |
|4 |1 |40.087717 |0.226104 |0.131751 |72.207 |1.0158 |
|4 |2 |30.764154 |0.141014 |0.081547 |72.207 |1.0166 |

Metric MAX is.0086–.0429s,solve(with inner monitoring/output).0234–.1328s,
mesh-output.0077–.0208s per cycle. Full backend remesh includes import/adapt/export;
replacement includes repartition/reconstruction and transfer. Local replacement
minus transfer is separately reducedMAX. Each phase MAX can belong to a different
rank; maxima are NOT additive and transfer is a subset of replacement.

Initial driver/reference preparation, snapshot gathers/CSV I/O and independent
audits are outside these timers. VmHWM includes startup and prior snapshots;
local elements include CFD overlaps. This is a short viscous coupling workload,
not production-length CFD. Its measured remesh cost greatly exceeds transfer;
rank-dependent output/work, one sample per rank and shared-host variability
prevent identical-work or reliable typical-speedup claims.

## Capacity and real-airfoil cost limits

| Axis | Last demonstrated completion | First observed cost stop |
| --- | --- | --- |
| Serial vertical strip |8192input |16384timeout124,241.410s |
| Two-rank vertical strip |8192input |16384timeout124,241.910s |
| Four-rank vertical strip |16384input→11718output,adapt98.625s,whole176.847s |32768timeout124,241.040s |
| Serial NACA complexity4k/12k/3k,h0=.0002 |cycle0:23368tri;cycle1:40968tri |next coarsening from40968,timeout124,902.433s |
| Serial NACA h0/HMIN=.0001,4k/6k/3k |cycle0:38852tri |next remesh from38852,timeout124,902.055s |

The strip budget is240s for the whole job, with120s observed adapt escalation
stop. The NACA budget is900s/job. Timeouts were reaped, failed artifacts retained;
no timed-out phase, infeasibility, memory bound or invalid topology is established.
Two-rank32768/65536,four-rank65536,h0=5e-5 and higher-rank failed-demand NACA
controls were NOTRUN under the predeclared stopping policy. Thin/high-demand MPI
capacity remains unproven. Four-rank16384 independently passes full structure/
constant-target/height checks. Pins integration_capacity_v2.json and
capacity_ad_followthrough_complete_v1.json.

NACA baseline4k/6k/3k,h0=.0002 runtimes and9independent actual-metric/original-
reference targets pass1/2/4 in integrated_airfoil_baseline_v8. Larger-demand
cycle0 is byte-identical to an already executed baseline audit. Larger cycle1
(40968tri) and thinner cycle0(38852tri) now independently PASS all topology,
actual-P1 metric/height and original-reference contracts, with all inputs rechecked.
Their min-quality/max-length/max-relative-height-error are respectively
.2379140595/1.7982356311/1.6354e-13 and .2758668000/1.7999779076/4.0090e-13.
These completions do not turn their next-remesh timeouts into successful cycles.
Consolidated proof: native_phase_partial_complete_v1.json. Cost pins:
airfoil_complexity_cost_limit_v1.json, airfoil_height_cost_limit_v1.json.

## Stage G integration correction and solver limits

Integration uncovered duplicate halo adjoint seeds after SwapMesh: transferred
halo copies were seeded and their taped primal halo exchange added owned seeds
again. Clearing only nonowned seeds before recording fixes the shared driver
path (79ce4bc81d); transfer utility halo copies retain existing semantics.
Tight-Jacobi diagnosis reproduced the original failure before the fix. v14
original-settings1/2/4 controls pass all3cases/26assertions per rank,ten lifecycle
replacements,counters beyond LIMITER_ITER,restart/RSS and exact continuation
residual/adjoint differences0. Ownership is unchanged in this control: it does
not prove partition-independent iteration histories or sensitivity accuracy.

Actual warm1/2,cold1 and warm/cold4 short goal lifecycles pass. cold2 diverges
above1e20 final adjoint residual. A fresh ordinary static two-rank replay on the
exact saved cycle2 grid/flow reproduces all131printed residual/sensitivity rows
within1e-6log10,one residual print unit; printed sensitivities agree exactly.
Density/internal energy and flow/grid pairing are valid. The observed failure
therefore does not require in-process mesh replacement; no convergence proof
follows. Evidence goal_cold_failure_classification_v1.json and static diagnostic.

Warm4 final adjoint log10RMS1.54155 grows; cold4 -2.68074. Neither final primal/
adjoint is converged. These are lifecycle/finite-summary passes, not validated
estimators or sensitivities. All four own-descendant SIGTERM tests(primal and
adjoint recording on1/2ranks) collectively stop cleanly, save checkpoints and
perform no remesh. Actual native-in-AD rejection2 has exact diagnostic before
first primal solve. Evidence goal_runtime_v7.json and goal_remaining_v1.json.

## Limits, failures retained and reproduction

Native supports static single-zone primal-double2Dtriangles. Immutable reference
means the original marker polyline stays fixed as geometry, while its mesh samples
adapt. Native3D,CAD,mixed volumes,periodic,moving,time-domain and native adjoint
paths remain unsupported. MMG/TWO_PASS and Stage G are separate supported controls.

An inherited three-cell/six-vertex CFD fixture aborts at4ranks with an empty sparse
pattern; the expanded45vertex/48cell mixed-order fixture passes. Harness count,
boundary-replication,coloring,symmetry/reference-name and build failures were fixed
and saved. The first production config rejected PARAVIEW_BINARY before solving;
this branch's correct binaryXML token is PARAVIEW. Failed old supervisors were
not restarted; independent validation resumed only after failure classification.
The final closure stopped after a successful one-rank TWO_PASS rejection because
its Python helper omitted return row. The one-line helper fix is committed; the
remaining actual2/4 negatives passed in native_twopass_rejection_v2. Both accepted
partial meshes and all9phase/field checks were completed before that bookkeeping
exception. native_closure_failure_classification_v1.json preserves this distinction.
All exact native+TWO_PASS negatives exit1 before the first CFD cycle.

One own heavy job at a time,MPI<=4,build-j2,OMP/OPENBLAS1; defer to foreign jobs.
Host is8-coreIntelCorei7-9700(noSMT),shared workstation; initial load5.63/4.53/4.72.
InstalledOpenMPI defaults,no explicit binding override. Source/binary/input/tool
hashes and exact commands live in each evidence folder; executed source coverage
must accompany results, not substitute for compiled scope/runtime.

Reproduce with the existing tools and a NEW label, preserving prior files:
run_native_driver_checks.py for MPI filters; run_native_robustness_campaign_v8.py
for declared engine/demand axes; run_native_bl_audits.py/run_native_airfoil_audits.py
for frozen-field/reference snapshots; run_integration_capacity_v2.py and
run_integrated_goal_remaining.py for follow-through; run_native_production_checks_v3.py
for actual outputs. Read their --help/source and recorded commands before running;
some campaign wrappers intentionally refuse existing state files.

Transaction defaults2MiBwork/256donors/128discovery/64cache are explicit caps,
not whole-process bounds. Original reference/master-reader boundary rows areO(B).
The strip enlarges a fixedny4 rectangle,B grows withN; no fixed-domain general2D/
3D scaling law is established. Encoded payload includes self buckets and init;
World counters omit direct CPassiveComm and failure votes. Phase maxima are not
additive,transfer is inside replacement,solve includes inneroutput; snapshot
I/O/startup/initial reference preparation must not be attributed to remeshing.
