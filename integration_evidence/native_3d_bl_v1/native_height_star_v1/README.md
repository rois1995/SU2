# Native height-star reconstruction — current scope

Full Goal3/general symmetry capability remains ACTIVE/incomplete. Build_v7
is terminal/PASS; all four accepted lifecycles and independent audits pass. Optional Opus5.5/medium review
21708 timed out without a final report and supplies no validation evidence.

## Current connected qualification

Bounded binary64 leaf admission is compiled and qualified by the unchanged full
annular workflow in serial, N2/M2, N4/M2 and N4/M4. Every case independently passes
finite geometry/hole coverage, exact volume/topology/no overlap, original full
composed field, prescribed height, conservative flow/current+BDF/SA transfer,
SU2+CGNS/reference output and real restart. Full Goal3/general geometry remains OPEN.
All owned jobs are terminal. Current source/build pins are in build_provenance.json;
the preceding manifest belongs to04e9c75dd1 and remains historical evidence.

| N/M | Final tetrahedra | Shared work | Min q / J | Max full edge | Integral defect (max flow/SA) |
|---|---:|---:|---|---:|---:|
|1/1|218|1035|.20333/.05729|1.79552|6.59e-16|
|2/2|194|1136|.21256/.05890|1.79552|6.06e-16|
|4/2|194|1133|.21256/.05890|1.79552|5.80e-16|
|4/4|215|776|.20239/.05648|1.79828|6.06e-16|

All wall-height errors and both restart residuals are0 in these audits. Different
partitions can generate different valid meshes. Profiles are collective within
each case; bitwise cross-N equality is not claimed. Serial and N2 profiles match
their prior inputs exactly, and the accepted serial SU2 mesh is byte-identical to
v9. N4/M2 and N4/M4 use the same original field. `lifecycle_summary.json` retains
complete per-rank clocks/counters/provenance and uncertainties.

Contended correctness timings (metric initialization/native remesh/replace, seconds):
N1 7.826/6.275/.0102; N2 3.858/6.298/.0116; N4M2 4.287/6.368/.0122;
N4M4 4.975/7.014/.0133. These are not a scaling/speedup or affordability benchmark.
Normalization remains material; native cost includes partition/migration/reassessment/
return, and replacement includes CFD rebuild/transfer. Nested clocks are not additive.
No separate timer isolates coordinate admission; its explicit upper bound is432
candidate representations times imported wall-face count, included in reconstruction.
No new density query or MPI exchange is added by that point-admission preparation.

The full-workflow folders are `../planar_symmetry_reference_v1/annulus_v10_n1`,
`annulus_v10_n2`, `annulus_v10_n4` and `annulus_all_v10_n4` in the same parent.
Open each `_written_SU2.su2` or `_written_CGNS.cgns` for the accepted mesh. Rejected
snapshots in a passing folder are its intentional one-work rejection control.
`mpi2_patch_v3` independently passes the complete352cell saved field (7929exact
intersection checks); `serial_v4` repeats11cases/81755assertions and the355cell audit.

The native MPI failure at04e9c75dd1 is now repaired for these cases. Historical
negatives, initial successful serial evidence and the rejected midpoint-bracketing
experiment are preserved below; they are not current failures. Proposed rounding
neighbours are checked by the original finite-wall height/footprint checks; the
metric, original donors, geometry authorities, tolerances and final gates stay fixed.
A27-neighbour box and independent leaf choices do not guarantee a compatible
combination or arbitrary rotated/junction representability.
[Current source and artifact pins](coordinate_admission_manifest.json).

## Complete accepted serial lifecycle

`../planar_symmetry_reference_v1/annulus_v9_n1` passes the actual initialized RANS/SA
workflow:24->218tetrahedra/71vertices,91refine+36coarsen edits,1035/8192work.
Current/twoBDF conservative transfer, SU2+CGNS/reference IO and actual restart pass.
Independent finite-hole/authority/topology/volume/3986overlap checks pass, with
q=.20333454/J=.05729226/Lmax1.79551645/height0. Flow and SA integral defects are
5.75e-16/6.58e-16; restart residuals0. No physical CFD advance or viscous accuracy
is claimed. Rejected files in this accepted folder belong to the intentional
one-work atomic-rejection control; use result.exit_code/profile.accepted.

View `../planar_symmetry_reference_v1/annulus_v9_n1/symmetry_annulus_bl_budget_20_m1_written_SU2.su2`
or the corresponding `_written_CGNS.cgns`.

Native saved-state serial_v3 independently passes the complete355cell mesh and
7818overlap checks; q=.20333454/J=.05128452/Lmax1.79551645. It uses original24-cell
sensor-only donors with actual-query BL. Native compound replacement comprises
three private cuts/52cells; no target/tolerance/cap was relaxed. Four-leaf proposals
remain a bounded heuristic, not a general front/multilayer generator.

## Preserved MPI failure before coordinate admission

`../planar_symmetry_reference_v1/annulus_v9_n2` exits with operator_stagnation at
3759/8192work,103combined accepted private edits,335cells/104nodes. N4/M2 and
N4/M4 were NOT launched. Both ranks preserve the accepted CFD state.
`mpi_rejection.py` merges uniquely owned snapshots, rejects conflicting shared
coordinates and reconstructs the original composed field. `mpi2_residuals.json`
finds no shape failures (q=.23357/J=.05975) but four1.8659296edges from wall corners
to one first-height apex. Its complete34-cell star is within the existing64 cap.
The normalized sensor differs from serial by4.44e-7 relative, within the observed
integration uncertainty; identical bitwise targets are not claimed.

`run_mpi_patch.py v1` replays that exact state through the native operator, without
CFD or distributed admission. It rejects all four proposals at the first binary
cut. Thus this is a native proposal/constraint issue, not merely MPI admission.
An offline partition probe clears the same star at the first spacing factor
(q=.20164/J=.05657/no oversized edges), but that probe is not native acceptance or
an independent complete geometry/transfer audit. Diagnose this difference before
adding a fallback/cap increase or retrying the full MPI event.

## Accuracy, costs and limits

Frozen original-field reference complexity20.00019535 is9.77e-6 relative above20.
Final same-field reassessment20.00728648 has heuristic indicator.18512706; its
observed error.00709113 is covered, not a proven bound. Finer-partition estimates
are not automatically more accurate: this assessment is less accurate than the
initial20.00001325 estimate. No original donors were refitted.

On this contended correctness run: actual BL metric build7.87918s (complexity and
audit7.87892s), native remesh5.73169s, replace/transfer/rebuild.01757s, complete
fixture14.52894s. Nested clocks must not be summed twice. No speedup/scaling or
unsteady-affordability claim follows. Composed nodal gradation still has18directed
violations/max ratio4.59899; the sensor fixed point is not a composed certificate.
Centroid-to-apex tilt71.74degrees is diagnostic, not a viscous accuracy certificate.

The finite-face coarsening guard now rejects tentative hole-bridging faces before
strict SurfaceError; focused geometry gate passes3cases/87assertions. Existing
full accepted geometry checks stay strict. The test-only all-worker selector
supports N4/M4, currently unexecuted for this annulus.

General oblique runtime adoption, multiple bodies/junctions,2D/exterior/M6,
interacting layers, calibrated global uncertainty and full composed gradation
remain open. Source/binary/config/input capsules and every rejected experiment
are retained. Earlier build/fixture correction history is in
[the preserved history](HISTORY_BEFORE_SERIAL_ACCEPTANCE.md); withdrawn incomplete
7/8-face fixtures establish no complete-metadata construction failure.

## Tensor trace, rejected bracketing and qualified coordinate admission

Diagnostic-only build_v6 completed. `mpi2_patch_v2` still rejects; actual composed
xx5.0471242643611474/yy5.0471242643611429/xy-4.18e-16 differs slightly from the
analytic diagonal core5.0471242643611385. These are real frozen query values; do
not edit the metric to remove them. Exact-rational reconstruction from that trace
shows all four metric-scaled proposals cover only6/8required wall faces. The miss
is~1.1e-16 barycentric, outside the existing long-double footprint allowance.
It is a sufficient proposal obstruction, not proof of target infeasibility.

`coordinate_admission.py` compares exact affine-midpoint bracketing with immediate
binary64-neighbor admission. Exact midpoint bounds stillcover6/8; their endpoint
rounding can leave no compatible point in that box. The bounded27-neighbor boxes
cover8/8 and an offline three-cut candidate has q=.20164/J=.05657/no oversized
edges. Exact algebra and observed native values support this diagnosis; native
completion, full independent geometry and MPI/transfer still need validation.
The first diagnostic script assertion/log is preserved; corrected comparison
records both methods explicitly instead of treating the unsuccessful method as
missing evidence.

Current connected production change admits each leaf's representation before
building the private cut: at most27immediate binary64 neighbours, unchanged
finite-wall first-height/footprint checks, maximize covered wall faces then choose
nearest. It does not change the target, tolerances, body coordinates or final
geometry/metric/transfer gates. Work is bounded by4proposals*4leaves*27candidates*
wall-face count; no metric query or MPI exchange is added to this preparation.
Existing reconstruction stage timing includes its cost. This is limited local
coordinate admission, not proof of arbitrary rotated intersections or richer
wall-front construction. Build_v7 now passes the saved native replays and all four complete lifecycle audits
listed above. No arbitrary geometry/viscous accuracy or affordability conclusion follows.

A further optional Claude review was rejected by automatic approval review before
execution because it would send source/failure details externally. No workaround
or new consultation occurred; implementation and validation remain local.
