# Read-only source survey of build166 quality blocker

Production source688e13d29a;166test-only correction/qualification00f25cc49e.
Live166sources stay frozen. No edits/builds/tests performed by the survey agent.
Primary retains integrated design and acceptance assessment.

- Engine3D:1111–1123 height_check/geometry_trial and2907–2908 guarded_geometry
  route mandatory actual height/clearance checks and collective validity votes.
  Boundary3D:181–197 CheckSurface uses physical-face containment; Cavity3D:269–285
  ValidatePrescribedInterface checks cavity incidence/topology and expected skin.
- Engine3D:124–139 GeometryGuidanceExcess loops physical faces, calling validated
  SurfaceDeviation and integrating area-weighted squared tolerance excess. These
  are per-face/support fitting bounds, not repeated full-event union certificates.
- Engine3D:3280–3315 normal quality Repair delays fitting until metric admission,
  then2920–2945 geometric_change can veto a shape-debt improvement if fitting grows.
  GeometryRepair requires strict fitting progress through its separate path.
- Engine3D:5025–5026 SmoothBoundary also computes fitting for candidate targets;
  immutable old fitting is lazy once per invocation, fresh fitting repeats for
  improving targets. Projection/MoveVertex, actual height and edge checks precede it.
- Engine3D:1280–1288 finish performs final full two-sided GeometryCertificates;
  final height checks follow. No repeated full two-sided certificate was found in
  each normal quality proposal/commit. PlanarReference SurfaceDeviation reaches
  SurfaceErrorForValidatedFace (BoundaryParameters3D:1185–1188); its internal union
  query names must not be confused with full-event final certification.
- Engine3D:2373–2377 outer selection services a sparse repair feature. Existing
  OptimizeMetricMesh/MeshOptimizer::Run can sweep all supplied Owned rows, but
  OptimizedRepairPatch:813–828 supplies one imported cavity, boundary=false, with
  bounded replacements. No distributed complete-owned-star quality sweep call
  exists. VertexStarSizes supplies incidence counts, not quality measurements.

Evidence:quality_window1_assessment.json. Total q/J debt falls only.783% in760.286s;
52k bad cells remain. Geometry query deltas are highly imbalanced. These findings
support reassessing coordinated whole-field quality/fitting work after the frozen
attempt is terminal; no gate removal, new fallback or implementation is authorized
by this note itself. Existing final validity/geometry/height/q/J gates stay strict.

## Existing complete-star route and finite service

Selection::AddRepairs (Engine3D:650–678) seeds one-vertex, edge and face features
from bad cells; demands are coalesced with user counts. Best(repairing):302–308
chooses oldest(epoch,feature). Service/Touch:370–390 retires/requeues or blocks
features and reactivates changed users; repair_epoch advances per admitted round.
The one-vertex direct route ALREADY imports the complete distributed vertex star:
Directory::LookupBatch atEngine:2827 routes by vertex rendezvous; Distributed3D:
571–640 gathers all incident rows, Fetch:735–790 verifies identity/version/incidence.
Large stars>128 use coordinate_only and explicit allowance atEngine:4189–4190.
Do not add a duplicate complete-star importer or claim direct motion is truncated.

Direct regular repair tries three nodal/metric-center movement steps atEngine:
3119–3149; streamed counterpart4166–4195. ConstrainMovePoint preserves actual wall
constraints, MoveVertex (Boundary3D:2910–2995) projects/remaps prescribed support,
checks chart containment and positive orientation, reconstructs boundary, and checks
complete coordinate-star topology (3026–3032) or regular Finish. Engine then applies
height/metric/debt/fitting gates. This route differs from optimizer cavity smoothing.

Concurrent repair/dependency election already exists (Engine:2630+). Final commit
checks row footprints/versions4230–4242, reserves final footprint4243–4260, publishes
versioned removals/additions4330+. Large coordinate moves preserve cell IDs/increment
versions4283–4308; Clearance receives its coordinate-star feature4217–4222. Existing
clean_quality and separate size/geometry demand passes2431–2456 provide finite pass
service/closure. Reuse these transaction/pass facilities for any next connected fix;
no source edits or implementation decision made while166is live.
