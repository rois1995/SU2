# M6 build-494 no-BL unprofiled check — 2026-11-14

**IN PROGRESS: second configured adaptation remains active.**
We could not wait for the quiet-load condition: after 24.5 minutes of failed rechecks, the user instructed launch at load 2.72 (required <1.0). The CPU-use gate passed; the load condition was explicitly waived.
No native source/build/config/gate/budget/limit changes; no perf, gdb or rank-stopping observer. nice 19, six ranks, single-thread libraries; /proc receipts every ~10 seconds.
Evidence: `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/nobl_clean_check_20261114`; exact hashes/argv/environment: `run/provenance.json`; program SHA 0311b5a2a4f871bd… and config SHA 5ad2dc1d9a1cd8bf…. All 1539 build-494 source pins matched.

## T1 — outcomes (first-adaptation clocks/counts are rank 0)

| Quantity | Build 293 | Build 494 profiled/busy | This unprofiled run |
|---|---|---|---|
| Collapse exit seconds | 194.723054 | 1423.509701 | 1410.686670 |
| Accepted collapses | 480 | 480 | 480 |
| First split seconds/splits | 241.14/5047 | 1609.19/3520 | 1593.361908/3520 |
| Second split seconds/splits | 274.10/9949 | 1737.87/8442 | 1722.372641/8442 |
| Adaptation 1 inner seconds/result | 3640.719259, accepted | 5344.540316, rejected | 5345.557143, rejected, time_budget |
| Adaptation 1 points/tets | 149051/736323 accepted | 61330/303359 retained | 61330/303359 (retained) |
| Adaptation 2 inner seconds/result | 2314.855222, accepted | 5324.459194, rejected | pending, pending |
| Adaptation 2 points/tets | 172001/870422 accepted | 61330/303359 retained | pending |
Second inputs diverge after the first result: current CFD retained the original mesh; reference CFD continued on its first accepted adaptation. Complexity 50,000 is nominal, not a point-count gate.

## T2 — first-adaptation boundary bins

Each cell=max-rank boundary seconds; global accepted edits; common per-rank World election calls; rank-0 mean election µs. World counts are not summed over six ranks.
| Phase | Build 293 | This run |
|---|---|---|
| C* collapse | 194.72; 2495; 262459; 498.0 | 1410.69; 2496; 261115; 4882.6 |
| S* surface | 298.62; 12852; 527333; 318.6 | 1922.82; 10995; 664276; 2828.1 |
| I interior | 76.37; 70479; 2543580; 14.1 | 56.83; 70537; 2288695; 10.4 |
| R* repartition/setup | 86.78; 0; 62156; 211.2 | 39.91; 0; 63947; 161.0 |
| Q* quality/return | 2436.23; 23043; 316210; 544.5 | 1914.33; 2549; 25251; 1734.9 |
| A* alternation/re-entry | 548.29; 5638; 65947; 2276.2 | 1.18; 0; 10; 4.2 |
| F finish | not separately emitted | not separately emitted |
C/S/R/A* include setup/re-entry; Q* includes quality through return, and the current duration is budget-censored. Finish has no separate emitted boundary. Named S/I lane maxima: 293 = 283.69/126.74s; current = 1907.41/108.17 s; these scopes differ from the bins.

## T3 — per-rank exit counters

Every cell is build 293 → build 494. Invocation/rank identifies rows. U=upper seconds/calls; E=World election seconds/calls; update/other are seconds; G=geometry trials/rejections. D=Directory election seconds/calls. E and D are distinct families; timers overlap, so do not add them.
| Inv/rank | U | E (World) | D (Directory) | Update | G | Other |
|---|---|---|---|---|---|---|
| 1/0 | 1007.76/159024 → 198.96/15979 | 669.87/3777685 → 3231.53/3303294 | 155.44/954079 → 46.50/837184 | 377.24 → 1915.88 | 55846/23224 → 25203/11140 | 2225.78 → 3071.23 |
| 1/1 | 1232.31/175198 → 419.24/19795 | 636.29/3777685 → 2969.34/3303294 | 164.04/954079 → 50.22/837184 | 377.19 → 1915.87 | 57442/24631 → 25450/10859 | 2094.98 → 3103.77 |
| 1/2 | 1129.75/159075 → 569.66/23613 | 658.31/3777685 → 3034.57/3303294 | 176.08/954079 → 40.74/837184 | 377.28 → 1915.86 | 69209/32835 → 34543/15342 | 2157.75 → 2868.79 |
| 1/3 | 966.84/160292 → 1939.13/26232 | 719.76/3777685 → 2043.32/3303294 | 176.39/954079 → 39.35/837184 | 377.25 → 1915.87 | 66201/30093 → 36988/16141 | 2209.76 → 2511.54 |
| 1/4 | 810.72/166398 → 1100.67/20449 | 676.92/3777685 → 3171.18/3303294 | 157.36/954079 → 48.36/837184 | 377.22 → 1915.93 | 60788/25684 → 22252/9672 | 2348.02 → 2428.73 |
| 1/5 | 1277.82/140633 → 1756.58/27794 | 635.38/3777685 → 1825.13/3303294 | 202.08/954079 → 42.27/837184 | 377.26 → 1915.86 | 57039/26903 → 32576/13365 | 2152.20 → 2925.63 |
| 2/0 | 739.77/110580 → pending | 583.74/1516407 → pending | 120.19/344487 → pending | 133.59 → pending | 27675/9071 → pending | 1338.04 → pending |
| 2/1 | 892.29/108265 → pending | 507.07/1516407 → pending | 138.62/344487 → pending | 133.45 → pending | 34358/14554 → pending | 1309.06 → pending |
| 2/2 | 800.98/111857 → pending | 574.16/1516407 → pending | 115.30/344487 → pending | 133.55 → pending | 25373/7914 → pending | 1296.95 → pending |
| 2/3 | 756.51/61503 → pending | 677.17/1516407 → pending | 179.16/344487 → pending | 133.41 → pending | 17672/6391 → pending | 1424.85 → pending |
| 2/4 | 741.83/70408 → pending | 682.20/1516407 → pending | 181.53/344487 → pending | 133.38 → pending | 17628/6361 → pending | 1423.15 → pending |
| 2/5 | 820.04/71673 → pending | 615.00/1516407 → pending | 165.82/344487 → pending | 133.47 → pending | 12840/3691 → pending | 1397.32 → pending |

## T4 — sampled host load

Cells=N samples: min/median/max in timestamped marker windows. C* begins at the preceding input PARTITION log; no native epoch was estimated.
| Phase | Adaptation 1 | Adaptation 2 |
|---|---|---|
| C* collapse | 140: 8.25/8.96/10.38 | 140: 8.10/8.83/9.77 |
| S* surface | 190: 8.09/8.88/10.29 | 158: 8.22/9.02/11.53 |
| I interior | 6: 8.60/8.84/9.22 | 5: 8.59/9.06/9.44 |
| R* repartition/setup | 5: 8.93/9.16/9.72 | 1: 9.50/9.50/9.50 |
| Q* quality/return | 190: 8.41/9.01/10.34 | no samples/window |
| A* alternation/re-entry | no samples/window | no samples/window |
| F finish | no samples/window | no samples/window |
Poll cadence ~10s; MPI log times have 1s precision and unmeasured forwarding delay. Unsampled extrema and subsecond handoffs are unresolved.

## T5 — verdict

**Provisional 2 — the no-BL M6 path is slower on build 494 under the authorized launch conditions.** Collapse is 7.24× and the surface/setup bin is 6.44× the historical wall time; the first native attempt was rejected instead of accepted.
First-collapse global upper-work units 54,229,713→1,034,769,513 (19.08×), calls 19905→26642, edits 2,495 → 2,496. Rank 3 proof 28.65→934.26 s and work 4.56M→830.25M, with 263 accepted collapses in both.
The original strict quiet-host condition was not met. Host effects and the older test-driver versus current CFD execution context are not isolated; no timing correction or causal attribution is claimed.

## T6 — ranked read-only source candidates

Exact old/new file contents were reverified against both pins (`source_comparison.json`, `source_primary_review.json`); matching per-file commits are not build IDs. Candidates were not tested.

1. `76f10c8310`: `EntityFaceError`, `BoundPhysicalSurface`, `QuerySurfaceUnion*` use a separate ErrorMagnitude purpose above tolerance and require the covering upper/lower gap to close; strongest explanation for added queries/refinement.
2. `35e173b043`: `EntityFaceError` introduced a follow-up certificate query relative to 293; this is a call-path ancestor later modified by 76f10c8310, not an independent additive cause.
3. `54d2ca2bd4`: `QuerySurfaceUnionAtResolution` adds diameter/interval arithmetic and Lipschitz tightening; it may reduce refinement, so its net cost direction is unresolved. No fix/design/refactoring recommendation.
