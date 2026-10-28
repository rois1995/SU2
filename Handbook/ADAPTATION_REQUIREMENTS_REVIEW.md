# Adaptation requirements and algorithm review

Current decision, 2026-10-28. DC-17/DC-18 are open. This review supersedes
attributions in older agent-written notes; those notes remain historical evidence.
No complete curved CFD or M6 adaptation/transfer/restart is accepted.

## Provenance

| Status | Requirement or setting | Evidence and consequence |
|---|---|---|
| Explicit user requirement | Robust native distributed adaptation; valid construction followed by metric/BL quality improvement | Current user messages and AGENTS.md. Construction omits unnecessary q/J measurements and ranking; final quality remains mandatory. |
| Explicit user requirement | Boundary mobility on prescribed immutable geometry, genuine features and body separation | Boundary coordinates need not remain fixed. Geometry error is an absolute configured length, independent of h0. |
| Explicit user requirement | Correct publication, transfer, history and restart | Acceptance requires a complete workflow, including repeated adaptation; primitive tests are insufficient. |
| Explicit user requirement | Cost governed by current geometry/mesh complexity; CAD readiness | Avoid retained history and speculative infrastructure. Resource caps may change with measured justification, not to conceal repeated failure. |
| Inherited numerical baseline; user provenance unconfirmed | q>=.20, J>=.05, metric edge<=1.8, h0 within 10% | Existing code/configuration defaults, not proven direct user selections. Preserve these settings for equivalent comparisons; this review does not authorize weakening acceptance. |
| Selected campaign setting; user provenance unconfirmed | M6 target metric complexity 500,000; native 1800s, wrapper 2400s | Complexity is not a tetrahedron-count ceiling. Time limits are campaign controls, not geometric requirements. |
| Agent-selected implementation | Work=max(4096,64*input cells), atlas work=(input cells+work limit)*16384, 16 MiB atlas scratch, candidate count8 | Admission/scheduling choices. Arithmetic units are implementation operations, not cell quality or validity. Exhaustion reports incomplete work. |
| Agent-selected implementation | Rational coverage, prescribed smooth-edge traces, harmonic reconstruction, exact old/new chart-region equality | A representation and its associated proof obligations; these are not user-imposed invariants. |

Numeric origins inspected: Mesh3D defaults at commit191f35fd58c; metric edge
at f5e94f46624; first-height defaults at91acfc00762; work formula atf782ddc4b20;
atlas admission at23037c0345e/61bf6cc4c14; campaign helper at519a4813a2c.
Git authorship and repeated agent documentation do not establish user provenance.

## Mechanisms and practical alternatives

| Mechanism | What is actually needed | Assessment and decision |
|---|---|---|
| Rational face coverage | Geometry-error and source-support checks without holes, double cover or wrong-body assignment | Exact rational arithmetic is useful for topology/intersections. Permanent per-working-face ownership of original chart regions is not a physical requirement. Replace inherited partitions with correspondence derived from current patch and fixed model; keep existing validators until an equivalent replacement passes. |
| Smooth shared-edge contracts | Adjacent physical faces must share one edge and genuine feature routes must agree | Current contracts coordinate model parameterizations but overconstrain smooth edges. Reconstruct the entire affected incident-face/owner closure together. Keep persistent curve identity/oriented intervals for genuine features; ordinary smooth mesh edges need no permanent model path. |
| Reconstruction certificates | Valid topology, orientation, separation and geometry tolerance | Keep these obligations. Harmonic charts, exact equality of old/new chart regions and a particular triangulation are proof techniques, not required outputs. Prefer current-patch topology plus conservative geometry bounds against fixed entity/support unions. Projection of nodes alone is insufficient. |
| Candidate searches | Find admissible movement/edits, then improve metric/BL quality | Standard local operators remain suitable. The many native search variants are custom and must justify their cost. Preserve the first failure before fallback changes its reason; separate geometric refusal, numerical failure and work denial. No new local fallback is selected here. |
| Split/collapse scheduling | Progress toward requested metric and geometry, followed by quality repair | Alternating length passes is established; whole-frontier rejection reset after any productive edit is custom. Failed unchanged cavities must not be repeatedly certified. Any reusable rejection must track its complete dependency scope; global-contact failures cannot be cached as local facts. Feature IDs alone are insufficient. |
| Source reconstruction | Conservative search envelopes for the physical donor mesh and boundary-domain accounting | Transfer kernels integrate straight physical simplices/control volumes, not cubic-reference Jacobians. Our current source-envelope implementation depends on curved correspondence, but transfer mathematics does not require its permanent per-face ownership. Replace that dependency together with geometry checks, never simply delete it. |

[Gmsh](https://www.gmsh.info/doc/texinfo/gmsh.html) documents model entities,
node parameters and geometry evaluation; these support persistent entity
association distinct from mesh topology. Its discrete geometry is not exact CAD.
[CGAL remeshing](https://doc.cgal.org/latest/PMP_Remeshing/group__PMP__local__remeshing__grp.html)
combines local remeshing, relaxation and reference projection with explicit patch
and constraint handling. This is useful operator/association design, not a native
MPI/BL/transfer implementation.
[Geometry Central common subdivision](https://geometry-central.net/surface/intrinsic_triangulations/common_subdivision/)
is an overlay between two triangulations. It illustrates derived correspondence;
it does not supply a drop-in cubic-geometry or distributed volume replacement.
Reuse native predicates, spatial indices, Bernstein bounds, projection, Directory
transactions and conservative kernels before adding a dependency. No new library
integration is justified by this review.

## What the captured failure proves

Trace73 has productive splits through split17. A subsequent zero-success collapse
pass consumes8,160,949 arithmetic operations (56,519,137 to64,680,086), then the
next split reaches67,207,168. The final capture has no explicit stored face covers
or traces: it does not demonstrate a remaining retained-subdivision recurrence.
Current reconstruction receives const prescribed traces and does not publish its
microtriangle knots as future paths. Historical build36 growth remains real, but
must not be presented as an identified current feedback loop.

Fresh-work actual SplitEdge16–6496 on the final captured state succeeds:2→4
positive cells;1,528,800 operator work; geometry bound .00375→.00125 and weighted
squared excess9.4542612e-7→3.9078593e-9. The bound still exceeds .001. Replay76
also distinguishes deliberate100,000-work refusal from numerical/query errors.
This is a fresh final-state proposal, not replay of the exact earlier transaction,
not a complete accepted mesh, and not proof that every demanded operation exists.
Its successful first operator has no failure to preserve. Exact first-failure
capture for an unsuccessful fresh-work transaction remains an explicit obligation.

## Connected design and affected callers

Select fixed-model entity association plus coordinated current-patch replacement.
Retain the physical mesh, current associations, genuine feature/body topology,
existing cubic authority and native distributed transaction/transfer facilities.
Smooth correspondence is derived verification data, not authoritative history.
Do not create an independent reconstruction engine or format framework.

1. Geometry checks must cover both model→working patch and working patch→model,
   restricted to the correct body/sheet and genuine curves. Reuse cubic subdivision
   and control-hull bounds; inability to certify is an unresolved query, not proof
   of impossibility. Patch topology and separation remain independent checks.
2. Rebuild the complete affected incident-face/edge closure from current mesh and
   fixed model. Discard obsolete interior subdivisions; replace all affected owners
   atomically. Ordinary smooth edges share physical endpoints/connectivity, not
   inherited paths. Real curve branches retain unambiguous association.
3. Derive source envelopes from the frozen physical skin plus fixed-model support
   with conservative coverage/separation bounds. Keep current conservative volume,
   fill/loss, finite overlap and history checks. Changed physical domain volumes
   require explicit accounting; constant preservation alone is insufficient.
4. Publish geometry associations with the same owner versions as physical edits.
   Restart stores authority, current mesh, entity associations and feature policy;
   derived overlays are rebuilt. Legacy descriptors require validated migration,
   not silent omission. Keep old decoding until migration is qualified.
5. Scheduling must invalidate local rejection facts when their dependencies change;
   remote/global-clearance refusals remain scoped to their actual observations.
   Do not replace correctness with a permanent blacklist or global reset loop.

Affected code: GeometryBackend3D::RegionErrorBounds (currently exposes native
FaceCoverage), MappedFacetReference::{SurfaceError,PrepareSourcePatch,
ValidateCoverageReplacement,JointAffineCoverage}; RemapAtlasSurface and
RebuildWorkingSurface; BoundaryParameterState::{Prepare,Commit,FromPayload};
Directory publication/version/ChangedVertices; Engine selection and source freeze;
ReferenceIO schema decoding; SU2 native import/remesher and conservative-transfer
callers. Tests must exercise the same paths, including compiled filter inventory.

The missing primitive is a conservative current-patch/fixed-support certificate
that also supplies source bounds without requiring an inherited face partition.
Implement and exercise this at the existing backend boundary before removing
coverage consumers. CAD remains the existing small evaluation/projection/support
interface: cubic now, optional OCCT STEP later, then faceted STL and IGES. Do not
pretend STL defines an exact smooth CAD surface.

## Acceptance and implementation order

First preserve baseline/failure evidence and complete pending typed-work-stop
qualification. Then implement the geometry/source certificate and coordinated
association/publication/persistence replacement as one connected capability;
scheduling changes must use demonstrated dependency/cost evidence. Do not treat
a scheduler speedup as completion of the geometry design.

Run the captured curved CFD workflow early under equivalent geometry, quality,
height and transfer conditions; preserve its first unsuccessful operator with
fresh work. Run M6 early at the connected milestone, not after a chain of toy
successes. Require actual movement, repeated overlapping neighboring edits,
publication, physical transfer, BDF history and SU2/CGNS restart. Record descriptor
counts, arithmetic work, admitted memory, sampled process-tree RSS and runtime by
event. A budget sweep may diagnose cost only if inputs/gates remain comparable;
a larger cap alone is not acceptance evidence. Completion requires accepted
adaptation/transfer/restart, not this design review or primitive tests.
