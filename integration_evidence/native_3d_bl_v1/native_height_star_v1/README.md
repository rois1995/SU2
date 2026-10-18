# Native height-star reconstruction — current scope

Full Goal3/general symmetry capability remains ACTIVE/incomplete. Current build_v5
is terminal/PASS; no owned compute job is live. Optional Opus5.5/medium review
21708 timed out without a final report and supplies no validation evidence.

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

## MPI failure — exact evidence, no unchanged rerun

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
