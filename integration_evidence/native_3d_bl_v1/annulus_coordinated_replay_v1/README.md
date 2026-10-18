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

## Connected production experiment and pending validation

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
Native results, a complete annular lifecycle and MPI checks are still pending.
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
before applying the reviewed merit correction. `build_v3.log` is the incremental
continuation. These are explicit development stops, not failed mesh experiments.
