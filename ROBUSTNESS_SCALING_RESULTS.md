# Integrated robustness and scaling results — in progress

Updated 2026-10-06. Goal active. Branch `codex/native-integrated`, sibling worktree
`/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`. AdapNoExt remains at
`88828474db587652ee1b0c09010de838ddbcfd06` with its original `.gitignore` change.

Source reconciliation is complete for the pins in BRANCH_RECONCILIATION.md.
The practical envelope is still being measured; the following are verified results.

## Integrated gates

| Gate | MPI ranks | Result |
|---|---|---|
| Fresh primal MPI/MMG/CGNS build, assertions on | — | PASS |
| SU2/binary SU2/CGNS output, mixed order and marker names | 1,2,4 | 7 cases per rank, PASS |
| Native geometry/field/BL/adaptation/output focused matrix | 1,2,4 | 49 cases per rank, PASS |
| Native transaction memory controls | 1,2,4 | PASS |
| Default MMG remesh/transfer/resumed-flow controls | 1,2,4 | PASS |
| Independent saved mesh/target/CGNS output checks | 1,2,4 | 84 audits, PASS |
| Goal metric/custom sensors/two-pass/reference/adjoint transfer | 1,2,4 | 58 cases per rank, PASS |
| Independent constant-target pilot geometry/topology audits | 1,2,4 | 12 cases, PASS |
| Fresh AD build | — | PASS, executables archived with hashes |
| AD GoalSwap controls, corrected MPI halo seeds | 1,2,4 | 3 cases/26 assertions per rank, PASS |

Evidence: integration_evidence/integrated_output_controls_v6,
integrated_primal_controls_v6, integrated_primal_memory_v6, integrated_mmg_default_v6,
integrated_primal_controls_v6/independent_outputs_v6, integrated_auxiliary_v9,
and scaling_pilot_v6/independent_audit.json. Each primal runner archives its
actual executable and source/config hashes; original failed runs remain intact.

## Initial engine pilot

Constant frozen tensor, aspect ratio10, opposing walls with exact h0=.004,
adaptive physical boundary sampling, cyclic ownership. One fresh run per case;
times exclude the test-only audit gather and CFD/transfer/output. All12 cases
meet q>=.18, metric length<=1.8, wall-altitude error<=1e-8 and reference
deviation<=1e-10, and pass independent exact orientation/topology/area checks.

| Input cells | Ranks | Output cells | Adapt seconds | Selection seconds max | Collective seconds max | Cross-owner / commits | Conflicts | Peak RSS KiB/rank |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 16 | 1 | 14 | 0.033381 | 0.000301 | 0.002133 | 0 / 42 | 0 | 20324 |
| 16 | 2 | 14 | 0.040145 | 0.000204 | 0.009440 | 11 / 24 | 129 | 20716 |
| 16 | 4 | 14 | 0.124596 | 0.000208 | 0.046673 | 17 / 24 | 423 | 20720 |
| 64 | 1 | 50 | 0.136422 | 0.002244 | 0.007004 | 0 / 168 | 0 | 20496 |
| 64 | 2 | 46 | 0.178915 | 0.001505 | 0.029264 | 99 / 155 | 270 | 20636 |
| 64 | 4 | 56 | 0.307765 | 0.001338 | 0.101928 | 154 / 197 | 656 | 20788 |
| 256 | 1 | 222 | 0.719293 | 0.021229 | 0.029651 | 0 / 700 | 0 | 20384 |
| 256 | 2 | 200 | 0.773803 | 0.014022 | 0.103090 | 678 / 829 | 407 | 20536 |
| 256 | 4 | 198 | 0.769291 | 0.009674 | 0.232830 | 626 / 661 | 1065 | 20936 |
| 1024 | 1 | 766 | 3.443461 | 0.099079 | 0.066021 | 0 / 2145 | 0 | 21508 |
| 1024 | 2 | 772 | 3.514976 | 0.073940 | 0.269569 | 2086 / 2646 | 282 | 21036 |
| 1024 | 4 | 764 | 3.326471 | 0.051029 | 0.639058 | 2898 / 3079 | 1277 | 21172 |

At1024 input cells, the observed1/2/4-rank times are3.44/3.52/3.33seconds.
This pilot shows little benefit from additional ranks at this size. It is not
a reliable speedup estimate: single samples, different output cell/transaction
counts, and adversarial ownership. In the four-rank1024-cell case,2898 of3079
commits cross ownership, with1277 conflicts. Repeated low-cut geometric
ownership controls will distinguish this effect from unavoidable collective cost.

Whole-process VmHWM includes startup and the replicated manufactured input.
It does not describe production CFD memory or prove the2MiB transaction-work
bound. Instrumented World counters do not include every MPI-library operation.

## Demonstrated constraints and pending limits

- Native supports static, single-zone, primal-double2D triangles. Native3D,
  CAD projection, mixed volumes, moving/time-domain meshes and native adjoint
  adaptation remain unsupported. These are capability limits, not benchmark failures.
- Geometry follows the immutable original marker polyline while its sampling
  adapts. The tests do not establish CAD conformance or3D prism-layer generation.
- Explicit defaults:2MiB transaction work,256 donor cells,128 discovery regions,
 64 cached candidates. Original boundary/reference and reader master boundary
 rows remain O(B). Larger-volume work and communication costs still require measurement.
- A three-cell/six-vertex mixed CFD fixture aborts at four ranks with an empty
 sparse pattern. Expanding its unchanged interleaving motif makes the writer
 regression pass. This is an observed too-small CFD partition limitation.
- MPI test-harness defects found and repaired: serial mesh counts, replicated
 SU2 boundary rows, omitted box coloring, rank-local symmetry/wall coverage,
 and colliding reference-file names. Saved failures are evidence of those
 defects, not native mesher construction failures.

Remaining campaign: escalate engine size with vertical geometric strips;
three repeated samples per rank for cyclic, horizontal contiguous and vertical
ownership; matched AR10/100/1000 and a separately proved incompatible AR25
wall/metric request; fresh actual NACA conservative transfer and resumed
viscous solves with thinner h0/higher complexity; opposing-wall audits; AD
warm/cold lifecycle and single-rank interruption propagation. See
ROBUSTNESS_SCALING_PROTOCOL.md and live JSON states in HANDOFF_Codex.md.

Do not call the goal complete or publish a large-rank/3D scaling claim from this pilot.

## MPI adjoint mesh-swap defect found during integration validation

Original GoalSwap v9 passed1 rank but failed2. Owned primal/adjoint transfer and
objective were exact, yet continuation residual discrepancy was3.2863 and final
adjoint relative difference.999103. Tight Jacobi solves reproduced the failure
with unchanged point ownership and recorded primal linear residual8.2855e-14,
ruling out the initial incomplete-ILU hypothesis. The transfer utility supplies
owner copies on halos, but the DA driver seeds every recorded primal output;
its recorded halo exchange then duplicates those contributions. Clearing only
halo adjoint seeds after SwapMesh retained owned warm starts and restored exact
continuation in the2-rank v13 diagnostic. Original ILU-settings regression at
1/2/4 passed as v14: all3 cases/26 assertions per rank; same-mesh transfer,
objective, per-iteration residuals and final adjoint differences are exactly0.
All measured point ownerships are unchanged in this continuation fixture.
This establishes the correction, not partition-independent iteration histories. Production correction is limited to DA mesh swap;
primal interpolation and its halo field semantics are unchanged.

Measurement scope clarification: World traffic/exchange-work/collective counters
include engine initialization and adaptation, while adapt_seconds excludes
initialization and selection_seconds_max measures adaptation selection only.
The maximum exchange-work counter can include the initial incidence-directory
transport, which uses no transaction ceiling; a value above2MiB is not itself
an admitted transaction violation. The probe enlarges a tiled rectangular strip
at fixed ny=4 and constant target, so boundary size grows with input cells. It
measures that workload, not a fixed-domain general2D/3D scaling law. Reference
construction and the SU2 import/transfer/CFD path are outside its timing.

## First low-cut size group — independently verified

At2048 input cells, all1/2/4-rank vertical-strip cases are complete and pass
independent exact geometry/topology/reference/target audits (large_t128).

| Ranks | Input cut edges | Output cells | Adapt seconds | Cross-owner / commits | Conflicts | Peak RSS KiB/rank |
|---:|---:|---:|---:|---:|---:|---:|
| 1 | 0 | 1532 | 9.453158 | 0 / 4220 | 0 | 22660 |
| 2 | 4 | 1548 | 6.403417 | 48 / 4360 | 3 | 22292 |
| 4 | 12 | 1512 | 4.797245 | 204 / 5316 | 28 | 21876 |

These are single samples with different adapted cell/transaction counts. They
show a useful low-cut control; repeated partition comparisons remain pending. Raw evidence:
integration_evidence/robustness_campaign_v8/large_t128/independent_audit.json.

The4096-cell vertical-strip group also passes1/2/4 independently (large_t256):

| Ranks | Input cut edges | Output cells | Adapt seconds | Cross-owner / commits | Conflicts | Peak RSS KiB/rank |
|---:|---:|---:|---:|---:|---:|---:|
| 1 | 0 | 2948 | 28.019265 | 0 / 8403 | 0 | 26232 |
| 2 | 4 | 2883 | 15.194881 | 58 / 7932 | 10 | 23792 |
| 4 | 12 | 2957 | 9.204581 | 175 / 8534 | 11 | 22984 |

No transaction-memory rejection occurred in these six larger cases. The first
failure/cost boundary is reported below; repeated comparisons remain pending.

The8192-cell vertical-strip group passes1/2/4 independently (large_t512):

| Ranks | Input cut edges | Output cells | Adapt seconds | Cross-owner / commits | Conflicts | Peak RSS KiB/rank |
|---:|---:|---:|---:|---:|---:|---:|
| 1 | 0 | 5808 | 98.935874 | 0 / 16714 | 0 | 31928 |
| 2 | 4 | 5834 | 52.893111 | 48 / 16622 | 6 | 27236 |
| 4 | 12 | 5766 | 28.137930 | 163 / 16012 | 11 | 25636 |

This is the largest independently verified rank-common strip size so far.
The16384-cell1-rank job exceeded the declared240-second cost budget, as reported
below. No construction failure is inferred from that timeout. At8192 cells the serial
selection time is7.158s and World collective time is.632s (init+adapt), compared
with98.936s adaptation. Those counters do not account for every cost, and do
not profile individual geometry/transaction routines. Source inspection shows
full component scans in PolylineReference::Parameter and ::Deviation and an
owned-cell scan for boundary CollisionVeto. These are concrete candidates for
profiling; their measured fraction is not yet established. No scheduler or
geometry validation change has been made to improve the benchmark.

The real-airfoil fixture uses10216 seed triangles and three short primal
iterations per cycle before producing each actual sensor/BL field. Its resumed
viscous run tests lifecycle/admissibility and transfer, not converged CFD accuracy.

## First observed cost boundary

large_t1024_p1 (16384 input triangles, vertical-strip layout) exceeded the
240-second whole-job budget; the supervisor returned124 after241.41s including
termination. No finished native_scaling.json was produced. This is a measured
budget limit for that serial strip workload, not proof of infeasibility or of
invalid partial topology. Larger2/4-rank limits remain unmeasured; follow-through
is prescribed in the protocol. Repeated comparisons use the8192-cell largest
complete common size. Encoded payload counts include self buckets, and World
collective counters omit some direct MPI calls/votes; do not interpret them as
network traffic or a complete communication-time decomposition.


## Repeated cyclic ownership — independently verified interim result

All nine layout1 repetitions at8192 input triangles pass independent exact
orientation/topology/perimeter/area and frozen metric/height/reference checks.
The auditor used immutable copies of only finished cases, leaving the live
campaign group and its scheduled final audit untouched. Evidence:
integration_evidence/partition_layout1_audit_v2/independent_audit.json.
Raw statistical snapshot: integration_evidence/partition_progress_summary_v1.json.

| Ranks | Samples | Adapt seconds min / median / max | Output cells each | Cross-owner / commits each | Conflicts each |
|---:|---:|---:|---:|---:|---:|
| 1 | 3 | 97.988 / 99.516 / 101.773 | 5808 | 0 / 16714 | 0 |
| 2 | 3 | 94.756 / 95.533 / 115.950 | 5804 | 12806 / 15699 | 346 |
| 4 | 3 | 52.242 / 52.513 / 53.654 | 5750 | 12172 / 12954 | 2377 |

Within each rank/layout, these repetitions have identical output counts and
transaction counts. Cyclic ownership is materially more expensive than the
single vertical low-cut sample at2/4 ranks (52.893/28.138s); repeated low-cut
controls are required before quantifying the comparison. Rank-dependent output
and transaction counts still preclude an identical-work speedup claim.
No memory admissions were rejected in these nine runs.

The failed partition_layout1_audit_v1 contains zero cases: the preliminary copy
command used a Python method unavailable in installed Python3.8. It never
examined a mesh. The corrected v2 copied and audited all nine cases successfully.
Do not classify v1 as a mesher failure.

Host observation during the next horizontal-strip repetition also found a
foreign two-rank periodic SU2_CFD_AD job running alongside our one-rank probe.
Our supervisors start each heavy job only after the observed machine is quiet;
this cannot prevent another user starting a job afterward. No foreign process
was stopped. All reported timings remain shared-workstation observations.

Measurement gap to close before completion: actual driver logs report
ReplaceMesh/solution-transfer times on the master rank, without an all-rank MAX.
Native phase logs report per-phase rank maxima, which cannot be summed into an
exact remesh wall-clock maximum. Keep those diagnostics distinct from the
engine probe's measured wall maximum. The final report still needs actual
remesh and transfer/repartition phase measurements meeting the protocol scope;
add only the necessary test instrumentation after live binary-pinned campaigns
finish, then run a separately labelled control rather than changing a binary
under an active campaign.


A minimal test-only phase-timing patch is prepared and passes git apply --check
(integration_evidence/pending_airfoil_phase_timing_v1.patch). It is deliberately
not applied while binary-pinned campaigns run. Its planned rank maxima include
actual backend import/adaptation/export in remeshing; replacement covers new
partition/geometry/solvers and teardown, with its transfer subset reported
separately; actual adapted-mesh output uses the production writer. Solve timing
includes existing inner-iteration output. None of these planned measurements
is a result yet. Compilation, fresh1/2/4 runtime, nine CSV validation and the
same independent mesh/target audits remain required before closing this gap.


Coverage audit also found abrupt spatial nodal target changes untested by the
executed smooth/affine/constant fixtures. The prepared hidden NativeBLStep2D
fixture reuses the existing BL cycles and transfer policies with an abrupt
.004/.014 nodal tangential request. Compilation, actual1/2/4 controls and
independent frozen-P1 audits are pending. This gap is separate from matched
constant AR robustness and from changing the smooth metric between cycles.


Source coverage check: executed_source_coverage_v1.json compares current source
with the archived primal v6 manifest and corrected AD v14 manifest. The only
primal-manifest differences are the freshly validated adjoint-driver correction
and the rebuilt opt-in scaling probe. All759 AD-manifest production files match.
The pending timing/step patches remain unapplied, so they have not changed the
source associated with these gates. Actual AD lifecycle, stress/size follow-through,
measurement closure and explicit expected support failures remain incomplete;
see the interim completion matrix in HANDOFF_Codex.md.


## Repeated horizontal ownership — independently verified interim result

All nine layout2 repetitions at8192 input triangles independently pass structure
and the complete frozen target. Evidence:
integration_evidence/partition_layout2_audit_v1/independent_audit.json.
The immutable snapshots' hashes match the original finished meshes/metadata.
Raw snapshot aggregation: integration_evidence/partition_progress_summary_v2.json.

| Ranks | Initial cut edges | Adapt seconds min / median / max | Output cells each | Cross-owner / commits each | Conflicts each |
|---:|---:|---:|---:|---:|---:|
| 1 | 0 | 104.948 / 106.234 / 108.123 | 5808 | 0 / 16714 | 0 |
| 2 | 1024 | 52.268 / 52.416 / 52.755 | 5794 | 4355 / 15883 | 1043 |
| 4 | 3072 | 53.843 / 53.982 / 54.318 | 5785 | 11418 / 14019 | 1329 |

Within each horizontal rank group, points/cells/faces CSV hashes are identical
across all three repetitions, as are the work counts. No memory admission was
rejected. Four ranks give no measured improvement over two for this layout,
despite the same fixed input/target/acceptance contract. This establishes a
practical partition-sensitive limitation of this strip workload; it does not
isolate scheduler, collision checks, transfer or communication as its cause.
Cyclic two-rank median95.533s contrasts with horizontal52.416s, while four-rank
medians are52.513/53.982s. The repeated vertical low-cut group remains pending.
These comparisons use rank-dependent adapted meshes and transaction counts;
shared-machine timing and counter scope caveats still apply. Do not infer a
universal choice of MPI rank count or a general speedup law from them.


Upcoming actual-airfoil/opposing audit reports now include SHA256 pins for the
exact donor, metric and candidate inputs (plus original reference for NACA),
and reject changes during evaluation. The shared check passes a saved integrated
four-rank cycle7 BL mesh and detects a deliberate edit of only a temporary copy.
This strengthens report provenance without changing any target, algorithm or
binary used by the live campaign. Earlier archived audits are preserved.


## Completed three-layout repetitions

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
Engine timing excludes CFD, transfer and audit. This supersedes the pending
vertical/18-of27 statements in earlier interim sections; larger capacity and
real-airfoil measurement closure remain pending.

Actual prior native-baseline NACA meshes have been exported for inspection in
`real_airfoil_gallery_v1` (see GRID_GUIDE.md). They include three four-rank
adaptations, boundary-layer cells and short resumed viscous solves, but do not
establish full CFD convergence or current integrated-tree runtime success.


## Matched anisotropy and conflicting BL demand

All nine64-input-triangle affine-matched controls (aspect ratios10/100/1000,
MPI1/2/4) completed and independently pass exact orientation, topology, boundary,
area and full constant-target criteria. Audit and current raw input hashes were
rechecked: `robustness_campaign_v8/affine/independent_audit.json`.
The normal geometry and prescribed h0 scale with the tensor, so the metric
first height stays1. Quality minima range0.40924–0.65553 and maximum metric edges
1.26410–1.57188. This is numerical robustness under affine scaling of a small
straight-wall control, not a1000-aspect-ratio curved-airfoil capacity proof.

All three fixed-geometry AR25 controls report incomplete and independently
retain valid mesh structure, perimeter and exact prescribed first height:
`robustness_campaign_v8/incompatible/independent_audit.json`. Here metric first
height is2.5, exceeding the edge limit1.8. Any incident edge reaching the apex
has metric length at least2.5, so the requested height and maximum edge length
cannot both hold. Audited maximum length2.53179778 and minimum quality0.21890749
agree with the engine; no admission-memory rejection occurred. This is an
explicit conflicting request, not a deadlock or a relaxed acceptance threshold.
The engine-only probe does not install candidates into CFD; accepted driver
state retention is exercised separately by integrated failure controls.

Fresh integrated NACA three-cycle runtime passed1 and2 ranks (whole-job316.525
and180.644s);4 ranks are running at the last observation. Serial adapted cell
counts23368/25322/19502. These include CFD, test snapshots and checks, so are not
isolated remeshing costs. Independent original-reference/frozen-P1 audits remain
pending until the baseline runner completes and archives its executable.

Measurement-gap closure now uses prepared **pending_airfoil_phase_timing_v2**,
superseding v1 but retaining both files. It adds local phase rows, cumulative
rank Linux VmHWM and owned/total CFD point counts plus overlapping local element
counts. This is whole-process memory including startup and earlier snapshots,
not remesher-only allocation. `check_airfoil_phase_timing.py --self-check` passed;
actual compilation/runtime, MPI-max-to-local consistency and ownership checks
have not run. No live source or pinned executable changed.


## Ownership, process memory and encoded payload at8192 input cells

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
2MiB admitted-transaction breach. Per-rank actual CFD memory/durations are still
pending the prepared timing fixture. Exact inputs/aggregation:
`integration_evidence/partition_work_memory_summary_v1.json`.


## Fresh integrated actual-sensor NACA baseline verified

Runtime and independent nine-mesh audit now complete successfully on1/2/4 ranks:
`integrated_airfoil_baseline_v8/evidence.json` and `independent_v8/evidence.json`.
Every report's exact donor/metric/candidate/original-reference input hashes were
rechecked against current files. Boundary sampling changes while the immutable
original polyline, connected components, declared features and leading extremum
remain retained within the original reference criteria. Four-rank wall faces
change200→204→209→215; this is adaptive boundary sampling, not a fixed boundary.

| Ranks | Adapted triangles across cycles | Min quality across cycles | Max metric edge | Max relative first-height error | Max corrected transfer defect |
|---:|---|---:|---:|---:|---:|
| 1 | 23368 / 25322 / 19502 | 0.244952 | 1.79998224 | 1.64e-13 | 2.03e-16 |
| 2 | 23282 / 25289 / 19575 | 0.225805 | 1.79995807 | 1.64e-13 | 7.50e-16 |
| 4 | 23328 / 25549 / 19487 | 0.227559 | 1.79999999 | 1.64e-13 | 2.03e-16 |

Transfer defects come from saved conservative projection summaries and runtime
assertions (corrected target includes open-boundary fill/sliver accounting), not
an independent integration of saved nodal fields. This fixture saved no nodal
flow or restart files: fields were in memory and its Run/Postprocess/Update path
did not call Output. Initial/donor/adapted meshes and metric CSVs are present;
CASE_FILES.md and GRID_GUIDE.md give exact locations. Fresh VTU gallery is
`integrated_airfoil_gallery_v8`, reusing the exporter with explicit source/audit
arguments and exact input-pin/VTU round-trip checks. Preview visually inspected.

Prepared timing patch **v3 supersedes v2/v1** and additionally gathers actual
conserved nodal flow before each remesh and immediately after transfer, paired
with donor/adapted global point IDs and coordinates. No production/test source
changed; v3 is not compiled or executed. Existing timing checker now also checks
field identity/coordinates, finite values and positive density/internal energy;
its minimal synthetic self-check passes, not an actual field-export runtime.
Neither baseline nor pending exports establish converged CFD accuracy.


## Real-airfoil serial cost boundary: larger refinement then coarsening

The4k/12k/3k, h0=0.0002-chord serial demand run ended with timeout124 at
902.433s under its900-second whole-job budget. MPI/rank processes were verified
terminated; this was not an observation timeout and no job was restarted.
It completed cycle0 (23368 triangles) and cycle1 (40968 triangles/20617 points),
then timed out during cycle2 remeshing from40968 triangles. Thus refinement to
40968 cells was accepted; the observed limit is completing this subsequent
coarsening lifecycle within the whole-job budget, not producing that cell count.
No cycle2 accepted/rejected mesh was written; donor and frozen metric remain.

Exact provenance and saved transfer defects:
`integration_evidence/airfoil_complexity_cost_limit_v1.json`. The first cycle's
donor/metric/candidate/original inputs are byte-identical to the successfully
executed baseline np1-cycle0 independent audit; all four hashes were compared.
This reuses that numerical proof for identical inputs, not a newly executed
audit. The40968-cell cycle1 still needs its own frozen-P1/original-reference
audit before a full independent target claim. Reported replacement/transfer
costs0.245765/0.129686s and0.561981/0.353944s are serial driver log values; there
is no isolated remeshing duration or per-rank phase matrix for this failed run.
No infeasibility, memory admission or MPI deadlock was demonstrated.


## Thinner-wall serial cost boundary

The h0/HMIN=1e-4-chord,4k/6k/3k demand run timed out124 at902.055s under its
900-second whole-job budget. It accepted the first adapted mesh with38852
triangles/19563 points, then timed out during cycle1 from that mesh. Cycle1
incoming frozen-target residuals were8382 shape and15685 edge-length violations;
no cycle1 accepted/rejected candidate exists. These are input-to-remeshing counts,
not a final incomplete outcome or proof of impossible demand. The completed first
mesh still requires an independent actual-target/reference audit at h0=1e-4.
Exact snapshots/runtime pins: integration_evidence/airfoil_height_cost_limit_v1.json.
The5e-5 height axis was not executed under the predeclared serial failure stop.
This contrasts with complete0.0002-chord baseline lifecycles on1/2/4, but does
not establish higher-rank thinner-wall capacity or an isolated adaptation cost.

Actual native production-loop/restart/ParaView/metric validation is now queued
behind all current robustness/goal/follow-through prerequisites via
run_native_production_checks_v1.py (supervisor416452). The restart-to-mesh mapping
self-check passes; all759 production source hashes still match the executed
manifest. Prepared np1/np2/np4 folders contain input.su2/run.cfg; no simulation
has started in them yet. This runner freezes its loaded script/field-checker
hashes, archives the application and readers, and independently audits nine
actual double-restart-derived P1 targets after successful runtimes. It is an
additional integration/output gate, not a replacement for pending phase/step
instrumentation or full CFD convergence validation.
