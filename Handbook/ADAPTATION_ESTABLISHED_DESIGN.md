# Established adaptation foundation — 2026-10-28

## Requirements versus decisions

User requirements are native distributed SU2 integration, validity-first construction
then metric/BL improvement, movable boundaries on fixed geometry, genuine features
and separate bodies, geometric tolerance independent of h0, conservative transfer,
history/restart and practical repeated-event cost. None prescribes rational covers,
smooth paths, source prisms, certificates, schedules or numerical budget formulas.
Current numeric acceptance settings remain comparison baselines, not newly attributed
user requirements. DC-17/DC-18 and the complete curved/M6 workflow remain open.

## Selected foundation and actual source inspection

Use SCOREC/MeshAdapt's classified-cavity and nodal size-field design **inside SU2**.
Retain SU2's operators, owner/version Directory, cubic geometry backend and separate
conservative CFD projection. Do not invoke an external remeshing executable.

Reference checkouts (reference-only; no new dependency):

- SCOREC/core fd4386fddb3543b7dbc9cfe41f98af04a57a3df7: ma/ma.cc 23–44
  coarsen/refine/snap then shape improvement; ma/maSize.cc 365–540 transports
  nodal anisotropic/log metrics; ma/maSolutionTransferHelper.cc 35–120 chooses
  the old cavity parent by maximum barycentric insideness and interpolates fields.
  apf/apfCavityOp.cc 96–136 obtains cavity locality and resolves ownership;
  ma/maCollapse.cc 27–39 requests endpoint cavities. Classification constrains
  geometry. Root/ma licenses are BSD-3-Clause; no source copied at this checkpoint.
- MMG 8ed2259164fa4c90be6301d247ecb1db7bd61228: libmmg3d.c 975;
  mmg3d1.c split/collapse drivers; movpt_3d.c 483/1263 boundary movement;
  delaunay_3d.c 581 anisotropic cavity; bezier_3d.c 326 derives geometry from
  current-mesh normals. No fixed-original projection callback found in public API.
- ParMmg d2eddc502cae46084ca2947859f333d7a7b23943: libparmmg.c 192
  calls local MMG; tag_pmmg.c 39–187 protects/releases artificial interfaces;
  libparmmg1.c repartitions and rebuilds communicators. MMG/ParMmg LGPL-3+.

Paths live under integration_evidence/native_3d_bl_v1/established_adapter_sources_20261028.
Unmodified SCOREC also measures/vetoes collapse quality during construction; that
policy is deliberately not adopted. Its default field transfer is not evidence
of conservative CFD transfer. Its evaluable-model parameter interface would require
a nontrivial adapter for SU2 facet/u/v cubic associations. Implementing its design
in existing SU2 facilities avoids that adapter and preserves native integration.

## Demonstrated limitation and connected implementation

M6 build92 rejects before operators: folded/degenerate joint original-source prism,
face734, 386.112s total. Those prisms contain **sensor metric tensors only**;
they are not the conservative CFD transfer mesh. Requiring a globally conforming
prismatic extension just to measure the metric is an unnecessary coupling.
Captured small curved failure also builds valid cells with fresh work; neither
conservative geometric rejection nor exhaustion establishes impossibility.

Replace metric-support prisms with current-mesh nodal metric transport through
complete old cavities. Preserve surviving node values; interpolate new nodes from
the best old parent. Convex clamped P1 interpolation preserves SPD outside the old
chordal cavity after constrained projection; this interpolation policy is an SU2
implementation decision, not an exact copy of SCOREC log-size interpolation.
Measure replacement cells against their transported nodal field. Keep geometry
validity and final geometry checks separate from metric sampling.

Affected chain: ImportGeometry seeds attributes; OwnedCell/Directory/partition
carry them atomically; engine selection/private measurement/publication transports
them; WorkingWeights, reassessment, normalization and ReaderSlices consume the same
field. CFD conservative projection and history/restart keep independent acceptance.
The .native_ref geometry format need not change for transient metric attributes;
the internal OwnedCell wire record gets an explicit version to reject stale payloads.

**Identified dependency:** existing candidate normalization binds every quadrature
point to original donor IDs and recomputes nonlinear original-family trials. Nodal
transport cannot simply substitute candidate IDs there or leave stale trial stencils.
The connected implementation replaces that agent-selected original-family policy
with bounded uniform scaling of the transported current nodal field. Global
spectral bounds preserve hmin/hmax; uniform scaling preserves tensor anisotropy.
The same prepared samples and BL geometry composition assess trials, and all
working node attributes update together before the second adaptation stage.
Current source stencils are temporary per-stage quadrature data, never ancestry.
No new source-hull fallback or arithmetic-cap increase is planned. Build94 is
pending: this is an implementation decision awaiting integration validation.

## Acceptance

First: small curved Euler and M6 Euler accepted adaptation, final geometry/q/J/size,
conservative state/history transfer and restart under comparable settings. M6 early.
Then requested BL/RANS through general metric/height operators. Also require actual
boundary movement, overlapping neighboring edits, repeated events, MPI publication,
correct selected tests and runtime/memory/current descriptor/work counts across events.
Primitive checks support integration; they do not close this milestone.

## Current implementation checkpoint

Current-nodal field edges use endpoint-linear metric interpolation and existing
analytic/composed integrators; they do not trace global old-donor coverage. Their
point evaluator preserves exact nodal values and uses bounded best-parent convex
interpolation elsewhere. Only metric interpretation changes; no geometry validity
permission follows from this evaluator. Legacy original-field queries remain
strict for separate callers. The region constructor still has a strict stage
snapshot path; BL regional mobility and FIRST_ROW stage-two sensor construction
remain unqualified and must be connected before BL completion.

Normalization core units were verified in CSolver.cpp and NativeRemesher3D.cpp:
core is an eigenvalue and scales by the tensor factor. Preparation errors are
elected before collective trial evaluation. Partition work estimates now use
metric density/volume and edge demand without q/J measurements. The known atlas92
redundant-feature assertion led to a structural canonical-piece count check rather
than another geometric proof. All these changes await build94/runtime evidence.

### Workflow evidence must include actual M6 continuation

The unchanged run_m6.py case is a steady one-event early diagnosis; its acceptance
parser explicitly does not certify transfer/reload. Retain that comparable run,
then derive a separate M6 BDF2 campaign with unchanged mesh/geometry/metric gates:
TIME_DOMAIN=YES, TIME_MARCHING=DUAL_TIME_STEPPING-2ND_ORDER, TIME_STEP=.001,
TIME_ITER=3, INNER_ITER=2, ADAP_FREQ=1; remove steady ADAP_SUBITER/ADAP_FLOW_ITER
controls as required by CConfig. Keep ADAP_SIZES=(500000), CONSERVATIVE and the
same reference. Require both event groups on all six ranks, current and prior-time
conservation/positivity, written mesh/sidecar and matching saved history, then an
actual restart continuation. Existing audit_native_unsteady.py is explicitly 2D
and cannot establish M6 conservation. Reuse the driver/output/transfer facilities;
qualify a 3D independent audit before claiming the full milestone. No such M6
workflow is accepted at this checkpoint.

Build94 actual workflows are terminal with unchanged pins. M6 Euler371.127s:
303359 input tetrahedra; two private edits per rank (12 global work), then an
internal atlas storage refusal on rank4, requested721130288 versus714987896.
Native configured limit remains12288MiB/rank; sampled whole process tree about
3.44GB, largest sampled rank631MB. No work/time exhaustion or accepted mesh.
Import203.69s; short full-stack profile finds86% of sampled cycles in
ImportGeometry→FindFace→ContainsEntityFace→OrientedFace/Bezier Blossom.
Small mapped94 FAIL159.373s: flat Euler/RANS transfer/reload pass; curved variants
295 splits/238 collapses, one quality stage,20 optimizer moves/8 insertions/16 swaps,
609 private edits total,180 cells and33 still invalid at the same67207168 geometry
work cap. First collapse107576←103688 fresh replay builds valid cells with35649
work and1.88e-16 surface bound; actual refusal is size, not geometric impossibility.
Next connected fixes reuse existing exact accepted-face associations for unchanged
entity faces (changed faces retain geometry checks), fix fixture backend setup and
identify the exact geometry operation/scratch in storage refusals. No cap raised.

### Current-stage target and memory contracts, build96 (pending)

A new supplied stage field is authoritative even when input cells already carry
attributes from an earlier adaptation. Exact matching source cells initialize
attributes directly; unmatched cells use bounded collective point sampling once
at stage entry, including empty-rank participation. NativeRemesher supplies current
working snapshots between construction, normalization and AdaptMetric, so corrected
fields are not overwritten by stale event-original data. Operators still transport
current nodal values through complete cavities; no interpolation ancestry persists.
The source field must cover unmatched input nodes; absent data rejects collectively.

M6 build95 identifies QuerySurfaceUnion's actual allocation refusal:14.03MB scratch
against7.89MB remaining in an incidental16MB reservation, while configured per-rank
memory is12GB and sampled process-tree RSS is about3.44GB total. Geometry query
arrays now admit against the existing configured memory pool with their actual
caller/scratch requests. Integer workspace and arithmetic work bounds are unchanged.
This fixes memory-pool accounting after the representation change; it does not
raise the configured cap or address quality stagnation by increasing arithmetic.
All geometry byte peaks feed the native admission report. Build96 qualification
is pending; complete curved/M6/BL workflow acceptance remains open.
