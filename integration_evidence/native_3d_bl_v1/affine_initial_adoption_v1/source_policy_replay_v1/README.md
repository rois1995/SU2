# Frozen source policy comparison — 2026-10-19

This is a bounded saved-data experiment, **not production source coverage or an
accepted adaptation**. Native C++ and the CFD state are unchanged. The complete
general-symmetry/Goal3 scope remains open.

- `run_v1_assembly_failure.py` is the exact first executed source. It completed all
  three numerical cases in about21.4s, then report assembly failed because Python3.8
  supplied a relative `__file__`. Original log/failure/source are retained.
- `result.json` seals those completed cases without repeating their numerics;
  `numerical_results_manifest.json` pins the unchanged outputs. Corrected `run.py`
  remains reusable and labels the comparison without claiming general admission.
- `audit_envelope.py` completed in13.44s, maxRSS285672KiB (about279MiB), including
  exact complete saved-annulus envelope checks and the two exact M6 fold replays.
  `envelope_result.json` pins sources/artifacts. `validation_receipt.json` checks
  the resulting hash chain. Original comparison peak RSS was not captured.

## What worked and what failed

Nearest original-boundary projection is not generally continuous at reflex fluid
edges. `exact_counterexample.json` proves unequal one-sided trace limits on an
exact rational wedge with a continuous, positive-definite original P1 trace.
A canonical nearest-face tie rule does not remove those unequal limits.

The numerical M6 diagnostic uses the saved **canonical owner-node sensor P1**;
raw cell-parent/halo samples remain untouched in the original archive. It is not
claimed identical to every old rank-halo coefficient or a newly normalized target.
The original NPZ hash and rank-coordinate equality were checked before use. Tensor
symmetry follows the recorded lower-triangle convention. No BL tensor is sampled
or spread, and no new Hessian/normalization/CFD computation is performed.

| Band | Largest M6 relative trace difference | Selected collar faces | Folded faces | Probe coverage |
|---|---:|---:|---:|---:|
|1e-13|8.2936e-11|24|0|12/12|
|1e-8|8.2944e-6|24|0|12/12|
|1e-4|.0859353|24|2|12/12|

Relative difference is the spectral radius of `W.T*(A-B)*W`, with `W` whitening
`(A+B)/2`. These are numerical near-bisector observations; the separate rational
wedge supplies the exact discontinuity proof. Finite-triangle distance screening
includes face interiors, edges and vertices, not candidate vertices alone.

The saved constant annulus gives zero differences. A separately identified
manufactured anisotropic P1 field on the same nonconvex/hole geometry gives
2.87e-13,2.88e-8,2.88e-4. That field is an experiment, not the saved flow sensor.

One common normal-cone column per original boundary vertex gives shared sensor
copies and avoids independent-prism trace mismatch. It does **not** guarantee
positive prisms. At band1e-4, M6 WING3D faces48047/48049 fold in slots1/2. Their
altitudes are approximately9.67e-5/9.18e-5, comparable to that band; column norms
are only1.0..1.54. This is a finite-width map failure, not merely huge normal vectors.
All12 selected points remain covered by retained cells, so sampled coverage alone
would miss the invalid shell. Exact coordinates/determinants/normal constraints
and the rejected raw grid are retained in `m6_folded_collar_exact.json` and
`m6_wide_collar_rejected.su2`. No invalid prism is published to native remeshing.

The follow-up complete annulus uses an explicitly wider **integration/source
collar**, twice the required1e-13 query band. This does not change the physical
geometry contract. It gives96positive collar tets around24original cells,3762exact
supporting-face conformity certificates and1024exact old/outer triangle separation
certificates. Its augmented oriented skin is exactly the mapped outer skin.
All tests use exact rational interpretations of actual binary64 coordinates;
original embedded-source validity remains an input assumption. The supporting-face
method is sufficient, not a complete intersection solver: uncertified pairs reject.
See `annulus_source_envelope.json` and `annulus_envelope_audit.json`.

`annulus_source_envelope_probe.su2` is an inspectable **source-domain** mesh, not
an adapted CFD mesh; its extremely thin cells have no CFD quality claim. The M6
rejected grid is likewise a source-collar diagnostic, not a rejected adaptation.

## Work and decision

M6 cold geometry/topology preparation4.832s; selected-edge experiment7.963s;
1663small active-set solves. Prepared numeric arrays29,977,744bytes exclude Python
maps, retained NPZ arrays and interpreter. Follow-up maxRSS includes its full process.
No MPI communication, production total-event timing or speedup was measured.

Do not enable nearest-face projection as a generic fallback, and do not assume
`ADAP_HAUSD` is the required uniform source width. For new joint **affine mapping**,
prepare a conforming original-source collar from the actual mapping envelope and
certify positivity, conformity, separation/coverage and resource admission globally.
Keep qualified analytic frusta/prisms for their physical approximation envelopes.
Broader joint curved coverage needs its actual required envelope assessed; this
experiment does not justify another general collar mesher or a local fallback ladder.

Still missing: production joint-collar admission/import/query/MPI, mapped body and
finite-feature bindings, full original-field edge cuts,2D runtime, complete arbitrary
oblique/exterior accepted meshes, transfer/persistence/restart and full goal gates.
No new linked C++ qualification, CFD run or unchanged M6/annulus attempt occurred.
