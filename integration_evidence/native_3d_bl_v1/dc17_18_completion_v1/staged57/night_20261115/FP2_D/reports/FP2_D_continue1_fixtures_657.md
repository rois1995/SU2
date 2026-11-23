# FP2-D continuation 1 build657 fixtures

The continuation addresses the build656 N1 failure at 40 marked sweeps. It ends the walk on a pooled nonfeature error edge after the applicable lock and floor checks. Feature parents retain the full route and the independent test probe accepts either a nonfeature edge in its rebuilt table or the common parent choice. [Signed continuation](../inputs/FP2_D_CONTINUE_1.md) · [Whole-item source check](../continuation1_source_check.md) · [Build656 failure](FP2_D_656.md)

Both build targets passed without warnings in 55.230 s. Source hashes matched the build receipt. The direction suite passed 15 cases and 1,641 assertions. [Build657](../build657/build_status.json) · [Directional receipt](../fixtures/build657/distance/attempt1/result.json)

| Fixture | Build656 pin | Continuation 1 pin |
| --- | --- | --- |
| a | 27 option-off splits, one refused residual | 27 option-off and 15 option-on splits, one marked sweep and one refused residual with the option on |
| b and g | Diagonals first, across edges second | Diagonals first, across edges second |
| d2 | One mark per sweep, maximum path 2, accepted order 3-6, 2-3, 0-2 | Candidates 3 / 2 / 1, deferrals 2 / 1 / 0, maximum path and waits 0, same accepted order |
| d3 | No placement, Border and NeighborLocked remain | One Border refusal and one placement on 0-2 in one sweep, no residual |
| f1 | One wait or more, maximum path 1 | 2 / 1 candidates, 1 / 0 deferrals, zero waits and path advances |
| f3 | First edge already in Err | First edge pa is centroid-only, next edge pc is shorter than the 0.028 floor. Maximum path 1, Short and NeighborLocked, no placement. |
| k | Rising error keys reach only the last diagonal | 7 / 15 marks for four / eight layers, 4 / 8 splits and 3 / 7 deferrals, one marked sweep each |
| l | Centroid-only walk reaches 3-6 | Same centroid-only walk. A pooled nonfeature error on 2-3 ends it after one advance. |
| m | Feature-parent rule | The feature request and centroid path both reach 3-6. First sweep has one mark, one split and no deferrals. |

The eight sectors have 16 rim chords. The 45-degree chord has a sagitta above the distance limit. Halving it reduces the sagitta below the limit. The option-on measurement accepted 15 splits and left the sector with the refused rim chord as its single residual. The build656 option-on count was recorded only as an inequality against 27 by the original fixture. The layered-cylinder rings keep the same order because the longer diagonal wins the midpoint tie, and the rim chords retain the feature rule.

The option-off comparison matched S0, including the corrected C2 cohorts. The 18 FP1 cohorts passed. G1 matched 375 / 114 / 228 records. [Fixture acceptance](../fixtures/build657/d_acceptance.json) · [Option-off exact comparison](../fixtures/build657/off_comparison.json)
