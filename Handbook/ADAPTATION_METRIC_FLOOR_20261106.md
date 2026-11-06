# Shared sensor metric regularization — production change and qualification

The side conversation explicitly authorized implementing the metric correction, then
instructed: **do not run ONERA M6 here; assess implementation correctness and hand it
to the main agent.** No ONERA M6 invocation occurred after this source change.

## Problem and resulting behavior

The saved build217 initial metric spends 79.7% of its 50k complexity on 4,621 nodes
with the same isotropic background spacing3.7304, despite HMAX100. In the old shared
ComputeMetric path, a globally normalized Hessian eigenvalue floor1e-16 lets a remote
curvature extreme manufacture this background. It is a metric-generation defect,
not evidence that the saved shock-containing CFD solution was unused.

CSolver::ComputeMetric now:

- Normalizes each point's matrix before decomposition and regularizes nonzero
  absolute eigenvalues relative to their LOCAL maximum, using the existing1e14
  sensor conditioning ratio. It retains the original global scale only as a
  numerical reference that cancels analytically.
- Evaluates determinant powers and per-sensor normalization in logarithms. One
  passive maximum reduction chooses a safe exponent reference, followed by the
  existing compensated sum (or existing active MPI sum in differentiated builds).
- Treats a locally flat sensor as absent. A locally flat point uses the requested
  physical size bounds when other points contain curvature. A wholly flat field
  retains the uniform fallback governed by complexity and physical bounds.
- Preserves nonfinite-Hessian warnings, dimension-dependent Lp exponents, sensor
  intersection, supplied-metric/replay paths, native gradation, physical bounds,
  BL composition and final acceptance gates. No new configuration option, norm,
  complexity target or adaptation operator was introduced.

The change is shared by2D and3D and reaches ordinary, windowed and goal-sensor callers
through the existing ComputeMetric API. Active sum semantics are preserved in source;
this qualification did not build a new AD or single-precision executable.

## Production validation

Build219 completed both SU2_CFD and test_driver in72.1666s, all1301pinned sources
unchanged. Source/test files remained frozen through all following checks.

| Check | Result |
|---|---|
| New production regression, serial | 17,613 assertions /1case PASS |
| Related serial metric checks | 207,605 assertions /40cases PASS |
| One MPI6 metric milestone | All6ranks PASS4cases each |

The new regression calls production ComputeMetric on2D/3D geometries. It checks the
analytic weak/feature metric ratio despite a remote1e13curvature extreme, sensor
rescalings1e200/1e-200, local/global flat sensor sets, rank-deficient spectra,
positive finite metrics, unchanged complexity and serial/distributed agreement.
Related checks include existing goal/symmetry, sharp-corner, boundary-layer and
tensor-gradation behavior. MPI checks also include existing goal, corner and
native tensor-gradation comparisons across partitions. No geometry-empty-rank
fixture was added; globally flat sensors and normal distributed ownership were
exercised. These tests do not establish general BL generation or M6 acceptance.

The first sandbox test could not initialize MPI local sockets and failed BEFORE
test entry. Its failure receipt is preserved; the same pinned check passed with
host execution. All owned build/test tool sessions are terminal. No processes
were signalled or unrelated sources altered.

Qualification authority:
`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/metric_floor219_qualification.json`.
It records source/binary hashes, case receipts, unchanged pins and exact scope.
Build/case evidence is local and the production edits are uncommitted here; the
main agent should preserve only the relevant files in its reviewable checkpoint,
leaving unrelated existing working changes intact.

## Earlier saved-M6 numerical evidence, distinct from production qualification

Small reproducible analysis artifacts are preserved in
`staged57/metric_floor219_analysis/`, especially FINDINGS.md and
graded_comparison.json. Large temporary binaries, Hessian caches and visualization
remain in `/tmp/m6_metric_floor_20261106/` and are not required for source review.

The independent original-metric reconstruction matches every recorded tensor
within7.64e-9relative error and reproduces198gradation sweeps. Its local-floor
candidate with the unchanged L1objective converges at50000.0215(within the existing
1e-6relative tolerance), passes all778330directed-edge gradation checks, reduces
the old farfield region's share79.7%→33.0%, and requests about5–6%chord rather than
11–15%across two sampled shock sections. Eleven compression sections all request
finer resolution. Corner requests also strengthen, potentially making construction
harder. This is earlier standalone numerical evidence, NOT a new production M6 run.

## Main-agent continuation

The shared production change is ready for integration review with its focused
qualification. The main agent must verify this exact build/source/input provenance
before using it. The next full M6 comparison should use the SAME saved CFD seed,
50k Euler target and unchanged gates/budgets. First assess actual production metric
allocation, shock directions and corner demands against this numerical evidence;
then judge actual mesh acceptance, conservative transfer, continued CFD and adapted
disk restart. No such outcome is claimed here. The full Euler/BL/RANS objective
remains unchanged and there are still zero accepted adapted M6 meshes.

## Main-agent integration review and actual production target assessment

Review verified all219source/binary/build and focused-case hashes; all old handles
are terminal/absent. No correction to the reviewed production patch was required.
No AD/single-precision or geometry-empty-rank claim is added. Existing supplied/
replayed metric paths, bounds, sensor intersection, corner/BL composition and
native gradation are retained. Unrelated218diagnostic fixture/helper edits remain
in the worktree and are not part of this metric correction checkpoint.

`staged57/metric_target219_production/` contains the actual production comparison.
L1andL2each run the219CFD binary on6ranks, loading the SAME saved seed and taking
the same one-iteration continuation as full M6. Only ADAP_LOOP=NO and explicit
ADAP_NORM differ from the full configuration; no remeshing occurs. All seven
conserved/primitive fields checked are BITWISE identical to build217and each other.
Final target tensors are read from actual production binary restart outputs.
Independent volume integration and complete directed-edge audits pass.

| Actual graded target | Corrected L1 | Corrected L2 |
|---|---:|---:|
| Complexity |49999.999397|49999.99999995|
| Allocation to the OLD isotropic plateau region |33.02%|3.45%|
| Allocation at radius>20from origin |33.70%|3.51%|
| Median compression-direction spacing across11strips, fraction of local chord |.0437–.0654|.0362–.0603|
| Root trailing-corner allocation, radius.05 |.787%|2.136%|
| Tip trailing-corner allocation, radius.05 |6.438%|13.590%|
| Finest sampled tip normal spacing |4.46e-5|6.64e-6|
| Finest sampled tip tangential spacing |5.83e-5|1.63e-5|
| Maximum metric aspect ratio |38.26|199.62|
| Directed-edge gradation violations /778330 |0|0|

Select **L1**, retaining the original norm while correcting the proven global-floor
defect. All11sampled shock sections improve versus the OLD target. L2is coarser in
4of11sections versus correctedL1and concentrates more budget at extreme recovered
corners. No physical or accepted-mesh evidence yet justifies that stronger demand.
This is an objective selection from current measurements, not a claim L1is universally
better or that farfield curvature is physical. The seed is force-qualified, not
fully residual-converged; sampled compression strips are not shock acceptance gates.

`comparison.json` records corner normal/tangential distributions, actual tensors'
restart hashes, complexity, physical bounds and independent gradation. `selection.json`
records source review, all18terminal handles and the choice. `assessment_pins.json`
freezes actual source/build/runtime/config/seed inputs. `run.py` is resumable and
preserves each completed norm independently. No repeated old-code baseline run.
Next: complete two-event saved-seed M6 at50k/L1with unchanged strict gates/budgets,
verify the emitted initial target matches this production assessment, then pursue
accepted meshes, actual conservative admissible transfer, continued CFD and isolated
adapted disk restart. BL/MPI and RANS500kremain subsequent full obligations.
