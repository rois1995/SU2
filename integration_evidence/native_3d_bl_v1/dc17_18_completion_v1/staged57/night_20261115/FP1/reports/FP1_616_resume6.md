# FP1 Resume 6, build616

Resume 6 was copied with its SHA256 and verified. Resume 5's full workflow controllers and descendants were terminal before Part S. [Order](../inputs/FP1_RESUME_6.md) · [SHA256](../inputs/FP1_RESUME_6.sha256) · [Terminal receipt](../resume6/prior_terminal.json)

## Part S

The whole-item source check found no contradiction requiring a scope to be omitted. All three lexical scopes close before checkpoints. Each uses the existing Scheduling operation. The production diff has exactly three added statements in two source files. [Source check](../resume6/source_check.md) · [Scope diff](../resume6/production_scope.patch)

Build616 completed in 102.720 s with zero new warnings. Solver SHA256 is `8be4d34422ef8e3e8577bfdc14b5a2adac086842a421b959d3d8d03e31a69470`. [Build receipt](../resume6/build616/build_status.json)

The copied comparator preserves the inherited exact fields and requires one construction/scheduling row in each specified snapshot. Its self-check rejects a missing scheduling row, a changed import call count, unequal rank deltas and unequal collapse deltas between G2 and GQ. The initial synthetic fixture placed its stage cutoff before the timer snapshot, so the parser correctly saw no timer rows. Moving the synthetic stage marker after the timer summary matched the real log order. A no-launch check against the archived build615 prefixes reproduced the 8,136 and 115,606 reference calls on all six ranks. [Comparator](../resume6/compare_scheduling.py)

The G5 driver verified the sealed build615 OFF proof without a solver launch. It authenticates the archived payloads, proof, terminal handles, configuration and OFF clocks, then binds that proof to the two specified changed source files in build616. The per-rank report also reproduced the two archived rank 0 coverage values from the raw timer rows. [G5 driver](../resume6/run_g5.py) · [Timer report helper](../resume6/compare_g5_timers.py)

## Scope checks

All 12 fixture selectors passed at their prescribed rank counts and matched build615's assertion totals. The first H invocation omitted the existing `SU2_FP1_G_OWNED_BASELINE` input and stopped at its presence assertion. R3 recovery supplied the pinned G F4.1 owned-state artifact with SHA256 `2aaf657a94818d4aef416d0fa742707fa4ce0707fb446f8a0ca1ffeffe45a599`. The same build616 then matched 552 assertions in three cases. [Original H attempt](../resume6/build616_checks/H/rank1/fixture_status.json) · [H input receipt](../resume6/build616_checks/H_corrected/owned_input.json) · [Passing H receipt](../resume6/build616_checks/H_corrected/fixtures.json)

G1 matched all 375 / 114 / 228 adopted cases and assertion outcomes at 753.148 / 10.116 / 9.773 s for serial / 2 ranks / 4 ranks. [Fixture receipts](../resume6/build616_checks/checkpoint_paths.json) · [G1 receipts](../resume6/G1_616/checkpoint_paths.json)

The first checkpoint invocation passed a string to the file-path hash helper. Converting the receipt field to a Path corrected the reporting helper before staging or launch. All completed gate evidence was reused.

G2 and GQ passed at 236.468 and 660.034 s. All strict rows matched, including modeled storage and `fixedstorage=4120`. Communication and the specified counters are report-only. Scheduling deltas were uniform across all six ranks and matched between G2 and GQ at collapse exit. [G2 comparison](../resume6/scoped/prefix/G2_comparison.json) · [GQ comparison](../resume6/scoped/prefix/GQ_comparison.json) · [Per-rank scheduling reference](../resume6/scheduling_calls_reference.json)

| Case | Remesh snapshot | Reference calls per rank | Build616 calls per rank | Delta | Expected delta |
|---|---|---:|---:|---:|---:|
| G2 | `length_collapse_exit` | 8,136 | 8,137 | +1 | +1 |
| GQ | `construction_entry` | 115,606 | 115,680 | +74 | +74 |
| GQ | `length_collapse_exit` | 8,136 | 8,137 | +1 | +1 |

G3 passed physical and observer-adjusted identity checks with 1,552 accepted splits and 1,113 identical common edits. G4 passed at 1, 2 and 4 ranks. The merit481, saved481 and saved_current safeguards passed. These three gate families do not compare scheduling calls against prior runs. [Scoped receipt](../resume6/scoped/scoped.json) · [Saved-state receipts](../resume6/scoped/safeguards.json)

**Part S gates 1 to 4 passed. G5 OFF pending (FX1).** Resume 6 uses the accepted build615 OFF workflow and its native maxima 1,778.627 / 1,029.081 s for the ON comparisons.

## G5 ON1

ON1 passed both events, continued CFD and isolated disk restart. Native maxima were 1,499.541 / 386.884 s and minimum named coverage was 99.041% / 98.658%. The six ON1 files are sealed for the repeat. [Run receipt](../resume6/G5/ON1/run_receipt.json) · [Build615 OFF binding](../resume6/G5/build615_off_binding.json)

## G5 verdict

**FP1 ON accepted.** Both build616 ON runs passed the standing gates for both events, continued CFD and isolated disk restart. All six payloads match byte for byte. Both ON runs passed the native time comparisons against build615 OFF. All three event 1 files matched their corresponding build615 ON run in both cases. [Acceptance](../resume6/G5/acceptance_summary.json) · [ON reference hashes](../on_reference.json) · [Build615 event 1 comparison](../resume6/G5/event1_build615_comparison.json)

The build615 OFF binding uses its accepted native maxima of 1,778.627 / 1,029.081 s and time limits of 1,867.559 / 1,080.536 s. Both ON runs used the same build616 solver and the archived ON configuration. All final ledgers retain `fixedstorage=4120`. The option default remains `NO`. The fresh build616 G5 OFF proof is pending FX1 as ordered. [Archived OFF binding](../resume6/G5/build615_off_binding.json) · [Terminal audit](../resume6/G5/terminal_audit.json)

## G5 report rows

| Event | Run | Native max (s) | Minimum coverage (%) | Cells | Points | Job peak (MiB) | Proof faces |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | Build615 OFF | 1,778.627 | 97.467 | 694,248 | 138,661 | 13,391.820 | 61,906 |
| 1 | ON1 | 1,499.541 | 99.041 | 554,969 | 111,697 | 11,052.551 | 68,006 |
| 1 | ON2 | 1,500.864 | 99.043 | 554,969 | 111,697 | 11,045.742 | 68,006 |
| 2 | Build615 OFF | 1,029.081 | 98.019 | 822,882 | 161,189 | 15,996.391 | 58,260 |
| 2 | ON1 | 386.884 | 98.658 | 553,652 | 109,124 | 11,113.320 | 64,758 |
| 2 | ON2 | 373.634 | 98.650 | 553,652 | 109,124 | 11,103.691 | 64,758 |

Phase, core and shift times are maxima over ranks in seconds. Engine construction subtracts core and shift within each rank. These maxima can come from different ranks.

| Event | Run | Preparation | Engine construction | Core | Shift | Quality | Final proof |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | Build615 OFF | 63.022 | 460.190 | 0.000 | 0.000 | 1,150.640 | 63.140 |
| 1 | ON1 | 64.431 | 464.264 | 15.047 | 0.592 | 855.544 | 57.016 |
| 1 | ON2 | 64.371 | 465.799 | 15.185 | 0.601 | 856.888 | 56.233 |
| 2 | Build615 OFF | 83.436 | 202.218 | 0.000 | 0.000 | 602.798 | 105.126 |
| 2 | ON1 | 54.316 | 157.551 | 9.930 | 0.751 | 99.501 | 28.825 |
| 2 | ON2 | 53.270 | 156.628 | 9.670 | 0.733 | 90.169 | 27.780 |

| Event | Run | Last local edges | Last short edges | Engine collapses | Engine splits | Engine rounds (rank sum) | Frozen stages | Complexity |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | Build615 OFF | 940,285 | 472,252 | 9,709 | 82,036 | 155,112 | 1 | 51,328.597 |
| 1 | ON1 | 736,095 | 336,474 | 2,821 | 28,728 | 115,164 | 1 | 51,484.195 |
| 1 | ON2 | 736,095 | 336,474 | 2,821 | 28,728 | 115,164 | 1 | 51,484.195 |
| 2 | Build615 OFF | 1,029,309 | 538,452 | 6,660 | 26,108 | 50,400 | 1 | 50,588.745 |
| 2 | ON1 | 716,154 | 324,865 | 4,282 | 5,844 | 47,574 | 1 | 50,627.363 |
| 2 | ON2 | 716,154 | 324,865 | 4,282 | 5,844 | 47,574 | 1 | 50,627.363 |

| Event | Run | Entry cells rank 0 | Rank 1 | Rank 2 | Rank 3 | Rank 4 | Rank 5 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | Build615 OFF | 48,316 | 51,155 | 56,223 | 51,202 | 50,531 | 45,932 |
| 1 | ON1 | 50,044 | 53,134 | 89,880 | 131,765 | 51,829 | 43,597 |
| 1 | ON2 | 50,044 | 53,134 | 89,880 | 131,765 | 51,829 | 43,597 |
| 2 | Build615 OFF | 115,746 | 102,554 | 127,730 | 119,151 | 121,207 | 107,860 |
| 2 | ON1 | 104,696 | 80,115 | 88,893 | 95,522 | 81,558 | 86,613 |
| 2 | ON2 | 104,696 | 80,115 | 88,893 | 95,522 | 81,558 | 86,613 |

| Event | Run | Peak MiB rank 0 | Rank 1 | Rank 2 | Rank 3 | Rank 4 | Rank 5 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | Build615 OFF | 2,140.172 | 2,268.309 | 2,237.473 | 2,191.039 | 2,251.703 | 2,303.125 |
| 1 | ON1 | 1,946.707 | 1,817.633 | 1,835.879 | 2,020.523 | 1,840.809 | 1,591.000 |
| 1 | ON2 | 1,948.434 | 1,817.902 | 1,825.555 | 2,020.395 | 1,839.730 | 1,593.727 |
| 2 | Build615 OFF | 2,713.285 | 2,493.180 | 2,824.504 | 2,745.973 | 2,647.422 | 2,572.027 |
| 2 | ON1 | 1,946.707 | 1,817.633 | 1,835.879 | 2,020.523 | 1,840.809 | 1,651.770 |
| 2 | ON2 | 1,948.434 | 1,817.902 | 1,825.555 | 2,020.395 | 1,839.730 | 1,651.676 |

Local-core and shift rows are sums across ranks. Full sums and per-rank rows are in the case receipts.

| Event | Run | Stage | Pass | Cells in | Cells out | Collapses | Splits | Edge removals | Face swaps | Moves | Memory skips |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | ON1 | 0 | 1 | 303,359 | 420,752 | 34,835 | 55,719 | 11,941 | 565 | 5,172 | 88,689 |
| 1 | ON1 | 0 | 2 | 420,752 | 420,249 | 3,256 | 2,917 | 287 | 13 | 240 | 102,110 |
| 1 | ON2 | 0 | 1 | 303,359 | 420,752 | 34,835 | 55,719 | 11,941 | 565 | 5,172 | 88,689 |
| 1 | ON2 | 0 | 2 | 420,752 | 420,249 | 3,256 | 2,917 | 287 | 13 | 240 | 102,110 |
| 2 | ON1 | 0 | 1 | 554,969 | 539,700 | 28,054 | 23,936 | 8,898 | 439 | 3,159 | 94,485 |
| 2 | ON1 | 0 | 2 | 539,700 | 537,397 | 1,816 | 1,346 | 237 | 12 | 240 | 99,906 |
| 2 | ON2 | 0 | 1 | 554,969 | 539,700 | 28,054 | 23,936 | 8,898 | 439 | 3,159 | 94,485 |
| 2 | ON2 | 0 | 2 | 539,700 | 537,397 | 1,816 | 1,346 | 237 | 12 | 240 | 99,906 |

| Event | Run | Stage | Shift | Cells before | Cells after | Sent | Received |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | ON1 | 0 | higher | 420,249 | 420,249 | 28,760 | 28,760 |
| 1 | ON1 | 0 | lower | 420,752 | 420,752 | 27,285 | 27,285 |
| 1 | ON2 | 0 | higher | 420,249 | 420,249 | 28,760 | 28,760 |
| 1 | ON2 | 0 | lower | 420,752 | 420,752 | 27,285 | 27,285 |
| 2 | ON1 | 0 | higher | 537,397 | 537,397 | 44,543 | 44,543 |
| 2 | ON1 | 0 | lower | 539,700 | 539,700 | 45,563 | 45,563 |
| 2 | ON2 | 0 | higher | 537,397 | 537,397 | 44,543 | 44,543 |
| 2 | ON2 | 0 | lower | 539,700 | 539,700 | 45,563 | 45,563 |

The cells below show build615 / build616. Timer rows are cumulative exclusive seconds in construction.

| Run | Event | Rank | Coverage (%) | Collapse exit unattributed (s) | Collapse exit scheduling (s) | First construction entry unattributed (s) | First construction entry scheduling (s) | Final unattributed (s) | Final scheduling (s) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| ON1 | 1 | 0 | 97.193 / 99.041 | 1.201 / 1.172 | 0.167 / 0.219 | 26.681 / 5.881 | 0.222 / 21.069 | 32.364 / 5.976 | 0.222 / 26.460 |
| ON1 | 1 | 1 | 97.377 / 99.140 | 1.196 / 1.161 | 0.166 / 0.223 | 23.742 / 4.300 | 0.215 / 19.450 | 29.148 / 4.396 | 0.215 / 24.654 |
| ON1 | 1 | 2 | 97.090 / 99.106 | 1.113 / 1.047 | 0.217 / 0.284 | 27.625 / 4.842 | 0.264 / 22.926 | 33.629 / 4.934 | 0.264 / 28.758 |
| ON1 | 1 | 3 | 96.952 / 99.049 | 1.210 / 1.130 | 0.230 / 0.320 | 29.488 / 5.549 | 0.283 / 23.618 | 35.561 / 5.636 | 0.283 / 29.405 |
| ON1 | 1 | 4 | 97.430 / 99.147 | 1.227 / 1.195 | 0.170 / 0.222 | 23.279 / 4.164 | 0.218 / 18.876 | 28.553 / 4.254 | 0.218 / 23.830 |
| ON1 | 1 | 5 | 97.473 / 99.194 | 1.145 / 1.112 | 0.160 / 0.219 | 22.406 / 3.424 | 0.197 / 18.792 | 27.717 / 3.516 | 0.197 / 23.900 |
| ON1 | 2 | 0 | 95.110 / 98.658 | 2.506 / 2.301 | 0.289 / 0.411 | 12.392 / 3.254 | 0.306 / 8.379 | 16.837 / 3.254 | 0.306 / 12.704 |
| ON1 | 2 | 1 | 95.911 / 98.888 | 2.257 / 2.123 | 0.250 / 0.379 | 10.102 / 2.380 | 0.260 / 7.243 | 13.801 / 2.381 | 0.260 / 10.873 |
| ON1 | 2 | 2 | 95.803 / 98.939 | 2.059 / 1.859 | 0.212 / 0.342 | 10.510 / 2.240 | 0.221 / 7.465 | 14.231 / 2.240 | 0.221 / 11.107 |
| ON1 | 2 | 3 | 95.404 / 98.768 | 2.015 / 1.841 | 0.223 / 0.362 | 11.706 / 2.835 | 0.241 / 8.113 | 15.854 / 2.835 | 0.241 / 12.267 |
| ON1 | 2 | 4 | 95.900 / 98.951 | 2.222 / 2.077 | 0.290 / 0.418 | 10.256 / 2.337 | 0.299 / 7.416 | 14.044 / 2.337 | 0.299 / 11.021 |
| ON1 | 2 | 5 | 95.779 / 98.908 | 2.303 / 2.190 | 0.276 / 0.406 | 10.520 / 2.457 | 0.286 / 7.574 | 14.319 / 2.457 | 0.286 / 11.352 |
| ON2 | 1 | 0 | 97.192 / 99.043 | 1.197 / 1.171 | 0.170 / 0.215 | 26.938 / 5.881 | 0.225 / 21.010 | 32.488 / 5.974 | 0.225 / 26.382 |
| ON2 | 1 | 1 | 97.396 / 99.141 | 1.201 / 1.170 | 0.166 / 0.222 | 23.725 / 4.331 | 0.213 / 19.651 | 28.965 / 4.427 | 0.213 / 24.832 |
| ON2 | 1 | 2 | 97.119 / 99.107 | 1.111 / 1.065 | 0.221 / 0.289 | 27.411 / 4.845 | 0.269 / 22.722 | 33.299 / 4.936 | 0.269 / 28.406 |
| ON2 | 1 | 3 | 96.978 / 99.049 | 1.217 / 1.139 | 0.231 / 0.317 | 29.457 / 5.586 | 0.283 / 24.210 | 35.335 / 5.673 | 0.283 / 30.001 |
| ON2 | 1 | 4 | 97.455 / 99.146 | 1.245 / 1.214 | 0.172 / 0.221 | 23.176 / 4.202 | 0.220 / 19.046 | 28.290 / 4.291 | 0.221 / 24.011 |
| ON2 | 1 | 5 | 97.493 / 99.194 | 1.148 / 1.119 | 0.162 / 0.217 | 22.257 / 3.438 | 0.198 / 18.865 | 27.534 / 3.531 | 0.198 / 23.991 |
| ON2 | 2 | 0 | 94.842 / 98.650 | 2.574 / 2.271 | 0.277 / 0.400 | 13.004 / 3.211 | 0.294 / 8.117 | 18.358 / 3.211 | 0.294 / 12.287 |
| ON2 | 2 | 1 | 95.655 / 98.917 | 2.341 / 2.148 | 0.248 / 0.364 | 10.771 / 2.405 | 0.257 / 6.870 | 15.306 / 2.405 | 0.257 / 10.272 |
| ON2 | 2 | 2 | 95.567 / 98.926 | 2.158 / 1.852 | 0.217 / 0.327 | 11.013 / 2.227 | 0.226 / 7.200 | 15.561 / 2.227 | 0.226 / 10.684 |
| ON2 | 2 | 3 | 95.145 / 98.752 | 2.136 / 1.872 | 0.224 / 0.346 | 12.294 / 2.850 | 0.243 / 7.808 | 17.297 / 2.850 | 0.243 / 11.754 |
| ON2 | 2 | 4 | 95.630 / 98.883 | 2.252 / 2.134 | 0.282 / 0.400 | 10.815 / 2.393 | 0.292 / 7.144 | 15.424 / 2.393 | 0.292 / 10.657 |
| ON2 | 2 | 5 | 95.500 / 98.855 | 2.375 / 2.224 | 0.267 / 0.393 | 11.138 / 2.490 | 0.276 / 7.346 | 15.811 / 2.490 | 0.276 / 11.032 |

In the final remesh snapshots, the reduction in construction unattributed time varied by rank as follows. These are differences between runs. Scheduling can also move with wall-clock variation within the same work.

| Run | Event | Smallest reduction (s) | Largest reduction (s) |
|---|---:|---:|---:|
| ON1 | 1 | 24.201 | 29.924 |
| ON1 | 2 | 11.421 | 13.583 |
| ON2 | 1 | 23.999 | 29.662 |
| ON2 | 2 | 12.900 | 15.146 |

Each ON run emitted both shift rows on every rank in both events. The job peak equals the sum of rank peaks and remains below 19,073 MiB throughout the qualified events. The process peak does not isolate the transient two-map window. Each run reported one normalization stage per event. The structured receipts contain all core counters summed across ranks, plus each rank's limits, sweep counts, rounds, memory skips, cells before pass 1 and cell counts after the lower and higher shifts. [OFF work](../resume5/G5/OFF/run_receipt.json) · [ON1 work](../resume6/G5/ON1/run_receipt.json) · [ON2 work](../resume6/G5/ON2/run_receipt.json) · [Per-rank ledgers](../resume6/G5/timer_comparison.json)

OFF-reference payload and first-collapse differences were captured as reports. Both ON repeats satisfy the result identity gate against each other. [ON1 OFF payloads](../resume6/G5/ON1/off_payload_comparison.json) · [ON2 OFF payloads](../resume6/G5/ON2/off_payload_comparison.json) · [ON1 OFF first collapse](../resume6/G5/ON1/event_one_construction_report.json) · [ON2 OFF first collapse](../resume6/G5/ON2/event_one_construction_report.json)
