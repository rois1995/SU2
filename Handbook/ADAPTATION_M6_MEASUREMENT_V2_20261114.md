# M6 measurement v2 — 2026-11-14

**IN PROGRESS: Run A completed normally; the sole six-rank Run B job is still running. This is a resumable checkpoint, not the final measurement report.**

R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`; evidence is `R/measurement_20261114_v2` (v1 preserved).
No native source, build, gate, budget, limit or config changes. CFD SHA0311b5a2…e74ee6 and test-driver SHAe6c25520…cd365 verified before each run.
Run A status: terminal0, all tests passed, all recorded pins unchanged and all owned handles terminal. Run B status/handles: `run_B/status.json`; do not restart it.
Run B controller exec session79883; marker observer session42192. Source/binaries and running recorder/clock helpers remain frozen.

## T0 — Run A clock boundaries

| Interval | Monotonic clock seconds | Wall seconds |
|---|---|---:|
| Preparation, OS process birth → PrepareSurface entry | [136736.210,136736.220] → 136879.191632075 | 142.972–142.982 |
| Public native API call | 136879.191632075 → 137272.885798218 | 393.694166 |
| Post-call observation/checks → process end | 137272.885798218 → 137304.092796831 | 31.206999 |
OS birth has10ms granularity; the loader stop was136736.475593031. No native CPU samples preceded it, so preparation includes all observed samples.
The public call includes final capture/materialization; its existing fixture timer is393.694989634s. The narrower internal exit timer is388.843170562s.
Post-call work includes the existing after-face observation; it is excluded from both requested profiles. Exact clock/provenance files are retained.

## T1 — identified exclusive leaf samples

Shares are sampled CPU, not elapsed wall fractions. Shared range arithmetic remains Other; no whole-phase inclusive caller cost is inferred.
A numerator below100samples is not resolved. B final profiles and all per-rank top20 symbols await normal termination.

| Profile | Total | Proof | Predicates | Target/BL | Tree | Memory | MPI | Pack | Cavity | Other |
|---|---:|---|---|---|---|---|---|---|---|---|
| A preparation | 13883 | 10/13883 (not resolved) | 733/13883 (5.28%) | 17/13883 (not resolved) | 139/13883 (1.00%) | 221/13883 (1.59%) | 0/13883 (not resolved) | 9/13883 (not resolved) | 11/13883 (not resolved) | 12743/13883 (91.79%) |
| A native | 38549 | 688/38549 (1.78%) | 3381/38549 (8.77%) | 11297/38549 (29.31%) | 579/38549 (1.50%) | 821/38549 (2.13%) | 2/38549 (not resolved) | 56/38549 (not resolved) | 83/38549 (not resolved) | 21642/38549 (56.14%) |
| B ranks0–5 | Pending | Pending | Pending | Pending | Pending | Pending | Pending | Pending | Pending | Pending |

Full A top20 names/counts/DSOs are in `run_A/split_profiles.json`; the final report will include the compact ranked lists.
A preparation interval arithmetic(Multiply/Add/Adjacent/Blossom)+__nexttowardl=10896/13883(78.48%); __nexttowardl=3094/13883(22.29%).
A full native-call interval+libm=17135/38549(44.45%); the reviewer’s approximate v1 cuts are not the measured v2 phase boundaries.

## T2 — Run B phases (final totals pending)

| Completed scope | Measured result |
|---|---|
| Initial collapse, rank0 (includes initialization) | 1423.509701s,480collapses,1623rounds,261115elections/1285.774713s,119540tracked collectives |
| First surface pass, rank0 lane timer | 157.971260s; complete per-rank counter snapshots retained |
| First interior pass, all ranks | 40464splits / max-rank elapsed22.228452s =1820.369642global edits/s |
| Repartition and alternation | Completed ownership/lane markers retained; final aggregate pending |
| Quality | Entered at3593.528728s; rank0 first window332commits/1240.767020s; still running |
| Strict final finish | Not reached at this checkpoint |
Literal world-exchange counts are not emitted; tracked collectives are identified as such, never substituted for exchanges.

## T3 — Run B compute against wait

Pending final sample export. `analyze_B.py` bins only timestamps guaranteed inside the measured engine-epoch brackets; ambiguous boundary samples are excluded.
The optional direct variable read failed; source/disassembly places the epoch between the public-entry and post-clock breakpoint observations (~0.35s bracket).
MPI DSOs include stripped OpenMPI plugin symbols. MPI-library/other sample fractions follow the requested convention; off-CPU wall wait and slowest-rank causality remain unmeasured.

## T4 — timer coverage

B exclusive coverage (1−other/total) awaits final DETAIL rows. Nested proof/election/admission timers cannot be added as exclusive wall buckets.
A internal upper proof173.858064448s and geometry-admission guidance144.738106061s overlap. The latter is not the target BoundaryGuidance lambda.
The fixture JSON upper233.722574021s includes two external before/after observations; the engine exit timer excludes them.
InStrip and target BoundaryGuidance lack a dedicated wall timer; initial census target measurements can fall into residual other. Exact union coverage is unmeasured.

## T5 — peak resident memory

A completed per-rank time and VmHWM receipts are retained. B final per-rank peaks await termination; the midrun host receipt had32731800KiB available RAM.
Current/high-water snapshots are not a whole-job peak. Final report will list each native observed HWM and rank command GNU-time maximum separately.

## T6 — claims (B assessments pending final totals)

| # | Current evidence / limit |
|---:|---|
| 1 | B final native MPI sample shares pending; wall waiting for the slowest rank is not directly measured. |
| 2 | First interior per-rank splits range3490–8438 over8492rounds; literal one edit on every rank/round is unsupported. |
| 3 | First collapse tracked collective119540 and election261115 counts /1623rounds are logged; literal world synchronizations/exchanges not separately counted. |
| 4 | First collapse rank0 elections1285.774713s/261115=4.924ms; historical corresponding pass130.713475s/262459=0.498ms. Waiting/message split unresolved. |
| 5 | Strict final finish not yet reached; no new520s result claimed. |
| 6 | First interior1820.37global edits/s; all-interior rate and quality/surface totals pending. |
| 7 | A1897reservation conflicts/2640surface rounds=71.86%; conflicts are not the explicit deferral counter. Upper/guidance timers overlap. |
| 8 | A native tree579/38549(1.50%), memory821/38549(2.13%); identified leaves only, inclusive/inlined costs unresolved. B pending. |

## T7 — nearest visible LBR callers, finite60s windows

**Current evidence is `run_A/nearest_callers.json`; broad `caller_summary.json` is superseded by `caller_review.json`. Far LBR tails are not accepted ancestry.**

| Question | Measured nearest-caller evidence |
|---|---|
| Preparation interval callers | Of3178interval samples: mobility2461(77.44%), parameter/coverage510(16.05%), unresolved138(4.34%); other small categories not resolved. |
| Native interval callers | Of808interval samples: target surface evaluation527(65.22%), parameter/coverage126(15.59%); point/normal83, local face deviation40, union proof6, mobility12, unresolved14 are each not resolved. |
| __nexttowardl / nextafter caller | Preparation Adjacent1319/1355(97.34%); native90/98 not resolved. Other near symbols do not distinguish direct nextafter from an inlined Adjacent. |
| InStrip | 2301/2301 sampled near parents are the BoundaryGuidance lambda; exact calls per guidance evaluation not measured. |
| BoundaryGuidance lambda | 1634/1634 samples show the near method→fixture handler→MeasureSurfacePlanning chain; outer seed/scoring/candidate/census step unresolved. |
These are conditional sampled appearances, not invocation counts or full-phase inclusive shares. No extrapolation from60s windows.

## Overhead and remaining work

A matched existing API timer393.694989634s vs389.572521653s: +1.058%; edits300vs305: −1.639%; neither exceeds5%. No correction.
B current native call has exceeded the older3640.7s reference by >5%; final time/accepted cell count and second invocation outcome are pending.
Reference B used an older test-driver executable; this run uses pinned build494 CFD plus external debugger clock observations. Differences cannot be assigned solely to profiling.
Raw perf data stays local on4TB and out ofgit; optional LBR raw is removed after summary. Current total stays below2GB; final resource/size receipt pending.
No refactoring recommendation or design work. Continue the existing Run B to normal termination, then finalize T1–T6, top20 lists and all unmeasured items and stop.
