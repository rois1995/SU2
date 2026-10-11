# Native 3D coarsening dependency and rejection investigation

The local experiment confirms a full-star retry gap; the larger frozen event
still rejects. No production policy, target, acceptance gate, default budget,
2D/MMG path, or qualified configured driver was changed.

## Real rejected-collapse reproduction

Folder: `coarsen_dependency_replay_v4`.
`probe.cpp` includes the actual production engine source to access its private
Selection implementation; its geometry, P1 queries and metric checks link the
configured native libraries. This is an isolated local harness, not new runtime
instrumentation or a replacement of the distributed engine.

The bipyramid domain has outer vertices(0,0,0),(1,0,0),(0,1,0),(0,0,1),(1,1,1)
and an interior point(.25,.25,.25). Original five-cell P1 donors are frozen:
M=.25I everywhere except xx=7 at the interior point. Strict shape/sliver/length
gates stay q>=.20,J>=.05,L<=1.8.

| Mesh or proposal | Cells | Minimum q | Minimum J | Maximum original-P1 edge |
| --- | ---: | ---: | ---: | ---: |
| Initial valid mesh | 5 | .226570 | .053119 | 1.375724 |
| Valid mesh after face reconnection | 6 | .250466 | .056442 | 1.375724 |
| Full mesh after newly admissible collapse | 3 | .687230 | .408248 | 1.232168 |
| Original rejected collapse patch | 1 | .255057 | .048766 | .707107 |

The initial collapse is geometrically valid but fails its actual frozen sliver
check. A neighboring face2->3 reconnection changes the interior vertex star
from four to six cells, while the three cells of the candidate edge star and its
coordinates/measurement remain unchanged. The same collapse then passes the
actual geometry and original-P1 metric gates. Selection does not reactivate it.
An unrelated-cell replacement stays blocked; an edge-touching replacement
reactivates it, showing exactly where the current invalidation stops.

`independent_audit.json` checks exact positive volumes, oriented incidence,
pairwise SAT interior disjointness, unchanged skin/coordinates/physical facet
bindings and volume1/2. It independently evaluates the original spatial-P1
metric at centroids and integrates its piecewise-affine edge norm analytically.
All four inspectable grids are exported. The small SU2 exports group the skin
under private_interface; preserved physical facet identities are audited in the
JSON, rather than claiming marker-qualified driver output from this harness.

Earlier experiments are preserved: v1 failed before numerical work because the
harness omitted MPI initialization; v2 is a synthetic blocked-selector PASS;
v3 correctly rejected collapse but failed a harness assumption that a rejected
shape check returns a completed measurement. v4 measures that diagnostic value
separately and passes. These are harness corrections, not relaxed gates.

## Actual frozen n8 replay

Folder: `coarsen_trace_n8_v1`.
An isolated engine object adds exactly three hooks: one include, attempted-star
trace output, and a private mesh export at engine exit. It links the same68
configured test objects/libraries as the qualified driver. The production source,
objects, libraries and driver remain untouched and pinned. The variant has its
own binary hash and is diagnostic evidence only.

`analyze.py` verifies five byte-identical baseline inputs/config/state files,
170 equal integer work/communication counters, and the same failed COMPLETE
assertion after5038 passing assertions. The parent adaptation remains FAIL:

- Refinement:457 accepted edits,3552 private cells.
- Coarsening:512 attempts,272 accepted,240 rejected;2489 private cells,
  zero cached invalid cells,91 pending candidates, zero pending repairs.
- Rejected directions:231 distinct; zero repeats with identical complete-star
  IDs/versions/coordinates/physical bindings; nine repeats after dependency changes.
- Current edge-based invalidation misses120 full-star changes across53 blocked
  directions. This is not proof that those directions became admissible in n8,
  or that adding those retries alone makes the budget sufficient.

`audit_obstructions.py` independently checks the recorded geometric witness for
all183 inversion/degeneracy rejections using exact binary64 rational coordinates:
176 determinants are zero and seven negative. An interior replacement point
makes the determinant positive in every one of the183 controls.56 rejections
would introduce an oversized metric edge, and one leaves an incident finite
reference facet.

The preserved grids `refine_private.su2` (862 nodes/3552 tetrahedra) and
`coarsen_rejected.su2` (590 nodes/2489 tetrahedra) expose private progress for
inspection. Boundary groups are labelled reference_marker_<index>, where the
index refers to marker_names in envelope_s0_initial.json. These grids are not an
accepted CFD replacement, not a CGNS writer qualification, and not independently
certified whole-mesh/field outputs. The original failure and rejected CFD/BDF
state preservation remain intact.

## Implementation direction and remaining gates

The evidence supports dependency-correct retry invalidation and an exact early
orientation filter. It does not support caching identical failures as the main
optimization: no such repeats occur in this trace.

A safe early filter needs only one current surviving-cell nonpositive-volume
witness. It must exclude cells containing both endpoints, because those cells
are deleted by collapse. Missing local witnesses still require the existing
complete-star private transaction. Suppress removal only when the frozen field
is already admissible, preserving rejected-collapse movement/reconnection in
invalid fields. Keep exact orientation predicates; do not use tolerances to turn
zero determinants into positive cells.

Full-star changes must invalidate blocked directions and any cached witness,
including when another owner changes a non-edge cell. Route notifications through
the existing authoritative directory or another bounded admitted protocol;
requeue only affected local candidates, avoid whole-mesh scans, account storage
and communication, and retain atomic publication/rejection. A serial-only queue
change is insufficient.

Then run cross-owner/stale/veto/budget checks on MPI1/2/4, N=M and M<N, the
unchanged frozen larger campaigns and independent accepted-grid/state/output
checks. Evaluate changed mesh trajectories independently. Complete repeated
cluster timing, instrumentation overhead, MPI wait/traffic/memory accounting and
practical size/rank evidence are still required for Goal1. Local elapsed times
are contended correctness diagnostics, not speedup/scaling claims. Goal2 may
start only after the full Goal1 gate, as explicitly authorized by the user.

## Recheck retained evidence

From the repository root, with a single low-priority process:

```bash
PYTHONDONTWRITEBYTECODE=1 nice -n 19 python3 integration_evidence/native_3d_core_v1/coarsen_dependency_replay_v4/audit.py
PYTHONDONTWRITEBYTECODE=1 nice -n 19 python3 integration_evidence/native_3d_core_v1/coarsen_trace_n8_v1/analyze.py
PYTHONDONTWRITEBYTECODE=1 nice -n 19 python3 integration_evidence/native_3d_core_v1/coarsen_trace_n8_v1/audit_obstructions.py
```

The recipes require the matching repository and recorded receipts. New numerical
reruns belong in fresh announced folders; do not overwrite the preserved terminal
receipts. Generated probe objects/executables are disposable scratch and are not
published; sources, compile/link commands, hashes, logs, audits and grids remain.
