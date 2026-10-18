# Saved annular surface/volume coordination experiment

Full Goal3 and the general symmetry requirements remain active. This is the exact
last50-cell patch from `planar_symmetry_reference_v1/annulus_v4_n1`, whose complete
native cold construction exhausted8192 work without an accepted mesh. No M6 or
CFD rerun is part of the placement experiment.

## Evidence that changes the construction direction

The last failed reconstruction replaced two interior support points with one
background apex. The patch contains no wall faces. Two smooth symmetry vertices
and both interior points have complete global stars inside this same admitted
patch. Enlarging it is therefore unnecessary to expose these particular freedoms.

`probe.py` performs a bounded floating placement search with inherited topology and
one surface split (50->54 tetrahedra). Its `interior_only` label means only the old
interior points move; the inserted point also has two in-plane freedoms. It does
**not** represent the old native edge-constrained placement. `probe_on_edge.py`
records that restricted comparison separately; no previous result is overwritten.

| Placement freedoms | Minimum q / J | Native-form changed-edge squared excess reduction |
|---|---|---|
| On-edge insertion + two interior points | .180305 / .040198 | Shape fails; no acceptance claim |
| Off-edge insertion + two interior points | .201933 / .050489 |77.24% |
| Off-edge insertion + two interior + two symmetry points | .201996 / .050491 |86.97% |

`audit.py` reuses independent exact-rational geometry helpers. Both off-edge
proposals pass positive volume, exact volume equality, oriented manifold skin,
unchanged artificial interface, complete moved stars, finite physical footprint
coverage (including the hole), and no overlap inside/outside the replaced patch.
1531/1479 broad-phase-surviving pairs were checked exactly. Full edge integrals use
actual single-wall composition;24/48-order agreement is at most6.7e-16 here. This
is empirical agreement, not a proven integration-error bound. Unchanged oversized
edges remain: patch refinement progress does not establish whole-mesh acceptance.

An initial audit completed its geometry calculations but failed serializing a
relative provenance path. `audit_v1_failed.log` preserves it; the corrected receipt
is `independent_audit.json`. This was an audit-output failure, not a geometry pass
substituted for native acceptance.

## Connected production experiment and current validation

The joint placement path now uses the existing coupled metric-shape derivatives
for the inserted point and admitted surface/interior vertices, rebuilding every
trial from the original complete stars. The existing bounded connectivity repair
and all final geometry/height/full-field acceptance gates remain. Smooth physical
vertices enter the existing MPI complete-star cost selection; features with multiple
canonical authorities are not automatically freed. No solver labels or saved
coordinates dispatch production construction; coordinates occur only in replay data.
The old shape/edge coordinate-direction ladder is replaced, not extended.

`patch.dat` retains exact binary64 saved coordinates, reference groups, original
coarse source mesh and the independently placed witness. `run_native.py` uses the
existing resumable runner for native frozen-field witness validation, actual native
construction, incomplete-star/atomic rejection and affected geometry/height controls.
Native saved-patch and current Cartesian MPI lifecycle checks now pass. The complete
unchanged annular native construction still rejects; see the results below.
An independently feasible patch does not prove a complete accepted mesh, arbitrary
geometry, aerodynamic readiness, solution transfer, restart or affordable adaptation.

## Independent code review and build history

User-authorized local Claude Code review used exactly `claude-opus-5-5`, medium
effort, self-contained prompt, no tools/edits. The initial high-effort request hit
its240s deadline without a report; a malformed empty MCP schema failed before a
model request. Those failures are retained. `claude_review.json` preserves the
returned review and source-checked dispositions; raw streams remain local.

The confirmed premature shape-only stopping issue is corrected: the bounded
coupled solve includes edge-proxy rows and the same weighted soft-shape merit as
its trial score. Every trial requeries the original frozen field; full composed
integrals still determine acceptance. Edge tensors are held only within a proposal
linearization, not frozen as a replacement field. This review predates that final
correction and is not validation of the final code.

`build_v1.log` was deliberately stopped after discovering a test-fixture constructor
mismatch; completed production objects were retained. `build_v2.log` was stopped
before applying the reviewed merit correction. `build_v3.log` is the completed successful incremental
continuation (session99935 exit0). These are explicit development stops, not failed mesh experiments.

## Completed qualification and remaining construction blockage

`native_serial_v1`:7cases/18743assertions PASS. Native placement takes12trials and
four linearizations. `independent_patch.json` proves exact complete-star/finite
geometry/embedding, q=.2018085/J=.0502886 and44.56% changed-edge squared-excess
reduction. The original independent oracle remains useful but native placement
is not numerically identical to its77%/87% proposals. The audit receipt's generic
limitation that native field validation is needed is satisfied separately by this
native replay; it does not itself constitute a native full-mesh pass.

`planar_symmetry_reference_v1/workflow_v7_n4` passes actual N4/M2 RANS/SA/BDF,
conservation, full geometry/height/metric, SU2+CGNS persistence and actual restart;
`independent_symmetry.json` separately verifies the complete compatibility lifecycle.

`annulus_v7_n1` still rejects with96private edits,339cells/104nodes and stagnation
at2300/8192work. `terminal_residuals.json` independently locates four edges of
length1.8659293. `shared_height_fan.json` identifies the retained common first-height
apex obstruction; `background_kernel.json` diagnoses the single-background limit.
The historical154-cell run has60raw numerically oversized edges versus its native
59count; an edge within1e-12 of the threshold means these counts are not directly
identical acceptance decisions. The four current residuals are3.66% oversized.

The new [compound construction experiment](fan_split_v1/README.md) now yields a
complete independently accepted frozen geometry witness355cells/107nodes,
q=.2167007/J=.0558188/Lmax1.7955165/height0. **It is not implemented in native code**,
and has no adaptation/transfer/MPI/restart qualification. All earlier failures are
preserved; the overflowing min-cut draft is explicitly withdrawn.

The next production capability is bounded multi-apex complete-star replacement,
not another retry/cap increase. Full Goal3/general symmetry/2D/exterior and physical
accuracy/performance requirements remain open. See
[focused handbook](../../../Handbook/GOAL3_ANNULAR_COORDINATION.md).

`manifest_pending_2b48.json` preserves the original unvalidated checkpoint. Current
pins/status are in `manifest.json`. Original executed audit source is retained as
`audit_executed_v1.py`; old runtime source capsules retain the exact runner sources.
Runner edits require a fresh result version rather than falsely reusing old pins.
Native/independent output-path diagnostic failures are retained and distinguished
from scientific construction rejection.
