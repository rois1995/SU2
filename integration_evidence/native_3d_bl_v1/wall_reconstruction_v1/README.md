# Coordinated 3D wall reconstruction — planar stronger-field qualification

Branch: `codex/native-3d-bl`. Full Goal3 remains active and incomplete.

Retained volume topology, coordinated normal wall-point insertion, private actual-
field reconnection/movement and metric-directed face/complete-vertex dependencies
resolve the reproduced wall-seam blockage. Imported cavities stay64, constructed
private replacements stay128, growth12 and work4096; no target, fade, interpolation,
height or acceptance threshold changed. There are no Euler/RANS remeshing modes.

The complete stronger six-cell original sensor `diag(4+12*x)` plus square-wall BL
h0=.1,growth1.2,thickness.3,core4 and unchanged fade passes native construction/removal
and independent exact geometry/coverage/full composed metric/height/normal-foot audits:

- `sensor/`: MPI1,649 BL cells→265 sensor-only cells.
- `sensor_mpi2/`: MPI2,649→268.
- `sensor_mpi4/`: MPI4,649→260.
- BL qmin .2150760901,Jmin .0503312313,Lmax1.7987346333; height error0,
  normal-foot minimum0, maximum centroid tilt .8333333.
- BL coordinates/connectivity/markers identical across ranks; see
  `stronger_field_MPI_consistency.json`.

`summary.json` is current. `summary_before_retained_topology.json` and archived
`sensor_previous_*` receipts/folders preserve the old418/430/453-cell rejections,
work/elapsed failures, invalid fixture and rejected experiments. The current grids
are accepted; `_rejected` grids remain historical rejected output. The exact saved
430-cell diagnostic and dependency reconstruction are in `connectivity_replay_v1`.
The centroid insertion experiment did not improve the residual and was removed.

`geometry/` preserves earlier focused geometry-only witnesses; their unit-metric
checks are not full adapted mesh acceptance. `feasibility_control_not_native.su2`
is a576-cell separately constructed control proving feasibility under the declared
metric/height/footprint contracts, not native output or CFD evidence.

The explicit hidden `[NativeFinerSensorBL3D]` test remains required for Goal3.
Run `run.py sensor`, `run.py sensor_mpi2` or `run.py sensor_mpi4`, then
`audit_sensor.py` with the same stage. Collectors preserve superseded attempts,
require fresh builds and exact provenance for reuse. Current stronger-field runs
precede the test-only SA oracle normalization rebuild; verified unchanged production
binary and341 object pins are documented in `runtime_provenance_after_sa_oracle.json`.
Historical execution hashes have not been rewritten.

This is one planar wall and scalar sensor, not general BL generation. Altitude and
normal-foot containment do not certify orthogonality or a complete layer stack.
Thin/curved/interacting walls and features, analytic BL authority, actual RANS
convergence/wall shear/forces and complete lifecycle performance remain open.
Do not infer scaling or CFD affordability from these small correctness samples.
