# Goal3 finite-wall integration milestone — 2026-10-13

Working folder: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch: codex/native-3d-bl; preceding published checkpoint718f2e1c12.
Full Goal3 remains incomplete; production configured-BL remeshing guard remains.
All owned builds and checks are terminal. No CFD or cluster run was launched.

The prior tiny-wall omission was integration aliasing: native and independent
analytic densities agree at retained points to about1e-13, yet two angular rules
missed part of the support. The new hidden-support path uses parallel chords,
projected finite-wall/edge/corner/fade partitions, actual geometric radial bands,
and density-directed angular subdivision. A geometric support guard supplements
scalar error estimates. Projection conditioning avoids nearly vertical source
faces; an exact shared-edge side test excludes zero-volume face pairs. Positive
fan anchors avoid collinear boundary fans without extra centroid panels. Sub-ulp
rounded chords retain their positive original-source barycentric measure.

Original-P1 sensors, actual query composition, finer demands, frozen core, fade,
quality/height contracts and bounds are unchanged. Nonhidden band clipping stays
on its existing path. Returned points65536, refinement work16384 and depth32 remain.
Density error estimates and affine checks are heuristic, not universal certificates.

feature_partition_v1/validation.json now PASS on all nine stages: microscopic
BL, production complexity, immutable original reference, nearest-face changes,
finite hidden support, rotated/sheared source geometry, existing BL kernels,
and production complexity MPI2/MPI4. Hidden support exercises moderate and10x
smaller walls at two positions, unit and finer isotropic sensors, independent
parallel-body integrals, source volume and quadratic moments. Its same-point
analytic check rules out metric evaluation as the former omission's cause.
Unit-density relative errors are below0.011% against the unchanged0.05% gate;
these cases retain about36k–51k points each. accuracy_summary.json contains exact
numbers and links to the hashed receipt/log. This count is still substantial;
these correctness checks make no speedup or affordability claim.

Fifteen failed source variants reconstruct byte-identically using saved base
commits/patches/hashes; source_reconstruction.log records that check. Historical
failed receipts and geometry/cap failures stay preserved. Source reconstruction
is not binary replay. The resumable runner pins sources, objects, binary, build
commands, helper, command, environment and per-rank logs; no full-suite rerun.

Next substantial step: couple actual composed BL and geometric edge bands with
the existing first-height MPI engine, establish a complete accepted coarse-grid
construction plus release/coarsening pilot, and then wire that qualified policy
through the production wrapper. Do not remove the production guard based only
on these quadrature tests. Curved/interacting-wall BL, composed gradation,
conservative current+BDF/turbulence, restart/SU2/CGNS, RANS/M6/viscous accuracy
and complete lifecycle cost remain required. Prior first-height kernel/engine
proofs remain limited initialization/protection evidence, not full remesh success.

Replay the focused milestone:

```sh
python3 integration_evidence/native_3d_bl_v1/feature_partition_v1/run.py
python3 integration_evidence/native_3d_bl_v1/feature_partition_v1/verify_experiment_sources.py
```
