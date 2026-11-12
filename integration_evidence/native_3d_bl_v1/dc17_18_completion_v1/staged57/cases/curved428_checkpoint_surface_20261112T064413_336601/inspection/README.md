# Rejected428 surface-state inspection

Open `rejected_surface428_nodal_witness.vtu` in ParaView. This is ONE full latest
private surface-stage volume, with **surface_prepared=false** and
**final_mesh_accepted=false**. The time-limited run did not finish its census.
No per-trial meshes are exported.

PointData contains `transported_core` and `saved_composed_nodal_witness`, each a
9-component row-major symmetric tensor, plus global node IDs. Every current node
was joined by ID after exact coordinate and transported-core comparisons against
the396 original checkpoint and selected-core export. The complete396 profile,
immutable reference fingerprint, source hashes and frozen428 run pins are retained
in `manifest.json`. The producer's `target_rank*.bin` files hold the actual nodal
witnesses; its `target_witness_rank*.txt` files are timing records.

The saved composed tensor is a **nodal visualization witness**, not a reconstructed
arbitrary-point target. Production `CavityMetricSource` remains the current field
authority; original-P1 evaluation is only a control. Interpolation displayed by a
viewer does not establish production query values, metric edge lengths or acceptance.

CellData `q`, `J` and `geometry_excess_l2` are IEEE NaN, with corresponding
`q_known`, `J_known` and `geometry_known` equal to zero. This stage capture has no
per-cell certification sidecar. No quality, geometry, height or exact positivity
recertification was performed for this export. The original stage separately
reported301546 positive cells; that report is not a new export certificate.

`export_rejected_surface.py` reuses the existing416B record definition and VTU array
helpers from `inspect_native_failure3d.py`, and the hash-checked checkpoint parser
from `assess_target_directions396.py`. It refuses to overwrite existing artifacts.
Its validation checks serialized PointData/CellData tuple counts, connectivity,
offsets, tensor/coordinate round trips, tensor finiteness/symmetry/positive
definiteness, and unknown flags. Neither Python environment has VTK installed and
`pvpython` is unavailable, so there was no VTK-reader or rendered-view validation.

The script ran at nice10 with numerical libraries restricted to one thread.
The VTU is a local inspection artifact; no production sources, build or CFD jobs
were changed or launched. See `manifest.json` for hashes, exact size and timing.
