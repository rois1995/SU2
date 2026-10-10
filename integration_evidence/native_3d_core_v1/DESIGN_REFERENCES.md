# Native tetrahedral design: local paper checks — 2026-10-10

Working repository: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch: codex/native-3d-core. The papers are read-only references in
SU2_AdapNoExt/Papers; their paths, SHA256 and reading scope are recorded in
papers_reference.json. No PDFs were copied and that repository was not edited.
No additional paper is needed for the present incidence foundation.

## Cavity construction and its preconditions

[Loseille, Alauzet and Menier (2017)](https://doi.org/10.1016/j.cad.2016.09.008),
Sections 2.2–2.6, recasts insertion, collapse, movement and reconnection as private
cavity replacements. Section 2.5/Algorithm 1 grows through faces that obstruct
positive reconnection and couples surface and volume correction. Section 2.6
argues for work-proportional fronts and data maintenance instead of repeated
whole-mesh scans. These support bounded coordinated edits and explicit dependency
imports rather than a catalogue of isolated edits with unlimited retries.

The validity statement in Section 2.5 belongs to its prescribed cavity/point
reconnection construction on a valid source mesh. It must not become a generic
claim that any collection of positive tetrahedra forms an embedded conforming
replacement. Native still needs exact oriented cavity interfaces, complete
incidence, connected/manifold links, external entity collision checks and
independent geometric embedding audits. Physical surface reconstruction additionally
needs reference-facet coverage, marker/feature preservation and coupled volume
validation. A fixed private patch interface is distinct from fixing the physical
mesh boundary; the latter remains adaptable in Goal 1.

The scripts reference is consistent with useful bounded 2→3/edge-ring candidates,
but its entire fixed-boundary/MMG/serial controller does not satisfy this goal.
No published timing or script objective establishes our shape, size, BL or MPI
acceptance contracts. Larger cavities are escalations to test, not guaranteed fixes.

## Parallelism and work balance

[Tsolakis et al. (2021)](https://doi.org/10.2514/1.J060270), Section II,
compares distinct concurrency strategies. EPIC shifts subdomain elements after
operator passes to balance work and expose previously frozen partition interfaces;
other systems use different cavity synchronization or coarse-grained decomposition.
This makes changing partitions between completed transaction rounds a plausible
option, not a requirement to migrate cells while a transaction is outstanding.
The design choice must follow measured total cost, useful accepted work, interface
progress and communication, including return/transfer to CFD ranks.

Retain the native ownership/transaction model initially. Admit full edge/vertex
stars and every affected neighboring entity before private edits. Never infer a
complete star from two face-adjacent cells or from an arbitrary imported halo.
MPI completeness, conflicts, versions and memory admission remain unimplemented
at this checkpoint; sorted local incidence is their prerequisite, not their proof.

## Independent verification and staged cost

[Galbraith et al. (2020)](https://doi.org/10.2514/1.J058783), scalar-field
and TripleBL sections and conclusions, supports isolating metric construction,
mesh generation, interpolation and numerical solution checks. Analytic scalar and
3D boundary-layer examples are useful manufactured controls; their convergence
results do not validate an unrelated native implementation or every flow regime.

Current checks isolate exact binary64 orientation, SPD admission, constant-query
metric shape/slivers and supplied-cell incidence. Subsequent frozen spatial-metric
reconstruction controls must use independent embedding and transported-target
checks. Actual CFD transfer, curved geometry, BL construction and repeated
unsteady affordability are later staged checks from the authoritative goal file.

The new immutable index sorts 4 face, 6 edge and 4 vertex records per supplied
cell. Build cost is O(k log k), storage O(k), and a star lookup O(log k + s), where
k is admitted cell count and s the returned star size. Build it for bounded private
patches; rebuilding a whole-mesh index for each candidate would violate the cost
intent. FaceComponents reports face connectivity only. Neither it nor oriented
face cancellation establishes vertex-link manifoldness or no geometric overlap.
No performance scaling claim follows from the present tiny tests.
