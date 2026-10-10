# Distributed immutable original sensor donor checkpoint

2026-10-10. Repository: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch: codex/native-3d-core; predecessor b18b5d8ecd91e5975b7a8973679ac80807cad749.
Goal 1 remains active with its full objective. Production 3D adaptation is still
rejected until the remaining implementation and qualification are complete.

## Implementation and callers

CNativeDonor3D provides DonorImport on the existing World worker communicator.
Build freezes a private copy of original tetrahedra and sensor-only tensors.
Original local ownership is not limited to the 256-cell imported-patch size.
Source authority is hash-routed: shared nodes must have identical coordinates
and sensors; cell IDs and sorted simplex keys must be unique globally. Positive
orientation, finite geometry, distinct node IDs and exact SPD are admitted before
freezing. Failed construction permits a later correct retry; repeated construction
on a frozen object rejects without replacing the source.

The source remains independent of the working mesh. Per-owner spatial lookup
uses the house CADTElemClass two-corner box representation, with an explicit
local tree. At most eight BisectionBoxes owner boxes per rank feed CRankBoxTree
on the worker communicator. Only this small routing hierarchy is replicated;
the original mesh/sensor field is not. Imports route region requests to relevant
owners, query their indices, remove inflated-box false positives, deduplicate
local donor IDs across regions and exchange only selected source records.
There is no whole-original-mesh fallback scan per candidate. Broad requests can
still intersect every local cell; the memory model admits that indexed scratch.

Regions are bounded to 64 and returned donor unions to 256. Oversized or failed
imports return no usable partial patch, with a common rejection reason. Empty
owners and inactive callers still enter matching worker collectives. A failed
request does not replace the accepted source. Import is conservative AABB
selection, not a domain/coverage certificate: exact FrozenField point containment
and original-edge interval coverage remain required. Existing source embedding
and conformity are caller preconditions, separately audited in the mesh fixture.

Donor records carry no composed wall tensor. The receiver constructs FrozenField
and applies composition at each actual query, including sensor-cache hits. The
manufactured composition test uses a thin z<1/16 region with finer zz demand;
it proves these routing/cache semantics, not production BL geometry integration.
Composed TargetEdgeLength continues to reject without a geometry-resolved
integrator; no sensor-only edge audit is substituted for a composed target.

The strict passive meson library includes the new unit. The existing registered
CNativeDistributed3D test file contains four new distributed cases. The reusable
run_distributed.py builds the new unit and actual house memory model and records
DONOR profiles plus independent audit receipts. The split/inverse fixture now
constructs its event's original source on owners and imports it before private
metric validation. Its accepted mesh/metric results remain unchanged.
No 2D/MMG operators, config or production runtime guards were changed.

## Evidence

Final case folder:
/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_3d_core_v1/donor_import_controls_v2.

PASS, one sequential low-priority MPI1/2/4 campaign under measured host contention:

- MPI1: 50 Catch cases / 2841 assertions, including all 37 existing 3D cases.
- MPI2: 13 cases per rank / 181 and 194 assertions.
- MPI4: 13 cases per rank / 181, 182, 183 and 195 assertions.
- Original affine SPD sensors, shared-face canonical ties, actual-query geometric
  composition on misses and hits, eight requests/four hits/eight compositions
  per rank. Caller sensor-copy mutations do not modify the frozen source.
- Global shared-coordinate/sensor conflicts, duplicate cell IDs, duplicate
  canonical simplices and invalid SPD reject before freezing; corrected retry
  succeeds. Empty global sources, missing requests, invalid/oversized regions,
  tiny memory ceilings and saturating caller-byte overflow reject.
- 257 original disjoint cells: local owners may exceed the patch limit. A small
  region imports one donor; a broad 257-donor union rejects with empty output,
  after which a valid small import still succeeds.
- M=N, M=N-1 and M=N/2, plus a source owned solely by worker zero while other
  workers hold genuinely empty local indices. Idle CFD ranks stay outside the
  worker collectives.

The independent audit_donors.py uses actual DONOR_SOURCE and DONOR_SAMPLE records
from MPI stdout. Python Fraction validates transported coordinates and affine
sensor values against the declared input, positive source orientation, exact
canonical containing simplex, P1 interpolation and thin-region composition.
All 42 transported donor records and 28 sample evaluations pass with maximum
absolute residual zero. Seven audit controls cover the original accepted input
and six tampered/missing/duplicate variants; donor_audit_adversaries.json records
those results and pins. These are specific manufactured observations, not a
universal interpolation error bound.

All 15 mesh JSON and 15 SU2 files are byte-identical to worker_field_controls_v2.
Independent exact embedding/marker/P1 results match, with updated input-log hashes.
Refined/returned max metric length 2.828 is still oversized private progress;
coarsened max length 1.732 passes the strict gate. No complete sweep is claimed.

The serial_donors child compiles the actual units without HAVE_MPI and passes
source/P1/composition/cache/rejection controls. It explicitly verifies that a
point outside a tetrahedron is rejected even when its AABB intersects the import
region. /usr/bin/time reports 3580 KiB for this tiny process; this does not certify
the working-memory model. Receipts record commands, source pins before/after,
logs and binary hashes. Source and artifact hashes were verified before publication.
First donor_import_controls_v1 passing logs/pins are retained as historical
coverage before fixture integration and the external donor audit. Duplicate v1
manufactured grid files are omitted from Git and can be replayed from the pins.

## Cost, limits and replay

The service has optional default-OFF aggregate timers and always-on counts.
Build and Import parent phases are exclusive. Routing, indexed search and
transport are nested import detail and must not be added to the parent again.
Validation/elections, allocation and sort work remain within the parent; no
closed full-remesh accounting is claimed here. Existing World transport timing
still omits internal failure-election collectives and is partial. Its reported
bytes/time in the coupled fixture now include donor transport too.

Admission uses saturating caller/resident/scratch/packing/map-node estimates,
house index prediction and intersection scratch bounds. Estimates are deliberately
conservative and do not certify allocator overhead, peak process RSS or hard-OOM
recovery. FrozenField construction/cache storage belongs to the caller's next
admission phase; it is not included merely because Import returned donors.
The service is synchronous and each instance is private to one worker, not a
shared concurrent query service. Broad AABBs can admit irrelevant tetrahedra and
hit the 256-donor limit. Exact geometric filtering is a possible measured next
improvement; increasing that limit is not a hidden recovery policy.
Source metadata currently includes repeated local shared-node records; deduplicating
those is a potential build-cost improvement to measure with realistic inputs.
No speedup, instrumentation-overhead result, MPI scaling limit or affordability
claim follows from these tiny contended correctness checks.

From the repository root, select a new empty folder:

```
python3 integration_evidence/native_3d_core_v1/run_distributed.py /absolute/new/case/folder
```

No Git is required on compute nodes when a source checkout is supplied: the
runner retains full source snapshots there. Serial commands and assert source
are retained in serial_donors/validation.json and serial_donors.cpp.
No cluster launch is requested tonight. The next required work is incremental
candidate selection and repeated distributed metric completion, general planar
operators, weighted tetrahedral ParMETIS, runtime/CFD/history/CGNS integration,
full 2D regression and performance/memory/imbalance qualification. Curved geometry,
robust 3D BL construction, full unsteady adaptation and general affordability
remain later goals.
