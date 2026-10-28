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

### Fresh-work replay80 and current baseline

Build79 passes495.961s with all1294 pins unchanged. Replay80 passes compile/link
and three separate fresh-work proposals in16.183s, input/library/source pins
unchanged. Full removed-vertex stars are imported for both directed collapses;
there are no fallbacks:

| Proposal on final captured state | First operator result | Subsequent geometry evidence |
|---|---|---|
| Split16–6496,2→4 cells | Success;1,528,800 operator work; no exhaustion | Bound .00375→.00125; still above requested .001. |
| Collapse6496 into1,6→4 cells | Success;632,017 operator work; no exhaustion | Bound .00375→.01; worsens squared excess9.689e-7→2.082e-5. The geometry gate appropriately refuses this proposal. |
| Collapse1 into6496,3 cells | First failure: physical body/feature preservation;13 operator work; no exhaustion | No output cells. This one rejection does not establish that all possible contractions/reconstructions are impossible. |

The smaller curved CFD baseline was rerun early on build79:
`staged57/cases/mapped_work_trace_20261028T100753_1513891`, terminal FAIL14.180s,
pins unchanged. Flat variants complete; curved variants still exhaust after17
splits/zero collapses. The corrected stop explicitly reports shared atlas work
exhaustion and undetermined feasibility; the exhausted-budget rejected-state
diagnostic also fails, while the accepted input CFD mesh/solution is retained.
No geometry representation replacement is being claimed from that run.

The pending combined clean/legacy MPI6 regression passes177.816s:32 actual
neighboring events with publication, source envelopes and schema8 reload.
All16 per-phase cost comparisons pass:12–20 face pieces,13–19 trace pieces,
56–109 integer bits,12,395,777–40,581,663 work excluding decode and
5,989,288–33,325,592 aggregate admitted bytes. This is scoped repeated-event
proof, not CFD transfer or a general complexity theorem. Stage-stop MPI6 passes
9 cases per rank in15.850s. All23 compiled filter audits pass; the failed sandbox
listing receipt is preserved separately (MPI initialization required host access).

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

## Certificate construction selected for implementation

Use adaptive subdivision of the immutable cubic patches and current physical
triangles, with the existing outward interval/Bernstein arithmetic. This is a
sufficient Hausdorff certificate, independent of face-partition history:

- Let F be a flat triangle paired barycentrically with a cubic piece S, with
  uniform outward deviation bound e. For a physical triangle T,
  T→S <= max_vertex(T) distance(vertex,F)+e. Distance to a finite convex triangle
  is convex; each point of F has its paired point on S within e.
- S→T <= max_control distance(control,T), because the cubic lies inside its
  Bernstein control hull. Include interval widths and realization error.
- If no single witness triangle certifies a piece, subdivide it; different
  children may use different witnesses. Physical subdivision must cover its
  exact original triangle through barycentric domains/outward evaluation, not
  merely rounded midpoint triangles.

Reuse Blossom, CubicDeviation, Range/Box, NormBound, ClosestCurved subdivision,
model ADT candidate queries and SourcePatch hull/separation. An outward finite-
triangle distance-upper-bound kernel remains necessary; ordinary ADT distances
can propose witness points but cannot certify them. First exercise full semantic
body/sheet support to establish completeness; local acceleration must preserve
that completeness proof. This does not grant a patch permission to change its
feature topology or cross another body.

Model approximation must refine deterministically when e consumes the remaining
error margin; a fixed tessellation error would conservatively reject some meshes
that meet the requested tolerance. Zero margin/work refusal remains uncertainty.
Artificial chart seams can require many children even with tiny physical error;
reuse certified planar-component containment where applicable and measure the
curved cost. This design is not yet a demonstrated M6 performance improvement.
No new resource formula or geometry threshold is selected by this review.

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

## Implementation evidence after review

Independent SurfaceUnionBounds/backend query and physical SourcePatch::Tube are
written. Private union81 compiles all3 changed translation units and links; backend
runtime TIMES OUT300s (417.190s whole job), pins unchanged. A separate30s verbose
run locates the stall in the first alternate-diagonal planar square query, before
its result. This confirms the predicted one-witness seam cost, not an unavailable
physical operation. The implementation now reuses FinitePlaneReference to bound
coplanar unions across internal diagonals, with outward plane residual and lifted
projection-shift bounds. This is uncompiled after the correction; no resource cap
increase or production activation is made. Source, publication and migration
remain connected pending work; no full capability claim follows.

## Connected implementation, runtime qualification pending

The code now uses a semantic entity contract for curved production adaptation.
`WithEntityGeometry(ADAP_HAUSD)` migrates validated legacy state, discards smooth
traces/face partitions, retains normalized genuine-curve routes and records the
absolute target. New scalar/body/sheet data and current mesh associations persist
in explicit reference/payload version9; legacy decode is unchanged. Derived
vertex/face indices and geometry certificates rebuild from current state.

The new union query is a tolerance predicate with conservative upper bounds,
not an optimizer for an unnecessarily precise maximum distance. Success records
inside certification or an outside witness; numerical/storage/work uncertainty
refuses distinctly. Existing Bernstein/interval arithmetic, model ADT and finite-
plane union containment are reused. Single-chart barycentric error bounds remain
a cheap disposable one-sided witness, never inherited face-region ownership.

Construction uses entity/normal association checks, native cavity validity and
feature-only reconstruction. Publication retains complete owner participation,
exact directed feature-side and unsigned curve-coverage ledgers, genuine corners,
edge incidence and complete local vertex-link checks using a current derived
index. The final accepted mesh additionally requires a complete two-sided model
certificate. Source envelopes use the same model→frozen-skin bound with physical
triangle tubes; existing shell, contact, donor-volume, fill/loss and transfer
gates remain active. This is still unqualified on the actual curved workflow.

Private union84 passes9 cases/215 assertions; syntax86 passes11 translation
units. Full build86 passed; actual curved workflow87 still fails after53 splits/16 collapses at the unchanged arithmetic ceiling. Valid split proposals were rejected by inherited geometry-progress scoring; build89 removes this veto for entity refinement while retaining final geometry acceptance. Feature scratch accounting was corrected in build87. Build88 scope errors are preserved and corrected in build89. Earlier failed union81/82/83 receipts remain.
Actual movement, repeated-event cost, publication, CFD transfer/BDF history,
restart and M6 remain the acceptance obligations. No caps were raised.


### Productive operator sweeps, build90

Build89 PASS45.156s, pins unchanged. Compiled-filter audit passes23 selections:260 serial/237 MPI/71 atlas, including new entity-v9 and feature-normalization cases. Actual workflow89 FAIL55.134s with unchanged pins: flat variants pass; each curved variant38 splits/6 collapses,1459 rejections (1312 local geometry coarsening vetoes,147 entity-orientation refusals), arithmetic67,207,168 and no quality stage. Removing refinement scoring alone did not deliver an accepted mesh. IDs repeat with evolving volume stars; unchanged physical dependencies are not established. Four captured cells share .0010990956390742213, not every bad cell; symmetry and conservative bounds prevent inferring a plateau or actual distance from that number. Build90 groups productive generations of the same operator before switching split/collapse, using existing finite frontiers/reservations and unchanged gates. The regression now requires the first collapse sweep to follow refinement quiescence; the prior collapse-before-size-completion assertion was an agent-selected scheduling expectation, not a user requirement. Build90 running; sources/helpers pinned.
The operator choice is an implementation decision. Existing exact validity, body/feature, owner publication, two-sided final geometry, metric/BL quality, transfer/history and restart requirements remain. No arithmetic cap or tolerance changed.


### Build90 runtime evidence

Build90 PASS189.258s, source pins unchanged. Workflow90 FAIL72.964s, unchanged pins: each curved variant reaches295 splits/157 collapses and zero cached size/geometry debt, but coarsening verification exhausts67,207,168 arithmetic before quality; Euler recorded2588 rejections/579 final private cells. No accepted mesh/transfer/restart follows. The rejected-state diagnostic itself exhausted the event reservation while evaluating face errors; its complete binary shard is saved, but the text diagnosis is partial. Diagnostics must use fresh independently admitted work. Sweep scheduling resolves the previous early construction stall but does not resolve repeated coarsening verification. No cap or acceptance gate changed. M6 build90 euler_build90_entity_sweeps is now the sole owned heavy job (session57795); do not edit pinned C++/helpers until terminal. Prior mapped handles/processes are verified terminal/gone; the first M6 launch review rejection used stale live-job state and was resolved with process evidence.


### Remaining DC-18 sizing integration and coarsening work

Read-only production inventory at build90: `CurvatureSizeRule` and `CurvatureMetric`
exist in CNativeCurvedReference.hpp, but the latter has no production caller.
CBoundaryLayerMetric.cpp supplies Hausdorff tangential sizing for complete analytic
sphere/cylinder walls; CNativeRemesher3D creates its pointwise composition only
when BL is configured. The cubic backend already exposes principal curvature and
directions, explicitly as local differential data rather than a certified global
curvature bound. Generic cubic/Euler geometry sizing is therefore open, not done.
A shared geometry target must retain ADAP_HAUSD independent of h0, geometry/BL
composition and gradation, normalization order and exact final geometry checks;
adding sampled curvature alone would not certify the surface or complete DC-18.

A rejection cache would leave the target field unchanged but address only repeated
verification. The smallest candidate is a transient directed-current-edge entry
with the exact sorted physical-face identities/facet hints from its fetched star.
BoundaryParameterState explicitly makes point identities immutable within an
publication epoch; movement replaces identities. Such a cache must store only a
deterministic local geometry refusal, compare its complete physical dependency
set, overwrite older signatures, discard retired edges and admit storage. It must
not cache clearance, reservations, metric failures or exhaustion; no persistent
history or rational descriptor graph is needed. First establish repeated identical
physical dependencies and preserve a fresh-work first failure. This remains an
assessment, not implemented or validated behavior, and is not a user requirement.


### Purpose-specific geometry bounds and interior reconstruction, build91

M6 build90 was deliberately stopped after481.694s (solver -15, pins unchanged), before operators. All owned host PIDs are verified gone. A three-second userspace profile (120 samples, no lost samples) places100% of sampled stacks in PrepareMobility→WithEntityGeometry→GeometryCertificates→QuerySurfaceUnion; source inspection shows unconditional dense model approximation for the reverse direction before union queries. The profile does not distinguish that phase from later subdivision. This is a diagnostic interruption, not a native rejection or infeasibility proof. Source enclosures need model→frozen-skin bounds, while final acceptance needs both directions. The connected correction exposes direction in the geometry backend, omits the dense model approximation for source-only queries, and forces an uncached complete two-sided final query. Source-only results never claim within_tolerance. No arithmetic cap or geometric target was raised.

The first mapped90 refusal also exposed that RebuildFeatureTraces rejected empty candidate physical surfaces even for empty interior cavities. The shared helper now permits genuine interior reconstruction while still refusing removal of a physical surface. New focused coverage tests that distinction. Existing neighboring-move facilities now cover four16-event variants: clean/historical legacy and clean/historical→entity, owner publication, frozen source envelopes, schema8/9 reload and descriptor/work/storage receipts. The summary requires every variant/event. Optional transaction observation now captures the first local failed operator with matching private geometry and independently admitted fresh-work replay; final rejected-state diagnostics no longer reuse the exhausted event reservation. All changes await connected build91 and actual workflow/M6 qualification.
Affected callers: backend PatchErrorBounds; BoundaryParameterState migration/source cache; JointAffineCoverage consumes its unchanged forward bound; Engine finish explicitly requests both directions. Reference/payload9 stores the same immutable authority/associations and does not persist query subdivisions. Acceptance: actual curved workflow and M6, first-failure replay, repeated owner-publication/source/reload variants, unchanged final geometry/q/J/size/BL/transfer/history/restart gates. Generic curvature composition remains open.
