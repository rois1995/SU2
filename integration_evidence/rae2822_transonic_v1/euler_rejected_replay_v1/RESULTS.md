# Rejected Euler candidate: reproduced and saved

This diagnostic replay uses the old triangulated RANS seed, not the user's
subsequent Euler-type seed. Input, config, initial CFD history, double restart
and ParaView solution are byte-identical to `euler_native_v2`.

The solver reproduced the same 37,737 native commits and 36 quality residuals.
It exited with the intended incomplete-quality diagnostic after 96.454 seconds.
No candidate was accepted and no solution was transferred to the rejected grid.

- [Rejected SU2 grid](mesh_adap_00001_rejected.su2): 5,318 points, 10,256 triangles.
- [ParaView geometry and failure mask](mesh_adap_00001_rejected.vtu): color cells by `MetricQualityFailure`.
- [Failure locations preview](mesh_adap_00001_rejected_locations.png).
- [Raw native failures](mesh_adap_00001_rejected_failures.csv): all 36 cells, exact vertex coordinates and target values.
- [Located cells](mesh_adap_00001_rejected_locations.json): exported cell rows, native IDs, coordinates and nearest boundary.
- [Independent frozen-target audit](independent_rejected_metric_audit.json).

## Locations and severity

34 failures lie on the upper surface near the trailing edge:
`x/c = 0.99623–0.99664`, `y/c = 0.000894–0.000982`.
The other two are on the lower surface, near `(0.66673, -0.02656)` and
`(0.69068, -0.02313)`, with metric qualities about 0.153.

The worst trailing-edge cells are almost flattened against the wall; the
minimum metric quality is `1.88613895e-10`, far below the required 0.18.
Their centroids are roughly `1e-14` from the wall. The original donor's minimum
metric quality was about 0.001637, so adaptation made the worst quality much
worse; these extreme slivers are not simply inherited unchanged from the seed.
The saved state localizes the failure, but does not record which specific
transaction first created each sliver. Operator provenance is not established.

Independent exact orientation and manifold/boundary-conformity checks pass,
and all metric edge lengths meet the 1.8 upper bound (maximum 1.79947126).
The independent P1 audit reproduces exactly the same 36 bad-quality cells.
The separate near-duplicate-point check FAILS: two pairs are separated by only
about 1.6e-11 and 3.1e-11 chord units. No coordinates are exactly duplicated.
Minimum double-computed triangle area is 1.471e-27. These defects are reported,
not hidden by the successful topology checks.

Boundary sampling changed from 192 to 320 airfoil edges and 40 to 60 farfield
edges. The original feature and full boundary coverage are retained; maximum
original-polyline deviation is about 9.22e-7, within the 1e-6 request.
The rejection is an interior shape failure near the wall, not a failed
boundary-reference tolerance.

The new production diagnostics export incomplete native candidates when
`WRT_ADAP_MESH=YES`, naming them `<mesh>_adap_0000N_rejected.su2`, plus a failure
CSV. This debug export uses SU2 format even if accepted output uses CGNS.
It runs before the existing rejection guard; accepted geometry, solution and
reference bindings remain untouched. Forced termination cannot export an
in-memory candidate: the stopped RANS run has no rejected candidate file.

Regression: explicit incompatible height/length target passes on 1, 2 and 4 MPI
ranks, including independent exported connectivity and failure-cell pairing.
The existing eight-cycle accepted-state regression also passes on four ranks
for both transfer methods. No quality target or engine operator was relaxed.
