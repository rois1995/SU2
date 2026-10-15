# Goal3 — coordinated metric-quality patch optimization

Repository `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`, branch
`codex/native-3d-bl`, followingcbd9066b6a. Full Goal3 ACTIVE and incomplete.

## Current connected implementation

Native private repair now has a constrained coupled metric-quality step before
its existing retirement/reconnection/movement fallback. It uses analytic tetrahedron
q/J geometry derivatives, six actual-centroid metric sensitivity samples per active
cell, metric-orthonormal height-tangent variables, sparse matrix-free damped normal-
equation CG, joint normal-foot projection and actual-field backtracking. No new
runtime dependency or config option. The field remains immutable original-P1 sensor
plus geometric BL evaluated at every actual point; no nodal target averaging,
coarse spreading, fade change or hard-normal reset.

The1% interior q/J aim is proposal-only. Actual q .20/J .05, geometry, altitude,
full composed lengths, changed-edge progress, original IDs/dependencies and atomic
publication gates remain. Fixed skin is private-patch scope; physical boundaries
continue to adapt through the existing geometric edits. The same256 private trials,
64/128 cell bounds and global work/time ceiling remain. Additional1MiB worst-case
solver workspace is explicitly admitted. Linearizations, sensitivity requests,
linear iterations and line trials are separate counters.

## Native evidence

- Constant/spatial anisotropic Jacobian finite differences pass470 assertions;
  five existing geometry/movement/height/interface repair controls pass58.
- Actual worst-cylinder50-cell reconstruction reaches85 cells:15 trials,
  10 linearizations,540 probes,116 CG iterations; no flips or retirement.
  Actual shape deficit reaches0, q .202018793/J .050247126, prescribed height and
  complete frozen changed-edge Refine progress pass. Independent exact local
  geometry/no overlap/normal feet passes (six wall faces, height error3.05e-15).
  This is an admissible private replacement, not complete mesh generation.
- Published scheduling plus the optimizer takes the same1402-cell frozen target
  to1984 cells,171 accepted edits/110 insertions,25 oversized edges, excess
  .041918515 (published baseline1832/38/.065325585). It rejects at the existing
  300-second kernel ceiling,3985/4096 work. No speedup or equal-runtime comparison.
  Counts:224 linearizations/7740 probes/1674 CG iterations/892 solver trials.
- Entire1984-cell independent geometry/closed incidence/no overlap (67222 pair
  candidates),98 exact feet/height (relative5.41e-15) and exported-tensor q/J
  arithmetic pass. Minimum q .200019435/J .050003463. Full composed edge diagnostics
  remain native measurements;25 lengths still reject. Remaining groups13 cylinder,
  12 cap/cylinder junction, none interior, maximum1.948298940.
- Current bounded MPI2 protocol passes512 work/1744 rejected cells. Independent
  original donors/reference preservation passes. This is not accepted distributed
  reconstruction, M<N/currentMPI4/current+BDF/SA/CFD/restart qualification.

Centroid-offset diagnostics are saved in `metric_alignment.json`: maximum tangent
offset/altitude is1.523 on the local patch and3.762 on the full result (previous
full triangulation2.215). Minimum unit-cylinder smooth clearance/altitude is .99493
on the full result. These changed-triangulation diagnostics do not certify column
orthogonality, a complete layer stack or viscous accuracy; those remain open.

Read `wall_reconstruction_v1/coupled_metric_summary.json` for receipt/artifact pins.
Primitive/direct-patch evidence uses the initial connected binary. The final engine
revision restores published scheduling; only its object/archive/binary (and the
intentionally different input) differ. Tested constructor/metric/field/test objects
are identical. Final full-target/MPI2/residual stages match current C++/binary pins.
No unqualified blanket binary reuse claim.

## Preserved rejected alternatives

The separate wall-front/scheduling study is on `codex/native-3d-wall-front-study`,
43d32f9b0bc6726b642bb65889d15fd14ba53ad1. Its failed native work and source snapshots
remain [published](https://github.com/rois1995/SU2/tree/43d32f9b0bc6726b642bb65889d15fd14ba53ad1).
It regresses38→58 edges. The first coupled-optimizer run also enabled eager wall
repair and regressed58 edges/excess .102835044; that current-study receipt/folder
is archived as `curved_metric_complete_previous_*`. Eager scheduling is withdrawn;
current engine scheduling matches the published baseline. No gates were relaxed.

The user's `MeshAdaptation/Scripts/FunScripts/MetricOptimize3D_Fun.py` provided
algorithmic reference for quality derivatives/spatial sensitivity. Its nodal-tensor
sampling and acceptance differ; neither was copied into native acceptance.

## Next actual blocker and full scope

All remaining oversized edges are physical. Diagnose their wall-child/transition
construction, point placement and complete-star admission on the actual residual.
The first strategy's hint may suppress another admitted construction, but simply
trying everything early already failed; retain measured strategy cost and primary
size versus quality progress in the next connected design. A valid private patch
does not establish all wall targets feasible. An elapsed/work exit does not prove
operator stagnation or infeasibility. Do not repeat unchanged-budget continuations
or grow toy/limit ladders. Add phase/progress records before the next meaningful run.

Full Goal3 still requires complete cold curved construction/removal, wider fields/
features/interacting layers and quadrature/gradation limits, currentMPI1/2/4 N=M/
M<N conservative current+BDF/SA transfer, SU2/CGNS/reference/restart, actual small
RANS/practical M6 and viscous accuracy, and complete accepted adaptation/CFD cost.
No claim of arbitrary aspect/curvature/gap feasibility or mixed-volume/layer stacks.
All owned jobs terminal. Current final binary matches the isolated engine source;
verify provenance before reuse. Follow the permanent grouped validation cadence.
