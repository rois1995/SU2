# Field generation and provider checks

**CURVED_GEOMETRY_SUPPORT: OFFLINE_ONLY.** No curved-wall flow calculation was run.

The new generator is a sparse, consistent P1 Helmholtz solve on a supplied
nonperiodic simplex mesh, followed by common scaling to the two declared bounds.
It stores the raw coefficients and geometry. The exporter checks supplied fields
without renormalizing them. The C++ reader independently recomputes the norm
certificate. Commands and the format are in [STRESS_FIELD_FORMAT.md](STRESS_FIELD_FORMAT.md).

## Results obtained here

| Check | Status | Evidence |
|---|---|---|
| Text/binary agreement; affine triangle/tetrahedron fields | PASS | `test_field.py` |
| Physical units, trace, planar restrictions | PASS | Provider and C++ coupling tests |
| Near-wall map, exact wall zero, weighted duplicate wall charts | PASS | Independent NumPy calculations |
| Two wall sheets; rigid 3D coordinate/tensor rotation | PASS | Independent NumPy calculation |
| Support taper, shared element edge, uncovered nonzero query rejection | PASS | Provider tests |
| Corrupt payload/schema, NaN, bad orientation rejection | PASS | Provider/exporter tests |
| Sparse generator, residual check, norms and deterministic replay | PASS | Tiny generated field test |
| Supplied circular saved-field Python/C++ comparison | PASS | All 4,000 saved queries |
| MeshAdaptation barycentric kernel comparison | PASS | 200 triangle/tetrahedron queries |
| Regenerate the complete supplied circular example | NOT_RUN | Saved artifact was imported and replayed |
| Curved-wall coupled SU2 calculation | UNSUPPORTED | Explicit startup rejection |
| Generic CAD/periodic field construction | UNSUPPORTED | No geometry/periodic provider implemented |

The circular input has 20,736 nodes and 40,960 triangles, `U_ref=30 m/s`,
`L_ref=1 m`, `epsilon=0.01`, `ell=0.1 m`, `ell_w=0.02 m`. The reconstructed
raw norms are `0.01` and `0.0029835984674100053`. The polygonal measure is
`4.122926391849266 m²`; maximum diameter is `0.03196909855710921 m`, giving
`H/ell=0.31969098557109205` (passes the one-third study-resolution gate).
The reader reports 8,732,672 bytes of persistent storage per rank. This excludes
parsing temporaries and is not a measured MPI peak RSS.

The maximum absolute difference from the saved Python query values was
`7.771561172376096e-14 m²/s²`. The original NPZ SHA-256 is
`e627cc16cb74c0ddef5844c15e8896d34c938045aad407e285ac53292ea5b3c8`.
The supplied manifest reports inner/outer chord errors of approximately
`3.76491e-5 m` and `9.41227e-5 m`; these are properties of its straight-sided
mesh, not newly measured CFD errors. Its generation timing and smoothing residual
are source provenance, not measurements of a run performed here.

The prepared square example has 81 nodes and 128 triangles, measure 1 m²,
maximum diameter `0.1767766952966369 m`, `ell=0.6 m`, and `H/ell=0.2946278254943948`.
The positive/negative raw shear fields have both norms equal to 0.01 within
roundoff. The bottom is an analytic wall; inlet/outlet are open, top is symmetry.
A box taper makes the applied correction zero at all physical boundaries.
Straight boundaries have no chord approximation. The bounded local suite now uses
this fixture and a 27-node, 48-tetrahedron cube. Neither is a physical benchmark.

## Reuse decision

I inspected `CBarycentricLocator` on
[`codex/native-3d-bl` at d219bf9](https://github.com/rois1995/SU2/blob/d219bf96272fd640a185c49df94d103125147852/SU2_CFD/include/adaptation/CBarycentricTransfer.hpp).
It supports mesh-transfer boundary projection and depends on the adaptation
locator/boundary machinery. The new field contract instead rejects uncovered
nonzero queries and only needs an immutable small simplex field. Importing that
whole subsystem would enlarge this change substantially. The compact strict
reader was retained.

Your existing Python `GeometryHelper.get_barycentric_weights` was used as an
additional independent reference without modifying MeshAdaptation. Its source
SHA-256 was `389d2bacd28c42ffb3bac74a1d688ca3a4784f721d7b6184e3f1386fbee85fdb`.
All 200 checks passed; maximum absolute difference was `2.2204460492503131e-16`.
`check_meshadaptation_reference.py` reproduces this check given the source path.

The generator accepts both SINE_PHYSICAL_V1 and LOCALIZED_PHYSICAL_V1 seeds.
The latter combines up to 64 Gaussian terms with physical centers, directional
widths and five signed coefficients. Both use the same P1 Helmholtz smoothing
and common normalization. Localized combinations and sign symmetry are tested;
flow-dependent scaling and directional smoothing operators are not implemented.

The provider now reports face/source/other query counts, failures, accumulated
query time and persistent storage per rank. The serial solver suite exercises
these counters. They do not establish parallel cost or large-case scaling.
Field/CFD refinement trends and independent physical coverage remain unmeasured.
Raw-to-final differences are checked outside the solver; there is no Python oracle
in the CFD loop.
