# Original-sensor background reuse across BL profiles

Qualified connected slice on6d39199225; initial WIPf6c1e5f83b and its type fix
are preserved. Exact current source/build/binary/config/input/helper pins and
executed-source capsules are retained. All owned jobs terminal. Full Goal3 ACTIVE.
Run `python3 report.py` to verify evidence without another SU2 run.

## Behavior and boundaries

CNativeComplexity3D retains the latest local background interval, quadrature and
numerical indicator. Reuse requires exact equality of every current supplied
sensor tensor component; original source cells/ownership/node references remain
fixed for the object's lifetime. Changed fields recompute. Changed BL core/profile
alone can reuse the sensor-only background. Every correction sample is still
composed with the current original sensor, core and geometric BL. Profile-dependent
geometry samples still refresh at every profile version change. No target,
interpolation, fade/support, protected band, first height or acceptance gate changed.
No additional whole-mesh gather, donor search, field refit or clipped-donor scaling.

One sensor-vector copy is included in retained storage and admitted against the
existing ceiling. Insufficient cache storage produces a collective resource
refusal; no stale estimate is substituted. This trades memory for repeated work
and is a limitation for nearly full integration workspaces. The cache ends with
the normalization object. New counters report rank builds/reuses; existing sensor
interval, correction and numerical indicators are now printed separately.

## Qualification

- Serial25 cases/2253 assertions PASS, including repeated/changed fields, changed
  core/profile, restored original profile, direct actual composition, high
  anisotropy and collective memory refusal.
- MPI4:4 cases/rank PASS (261--264 assertions), including the same cache controls
  and uncertain budget normalization. Local ownership is reduced correctly.
- Actual opt-in N4M2 and strict-default N4M2 RANS/SA current+BDF workflows PASS.
  Existing independent geometry/height/q/J/full-edge/P1/conservation, atomic
  refusal, SU2+CGNS reload and actual restart audits PASS. Opt-in budgets3/20 keep
  lambda.875/0 and reference masses2.95498841845/19.9993054819.
- Saved M6 original-family selection passes MPI2/4 at budget300k;100k still rejects
  as an estimated sampled-profile budget conflict. No CFD advance, mesh
  construction, normalization-success or transferred-M6 claim.

All25 serial controls were rerun only after the MPI fixture correction; full
workflow tests had not run before it. No full wing construction repetition.

## Measured M6 work, memory and accuracy

Experimental protect2e-6/growth2 limits unchanged; all seven300k profile trials
are compared against the earlier saved source scope. MPI4 estimates, uncertainty
and mandatory-demand rows are EXACTLY equal; MPI2 maximum relative row drift
8.998e-14. Frozen selectedlambda.75/version7,308423.47750346235 +/-12078.099538851526
HEURISTIC, still integration_limited.100k nine-profile conflict rows exactly match
the saved baseline. Neither result proves exact continuum infeasibility.

| MPI4 observation | Saved baseline | Candidate |
|---|---:|---:|
| Background determinants |46,717,286|6,673,898|
| Background rank builds/reuses |not instrumented|4/24|
| Correction queries |37,282|37,282|
| Profile geometry queries |31,956|31,956|
| Background seconds max |0.988724|0.178145|
| Assessment seconds max |1.308443|0.590563|
| Preparation seconds max |3.505461|4.748693|
| Retained bytes max/rank |15,463,712|16,592,816|
| Saved-fixture metric seconds |6.551785|7.514765|

Verified determinant-work reduction85.714%; extra retained memory1,129,104 bytes
(~1.077MiB,7.30%). The candidate pays one cold build/rank and six reused profiles;
actual composed work is unchanged. Observed background time falls~82%, while total
metric initialization is slower in this single contended observation. Cold
preparation dominates. Do not claim an end-to-end improvement or scaling from
these measurements. Current total sensor interval indicator0; correction
indicator12078.10 dominates the tiny roundoff term8.90e-14. This is evidence against
spending more background work on this coarsest field. It does not calibrate the
correction uncertainty or complete global absolute-error allocation.

MPI2 also uses6,673,898 determinants, two cold builds/12 rank reuses; retained
31,589,408 bytes/rank. That partition has a different local source footprint.
Metric10.12494s and launcher22.841s are observations, not an M comparison.

Native reported maximum single-process RSS610,631,680 bytes at MPI4,1,024,421,888
at MPI2; neither is whole-job peak memory. Outer timing on MPI abort can omit
rank resource usage, so the100k wrapper RSS is not used as a native peak. Host
observations39--40GiB available,935MiB swap unchanged. One low-priority owned
job, build-j1/libraries1/MPI<=4; no cluster work or foreign job changes.

## Preserved failures and open goal

First build rejects ambiguous Metric alias at cache admission; qualified tensor
name fixes it, completed solver object reused. V1 serial passes; MPI fails a new
fixture assertion comparing a local counter with a global sample count. Correct
SUM reduction in v2 passes. Failed source/logs/receipts remain; production
composition and error formulas were unchanged by the fixture fix.

Global uncertainty calibration/absolute-error allocation, general curved and
interacting construction, accepted difficult M6,2D opt-in, converged viscous
accuracy and affordable complete lifecycle remain open. Existing whole-front
construction is geometrically restricted; no further local fallback/cap ladder
was added. Historical M6 construction still rejects at300s (90 private edits at
6d39199225), recorded separately in `../query_campaign_reuse_v1/README.md`.
