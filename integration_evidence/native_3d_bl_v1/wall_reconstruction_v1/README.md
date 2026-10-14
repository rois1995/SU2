# Coordinated 3D wall reconstruction — incomplete milestone

Working branch: `codex/native-3d-bl`, base `508e52cf7b`.

The existing boundary split now carves requested wall children/transition together,
admits complete original wall-apex stars, reserves the bounded node range, grows
across interfaces obstructing actual prescribed wall points, and scores wall
children during placement. Old interior points are restored through private
complete containing-cell/face/edge cavities to preserve interior resolution.
Near-coincident admission expands a proposal only; strict Cone positivity,
conformity, geometry, first-height/footprint and complete frozen metric gates
remain mandatory. Source P1 sensors, actual-query geometric composition, finer
requests, fade and accepted-state atomicity are unchanged. No solver/mesh modes.

`geometry/` holds two passing focused serial cases: uneven wall split plus existing
curved reconstruction compatibility. `independent_geometry.json` proves the new
witness geometry/coverage/footprints; its unit-metric diagnostics are not adapted
mesh acceptance. `wall_split_*_geometry_only.su2` are inspectable geometric proposals.

`sensor/` contains the current stronger six-cell original sensor `diag(4+12*x)`,
actual square-wall BL h0=.1, growth1.2, thickness.3, core4, unchanged fade. Native
rejects after117 accepted refinement edits and418 private cells at work4096.
No joint wall reconstruction passed. The independent audit proves shape and
coverage but rejects the full metric: longest wall edge0,2 remains4.3997755.
The exported `*_constructed_BL_rejected_n1.su2` remains rejected; no release
stage or CFD replacement occurred. Earlier96s/175-cell work rejection and
300s/173-cell elapsed rejection, plus the duplicate-surface fixture failure,
are retained with source/build/binary/object/artifact provenance.

The longer64-growth experiment gave no accepted wall reconstruction. The former
12-growth bound is restored; cavity64/replacement128, work4096 and elapsed300
limits remain. This is not a performance improvement or qualification.

`feasibility_control_not_native.su2` is a separately generated diagnostic control,
NOT native adaptation output. Its576 tetrahedra independently pass exact coverage,
full original-P1-plus-geometric-BL edges/shape, prescribed height and normal-foot
support (qmin=.5819092,Jmin=.3105078,Lmax=1.6644668). Centroid tilt is nonzero;
zero-centroid alignment, CFD, transfer, MPI and efficiency are not established.
This proves feasibility under the declared acceptance contracts and identifies
native operator blockage; it does not close Goal3.

The stronger case is an explicit long-running Goal3 qualification probe tagged
`[.][NativeFinerSensorBL3D]`; it is kept out of unfiltered Catch runs. It remains
required and FAILING in the goal ledger. Invoke its named tag through `run.py sensor`.
`run.py geometry` and `run.py sensor` reuse the production provenance collector,
reject stale builds, preserve earlier attempts, and run one bounded serial stage.
`audit_geometry.py`, `audit_sensor.py`, `feasibility_control.py`, and
`diagnose_kernel.py` reproduce the independent certificates and numerical LP
experiment. The latter uses local NumPy/SciPy; production has no added dependency.
Audit parsing and sensor/BL-crossover quadrature failures are preserved separately.
The improved oracle adds intersection breakpoints without relaxing its tolerance.

Next: replay the saved blocked patches and repair their internal connectivity
with multiple retained/inserted points and actual-field checks, then complete
serial adaptation before the next connected MPI milestone. Do not repeat whole
SU2 workflows while local replay can resolve the obstruction.
