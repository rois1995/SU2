# Explicit planar symmetry reference — implementation in progress

Goal3 ACTIVE/incomplete. User priority: qualify declared geometry before another M6 attempt.
Spherical repeat is already qualified and published at fdbc20a92a; reuse its accepted cold evidence.

Current-binary follow-up: baseline_v5_n4 and independently audited workflow_v5_n4
PASS after the shared affine-core/finite-chord change. This retains Cartesian
runtime scope. General arbitrary geometry remains OPEN; see
[the geometry-core evidence](../affine_geometry_core_v1/README.md).

## Scope and geometry decision

Adopt an explicitly declared Cartesian plane during **fresh initial geometry definition**, before
CFD geometry quantities, flow/history initialization, original sensor donors, Hessian or metric freezing.
Move every copy of each shared vertex once, including wall–symmetry junctions. The resulting wall
facets and original volume donors therefore use identical coordinates. This is a declared geometry
change, not approximate facet merging or larger runtime containment tolerances. Original source
geometry and P1 values remain immutable throughout each subsequent remeshing stage.

`ADAP_NATIVE_SYMMETRY_REFERENCE= initial.planes`, default empty (no work or changed behavior).
File format (physical solver units):

```
SU2_NATIVE_SYMMETRY_PLANES 1 1
"symmetry" Y 0 1e-10
```

Records: declared MARKER_SYM name, X/Y/Z, exact plane coordinate, maximum permitted original
deviation. This first implementation deliberately uses exactly representable Cartesian planes.
The configured limit is an input-reference accuracy decision, **not** a relaxed geometry gate.

Collective audit checks finite declaration, true symmetry markers, original deviations, conflicting
junction planes and every affected tetrahedron's orientation. Coordinates publish only after the
common vote. Surface membership exchanges contain boundary IDs, not a gathered volume mesh. Memory
is admitted before preparation. Log reports deviation, moved/incident points, volume ratio and
requested storage. Existing native finite physical references, composed metric, height, quality,
positivity, transfer and output checks remain unchanged.

**Existing states/returned meshes must already lie exactly on the selected planes.** Restart or
return never projects a geometry beneath a frozen field. Fresh adoption rejects a preexisting
.native_ref if coordinates would change; an authority cannot silently become stale. Supporting
retroactive adoption of a different plane beneath an archived off-plane field is outside this
initialization policy: it would require an explicit geometry/field transfer stage. No old M6 field
has been relabeled as the new geometry.

## Acceptance and remaining work

Use a noncoplanar perturbed initial symmetry patch sharing vertices with a viscous wall. Require
complete accepted RANS/current+BDF/SA adaptation, actual-query full composed metric, prescribed
height, independent geometry/state audit, MPI consistency, SU2/CGNS/reference output and new-output
restart. Test deviation/existing-state/storage rejection atomically. Reuse the established budget
workflow fixture rather than introduce a second replacement or transfer harness.

Before M6, also resolve **finite nonconvex symmetry support**: an exactly planar wing symmetry
patch may have holes, whereas the current canonical planar union only certifies connected convex
disks. Declaration alone must not promote such a patch to an unbounded plane. No M6 rerun is
justified until this geometry policy and its finite-domain behavior are qualified.

The input audit helper changes no mesh or field. It reports original deviations, shared regions,
orientation screening with rational fallback and chart topology. It establishes no accepted mesh
or global embedding certificate. See the current results below; earlier build failures and rejected experiments are preserved.

## Results and general-scope limit

Serial cube workflow_v3_n1 and current N2M2/N4M2 workflow_v4_n2/n4 pass actual
RANS/SA transfer/current+BDF/SU2/CGNS/reference/real restart, with independent
complete metric/height/geometry/state audits. MPI4 focused15case reference check
passes. The symmetry error declaration changes initial geometry before states,
not an existing frozen field. The finite exact planar kernel now retains oriented
nonconvex borders/holes; this does not establish arbitrary rounded-plane support.

annulus_v4_n1 preserves the exact rejected154-cell/55-node working mesh and last
edge1--5 proposal, plus original frozen field/profile/reference and source capsule.
It reaches8192work, q=.145498/J=.0490366 for that last trial, no complete mesh or
transfer. Flow/SA retention checks execute before the expected-to-fail completion
assertion. v3 failure is retained separately. No unchanged M6 retry.

feasibility_witness_v1 is independently accepted against the SAME original field:
204tets/84nodes, q.305115/J.095893/L1.739949/height0, exact finite-facet coverage,
positive volume/no overlap/links; reference complexity20.000195. It is a manually
constructed controlled witness, NOT native adaptation, transferred solution or an
M6 feasibility proof. It distinguishes native operator blockage from this target's
feasibility and motivates construction redesign rather than another local fallback.

[General geometry requirements](../../../Handbook/GOAL3_GENERAL_SYMMETRY.md) include
both dimensions, arbitrary rotation/translation, multiple bodies/holes/disconnected
regions and simultaneous junctions, full MPI/donors/persistence/restart and exterior
workflow. These remain OPEN. Contended correctness timings establish no affordability.
