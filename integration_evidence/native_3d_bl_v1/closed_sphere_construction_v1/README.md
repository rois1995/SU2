# Complete closed-sphere native BL construction/removal

Full Goal3 ACTIVE/incomplete. NativeIntegrated, branch codex/native-3d-bl.
[Current design, scope and limits](../../../Handbook/GOAL3_CLOSED_SPHERE.md).
Production changes are generic full-sphere surface sampling and regional coarsening
of admissible meshes with short metric edges. No new config or testcase/solver-
transition mode. Original P1 donors/actual-query composition/fade/geometry/height/
quality/full-field/transfer gates remain unchanged.

## Inspectable grids

- `sphere.su2` + sidecar: original8-tet mesh and existing explicit full-sphere authority.
- `geodesic_v1/accepted.su2` + sidecar: native-accepted5440-tet BL mesh.
- `release_regional_v1/accepted.su2` + sidecar: native-accepted320-tet sensor-only mesh.
- `proposal_rejected.su2` and `budget_6144_v1/proposal_rejected.su2`: actual incomplete
 3493/5540-tet private proposals, with UNFINISHED_PRIVATE_INTERFACE explicitly labeled.
 They are viewing artifacts, not simulation/restart inputs or accepted authority updates.
- `release_v1/rejected.su2`: rejected local-collapse working mesh4205tets. Its accepted
 input remains unchanged. Exact `.dat` records preserve all proposal coordinates/IDs.

No CFD solution or converged RANS data is fabricated. Driver initialization is used
for original-source import; no CFD timestep, Hessian or normalization is rerun.

## Measured result

| Stage | Cells | Work | Native seconds | Status |
|---|---:|---:|---:|---|
| Inherited planner4096 |8retained|4097|2.961|atomic work rejection|
| Inherited planner6144 |8retained|6145|8.666|atomic work rejection|
| Geodesic construction6144 |5440|5604|202.612|complete|
| Original local release, remaining540 |4205private|540|7.919|atomic work rejection|
| Regional release, same remaining540 |320|483|57.384|complete|

First attempt's exact current-constructor band/planning floor4188 exceeds4096;
`work_admission.json` explains the single6144 comparison. The strategy change is
then compared under that SAME6144budget and unchanged physical gates. It changes
surface448->320faces and604->162planning trials. It is not a series of raised limits.
A rejected private append may report one attempted operation past its ceiling; it
never publishes. Neither failure proves physical infeasibility.

Combined saved-state native work6087<=6144, engine times259.996<=300. Native
construction qmin.39823288/Jmin.10043711, full edge max1.64204404<=1.8. Independent
exact geometry/height/shape audits pass; all6726composed edges are native evidence.
The release audit also checks every sensor tensor against the unchanged identity
field and every edge against its independent Euclidean value. Source records agree
exactly across both stages. Source snapshots are not fitted onto the adapted mesh.

The work/time accounting excludes unrelated driver initialization and independent
audit CPU. This is a saved-state mesh workflow, not an in-memory CFD/transfer event.
The202.6second construction and57.4second release do not establish affordability.
Release validation is explicitly50.3seconds (~87.7%); finer phase attribution is
pending. Direct small-source seed paths bypass region-service wrapper counters;
zeros there do not mean zero metric evaluations.

## Qualification and preservation

`qualification_v2` and `primitive_independent_v2.json` cover final-source serial and
MPI4 controls and15existing actual primitive Euler lifecycles/30events: N1M1 and
N4M1/2/3/4, geometry/source metrics/current+BDF/domain ledger/transport/restart and
SU2+CGNS. These small cases are not MPI generation/transfer of the5440-cell BL sphere.
`qualification_v1`/`primitive_independent.json` retain sampler-only results.

The construction's exact geometry audit resumes after180seconds without repeating
native construction. Its original helper is `audit_sphere_construct_executed.py`;
`audit_sphere.py` generalizes the same checks for the later release. Sources,
build/archive/binary/config/reference hashes, executed source capsules and raw logs
are preserved. `current_qualification_scope.md` describes retained construction
scope after the coarsening-only engine change. No202second duplicate construction.

`failed_build_v1_*`/`failed_build_v2_*` preserve two helper compile errors (missing
explicit reference-I/O include and serializer mutable-copy convention). The
historical-auditor import error is retained as `failed_audit_import.*`; only its
required exact helper is subsequently loaded, without invoking its old CLI.
`release_v1/failed_setup_v1` preserves a direct-replay setup error: the no-BL path
must explicitly load the archived reference before import. No production geometry
or mesh policy changed to fix that setup. The cell-count guard review is recorded
in `coarsening_review.md`, before its final qualification.

One heavy job at a time, nice19, build-j1, libraries1, local MPI<=4. Native/analysis
logs retain resource observations. Failed debugger attachment is recorded and no
kernel/ptrace settings were changed. This correctness run is not clean profiling.
Binaries are retained losslessly compressed locally, excluded from Git. Internal
symlink targets are tracked; no link points outside this repository.

Pending: final full-event affordability, MPI BL sphere/cold-update/CFD transfer,
wider anisotropic/curved/interacting/exterior geometry, independent composed-field
oracle/gradation and viscous convergence. M6's exact-facet obstruction remains
separate and unchanged. This is a finite complete curved construction/removal
capability, not full Goal3 completion.
