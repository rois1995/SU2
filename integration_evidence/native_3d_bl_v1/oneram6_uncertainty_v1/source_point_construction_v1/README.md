# Source-contained point construction: qualified scope and hard-case failure

Producer source checkpointb22b99e452. Full Goal3 remains ACTIVE. Controls pass;
the complete frozen M6 construction does NOT. `report.py` verifies current source/
build/binary/helper/input receipts and deliberately reports these separately.

## Implemented capability

`AffinePointBounds` uses the existing dyadic-integer predicate arithmetic to compute
adjacent binary64 bounds of the exact affine point. It handles cancellation,
subnormals, extreme exponents and nonrepresentable complements. `SegmentPoint`
checks at most8 corners, nearest first, against exact original/declared-extension
containment. It reuses the existing literal sensor cache. No expanded displacement
search or alternate containment tolerance. An empty admitted corner set rejects;
that is not proof no other placement exists.

Balanced splits and existing central physical-edge proposals use this constructor.
Raw outside midpoint substitution is removed; identical rejected central proposals
are not retried. Geometry authority, first height, positivity, conformity, q/J,
full composed edge and transfer checks remain unchanged. Ordinary Query and
TraceSegment still reject outside literal points; no semantic midpoint weights
are stored under an outside point's cache key. Target/profile/finer/fade unchanged.

The preceding exact packet82937de1a5 showed the M6 midpoint of242/3094 rounds its z
coordinate outward3.47e-18. The adjacent lower corner is contained. New regression
checks that construction succeeds while ordinary queries of the outward point
still reject, including after valid-point cache population. No source point is
silently clamped or extrapolated.

Balanced parameter trials now use binary64 (53 significant bits), comfortably
beyond the existing1e-8 relative length tolerance; unrepresentable/unresolved trials
remain explicit failures. Corner membership is one requirement, not a guarantee
of valid skin, first height, full metric or global construction.

## Scoped validation

- Serial16 cases/1993 assertions pass, covering composed edges, midpoint semantics,
  balanced placement and the actual captured M6 source witness.
- MPI4:21 cases per rank pass, adding original-support consistency, progressive
  full measurement and first-row batches.
- Independent Fraction oracle verifies126 coordinate bounds per process at42 input
  combinations. All4 MPI output streams are bit-identical. Includes exact
  cancellation, DBL_MAX with tiny tails, subnormals and signed values.
- Default-policy N4/M2 original-family correction at both initial scales1/10 passes
  independent geometry/full metric/conservation/transport/SU2/CGNS/restart audits.
- Actual initialized opt-in N4/M2 RANS/SA current+BDF adaptation, conservative projection,
  atomic rejection, SU2/CGNS reload and restart pass. Independent final masses
  2.95498841845 and19.9993054819 versus budgets3/20; source/height/quality/full-edge
  contracts retained. This is not converged CFD or a complete regression.

Accepted inspectable grids are in
`qualification_v1/runtime/bl_budget_3_m2_accepted.su2` and its budget20 counterpart,
with `_written_CGNS.cgns` outputs. Existing original inputs remain intact.

## Actual unchanged M6 replay

`../construction_source_point_v1` reuses the exact owner-UPPER6 sensor archive,
original reference/config, first height, strict target/fade and unchanged300s
backend ceiling. No CFD advance, budget normalization or transfer occurred.

The captured source failure is passed:485 rounds now accept93 private volume edits
(68 on rank0,25 on rank2), with7 useful-progress windows and no stagnant windows.
The event rejects atomically at300.531s. Last local rejection is edge1142274/1431522,
q=.16026893016906607, J=.050413075726180874 at cell9246092; required q>=.20/J>=.05
and all final gates remain unchanged. This does not prove infeasibility or establish
an accepted mesh. No full private rejected wing was exported by the current backend.

The source defect was real and is removed in the demonstrated scope. Completing
this difficult construction is a separate open capability. Do not raise the ceiling
or add another local fallback merely to extend this run.

## Cost evidence and next decision

Correctness pilot on a contended host, not scaling or a speed comparison. One bounded
gdb stack sample paused rank3; it was inside a failure-election Allreduce in private
placement. A subsequent peer sample found the job already terminal. Its presence
is recorded; elapsed time is not a clean performance benchmark.

| Section (max accumulated rank time) | Seconds |
|---|---:|
| Input/import |1.824|
| Initial work weights |18.502|
| Partition |1.761|
| Donor migration |.183|
| First row (inside refine) |29.848|
| Refine total |277.041|
| Private reconstruction on active rank0 (inside refine) |118.244|
| Donor work (inside refine) |35.857|
| Explicit metric section (inside refine) |20.489|

Nested rank0 field query53.895s includes51.222s search; edge26.572s includes25.376s
exact trace. These overlap reconstruction/metric totals and must not be added.
The four ranks execute40868 donor batches and477360 tracked calls each; rank0
inspects35.57M donor candidates and imports1.81M payloads. Large `other` sections
on mostly inactive ranks plus the stack sample indicate waiting/coordination;
`other` is not an exclusively timed MPI section. Source preparation/context reuse
and transaction coordination remain material gaps, not just tensor arithmetic.

Observed four-process RSS about2.1GiB during the run; maximum recorded single-
process peak759103488 bytes. Conservative admitted native bound peaks2.092GB/rank.
These are different measures; no aggregate peak or affordability claim.

Next connected work must address difficult reconstruction and its repeated source/
transaction work using exact local replay, rather than repeated full M6 campaigns.
The field is the historical provisional unnormalized target; combine the qualified
original-family/budget profile workflow separately before a production M6 claim.
Retain strict predicates/gates, frozen original P1 and atomic publication.
