# Goal3 — failed cap strategy and construction reassessment

Full Goal3 remains active/incomplete. Qualified production baseline is
69833b2a3fc0e502d93f44e50aa360e3bbf1a1f9. Evidence root:
`integration_evidence/native_3d_bl_v1/tangential_caps_v1`.

## Actual result and decision

The connected private2D cap-operator replacement compiled and linked, but the
strong native planar target `interacting_walls_v1/native_prism_tangential_v1`
FAILS:3731 surface attempts,30 private cap faces, projected maximum length3.095056,
reported work3777/4096,34.34s nested cap time. It cannot admit further transition
construction within the remaining work. No complete3D replacement exists. The
returned/rejected SU2 grid is the unchanged six-cell input; it is not a snapshot
of the private cap. No private cap snapshot was captured.

The attempt did not deliver the required milestone. Its production changes were
withdrawn after verifying all50 failed-case artifacts and exact experimental
source snapshots. At c103aa9cf3 five production files were byte-identical to the qualified
baseline; the subsequent connected construction is described in
[the whole-cap book](GOAL3_WHOLE_CAP_CONSTRUCTION.md). The complete patch, original sources, logs, receipt and negative grids
remain preserved. Stronger tangent-field tests and their independent oracle stay.
Read `rejected_strategy.json` for exact hashes and scope.

The prior local plan to tune redistribution/progress, count more operator variants
and rerun was stopped. A separate constant-spacing arithmetic replay exhibits4465
representable redistributions with only6.7e-12 final drift and zero size deficit.
This demonstrates available nonprogress work, not attribution of the3731 native
attempts. It does not justify another local recovery campaign.

At withdrawal the built test/solver binaries still represented the archived
experiment. Later builds/results belong to their own pinned receipts; do not
qualify restored production from the experimental binaries.
No fresh MPI/CFD campaign or performance assessment was run for this decision.

## Feasibility evidence and its limits

The independent midpoint-bisection/sorted-ID extrusion reproduction uses the same
finite unit-box cap, h.1/g1.2/T.3/fade/core4. Constant original-P1 tangent256:4
rotated30degrees gives Mxx193/Mxy109.11920087683926/Myy67/Mzz4. Inherited bisection
produces480 cap triangles/5760 tetrahedra, q.125646/J.023470, beyond4096 and shape
gates. Axis-aligned256:4 instead gives2592 cells/q.256462/J.073818. These are
offline diagnoses, not native acceptance or native work accounting. The2D acute
transformed-corner bound.468478 does not establish global3D feasibility.

An independent metric-space lattice/Delaunay proposal passes1248 tetrahedra:
q.357910936/J.153522937/full L1.699959306. Exact finite embedding/no overlap,
manifold/every vertex link/volume1 and height.1 pass. All full edges use independent
numerical quadrature, whose indicators are not rigorous enclosures. Matrix
coefficients differ by about one ulp from the native trigonometry-free expression.
This supports feasibility of the mathematical planar target, not an exact native
candidate or native work/time proof. The first792-cell proposal fails a full edge;
both negative and successful proposals/helpers/logs remain.

## Reconsidered construction direction

The missing capability is to choose point distribution and compatible connectivity
for a complete constrained cap/front region, rather than depend on inherited edges
and a succession of local repairs. The next design must establish the following
before another production integration:

- Distribute proposed points from actual local metric directions and finite
  geometry; reconnect the region jointly. Reuse robust native predicates and
  validators. The offline SciPy witness is a design reference, not a new SU2
  dependency or coordinates to embed in production.
- Judge cap proposals through the full3D wall/front/transition replacement. A good
  2D quality or spacing score alone does not establish tetrahedral shape, complete
  full-metric edge closure, prescribed altitude or geometry conformity.
- Preserve corners/side authority and consistent shared front connectivity. Keep
  original-P1 donors and actual-query BL/finer/fade unchanged. A cap-wide tensor
  envelope can overresolve localized/rotating demands; it is not a demonstrated
  general construction strategy or permission to spread wall tensors in the target.
- Establish a complete candidate on the known difficult target and a meaningfully
  varying/rotating original-P1 field before promoting a new native constructor.
  Preserve the same work/time/memory ceilings and rejection/state guarantees.

This planar stress can reject a proposed mechanism early, but it cannot substitute
for the next curved milestone: a complete accepted curved whole mesh with independent
geometry/no-gap/no-overlap/height/shape/full composed-edge checks under the unchanged
constraints. The earlier accepted2877-cell cylinder baseline retains its scoped
proof; no wider curved-feature/nonprismatic/exterior capability follows.

No additional local fallback, controller retry ladder, relaxed gate, new scenario
mode or budget increase is authorized by this reassessment. The remaining full
Goal3 MPI/transfer/physical accuracy/gradation/lifecycle gates remain open.
