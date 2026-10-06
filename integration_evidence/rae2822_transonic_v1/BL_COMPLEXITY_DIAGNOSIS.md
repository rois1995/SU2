# RAE2822 boundary-layer metric complexity

The RANS trial was stopped at the user's request after 851.186 seconds, during
its first remesh. The MPI exit code of 1 records cancellation, not a returned
quality verdict. No adapted RANS mesh was accepted. Its original mesh and initial
solution are preserved in `rans_no_bl_seed_v1`.

## Measured cause

The sensor metric was normalized to complexity 4,000. The BL request was a first
height of `1e-5`, growth ratio 1.2 and thickness 0.02. Adding it produced a nodal
complexity of **824,836.850731**, reproduced from the saved double restart.

**One trailing-edge node accounts for 89.29% of this value.** Input point 148,
at `(1.0, 0.000169999999343418)`, has the isotropic tensor `M = 1e10 I`: both
principal sizes equal `1e-5`. Its coarse median-dual area is
`7.365066389621105e-5`. The contribution is therefore:

`area × sqrt(det(M)) = area / h0² = 736,506.638962`.

The other 3,591 nodes contribute 88,330.211769, still much more than the sensor
budget. The five donor cells incident on the tip are 4542, 4555, 4684, 5044 and
6932 (zero-based input file rows). Together they cover a coarse patch around a
pointwise tiny-size request.

| Measurement | Value |
| --- | ---: |
| Sensor complexity before BL | 4,000 |
| Nodal complexity after BL | 824,836.85 |
| Contribution of the tip node | 736,506.64 |
| Frozen P1 centroid complexity | 1,220,091.96 |
| Native estimate of unit-edge triangles | 2,817,681.68 |

The five largest cell contributions, all near the trailing edge, account for
71.45% of the centroid estimate. Exact coordinates, connectivity and contributions
are in [the numerical diagnosis](bl_complexity_diagnosis_v1.json).
[The hotspot preview](bl_complexity_hotspot.png) shows the coarse seed and its
metric. These are locations of excessive refinement demand, not residual failures.

## Where it enters the implementation

`CBoundaryLayerMetric::Evaluate` limits tangential size near a sharp vertex to
`max(h0, 2*r/turn)`. It sets normal size to
`max(h0, 2*(h0 + (growth-1)*distance)/(growth+1))`.
At the tip, `r = distance = 0`, so both sizes equal `h0`.

`CSolver::ComputeMetric` applies this BL metric after sensor normalization and
bounds. This intentionally preserves the requested wall resolution and allows
complexity to exceed the sensor budget. Its complexity sum weights each nodal
density by the **old, coarse dual area**.

`CNativeRemesher` freezes the resulting nodal tensors as a P1 field. The extreme
corner value then occupies coarse donor cells instead of the small region given
by the geometric distance law. Its centroid-based work estimate is another 48%
above the nodal quadrature.

The sum is consistent with the saved field: there is no duplicate MPI contribution
or wrong point index in this measurement. The failure is representing the intended
rapidly varying BL field on a coarse seed and freezing that representation.
The common metric generation supplies either backend; native's additional work
estimate and its frozen target are separate consequences.

## Minimal experiment

A flat wall isolates the representation error without CFD or corner geometry.
Take one edge with tangential size equal to its length, `h0 = 1e-5`, growth 1.2
and donor depth 0.01. Integrating the geometric normal-size law gives **29.14397**
normal metric-length units. Nodal quadrature gives **502.73632**; integrating the
P1 tensor gives **666.68652**, or **22.88 times** the geometric result.

This is not an integral of the complete RAE2822 sensor-plus-BL field. It demonstrates
why a coarse no-BL mesh is a poor carrier of a thin wall metric.

## Correction to pursue

For native adaptation, freeze the CFD sensor field and evaluate the BL constraint
from geometric wall distance at every new point, quality sample and edge-length
sample. The geometric evaluator already exists. Its parameters and reference
geometry must stay immutable during a remesh, and the target-query protocol and
independent audits must cover the composite field. Changing the work estimate
alone would leave the inflated P1 target intact.

For backends that require nodal metrics, staged BL construction is a practical
alternative: refine to a resolvable intermediate height, recompute on the new mesh,
then progressively reach `1e-5`. Intermediate heights must be explicit, and the
final requested height must still be met and checked. Neither remedy has been
implemented or validated in this investigation.

Reducing `ADAP_COMPLEXITY` or disabling `ADAP_ISO_CORNER` alone does not remove the
separate BL corner-size rule. Silently weakening the final height or accepting an
incomplete grid would conceal the problem.

Reproduce the checks with:

```bash
python3 integration_evidence/investigate_rae2822_bl_metric.py --self-check
python3 integration_evidence/investigate_rae2822_bl_metric.py \
  integration_evidence/rae2822_transonic_v1/rans_no_bl_seed_v1 \
  --output /tmp/rae_bl_diagnosis.json
```
