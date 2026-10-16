# Goal 3 — current implementation and qualification

Repository `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`, branch
`codex/native-3d-bl`. Goal3 remains ACTIVE and incomplete. The full objective and
acceptance gates are preserved in [the requirement audit](../integration_evidence/native_3d_bl_v1/GOAL3_REQUIREMENTS.json).

## Current interacting-wall extension

The new [joined prismatic fronts](GOAL3_INTERACTING_WALLS.md) construct the full
overlapping two-wall target on MPI1/2/4, independently1920 cells/work2309/q.467316/
J.193160/L1.668063, with exact20-slab/first-height/normal-edge coverage. Direct
removal accepts1/2 ranks;4 ranks rejects a496-cell state at proposal work4084,
even though independent geometry/source/full metric gates pass. Preserve that
native failure and audit completion semantics before another operator fallback.
Actual current-source curved RANS N4/M2 v13 passes; actual prismatic backend/CFD,
rotated tangential fields, wider junctions and full performance remain unqualified.
All owned jobs are terminal. Full Goal3 remains active.

## Current changed-source qualification

The complete high-ratio curved native mesh now accepts2877 cells/eight fronts/
work3771, q.223149/J.060674/full L1.783149 under unchanged h.01/T.2/length.2 and
4096/300s limits. Independent exact geometry/embedding/height/source and all2877
composed tensors/all4727 full metric edges pass. Rotated and varying serial
fields independently accept2448/2284 cells under their unchanged targets.
The strategy coarsens the terminal front and rebuilds incident transition/core
cells together; it replaces the structurally blocked fixed840-cells-per-band path.
[Current construction, evidence and limits](GOAL3_COARSENED_METRIC_FRONTS.md).

The prior15-case MPI integration/six-way actual RANS campaign is complete in its
recorded pre-front source scope. Changed-source high-ratio MPI2/4 and representative actual RANS N4/M2 now
pass independent geometry/source and actual workflow audits respectively.
The RANS lifecycle is24->621->48 with current+BDF/open-ledger, explicitly
replayed SA-floor accounting, SU2/CGNS/reference and exact restart. All owned
jobs are terminal; see HANDOFF_Codex.md for current state. No general
geometry, converged viscous accuracy, parallel generation or lifecycle affordability
claim follows. All full Goal3 gates remain intact.

## Recorded pre-front/core foundations

The complete original1402-cell curved target is now accepted by the native engine on
MPI1/2/4 under unchanged constraints:1368 cells,0 long edges, qmin>=.21137/Jmin>=.05284,
full length max1.795195. Independent serial exact geometry/height/embedding/tensor
arithmetic passes. [Construction/limits](GOAL3_REGION_CONSTRUCTION.md) and
[native provenance/results](../integration_evidence/native_3d_bl_v1/region_construction_v1/native_summary.json).

This replaces the previously blocked local-repair path for admitted visible radial
regions. The rejected local recovery remains separately archived. Construction is
currently one bounded region on one worker, with MPI source transport/versioned
publication. Rank outputs have different admissible triangulations/placements; do
not claim bitwise deterministic meshes or transported-metric qualification.

The subsequent cold/removal milestone now passes MPI1/2/4: original24-cell cylinder
->1368 BL ->68 sensor-only cells, shared work3132/3172/3252<=4096 and300s limit.
Independent serial BL and all3 released geometry/constant-P1 checks pass. Original
difficult-target acceptance is preserved; its mesh/ref bytes match the prior exact
audit. [Cold/removal scope](GOAL3_COLD_CONSTRUCTION.md) and
[provenance](../integration_evidence/native_3d_bl_v1/region_construction_v1/cold_summary.json).

The subsequent distributed-field milestone passes serial/MPI2/4: actual1368-cell
mesh ->3072 original donors ->72 sensor-only cells, refresh ->288 original donors
->1368 accepted BL cells with affine sensors `diag(1+.02*z)`. Target residual<=1.67e-14;
rebuild qmin>=.33135/Jmin>=.09572/full Lmax1.795195. Geometric BL is composed at each
actual point query; memoized points retain sensors only. Exact segment imports avoid
a whole-source gather. See [field service scope/evidence](GOAL3_DISTRIBUTED_REGION_FIELD.md).

Full Goal3 remains open. The actual curved RANS/current+BDF/transfer/restart
milestone below is qualified only for its recorded old constructor. The current
stronger-field construction strategy and its distinct-field checks are described
in [the metric-directed front/core handbook](GOAL3_METRIC_REGION_CORE.md).
Interacting/exterior regions and high-ratio multiple fronts remain open. The source is no longer globally limited to256 donors, but each
query footprint and resident cache still are. Geometry remains one visible radial/caps
region on one worker. No CFD transfer, lifecycle speedup or scalability claim follows.

## Actual curved workflow qualification

The full actual short RANS/SA lifecycle passes N/M1/1,2/1,2/2,4/1,4/2,4/4 with
24 ->1368 BL ->68..74 sensor-only cells. Actual CFD/pressure metrics, replacement,
current/BDF/SA transfer, SU2/CGNS/reference reload and real restart pass; each output
has independently checked exact geometry/height/embedding, physical flow/open ledger
and flow/SA restart. Original v5 failure remains; separate reference raw projection
plus solver-floor replay explains its SA changes at every node. The corrected test
checks the documented accounting with stricter field scales; production unchanged.
Post-clipping SA is not claimed exactly conservative. The pressure sensor clips to
the coarse bound. See [full evidence/limits](GOAL3_CURVED_RANS_RUNTIME.md).

The next construction limitation is proved: one core kernel cannot satisfy the1.8
edge limit for constant axial sensor16 over unit span. A metric-driven core with
several interior regions/shared interfaces now accepts the complete constant target
in serial with independent geometry/height/embedding and pure-core metric audit.
The varying mesh also independently accepts against exact frozen P1, including
its declared finite collar. The new shared edge controller passes11 focused cases/129 assertions with6688
microscopic calls (same10000 gate). A shared AxisFrame closest/projection fix now also accepts the complete rotated
stronger target, with independent exact geometry/height/links/P1 and pure-core
metric checks.12 focused cases/1014 assertions pass. Updated MPI/workflow checks
remain pending. See the front/core handbook for current source/evidence scope. No wall
fallback ladder, target relaxation or goal narrowing. Current actual-flow full-edge
independent quadrature, stronger fields, general geometry, accuracy/performance and
other full Goal3 gates remain open. The live job state is in HANDOFF_Codex.md; older workflow jobs are terminal.

The matching practical M6 input is located at
`/media/rausa/4TB/SU2_Versions/TestCasesForSSTIDDES/optimization_rans/steady_oneram6`,
with545438 tetrahedra/96252 points and `solution_flow.csv`. Do not run it through the
single visible-radial-region pilot as if a wing/exterior constructor were qualified.
This earlier frozen-metric input remains separate. The user-requested practical
M6 run now uses `/media/rausa/4TB/Verification/OneraM6/Meshes/Coarse.su2`,
303359 tetrahedra/61330 points. Its actual120-step four-rank RANS/SA QLS retry
fails thin-BL complexity quadrature at65536 points on original global cell13,
before remeshing. A shared missing QLS workspace was fixed first; the matching
flow/restart and cell-neighborhood diagnostic are saved. No M6 adapted grid.
All owned M6 jobs are terminal. Keep true symmetry, actual SA physics and matching
coordinates. See [current M6 evidence](GOAL3_ONERA_M6.md).

## Existing qualified scope

- One actual-query geometric BL policy is shared by weights/refine/coarsen/thin
  complexity. Sensor-only original-P1 donors, finer demand, fade and retained
  geometry are preserved; returned nodal metrics contain only the sensor.
- Recorded stronger planar construction/removal independently passes MPI1/2/4:
  591 BL cells to252/250/243 sensor-only cells; identical BL geometry/connectivity,
  qmin .213044, Jmin .050377, Lmax1.798735, exact first altitude and contained feet.
  Maximum centroid tilt1.05409 is a declared limit, not an orthogonal layer stack.
  Read [the reconstruction handbook](GOAL3_RESOLUTION_BACKTRACKING.md).
- Prior initialized RANS/SA current+BDF/transfer/restart checks independently pass
  N/M1/1,2/1,2/2. The matching four-rank campaign was interrupted; preceding complete
  matrices are historical evidence. SU2/CGNS/reference reload and actual restart
  were checked on the recorded planar field. No CFD convergence/viscous accuracy.
- Focused curved actual-query and single-sphere hidden-volume oracles pass. The
  interacting plane/hidden-sphere quadrature still exhausts its work limit.
- Sensor gradation and composed-field gradation are separate. The composed fade
  violates the tested sensor HGRAD transport bound. Preserve localization/finer
  requests; do not infer combined gradation or spread wall tensors.

## Remaining full-goal gates

Extend curved cold construction/removal beyond the qualified radial/constant and mild-affine sensor pilots; stronger distinct fields;
features/junctions/interacting layers and thin-volume quadrature; declared first-row
alignment and coverage beyond necessary feet; compatible composed-gradation limits;
current connected MPI1/2/4 N=M/M<N with conservative current+BDF/SA transfer, strict
bounds, geometry/full lengths, SU2/CGNS/reference persistence and real restart;
actual small RANS and practical M6 comparison with viscous accuracy references;
complete accepted adaptation/CFD lifecycle cost, memory/work/imbalance and affordability.
Partial/graph/mixed analytic components remain unsupported. Full physical unsteady3D,
general CAD, adjoints and mixed-volume/structured-layer guarantees remain separate.

Keep these gates intact. Goal4 starts after full Goal3 completion. SST/transition
and non-ideal-gas questions are [saved for later](FUTURE_PHYSICS_TRANSFER_QUESTIONS.md).
No cluster submission or external messaging is authorized by this handoff.

Earlier details and exact handoff bytes are retained in
[history](History/INDEX.md), [curved BL](GOAL3_CURVED_BL.md),
[interior placement](GOAL3_INTERIOR_PLACEMENT.md), and
[assessment reconciliation](STATIC_ASSESSMENT_2.md). Historical ACTIVE/job notes
must not be read as current execution state; check the actual process/session.
