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
