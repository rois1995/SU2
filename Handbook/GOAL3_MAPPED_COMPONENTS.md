# Finite mapped components — paused checkpoint, 2026-10-20

Worktree `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`, branch
`codex/native-3d-bl`. Goal3 is **PAUSED at the user's request**, incomplete.
All owned build/test handles were terminal at the pause. This documentation update
starts no implementation, build, test, remeshing or CFD work.
[Full scope](GOAL3_GENERAL_SYMMETRY.md); [requirement audit](../integration_evidence/native_3d_bl_v1/GOAL3_REQUIREMENTS.json).
[Recorded results and receipt hashes](../integration_evidence/native_3d_bl_v1/mapped_component_geometry_v1/STATUS_2026_10_20.json).

## Implementation and repository state

Published implementation checkpoints: `283587a6d1`, then audit draft `feb83a3f68`.
The subsequent index-memory correction, tangent/cross-hole fixture correction,
validation helper changes and new evidence remain local; documentation publication
must not be mistaken for publication of those C++ changes.

Explicit native_ref v6 releases only the immutable reference's certified planar
triangulation seams. Default v5 remains facet-local. Prepared finite topology,
exact ideal segment/face containment, oriented physical-border chains, actual-face
orientation and bounded mapping displacement preserve holes and junctions.
Private movement, common atomic publication, policy agreement in MPI payloads,
reader/runtime fixture activation and v6 persistence are connected in source.
Components with at most16 original facets use bounded scans. Larger components
use house ADTs with admitted preparation, retained workspace and result buffers.
This avoids a fixed tree workspace for every tiny patch; no speedup is established.

The BL factory recognizes facades sharing the exact immutable mapped authority.
Original sensor-only P1 interpolation, actual-query BL composition, finer sensor
demands, fade, and strict original-donor/configuration eligibility remain.
Faceted BL geometry comes from immutable original facets, not accepted-point tables.
No correction stage ran in the annulus, so repeated-stage profile reuse is not
runtime-qualified here. Opus5.5/medium review is design input, not validation.

Original physical border vertices remain protected. Mixed analytic/mapped joints,
affine BL walls, general physical source envelopes and complete2D/exterior/body
workflows remain unsupported or unverified as recorded in the full scope.

## Build and focused results

The official resumed build passed in708.69s, pinned C++ unchanged. Executable SHA-256:
`501393b67053e865b5c9f88c57295e72abaaed38cde049b21e480beee99eb3c1`.
Sampled descendant RSS sum reached2.53GiB, with at least38.12GiB host available;
these are samples, not guaranteed process-tree peaks. Compiler scratch stayed on4TB.
The initial missing-include failure and deliberate source-audit stop are preserved
in `build_missing_line_v1` and `source_audit_stop_v1`. No full regression was run.

- [Exact rational oracle](../integration_evidence/native_3d_bl_v1/mapped_component_geometry_v1/finite_predicate_oracle_v1/result.json):468samples/2340checks pass against the actual compiled object. Centroid-side,
  open-triangle segment entry, projected segment membership, plane-side and realized
  normal arithmetic cover tangencies, subnormal/extreme scales and all axes.
- [Finite domains](../integration_evidence/native_3d_bl_v1/mapped_component_geometry_v1/domains_n1/run.log):3cases/146assertions pass, including three chart axes, two holes, a notch,
  disconnected footprints, seam release, v5/v6 persistence and resource refusal.
- [Atomic publication](../integration_evidence/native_3d_bl_v1/mapped_component_geometry_v1/publication_n1/run.log):1case/76assertions pass across v5/v6 policies, including refusal/state preservation.
- [Combined component selection](../integration_evidence/native_3d_bl_v1/mapped_component_geometry_v1/components_n1/run.log) fails at the new corner assertion, line411.
  It calls tangent-only `ConstrainMovePoint` with a bottom-face patch and expects
  full-corner fixation. Complete dependencies and final geometry are caller duties.
  The preceding cross-seam `MoveVertex` checks pass, but the whole case remains FAIL.
  Correcting the fixture to supply the complete corner context is pending; no
  production corner defect is established. Preserve this failed receipt.

Caller audit found that coordinated boundary movement uses `ProjectMovePoint`;
connectivity repair fixes its boundary. A missing-binding defect was investigated
but not established. Do not add a binding fallback on that suspicion.

The independent uniform-output audit remains an unexecuted draft. It qualifies
only exact planar components and explicitly refuses declared affine footprints.
New v6 MPI, N=M/M<N, complete transfer/real restart and exterior readiness were
not run and are not established by the focused controls.

## Difficult oblique annulus: integration initializes, construction rejects

[Case folder](../integration_evidence/native_3d_bl_v1/mapped_component_geometry_v1/annulus_n1/)
uses the same target, geometry/height/quality/full-metric gates and8192work/120s
native limits. Complexity initializes at19.999579 for budget20, heuristic uncertainty
indicator0.00219995. Normalization remains integration-limited; the indicator is
not a proven bound.3,625,296density evaluations were recorded. The separate composed
transport audit still has18violations/max4.57957; no wall-tensor spreading/reset.

Construction stops on **elapsed time**, not the work ceiling:7399/8192work,
13private edits,75private tets/31nodes,68remaining edge tasks and67cached invalid
cells.6440joint-wall trials yield0accepted joint edits; wall reconstruction70trials/
0accepted. No regional-core/front reconstruction or normalization correction ran.
The last saved candidate has q0.1988447689 below0.2, J0.0695926908 and cell16825.
These are recorded native diagnostics, not an independent candidate audit.

Native event120.206s/63.450CPU seconds; refinement117.635s, reconstruction111.063s,
metric validation2.598s and publication0.0174s. Refine CPU61.609s includes57.331s
private reconstruction and1.383s private metric checks. Complete test149.118s;
quadrature21.838s. These are contended rejected-case observations, not a controlled
performance comparison or accepted-event affordability result. The different stop
condition and topology preclude a simple speed/progress comparison with v5.

Accepted CFD mesh/solution were retained. Preserve the exact private state and trial:

- [Rejected state](../integration_evidence/native_3d_bl_v1/mapped_component_geometry_v1/annulus_n1/affine_annulus_bl_budget_20_m1_rejected_rank0.json)
  and adjacent `.bin`, matching v6 `.native_ref` and reason `.txt`.
- [Trial diagnosis](../integration_evidence/native_3d_bl_v1/mapped_component_geometry_v1/annulus_n1/affine_annulus_bl_budget_20_m1_rejected_rank0_trial.json),
  before/after patch JSON and their forward-parameter binary payloads.

The rejected adapted state is currently JSON/binary; SU2 conversion is pending.
The `.su2` files in this folder are the input and initially adopted mesh.
No new accepted adapted BL grid, transfer or restart follows from this attempt.

## Resume boundary and remaining work

Work remains paused. On explicit resume, first correct the incomplete corner
fixture and inspect/replay the exact annular candidate to determine the missing
construction or coordination capability. Seam release alone did not complete this
run. Time exhaustion under contention proves neither infeasibility nor stagnation;
retain that distinction before changing strategy. Do not add another local fallback,
bypass a construction-class guard, raise limits or weaken q/height/geometry gates
without evidence addressing the actual blockage.

The accepted historical Cartesian annulus also had0regional-core/front edits but
77joint trials/3accepted: regional exclusion is a gap, not an established cause.
Retain the full arbitrary2D lines/3D planes, finite holes/disconnected/multi-body
regions, simultaneous junctions, Euler/RANS, N/M, transfer/persistence/real restart,
exterior workflow, uncertainty/gradation, viscous and complete-event cost requirements.
[Prior v5 lifecycle and accepted grids](GOAL3_MAPPED_PARAMETERS.md).
