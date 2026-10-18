# Compound multi-apex construction experiment

This is saved-state geometry research, **not a native engine implementation or an
accepted adaptation**. The input is the immutable annulus_v7_n1 terminal339-cell
state and its original frozen sensor-plus-BL field. The production run rejected
with four metric edges of length1.8659293031, all incident to a common prescribed-
height apex. Changing that retained ownership is necessary under this field;
whole-target infeasibility does not follow.

The offline operator replaces one complete interior vertex star with two apex
cones and bridge tetrahedra across the separating link edges. Orientation-derived
binary implications prevent inverted bridges. A quality-weighted minimum cut
chooses a feasible partition, followed by explicit constraint checks. Three splits
privately replace the36-cell star by52cells and its one apex by four: the complete
mesh becomes355tetrahedra/107vertices. Physical faces and all other vertices stay
unchanged. Axis/seed choices are experimental fixture inputs, never production
case dispatch. A generic native proposal/selection path is still required.

## What worked

`probe_v4.py` produces two complete candidates. The delta=.0625 candidate passes
`independent_full_mesh.json`: exact positive/total volume, manifold and107vertex
links,7819 exact overlap-pair checks, complete finite-face coverage including the
hole, original first-height/normal-foot requirements, q>=.2167007/J>=.0558188 and
full composed edges<=1.7955165.24/48-order edge agreement is4.44e-16; this empirical
agreement is not a rigorous quadrature bound. Independent complexity20.00019535
is9.77e-6 relative above20 and retains the original profile SHA. No field samples
from the new mesh replace original donors.

`export_witness.py` verifies the audit/candidate/constructor pins, exports
`annulus_frozen_prototype_witness.su2` with far/symmetry/wall markers, and checks
exact binary64 coordinates/connectivity/markers through the existing SU2 reader.
This is an inspectable geometry witness without flow states or a native restart.
The complete exact geometry audit took30.705s locally; the bounded proposal sweep
used.867s. Neither establishes native adaptation performance or affordability.

## What did not work

- `probe.py`/`probe_v1.py`: arbitrary positive partitions clear the four length
  residuals but leave6--12shape failures. Exact skin/volume alone is insufficient.
- `optimize.py`: joint movement of four apices on that topology still leaves
  seven shape failures after110 residual evaluations. Point movement does not
  substitute for coordinated connectivity.
- `probe_v2.py`: greedy hard-quality implication closure finds no proposed valid
  partition. This is a bounded proposal failure, not global infeasibility.
- `probe_v3.py`/`report_v3.json` are **withdrawn numerical results**: SciPy1.10.1
  maximum_flow uses int32 internal flow, and1e15 capacities overflowed. Their
  reported infeasibility is not scientific evidence. Sources/logs are preserved.
- `probe_v4.py` bounds finite totals and capacities below2^31, self-checks a known
  two-node flow, verifies all hard implications after the cut, and passes the
  independent complete mesh audit. Native code must retain corresponding checks.

## Next capability, unchanged gates

Implement bounded compound multi-apex reconstruction in the native complete-star
transaction architecture. Generate proposals from actual wall/height/metric
constraints; keep artificial interfaces fixed, original source ownership and MPI
reservation atomic, and validate the final replacement before publication.
Intermediate private construction is not accepted CFD geometry. Then require the
unchanged full annular native workflow and affected MPI/transfer/restart checks.
No accepted M6, general oblique/junction/exterior/2D capability, viscous accuracy or
affordability has been established by this experiment.
