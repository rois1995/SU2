# Strict exact-facet resolution diagnosis

This connected change detects a sufficient binary64 subdivision obstruction on a
selected composed-field physical crease, before further private reconstruction.
It preserves the opt-in BL profile, original sensor-only P1 donors, actual-query
composition, fade, geometry, first height, positivity, quality, full-field and
transfer gates. It does not make the rejected M6 mesh acceptable.

## What changed

`Binary64SegmentFreedom` reuses the existing fixed-width exact coordinate integers.
For each varying coordinate, the finest binary64 lattice within the endpoint range
bounds admissible interior positions. A chord-coefficient GCD of one proves no
interior point; GCD two permits only the midpoint, which is checked for exact
representability. Wider-than-64-bit coefficients and other GCDs return Unresolved,
without a feasibility claim. Finite distinct endpoints are required. Tests cover
adjacent doubles, subnormals, zero crossings, binades, wide spans and invalid inputs.

The engine applies this certificate only to a selected oversized edge whose two
physical canonical charts are distinct exact planes with no analytic authority.
A primitive crease reports the obstruction directly. For a midpoint-only crease,
both child edges use the actual frozen complete composed metric; an oversized
irreducible child triggers the existing collective failure election. The accepted
input remains unchanged. Metric measurements retain native quadrature accuracy;
only the coordinate-lattice result is an exact arithmetic certificate.

The diagnostic reuses the existing mandatory composed-field seed import. It adds
at most two child-edge integrations for a midpoint-only candidate, no new MPI
collective or whole-mesh gather, and no new donor or cavity admission requirement.
Sensor-only/uncomposed operation paths remain unchanged. The small diagnostic
margin is conservative screening, never a changed mesh acceptance tolerance.
This is selected-feature diagnosis, not a whole-mesh feasibility preflight.

## Evidence and reproduction

Final-source evidence is in `qualification_v2` and `probe_np1`, `probe_np2`,
`probe_np4`. `qualify.py` reuses existing resumable campaigns and independent
actual N4/M2 RANS/SA/current+BDF/conservation/geometry/full-field/SU2+CGNS/reload/
restart audits. The focused serial campaign has11cases/760assertions; MPI4 runs
the same11cases on each rank. Enabled and strict-default workflows are separate.
These are scoped compatibility checks, not arbitrary curved-mesh qualification.

`run_probe.py` builds the captured actual three-cell edge star with its37original
P1 donors and the complete original M6 geometry reference. It preserves the
selected parameter.875, core.003215747252591252 and original fade/support. It runs
one/two/four ranks, including empty local ownership, checks an identical elected
witness and byte-identical accepted input. Native budgets remain32rounds/4096work/
20seconds with an outer60second launch ceiling. This is the actual obstructed
feature, not a complete wing/domain adaptation or an accepted solution transfer.

The elected witness is edge211--3481, irreducible child length
11.571781543440306 versus1.8. The sensor-only parent is below1.8. The mathematical
certificate and complete rejected mesh remain in
[the original evidence](../cavity_operator_replay_v1/evolved_volume_v1/full_state_v1/geometry_lattice_v1/README.md).
Do not rerun the unchanged whole M6 case expecting larger cavities to fix this.

Run `python3 <this-folder>/verify.py` from any folder to check current source/build/
binary/inputs, final receipts and MPI witness agreement without CFD. The verifier
requires the local executed build; losslessly compressed helper binaries are local
artifacts, excluded from Git. Raw logs and source capsules are retained. No complete
performance claim follows from these bounded, contended correctness checks.

## Preserved rejected experiment and limits

Preliminary code/results are retained under `preliminary_v1_*`,
`qualification_v1`, `probe_np*_preliminary_v1` and `engine_preliminary_v1.cpp`.
Static review found that an initially broader diagnostic added donor-import
requirements to the old sensor-only streamed path. Final scope removes that
compatibility risk; see [scope review](scope_review.md).
`failed_v1_*` preserves a helper-build variable-name collision, corrected before
any final probe execution. No production fallback or acceptance limit was added.

This does not resolve geometry authority for the almost-planar M6 symmetry surface.
No reference change is authorized here. A separately declared geometry reference
or other feasible declared geometry requires separate construction qualification.
Full Goal3 remains active/incomplete, including accepted difficult construction,
composed gradation, global error calibration/allocation and full event cost.

## Final observed receipts

| Ranks | Launcher seconds | Engine seconds (range) | Global work | Accepted edits |
|---|---:|---:|---:|---:|
| 1 | 1.7506 | 0.1693--0.1693 | 192 | 0 |
| 2 | 1.8531 | 0.1580--0.1589 | 196 | 0 |
| 4 | 4.5140 | 2.3739--2.4173 | 200 | 0 |

All three cases reach the same conflict in four rounds with immutable input.
These local-star times are not whole-wing savings or scaling measurements.
