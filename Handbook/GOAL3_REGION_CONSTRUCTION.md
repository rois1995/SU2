# Goal3 — whole-region construction proof and native port

Repository `SU2_NativeIntegrated`, branch `codex/native-3d-bl`.
The required milestone remains complete accepted native curved construction under
unchanged constraints. Full Goal3 is active and incomplete. Production C++ still
matches00e3; this checkpoint is a geometric/field-validated **prototype**, not native
runtime construction, MPI, CFD transfer/restart or physical RANS qualification.

## Complete candidate, actual original target

`integration_evidence/native_3d_bl_v1/region_construction_v1/summary.json` pins all
source, inputs, rejected candidates and final evidence. Input is the actual1402-cell
frozen target, with original P1 sensor donors/reference unchanged: h .05, growth1.2,
thickness .2, Hausdorff .04, q≥.20/J≥.05 and full composed lengths≤1.8.

The complete1368-cell/423-node candidate has native qmin .215224947,
Jmin .053450832, all2074 full edges admissible, Lmax1.795194775. Native measurements
use the exact pinned39a1f487 study binary in read-only field-audit mode. It does not
execute that rejected recovery or CFD iterations. The wrapper refuses changed
binaries; do not silently substitute current restored sources or future builds.

Independent exact rational checks pass: positive volumes, oriented connected ball
skin, all423 vertex links,25091 overlap-pair candidates,64 normal feet. Maximum
relative first-height error3.33e-15, centroid tilt1.66e-15, authority node error
1.11e-16; maximum wall chord sag .01921472≤.025. Exported actual-centroid tensor
q/J arithmetic agrees within4.45e-16. Full edge integration remains native evidence,
not an independent quadrature proof. This establishes a complete admissible
prototype candidate, not integrated native construction acceptance.

Inspect `metric_region/candidate_validated.su2` plus its unchanged authority sidecar,
or `candidate_metrics.vtu` with quality/region fields and `region_legend.json`.
No CFD solution is fabricated or included. Goal3 workflow gates remain pending.

## Connected construction, replacing inherited volume connectivity

1. Plan projected physical-surface splits from the original native full lengths.
   Coarsen redundant wall sampling under link, feature, geometry, first-tet quality
   and metric guards:136→64 wall faces/36 collapses. Boundary sampling is free to
   change; the previous fine wall triangulation is not frozen.
2. Fit one inward affine flow constrained to preserve nonwall cap planes. Plan the
   first normal band using requested height/growth; wall-face apices lie one exact
   prescribed altitude from the face, with centroid normal feet.
3. Construct surface-face cells and shared edge-dual connections across neighboring
   band cells. Enumerate positive triangulations of each shared bipyramid; do not
   assume every dual edge is visible. Junction quadrilaterals receive shared
   physical-cap points. Connectivity is planned for the entire region together.
4. Reconstruct the transition as another wall band ending outside the original
   BL fade. Preserve cap planes rather than extruding thin isotropic cap layers.
   Coarsen cap interiors regionally and fill the certified star-shaped core.
5. Plan metric-aware cap junction points within their quad constraints; final
   actual-centroid metrics and every actual full edge are re-evaluated natively.
   Final geometry/height/embedding is independently checked before any qualification.

Python geometry-derived metrics and captured tensors are **proposal proxies only**.
No native sensor interpolation, BL/fade/geometry policy, q/J/length contract or
ADAP_HESSIAN_NOISE was changed. These proxies/CSV tensors and Python/SciPy must not
enter production. Prototype work proxies are not equivalent to native4096 local
operator trials, and no native event-budget or speedup qualification follows.

## Rejected experiments retained

- `affine_band`: positive1534-cell prism-star/core proposal; native samples show418
  shape failures. Its full edge export stops at an unresolved edge integral; retain
  the exact offending edge in `diagnosis.json`, not a hidden integration success.
- `coherent_band`: blindly choosing the dual edge produces56 nonpositive tets;
  no native run was launched. Admissible shared-interface triangulation is necessary.
- `coherent_region`: positive2592-cell proposal retaining redundant wall sampling;
  the design proxy rejects cap junctions. No new native run was warranted.
- `surface_region`: native complete audit has0 long edges but6 J failures, minimum
  .049910001; this is a real contract failure, not roundoff. Final constructor
  point planning addresses it without relaxing a threshold.
- A report-serialization failure and its already-built candidate/source snapshot
  are retained separately. Final reports use native Python scalar counts.

## Native implementation required before the milestone

Port the connected region constructor, not another edge-repair fallback. Reuse
PlanarReference authority/projection, existing topology/predicates, actual-query
FrozenField and TargetEdgeLength, LinearizeMetricShape and current wall-height
checks. Obtain height/growth/thickness from existing configuration; do not dispatch
from Euler/RANS labels, fixture names, saved coordinates or generated mesh files.
Replace design proxies and saved tensors with actual frozen-field callbacks.

Preserve globally consistent reserved point/cell IDs, exact unchanged-node identity
rules, complete versioned accepted-input dependencies, memory/work/time admission
and atomic whole-event rejection. Constructor data is a region, not an enlarged
64/128 primitive cavity. The surface/volume plan should be shared consistently
across worker ranks; no unguarded per-query MPI collectives or global donor gather.

The proof covers one visible closed region, one analytic cylindrical wall, planar
caps and constant identity sensors. General nonstar/holey regions, multiple walls,
finer/nonuniform sensors and partial references are not proven. Do not introduce
cylinder-test dispatch as a substitute for general capability. Applicability and
operator limits must be explicit. Already-admissible meshes should retain their
cheap path; no MPI/performance claim follows from this serial proof.

First native check: the original full1402-cell target under unchanged constraints
and4096/300s limits, then meaningful MPI1/2/4 and cold/refine/remove/transfer/restart
workflows. Preserve every full-goal gate in GOAL3_REQUIREMENTS.json. Performance must
measure the actual complete accepted lifecycle, not this Python construction time.
