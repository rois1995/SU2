# Original-field sensor plus geometric-support correction experiment

Full Goal3 is active/incomplete. This is a test-only alternative integration
provider, not production normalization or an accepted remeshing workflow.
Paths below are relative to `integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1`.

## Strategy and preserved field

Use the exact decomposition C = integral(sensor density) + integral(composed
minus sensor density). Reuse the saved positive Q18 original-P1 sensor integral
and its separate analytic exact-arithmetic bounds. Sample expensive corrections
only in the complete faceted-wall support, including the fade region. No sensor
or BL tensors are fitted to sampled/adapted vertices. Every target query uses
native geometric BL composition of its original hosting-rank RAW9 P1 sensor,
with unchanged finer-demand preservation, fade, geometry and mesh gates.

`correction_support.py` proposes a mixture of two-sided triangular face prisms,
edge sectors and enclosing two-halfspace vertex lunes. Reciprocal-height radial
importance includes the appropriate r^k Jacobian. Tangential geometry weights
are proposal proxies only. The PDF sums ALL covering components, so overlap is
not counted twice. All positive components remain; candidate/time ceilings reject
rather than truncate a PDF. Geometry is prepared once per proposal and can in
principle serve different scale queries; persistence across production trials is
not implemented. The provider currently handles one faceted wall, h0 <= T and
growth >= 1. It is not analytic-CAD/multiple-marker support qualification.

`support_correction.py` exports all303359 original cells with original node IDs
and host-rank RAW9 coefficients. `correction_domain_v1/manifest.json` pins the
128624240-byte deterministic export. The exact executed exporter is archived;
the older tracked `correction.py` decomposition experiment remains unchanged.
Do not commit the large reproducible export.

The generic hidden-test backend in `NativeCorrectionReplay3D.hpp` reuses the
production bounded donor index. New `DonorImport::ProbePoints` returns a valid
empty union only for a complete outside-source search. Existing strict segment
imports keep rejecting empty searches. Exhausted, malformed or oversized point
searches fail collectively. Original RAW9 tensors are evaluated on their owners;
identity tensors in locator metadata are NEVER the physical field. Hash ownership
is a prototype, not a measured production partition/scaling policy.

This replay integrates over the original domain using the same cells as donor
support. A later changed geometry needs separate domain membership and immutable
original-field/extension rules. Reassessment on an adapted grid is not implemented.

## Independent checks and rejected drafts

`planar_support_correction_v2`: a finite unit-square wall embedded in a containing
six-tetrahedron box, original sensor2I/core2, h0=.1, growth1, T=.2. The complete
tube has volume V(r)=2r+2*pi*r^2+4*pi*r^3/3. Its proposal must be uniform, exercising
faces, outer edges, corners and the zero-width coplanar diagonal. Independent
coarea integration gives correction10.71931983514507; order32/64 agree numerically.
8192 samples give10.713626743914942, error0.00569309, empirical SE0.03287523
(0.173 SE). Every native point density agrees with the independent distance/fade
formula to9.99e-15 relative. MPI1/2/4 rows agree exactly. This also detects accidental
use of locator identity instead of original2I. It is a mathematical integration
check, not a physically embedded CFD wall case.

Preserved `global_correction_seed947_v1`: radial inverse endpoint/self-check failed
before native queries; rounded inverse endpoints are bounded by the supplied
radius. `planar_support_correction_v1` exposed an ill-conditioned axial residue
in nearly coplanar edge sectors. The shared proposal now builds a common 2D edge
plane and uses a stable half-angle frame for edges and vertex lunes.
`global_correction_seed947_v2` predates this correction and is explicitly
UNQUALIFIED; native rows/results and executed producers remain inspectable. The
runner refuses to relabel/reuse it as qualified data. No rejected result was lost.

## Current M6 evidence

Corrected `global_correction_seed947_v3`, seed947/8192 samples, full original domain:
sensor Q18=152994.2009390569, correction808504.2915492739, total961498.4924883308.
Empirical correction SE9410.95983 (3SE28232.87949),5412 source hits. Native kernel
2.735s MPI4/index build2.032s; preparation23.170s, of which PDF16.861s and static
proposal5.309s. These contended experimental costs are not lifecycle speedups.
Independent seed3557/8192 gives961690.7586370166 (SE9414.26309). Their
mean961594.62556 has pooled empirical SE6655.72; the seeds differ192.27. This is
9.616 times the100000 request, not a feasibility proof derived from that mean.
The separately saved strong-BL subset lower diagnosis already excludes100000
under its stated conditional statistical assumptions. Whole-volume Q18=780725.70
is180868.92 below this mean (18.81%); the whole-field sampling estimate is not an
independent rigorous full-field reference.

`verify_support_correction.py` independently reconstructs every inside sensor
density using NumPy and original RAW9: maximum discrepancy7.22e-15 across both
seeds. Exact rational binary64 containment/barycentrics check64 points per seed,
including the worst-conditioned cells. Every saved M6 donor/query/density/weight
row is identical MPI2/4. This does not independently certify outside membership
or full composed target density on M6.

Local support conditioning is INEFFICIENT:16384 samples per saved cell yield:

| Cell | Source hits | Estimate | Saved reference | Relative error | Empirical3SE |
|---|---:|---:|---:|---:|---:|
|88|22|6.495907|4.692763|38.42%|5.358431|
|157|16|1.368559|1.396852|2.03%|1.011346|
|329|11|0.195954|0.260534|24.79%|0.080215|

All three observed errors lie within their very broad3SE indicators, but all have
fewer than32 nonzero samples and are explicitly flagged sparse. This is evidence
against treating global-support sampling as an efficient per-cell integrator,
not evidence to trust sparse SEM tails or raise a sample ceiling. No new expensive
Duffy references were generated. Exact receipts: `support_correction_summary_v1.json`.

Empirical standard errors are HEURISTICS, not proven confidence/error bounds.
The separate sensor bounds are analytic exact-arithmetic diagnostics, not outward
machine certificates. Independent seeds and numerical references assess indicator
calibration, not a mathematical proof of all missed support mass. No production
tolerance is selected from these provisional results.

## Scope and resources

Focused point-probe controls and both existing donor-reuse controls pass MPI1/4, including remote inside/outside, tiny
budget, one-rank NaN and unchanged strict import semantics. The connected18-object
build passed with one worker. Planar validation wrapper peak processRSS63.9MiB,
zero swaps. Corrected first seed wrapper29.49s/peak process382.7MiB; second
seed plus sequential MPI2 check38.42s/666.6MiB. Local3-cell+existing-control batch
69.94s/213.1MiB; independent audit1.92s/173.8MiB. Connected build532.97s/2.35GiB.
Every receipt reports zero swaps. These maximum processes do not establish
aggregate MPI memory. Native kernel query savings are largely consumed by Python
proposal/PDF preparation; there is no established total speedup. Do not add nested
rank-max timings as if they were sequential critical-path phases.
One owned heavy job at a time, nice19, thread1, MPI<=4; actual host checked with
about40GiB available. Foreign CFD/FSI processes are untouched. Automatic review
initially rejected a replay because it saw live MPI/Python jobs; identifying them
as foreign established no owned job was live, and the bounded retry was approved.

No new CFD runs, accepted M6 grid, normalized feasible budget, production controller
coupling, finer-mesh reassessment, solution transfer or lifecycle performance claim.
Donor-campaign retention/shared-edge reuse/batched transactions remain separate
pending production work. Do not silently change the100000 target or BL constraints.

## Reuse and next decision

The archived proposal producers and immutable query/PDF files allow the same
samples to be reused without rebuilding their geometry. Consumer binaries,
original inputs and source hashes are pinned separately. Current native source
indexes are rebuilt on each replay process; no production retained campaign cache
has been added. The next substantive step must connect a validated global
uncertainty/work provider to normalization and explicit frozen stages; it must
handle unattainable budgets visibly. Repeating tiny-cell Monte Carlo or adding
another subdivision ladder is not justified by these results. Existing recorded
production out-of-range behavior and all mesh constraints remain unchanged.
