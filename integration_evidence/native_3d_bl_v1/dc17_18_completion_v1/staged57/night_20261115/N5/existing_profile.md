# Existing F3b quality timing extraction

Read-only extraction from accepted F3b evidence; no numerical run was performed.
The timer receipts report both event gates passed. Exact marker paths, SHA-256 hashes, source commit/build, and binary/source-pin hashes are in `existing_profile.json`.

## M8 window rows by event and rank

| Event | Rank | Windows | Sum busy (s) | Sum join call (s) | Sum tracked collective call (s) | Join wait tree exclusive (s) | Quality unattributed (s) | Named quality exclusive, including join (s) |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 1 | 0 | 20 | 427.320893 | 525.093101 | 204.311156 | 552.995542 | 19.855202 | 1426.010801 |
| 1 | 1 | 20 | 492.083150 | 460.285187 | 200.991077 | 485.542446 | 27.589510 | 1418.270717 |
| 1 | 2 | 20 | 415.822992 | 536.581922 | 270.671955 | 553.176678 | 13.634024 | 1432.234556 |
| 1 | 3 | 20 | 435.624661 | 516.769102 | 237.049900 | 539.772681 | 22.683950 | 1423.176343 |
| 1 | 4 | 20 | 354.869783 | 597.530637 | 210.172966 | 635.706925 | 30.518157 | 1415.340217 |
| 1 | 5 | 20 | 410.301667 | 542.115924 | 271.251999 | 580.103478 | 13.396907 | 1432.471942 |
| 2 | 0 | 14 | 245.926024 | 326.548627 | 155.056887 | 327.289092 | 11.989470 | 873.552678 |
| 2 | 1 | 14 | 206.572309 | 365.867194 | 162.532916 | 366.675700 | 12.532306 | 873.087105 |
| 2 | 2 | 14 | 369.603465 | 202.757818 | 135.057481 | 202.794624 | 13.676852 | 871.864853 |
| 2 | 3 | 14 | 112.020900 | 460.324815 | 230.048241 | 461.195673 | 3.861188 | 881.690415 |
| 2 | 4 | 14 | 112.907008 | 459.577511 | 239.641278 | 460.457441 | 3.518031 | 882.038014 |
| 2 | 5 | 14 | 279.973184 | 292.475046 | 197.859083 | 293.353557 | 6.827227 | 878.720676 |

Window-level source lines and all extracted fields are in `existing_quality_windows.csv`; timer tree rows are traced by JSON Pointer and receipt SHA-256 in `existing_quality_timer_rows.csv`. The timer receipts independently validate `quality_unattributed_seconds` against the final-boundary `unattributed` exclusive timer row for every rank/event.

`busy_seconds` is wall time within the marker's busy interval, not CPU time. `join_call_seconds` and tracked collective time may overlap; their sums are shown as separate observations and are not additive. Timer inclusive values are nested; use exclusive values for attribution. `private_tracked_collectives` is included as printed, but is not the absent `private_work_used` counter, so M20 unit cost remains unavailable.

M16: the existing timer tree has aggregate quality operation calls/times (including candidate, geometry projection, quality optimization, and clearance) but no direct per-wall-move candidate split into export, projection, q/J, move, and guidance. Those requested component timings remain unavailable from these logs.

This is reuse of existing measurements only; it does not claim M8's newly requested handoff-step-2 profile was performed or that N5 is complete/accepted.
