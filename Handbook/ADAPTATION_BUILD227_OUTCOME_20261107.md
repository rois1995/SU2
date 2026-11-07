# Build227 terminal outcome and next diagnostic

Goal remains active: zero accepted adapted M6 meshes. The same saved real CFD
seed, corrected L1 metric at complexity50000, two events and strict unchanged
geometry/q/J/edge/height gates were used. No full-workflow success is established.

Evidence root: `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
Attempt: `m6_runs/euler_build227_witness_real_cfd50k` (A227).

## Natural rejection; evidence preserved before stopping the wrapper

Native first event naturally exhausted its time budget at5335.996158s, round37022.
Final working mesh1017523cells, final census flags all false. Unknown final counts
are never interpreted as passing zeros. Last complete census36992 at5303.549478s:
1017224cells,2691q-bad,1295J-bad,1700geometry-bad,0size-bad. Last complete q-bad
location:355surface/2336interior; J:241surface/1054interior. Interior classification
means no physical face, not necessarily far from boundary vertices. Of3729bad-union
cells,3727have an edge below0.9 and3557below0.5; correlation does not establish a
construction cause. Stuck-shape31, queue44427, blocked features512. Shape never
closed; geometry-only closure has not yet been established.

All30owned/reference/rejection capture files complete and stable, all6measurement
caches incomplete. After sealed capture, SIGTERM sent ONCE through verified pidfd
to owned launcher1929998/start4973666. All10owned solver handles absent;1519source/
runtime/input pins unchanged. Wrapper terminal1 after5824.165090s. See capture_ready,
first_failure_prefix,owned_stop_once,terminal_handles,terminal_source_receipt JSON.
Never repeat this historical stop. Original-grid post-rejection CFD output does
not establish adapted transfer or continued adapted CFD.

## Actual selected rejected mesh, with its metric

A227/inspection/rejected_mesh.vtu and rejected_mesh.su2 contain198364points and
1017523tetrahedra. The full captured nodal tensor is present on every point,
9row-major components. Surface extraction retains that same volume tensor; no
separate surface tensor is fabricated. Unknown q/J/geometry remain NaN and
cache_valid0. Full mesh+metric complete, classifications incomplete. EMPTY bad/cavity
views reflect unavailable classification, not zero bad cells. Reference complete.

Export terminal0,362.480822s, helper/raw inputs unchanged;3owned handles absent.
Independent meshio5.3.5 full-mesh/reference/SU2 reader checks pass, all point tensors
finite/symmetric/SPD, exact full VTU/SU2 coordinate agreement and expected counts.
Empty views checked structurally, including zero-byte binary payload headers;
meshio rejects empty ASCII FieldData. Checker-only failed attempts retained in
inspection_validation*_status.json. Reader4 passed in2.755319s. No ParaView GUI
qualification; diagnostic SU2 is not an accepted solver restart artifact.

## Comparison limits and next action

Witness screening saved17.6%whole-construction traversal; all76matching groups
and selected operation/preparation vectors match225. First19quality windows match
through35776. Then deadline-dependent migration differs legitimately:225skips,
227admits. Later work is not identical. At quality window8, net q removal32854,
quality1285.0197s versus1742.1607s, observed25.5669versus18.8582q/s; host contention,
cache effects and later ownership divergence prevent isolated timing attribution.
Use complete whole-field q/J counts and geometry counts, not commit rate alone.

Recover final centroid q/J diagnostics with EXISTING native Measure/EvaluateCavitySample
on sealed captured cells before selecting actual residual cavities. Replace the
old diagnostic's historical fixed count with exact shard/header completeness;
keep this separate from geometry/metric-edge/final acceptance certification.
Then assess poor placement, constrained motion, connectivity limitations and
certification refusals using existing coupled patch construction/admission.
No larger-cavity/operator/scheduler/geometry representation or quality-aware
construction policy is selected merely from literature. One connected hypothesis
must prove full-patch plus neighbor net q/J improvement and geometry/size/height
preservation; unrelated alternatives go to backlog.

Also correct the source-grounded optional ownership admission risk: rank-local
wall/ownership timers can yield differing proceed decisions before repartition
collectives. No actual split was observed; use existing collective facilities,
no new config/scheduler. Qualify within the same connected milestone, then full
same-seed L1/50k two-event M6 with unchanged5400native budget. Shape closure must
be followed by geometry/size closure and shape reclosure, transfer/admissibility,
continued CFD and actual isolated adapted disk restart. Then BL/MPI and RANS500k.

## Recovered actual final-cell diagnostics

Build228 diagnostic-only integration passed; captured census3052623assertions/1case,
all1017523records consumed with exact headers/end and frozen input hashes. It
recovers2368q-bad/1101J-bad, union3258. Explicit shape diagnostic bad/cavity VTUs
with metric saved, complete141392one-node-neighbor cells, exact raw ID/version join
and independent readback. Geometry remains unknown. Final shape location differs
from complete36992census, as expected after additional edits.

Actual classification from physical SU2 triangles and node incidence:
1414bad cells touch wing vertices without a physical face;1341are free interior;
452have wing faces;39other physical faces;12touch other boundary vertices.
This refines the earlier face-only interior label. Among free interior residuals,
400have at least one vertex star above the EXISTING64-cell active bound; among
wing-touching interior429, wing-face98. Do not increase bounds from this statistic.
Representative bounded contexts selected for existing-operator analysis; no new
repair strategy or quality-aware construction policy is established yet.
