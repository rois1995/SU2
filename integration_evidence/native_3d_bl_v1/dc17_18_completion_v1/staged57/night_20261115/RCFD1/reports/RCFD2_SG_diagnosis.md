# R-SG event 2 failure

The first failed standing result was native event 2 completion, `complete=0` on all six ranks against the required 1. Native maximum was 506.927 s and reported job peak was 9,048,166,400 bytes. At the exit census, rank 1 had `q_bad=2` and `j_bad=1` against zero for both. The two surface cells had persisted from metric cleanup through quality exit. The other ranks had zero q and J failures at exit, and all six had zero size and geometry failures in the exit census. [Rejected-event receipt](../G5/build647/SG/full647_run1/first_rejected_event.json) · [Rank 1 output](../G5/build647/SG/full647_run1/rank_output/1/rank.1/stdout.gz)

| Rank 1 surface cell | q | J | Existing M28 flags |
| --- | ---: | ---: | --- |
| 49361531 | 0.142482298667 | 0.048994984045 | q and J |
| 49704185 | 0.177206192653 | 0.064521031607 | q |

The driver rejected replacement with `Remesher returned incomplete quality`. The controller stopped under its first-rejected-event rule at 1,497.024 s. The second event's native timer receipt recorded coverage of at least 98.463%. The full two-event check still requires both accepted events and their complete timer/coverage and transfer evidence. There were zero final proof faces and no event-2 mesh publication. Cycle 1 completed all 500 iterations, and the stop prevented cycle 2 and its isolated restart. Inputs and source/binary pins verified unchanged. The read-only diagnosis used the exit census and existing M28/M29 counters. [Native timer receipt](../G5/build647/SG/full647_run1/f3b_event2_timer_gate.json) · [Terminal receipt](../G5/build647/SG/full647_run1/status.json)
