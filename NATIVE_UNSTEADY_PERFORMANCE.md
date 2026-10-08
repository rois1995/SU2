# Native unsteady adaptation and performance checkpoint

2026-10-07; branch `codex/native-unsteady-performance`, based on metric merge
`2abbd11769fc57f1862432b03bbd02202bb10283`.
Working tree: `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`.
Cases and raw evidence: `integration_evidence/native_unsteady_performance_v1/`.
AdapNoExt and the completed integration branches are untouched.

## Implementation

Static primal 2D triangular native adaptation now admits `WINDOW_AVERAGE` with
BDF1/2. It reuses the existing dual-time driver, distributed conservative or
barycentric solution/history transfer, geometry replacement and SU2/CGNS writers.
Predicted/fixed-point windows, native 3D, moving geometry, multizone and derivative
builds remain outside the supported scope. Mesh boundary sampling still adapts.

Original donor interpolation remains sensor-only and P1 on original connectivity.
Every computed candidate tensor receives the same original geometric BL
composition, including private/remote candidates. Sensor demands, main's fade,
noise=0 and gradation policy are unchanged. The composed-field transport audit is
still a separate diagnostic; sensor gradation is not its certificate.

The performance change replaces the saturated cache's refusal to retain further
queries with bounded FIFO replacement of computed samples. Unchanged-vertex
samples remain authoritative and never enter the eviction queue. The limit stays
2048 entries; admission uses128 bytes per entry to account conservatively for the
queue. One final packed reduction reports private requests/evaluations/evictions;
there are no new per-query MPI operations. A saturation regression checks recent
query retention, exact authoritative samples and the bound.

With `WRT_ADAP_MESH=YES`, existing requested output formats are forced at native
window ends. This saves the actual donor solution and frozen sensor metric even
when normal volume output is sparse. `VOLUME_OUTPUT_PRECISION=DOUBLE` is an
existing option; the capability reader now correctly reports its Float64 output.
The trial runner records the actual configured mesh filename, including CGNS.

## Interpolation and remeshing profile

Matched real RAE cases start from the same user-supplied coarse Euler mesh and
construct the same first BL at composed budget10000, h0=1e-5, noise=0, MPI4.
CPU-clock sampling uses49 Hz and8 KiB dwarf stacks. Both runs have sampling overhead;
phase timers provide wall costs. Inline decoding is disabled because installed
system debug information produced malformed-section warnings. Raw data, no-inline
stacks and receipts are retained; no machine perf settings were changed.

| Measurement | Original merged baseline | Bounded FIFO |
| --- | ---: | ---: |
| End-to-end elapsed |176.610s|138.878s|
| Native action time sum |107.281s|69.547s|
| Coordinated repair,97commits |64.118s|26.759s|
| Bulk split phase |87.752s|50.497s|
| Full adaptation, including metric/replacement/output |108.716s|70.9245s|
| CFD solve time, both meshes |67.4456s|67.4883s|

Native action time falls35.2%; full adaptation falls34.8%; end-to-end falls21.4%.
These are observations from one matched case, not a general speedup guarantee.
Both runs make46289commits, with identical per-operation counts, conflicts,
encoded transport bytes and zero residual/height/reference failures. Adapted mesh,
donor restart and final restart are byte-identical. Reusing the frozen query
results changes cost without changing the generated grid or continued flow.

FIFO run:132585546 private requests,27775647 evaluations,12045836 dynamic evictions.
The baseline's cache had stopped retaining new samples after filling up, causing
repeated expensive reconstruction queries. Transfer is already cheap here:
baseline first conservative projection is about0.044s, versus108.7sadaptation.
Current/history fields share donor search and supermesh/stencils. Profiling does
not justify changing their arithmetic or weakening their checks.

Before:34421solver samples; native21009, metric3875, CFD9494, transfer9.
After:27049samples; native13606, metric4071, CFD9335, transfer5. Native samples
containing MPI frames fall13986 ->8526; MPI still occupies about63% of sampled
native CPU. This includes waiting/spin and must not be called communication wall
cost. The initial root-only classifier missed truncated private callbacks; its
superseded result is preserved separately.

An original one-rank RAE comparison completes in247.781s. Its native action sum
is83.717s, lower than original MPI4's107.281s, but CFD is slower. Different
partition trajectories produce different grids, so this is a practical scaling
observation rather than a fixed-work parallel efficiency measurement. The new
FIFO has not yet received the matched1/2/4 real-RAE scaling comparison.

## Actual unsteady cases and inspectable artifacts

All folders below are under the evidence root above. Meshes are real adapted
SU2/CGNS files; VTUs and binary solution restarts are beside them.

- `vortex_bdf{1,2}_np{1,2,4}_v2`: each executes9steps, two adaptations and CFD
  continuation. Independent frozen-original-P1 q/Simpson-length, positive state,
  current/history conservation and mesh/restart pairing audits pass all six.
  These large-domain coarse grids are lifecycle controls, not vortex accuracy
  demonstrations.
- `plate_{sa,sst}_bdf2_np4_v1`: uniform coarse meshes, no initial BL, pressure
  sensor, h0=.005, growth1.2, thickness.15. Two actual accepted BL adaptations
  with resumed viscous/turbulence updates. Independent original sensor plus
  straight-wall BL composition, q/length and current/history audits pass. Maximum
  saved first-altitude relative error is below4.45e-16. This is startup/lifecycle
  evidence, not converged skin-friction or turbulence accuracy validation.
- `plate_sa_bdf1_np{1,2,4}_v2` and `plate_sa_bdf2_np{1,2}_v2`: post-optimization
  MPI/time-order controls. Audit results are in each completed folder. SST
  time-domain runtime coverage currently remains BDF2/MPI4.
- `plate_euler_from_bl_np4_v2`: starts from the accepted SA BL mesh, requests
  pressure-only Euler adaptation with no BL marker or height prescription, then
  continues two windows. Euler does not receive a BL metric.
- `vortex_bdf2_cgns_np4_v1`: native time-domain adapted CGNS meshes and ordinary
  restarts; independent mesh/target/history audit passes.
- `vortex_restart_bdf{1,2}_np{1,2,4}_v1`: complete-window restarts give exactly
  the uninterrupted conservative state. `vortex_restart_cgns_np4_v1` differs
  only by8.89e-16. Changed-rank and barycentric follow-up controls have their own
  receipts and diagnostics.

For a mesh named `mesh_00006.su2` or `.cgns`, pair `solution_00005.dat` and, for
BDF2, `solution_00004.dat` with it. These histories were rewritten after transfer.
`flow_00005.vtu` stays on `mesh_00003`; it is the original donor snapshot. The
native reference sidecar follows its mesh. Mid-window restarts do not persist the
window Hessian accumulator. Pre-first-adaptation reference checkpoint creation is
not established by these tests; neither case is claimed equivalent here.

## Real RAE window adaptation

The supplied Euler-type RAE mesh is the RANS input. The real unsteady SA case
`rae_rans_window200_np4_v1` runs BDF2/WINDOW_AVERAGE at dt1e-4 for600steps,
200-step windows,50inner iterations, fixedCFL10, noise=0 and budget10000.
Both adaptations are accepted; CFD continues on12252 and13386points. Independent
original-P1/geometric-BL audits give qmin.18456/.22356, Lmax1.80000/1.79993 and
first-altitude relative errors below3.29e-12. Boundary sampling adapts while the
original reference and declared features remain preserved.

`rae_euler_from_bl_window100_np4_v1` starts on that accepted13386-point RANS BL
mesh, requests no BL field, runs300steps with100-step windows,30inner iterations,
budget6000, and reaches7964/7233points. Independent qmin.23253/.56149 and
Lmax1.78227/1.78706 pass. The second donor contains the developed upper shock;
saved Mach/grid previews show the changing flow and grid. Both case folders
contain actual meshes, restarts, donor VTUs and `mesh_mach_windows.png`.

| Total reported phase cost | RANS from Euler mesh | Euler from RANS BL mesh |
| --- | ---: | ---: |
| CFD |393.974s|93.1675s|
| Metric sampling and construction |37.3825s|4.43258s|
| Remeshing |65.7099s|33.5927s|
| Geometry/solver replacement |.372054s|.226021s|
| Solution/history transfer, included in replacement |.192073s|.138122s|
| Adapted mesh and rewritten restart output |.0541746s|.0297344s|
| Adaptation-related work / CFD |26.28%|41.09%|
| End-to-end elapsed |497.985s|132.013s|

The costly RANS startup BL construction is54.507s versus20.993sCFD in its first
window. The next remesh is11.203s versus182.771sCFD. Averages must not hide that
initial cost. Final residuals and force coefficients represent developing flow;
these startup trials do not certify inner convergence, temporal accuracy or
converged aerodynamics. Configured inner limits are frequently reached.

An initial audit incorrectly required unchanged whole-domain totals while
resampling the curved open farfield. With the default conservative `CLOSED`
sliver policy, walls/symmetry retain content, while open-boundary totals follow
the changed polygonal domain. Farfield area changes of-4.66e-6 and-1.66e-5 account
for apparent relative integral changes1.49e-10/5.29e-10. Independent correction
using the near-constant farfield state reduces the policy residual below5.3e-13,
with the same1e-10 acceptance threshold. The initial failure, diagnosis, raw
global drift and corrected policy check are preserved. This particular correction
requires nearly constant farfield data; it is not a general sliver audit. Sparse
previous-history donor snapshots were not available, so those real-RAE states
have positivity/restart-pairing checks, without an independent integral claim.

## Full BDF2 checkpoint diagnostics

The real RANS histories exposed stale primitive fields: `WriteTimeHistoryRestarts`
swapped conservative/turbulence history states for output without recomputing
pressure, temperature or Mach. Their maximum pressure disagreement reached.56%.
Transferred primary histories and previously tested restart continuation were
correct. The shared writer now reuses existing output-mode flow preprocessing
and turbulence postprocessing before writing the previous history and after
restoring the current state. Compact restarts skip this work. Saved Hessian/metric
and wall-force diagnostics are not reconstructed for the previous history and
are excluded from the primitive consistency claim.

Final-source V3 Euler/SA/SST BDF2 MPI4 cases complete two adaptations each. Actual
rewritten pressure, temperature, Mach and velocity agree with conserved states
below4.5e-16 relative error, including SST turbulent kinetic energy. Their adapted
SU2 meshes and continued primary/turbulence fields are identical to V2. The
regression also detects the original defect; its expected rejection is retained.
A V3 full-checkpoint4->2rank restart passes continuation below1e-10. See
`check_unsteady_checkpoint_primitives.py`, per-case `checkpoint_primitive_audit.json`,
and the final restart receipt.

## Cost and accuracy limits

Resolved-vortex controls use1225initial points on a2x1domain, physical time.9,
300steps atdt.003 and100-step adaptation windows. FIFO run reaches752points:
CFD1.591s, metric.111s, remesh1.243s, replacement.0228s (transfer.0122s), adapted
output.00515s. Adaptation-related work is about87% of reported CFD work, including
ordinary output. The comparable fixed-grid run takes2.111s. This narrowly
satisfies adaptation cheaper than CFD for this control; it is not a comfortable
production budget. Small non-saturated patches show no cache benefit (zero FIFO
evictions); their timings show ordinary run-to-run variation and queue overhead.
The FIFO and original practical-cadence meshes/final states are byte-identical.

Exact-vortex density error at physical time.9, P1 lumped L2:3.840e-4adapted versus
3.364e-4fixed; Linf4.239e-3 versus4.477e-3. Adaptation uses fewer final points but
has a modest L2 accuracy penalty in this trial. No accuracy-efficiency advantage
or convergence order is established.

At three-step windows, MPI4 vortex remeshing is around.925s versus.093sCFD.
SA/SST startup remeshing is around1.697s versus.252/.291sCFD. These cadences are
unaffordable. The real RAE FIFO startup adaptation70.925s also slightly exceeds
its whole67.488sCFD campaign. A practical cost limit is now measured; these lifecycle checks do not establish
affordable cadence for arbitrary flows.

## Evidence and next work

Immutable app builds: `native_unsteady_build_v1`, `native_unsteady_build_v2`,
and final-source `native_unsteady_build_v3`.
V3 SHA256:ac36d778c1286abe67eb2dc775ba0f513479ee3da3e4a112feeeded86460833a.
V3 MPI1/2/4 combined validation passes45 cases per rank in
`native_unsteady_final_mpi_v3`.
V2 SHA256:3888c9067df010618b4f045ecb03a0c2d8c3805a925ad3dcf442913e7ea9e587.
760 production source pins accompany each app. `native_unsteady_history_v2` passes
history/support at MPI1/2/4. `native_unsteady_cache_mpi_v2` passes42 field/engine/
support/history/CGNS/rejected-output cases per rank. Initial history fixture v1
failed during config construction because its mandatory TIME_STEP was missing;
the failure and corrected result are preserved. New independent audit scripts
pin their inputs; frozen donors use original connectivity rather than a new
Delaunay triangulation.

The bounded static 2D lifecycle and measured-cost goal is complete. Production
affordability and accuracy remain case-dependent and the larger 2D/3D goal continues.
Next priority is same-target MPI scaling and communication/imbalance profiling,
then longer physical-time/inner-converged RAE accuracy and cadence controls. Separate remeshing partitions or
fewer active remeshing ranks remain a possible subsequent implementation, not an
existing runtime feature. Larger3D cavity work follows after 2D lifecycle and cost
limits are established. Preserve mesh gates and rejected evidence throughout.

## Same-target MPI follow-up

The next performance goal is active. Opt-in `NativeFrozenAirfoil2D` reuses the
production driver/backend with exact saved double sensor samples keyed by original
coordinates. CSV import and test artifact gathering are outside measured remeshing.
Both workloads are from the actual first unsteady RAE windows above; frozen
sensors, geometry and BL settings are identical across rank counts. The small
straight-wall BL smoke test and both real workloads pass MPI1/2/4. Every exported
real mesh also passes independent original-P1/composed-metric q/length and topology
checks. Both MPI4 meshes reproduce the real unsteady first adapted meshes exactly
(points and connectivity).

| Frozen workload | MPI ranks | Remesh wall time | Accepted points | Commits |
| --- | ---: | ---: | ---: | ---: |
| rae_euler_to_bl | 1 | 88.957s | 11285 | 56011 |
| rae_euler_to_bl | 2 | 79.947s | 11757 | 56028 |
| rae_euler_to_bl | 4 | 51.568s | 12252 | 44811 |
| rae_bl_to_euler | 1 | 62.066s | 7795 | 28623 |
| rae_bl_to_euler | 2 | 36.112s | 7778 | 28596 |
| rae_bl_to_euler | 4 | 23.800s | 7964 | 27814 |

These are same-target workload measurements, not fixed-work speedups: partitioning
changes reconstruction trajectories, output point counts and operation counts.
Raw cases/grids: `integration_evidence/native_frozen_rae_euler_to_bl_v1/runtime_np{1,2,4}`
and `native_frozen_rae_bl_to_euler_v1/runtime_np{1,2,4}`. Summary and fixture pins:
`native_frozen_scaling_v1/frozen_scaling_summary_v1.json`.

Saved-profile follow-up (`profile_native_stack_details.py --selftest` passes)
finds8526 native samples with visible MPI API frames,8035 in `PMPI_Allreduce`.
This includes waiting, utilities and truncated-stack limitations; it is not an
additive wall-cost measurement. Initial breakdown v1 misclassified a native type
in ComputeMetric's function arguments; that output is retained, corrected v2 is
authoritative and matches the earlier MPI sample count. Repo path names and
function argument types are tested explicitly by the classifier.

### Rejected communication trial and interpolation controls

Consolidating two message-preflight failure elections passed48core cases per
rank atMPI1/2/4 and one-invalid-rank negative controls. Every frozen grid and
operation/query/transport count stayed identical. Six real trials were mostly
0.1–2.8% slower: no measured benefit. The original protocol is restored; source,
binaries, logs and rejection report remain in `native_preflight_build_v1` and
`native_frozen_scaling_v1/preflight_rejected_experiment_v1.json`.

The frozen benchmark now records Linux per-rank VmHWM after remeshing, before
artifact gathering. It includes driver and replicated fixture setup; it is not
remesher-only memory or the requested-byte admission quantity. Observed peak
process RSS was about44/53/66MiB for Euler→BL atMPI4/2/1 and57/83/139MiB for
BL→Euler. These small test workloads do not establish a general memory limit.

A standard unordered coordinate cache trial preserves all six meshes and counts.
Euler→BL cost88.003/76.064/48.994s atMPI1/2/4; BL→Euler66.338/36.070/23.626s.
The immutable serial tree baseline repeated at65.810s versus62.066s original:
timing variability prevents attributing the initial apparent serial regression
to the hash table. Saved comparison and control are `std_hash_trial_comparison_v1.json`
and `serial_timing_control_v1.json` under the frozen-scaling evidence folder.

A standalone warm lookup probe favors canonical exact-bit hashing, but excludes
cache construction, eviction, donor search, geometry and MPI. The full trial
passed native gates atall six frozen workloads but changed coordinates at
roundoff scale:276/531/304 RANS lines and one Euler line per rank. The cause is
not established. Strict reproduction failed, so this experiment is rejected;
no independent-audit reuse or fixed-work speedup claim. Source, binary, grids
and logs remain in `native_query_bits_hash_build_v1` and
`native_frozen_scaling_v1/bits_hash_rejected_experiment_v1.json`.

The next trial uses the simpler standard hash plus closed coordinate-box and
early negative-orientation rejection only during exact donor containment.
Original donor ordering/weights and all roundoff/extension searches remain
unchanged. An outside-box nextafter regression guards against making the box a
domain gate. Measurement is pending.


## Adaptation partition goal activated — 2026-10-08

The user explicitly replaced the previous performance goal and cleared its tracker.
The new goal is ACTIVE, defined in NATIVE_ADAPTATION_PARTITION_GOAL.md: establish
contention-aware timing controls, accelerate donor discovery, introduce weighted
adaptation partitioning at M=N, and support explicit M<=N on a separate communicator.
Accepted CFD state/time histories remain on N ranks, with direct donor-to-final-mesh
transfer. Partition/migration and return costs count toward total adaptation cost.

Four identical fixed-mesh CFD-only controls (60 BDF2 steps, inner cap30, MPI4,
13386-point BL mesh, metric/adaptation OFF) completed. Old bare/current bare take
54.237/55.525s; old GNUtime/current GNUtime take49.690/48.144s. Primary final
states are exactly equal across all four, and input/config hashes agree. CPU
busy fractions0.83-0.98 and CPU PSIavg10 up to31.94 demonstrate in-run contention;
launch gating alone does not establish an isolated benchmark. These controls do
not show the large old/current slowdown or wrapper penalty suspected from the
historical comparison. They also do not prove every full-run phase regression is
environmental or establish a small software speedup. Full evidence and auditor:
native_unsteady_performance_v2/fixed_mesh_cost_controls_v1.json and
fixed_mesh_cost_controls_auditor_v1.py. The actual v4 unsteady RANS/Euler cases
both passed independent mesh, geometric BL where requested, primitive and
state/history audits. Their adaptation/CFD ratios17.11%/40.24% remain workload
measurements on this shared host; the lower RANS ratio is not a software gain.

The local immutable-donor broad phase is validated in native_donor_index_build_v2
(app SHA f3a03ef23612ac336c98386cf250626ea6343a82640f677774e714c5ee69811f).
The v1 missing-enum-header build failure remains preserved. It reuses the existing
local CADTElemClass with bounding-box LINE records; exact old box/triangle tests
and canonical donor ordering remain authoritative. Primal-double production only;
other numeric builds retain the original scan. Existing worst-case intersection
scratch admission is checked before traversal; too-large donor partitions/tiny
ceilings use the original allocation-free scan. Index storage is O(local donor
cells), reported separately; query candidates enter dependency admission.

MPI1/2/4 each pass64core cases, including a new differential exact donor-cover check
with translated geometry and reversed IDs. Two MPI4 repeats per source version and
frozen workload complete under recorded contention. Euler-to-BL baseline kernels
57.673294/57.619868s versus index52.759290/54.940094s, mean6.59% reduction.
BL-to-Euler baseline40.695734/41.106110s versus index16.104937/16.455077s,
mean60.20% reduction. This is fixed-work evidence: all grids are byte-identical
within each workload; operations, private target-query counters and input/config/
frozen CSV hashes match. The two index grids independently pass original-P1/
geometric-BL where requested, quality, length, topology, reference and first-height
checks. Candidate counts fall719358200->984576 and1217151953->3214796;
retained index memory maxrank405272/1486072bytes. Kernel improvements are measured
on this shared workstation with two repeats, not universal hardware guarantees.
The Euler-to-BL private reconstruction imbalance remains (0.684/9.955/26.733s
min/mean/max); donor discovery lies outside that timer.

Summary and source checker: native_adaptation_partition_v1/
donor_index_fixed_work_comparison_v1.json and summarize_donor_index_fixed_work_v1.py.
Frozen cases contain their starting grid, frozen sensor, adapted grid and labeled
original source-flow VTU for inspection. Weighted partitioning, explicit M<N,
final-source unsteady/restart validation and practical scaling limits remain pending.
Keep original sensor tensors when repartitioning; derived composed work estimates
must not become donor values. Subset collectives/failure handling must use an
explicit adaptation communicator while CFD geometry/state/history stay on N.
