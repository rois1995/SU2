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

## Actual existing-operator and combined-candidate results

Current229diagnostic36241assertions/1case passed:16stars on4actual contexts;
each interior context4/4useful, both wing-face contexts0/4useful. Complete context,
interfaces, current centroid q/J oracle and geometry/size effects checked. Current230
compound diagnostic:23of24directed collapses cannot construct; the sole collapse
plus optimizer worsens complete counts q2→3/J1→4 and geometry excess. Both existing
surface-shell candidates construct and reduce q/J, but raise geometry excess27–32x.
Their refusal is correct. Passing diagnostics do not imply useful repair power.

Current231tests ONE connected hypothesis: existing shell then existing optimization
inside complete50-cell context (66-cell context excluded, active limit64unchanged).
34927assertions/1case passed;51trials/2boundary moves close q2→0/J1→0, but geometry
excess2.78685e-12→6.63231e-11(~23.8x), geometry-bad4→4. Still inadmissible; no
production closure change integrated from this failed hypothesis. All handles
terminal and pins unchanged. R/current_{residual229,compound230,compound231}_assessment.json.

Eight actual old/raw-shell replacement queries,180assertions/1case, all pins unchanged:
one exposed face per shell has a rigorous outside-support witness125.5107or104.3380µm
versus100µm tolerance, its other new face certifies within. Exact production guidance
reproduces both native shell whole-guidance deltas to relative1.9e-16orbetter.
R/shell_geometry231_assessment.json. This is not optimized-position replay or a
statistical sample of all smoothing costs; no wall-time claim with work trace enabled.
Witnesses concern exact queried support, not arbitrary other bodies/sheets.

The next target for these actual boundary residuals is geometry-conforming coupled
repair power. Faster certification alone cannot admit the demonstrated raw violating
faces. Reconsider missing surface/adjacent-volume construction coordination using
existing facilities; avoid a ladder of local fallbacks, larger supports or raised
limits. No new scheduler/geometry representation or quality-aware construction.
Optional ownership collective-AND correction compiled in229/230/231but MPI behavior
qualification remains pending; do not launch full M6 solely for this guard or fixture.
After a justified connected closure change: affected serial/ONE MPI6 milestone,
then immediately full same-seed correctedL1/50k two-event M6 under unchanged gates.

## Geometry-conforming existing-kernel candidate232

Actual next diagnostic followed the proven need to fit the new surface diagonal:
existing shell→existing reference-projected SplitEdge→existing volume optimizer.
Build232and pinned serial case pass (95.715764s). Complete original50-cell context
becomes53; q2→0/J1→0/geometry4→2, no introduced bad cells, geometry excess falls
89.39%, maximum metric edge unchanged.99optimizer trials,3boundary moves/1edge
removal. R/current_compound232_assessment.json. No new active support or budget.
This is local proof of a connected existing-operator hypothesis, not whole-field
closure or nonlocal publication/height/MPI qualification. Next is production
quality integration with work/ID/geometry-context/edge/height/transient accounting,
affected serial plus ONE MPI6 milestone, then full same-seed L1/50k two-event M6.
