# M6 build-494 no-BL clean check — 2026-11-14

**IN PROGRESS. One unprofiled six-rank run; no conclusions yet.**

The required load below 1.0 could not be reached after 24.5 minutes of waiting/rechecks. The user instructed “Start now and explicitly say in the report that you could not wait”. Launch load was 2.72; CPU-use gate passed. This is a documented deviation from the original quiet-host condition.
No native source/build/config/gate/budget/limit edits, profiler, debugger or rank-stopping observer. Native acceptance gates are unchanged. Resource receipts are sampled every 10 seconds.
Evidence: `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/nobl_clean_check_20261114`. Binary/config/input hashes and exact command: `run/provenance.json`; override and launch checks: `launch_override.json`, `launch_gate.json`.

## T1 — outcomes

| Rank-0 first adaptation | Build 293 | Build 494 profiled | This unprofiled run |
|---|---:|---:|---:|
| Collapse exit seconds | 194.723054 | 1423.509701 | 1410.686670 |
| Accepted collapses | 480 | 480 | 480 |
| First split seconds / splits | 241.14 / 5047 | 1609.19 / 3520 | 1593.361908 / 3520 |
| Second split record | 274.10 / 9949 | 1737.87 / 8442 | 1722.372641 / 8442 |
| Inner native time/result | 3640.719259s, accepted | 5344.540316s, rejected | pending |
| Accepted mesh points/tets | 149051/736323 | retained original | pending |
Second adaptation remains pending; build 293 accepted 172001 points/870422 tets in 2314.855222s inner native time.
Unprofiled first-collapse rank3 upper proof=934.259092s/4674 calls, versus build293 28.654482s/2794 calls; final per-rank exit counters are still pending.

## T2 — first-adaptation phases

Pending. Reference phase boundaries are retained from isolated rank logs; setup and final-return scopes will be stated.

## T3 — per-rank exit counters

Pending. World and Directory election families remain separate.

## T4 — phase host loads

Pending. The launch load condition did not pass; 10-second load receipts are being retained.

## T5 — verdict

Pending. No quiet-host performance conclusion is claimed.

## T6 — conditional read-only comparison

Read-only comparison of the slow proof phase is in progress, based on the observed 7.24× collapse slowdown and increased proof counters. The report will retain the user-waived quiet-load condition. No design, fix, test or recommendation.
