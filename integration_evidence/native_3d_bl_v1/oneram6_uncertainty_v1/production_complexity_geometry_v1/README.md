# Geometry-aware bounded initialization and coarsest-field preflight

Parent checkpoint: `8c781c6b8aacc8174d953379d14f4f0fd0888447`, branch
`codex/native-3d-bl`. Full Goal3 remains ACTIVE/incomplete.

## Change and invariant field

Eligible passive faceted3D walls that are not individually coplanar now select the
existing bounded sensor-plus-BL-correction estimator before expensive strict cell
integration. Coplanarity uses original coordinates and a roundoff-level tolerance;
it never projects geometry or alters metric queries. Separately planar markers
retain strict integration and its existing limits. Analytic, differentiated and2D
paths retain their dimension-specific integrators. Invalid geometry/densities still
fail. This is a geometry-based integration strategy heuristic, not an error bound.

Normalization first assesses the coarsest permitted sensor with the SAME clipping,
gradation, core and actual-query BL composition. If its estimate minus uncertainty
already exceeds the budget, retain that endpoint and report outside_estimated_bounds,
provisional=1/proven_error_bound=0. Otherwise the existing bounded search continues
from its original guess; total callback limit remains24. This does not change the
field during local mesh operations or discard BL mass/finer sensor demands/fade.
Original cell volumes are retained with geometry rather than recomputed per trial;
their storage is included in the helper admission and retained-byte accounting.

## Saved-state M6 comparison

Exact same configuration, source mesh/reference and120-step saved RANS/SA flow,
four ranks, one CFD iteration, ADAP_LOOP=NO. Both executables initialized the real
Hessian/normalization. Runtime source/binary/build/input pins are in provenance.json;
comparison assertions and result hashes are in m6_runtime_metric/verification.json.

| Work | Previous | Current |
|---|---:|---:|
| Metric elapsed, maximum rank | 415.968s | 16.472s |
| Metric summed rank CPU | 1480.462s | 48.180s |
| Metric-field assembly, maximum rank | 408.368s | 9.898s |
| Whole saved-state pilot elapsed | 425.421s | 29.142s |
| Distinct complexity trials | 7 | 1 |
| Gradation sweeps | 1218 | 1 |
| Owned-point visits | 51,913,424 | 61,330 |
| Strict cell rule builds | 178 | 0 |
| Background determinants | 46,717,286 | 6,673,898 |
| Correction queries | 37,282 | 5,326 |
| Bounded cold preparation, maximum rank | 5.831s | 7.687s |
| All bounded assessments, maximum rank | 1.383s | 0.233s |
| Retained helper payload, maximum rank | 14,207,936B | 15,442,048B |
| Maximum process lifetime RSS during metric | 613.45MiB | 577.72MiB |

The25.25x metric elapsed /30.73x CPU ratios concern this previously outside-budget
initialization, NOT remeshing, scaling, or all reachable targets. A reachable target
pays an additional coarsest-field preflight; that overhead is not isolated here.
Cold preparation is slower in this contended run and now dominates assembly.
No separate measured speedup is claimed for caching volumes. Nested stage maxima
are not an additive timeline. Maximum process lifetime RSS is not aggregate job RAM.

Both estimates are exactly790200.49976272881 with heuristic uncertainty27958.454561979677
(3.5381%) for target100000: +690.2005% budget discrepancy. Correction790198.4113009182,
background2.0884618106155486,8192 proposals/5326 inside. Same coarsest endpoint, same
reported donor-density extrema, same support terms and byte-identical CFD restart
SHA50e8c6f13e3a83d5de8f17845ba8678792cfc4d96d0aa06adf8bccc3648e294a.
No full old/new donor-array dump was made; endpoint identity also follows from the
unchanged deterministic clipping/gradation callback at the same lower scale.
All sensor transport edges pass here; the composed nodal audit still reports
102311 violations/max1.59654e11. Sensor gradation is NOT a combined-field certificate.

These uncertainty indicators remain heuristic, not a99% guarantee or calibrated
production tolerance. Same-field refinement cannot turn its continuum mass into
100000. This comparison does not attempt construction, correction epochs, transfer,
viscous convergence or claim a new accepted mesh. Prior rejected construction and
independent lower-mass evidence remain preserved.

## Connected validation and resources

Nine sequential groups pass:18 existing2D/3D composition/controller cases (1795
assertions), unchanged planar solver reference gate0.05% (82), changed/clipped/
rotated-anisotropy and planarity controls (24), reachable-budget actual solver
(3), intersecting-wall actual solver/zero strict queries (86), four-rank dynamic
controls, planar original-field replay and whole M6 original RAW9 replays2/4 ranks.
All5326 M6 inside points per assessment compare cached versus actual-query tensors.
Spatially clipped trial/new core and return-to-baseline preserve original field
semantics. MPI2/4 agreement criterion2e-12 remains unchanged. The complete planar
support estimate remains13.89942324 versus independent13.90130035 (0.0135% error);
whole frozen M6 estimate940969.8012 +/-34056.6968 is unchanged, not an exact reference.
No repeated independent quadrature export or convergence run was necessary.

Build-j1/nice19/thread1; one owned heavy job at a time, test MPI<=4. Host38–40GiB
available, another session's CFD jobs untouched. Build8m28s/peak process2.37GiB;
focused wrapper81.29s/peak process704.88MiB; actual pilot29.14s/peak process577.72MiB.
GNU time reports0 swaps for those processes. Host swap801->803MiB is not attributable
to this job. Host snapshot and raw resource receipts retained. Compile temporary
files stayed on4TB. Passing groups were reused by summary_refresh.log; they were
not executed again. Earlier rejected query cutoffs and invalid fixture remain in
production_complexity_v1 with verified source capsules.

Reproduce/resume from the repository root after building both binaries:

```sh
OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 MKL_NUM_THREADS=1 nice -n19 python3 integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/production_complexity_geometry_v1/campaign.py
OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 MKL_NUM_THREADS=1 nice -n19 python3 integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/production_complexity_geometry_v1/run_runtime.py
```

Provenance must match; changed source/binary/config/helper inputs require a new
case folder. This directory's links close inside this checkout. The large original
RAW9 domain is reproducibly restored/verified by the existing campaign provider.

## Still missing

Global rare-tail/uncertainty calibration and error-directed work allocation;
post-refinement reassessment of the SAME original donor field and explicit bounded
normalization/remesh correction epochs; accepted difficult curved/M6 construction
under unchanged geometry/height/positivity/q/J/full-edge/transfer gates. No new local
fallback or higher work ceiling was introduced. Donor retention across reconstruction
campaigns, longer shared-edge reuse and independent transaction batching remain the
separate recorded performance package, not implemented by this normalization cache.
