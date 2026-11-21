# FP2-A S1 rejection, build627

One read-only R5 diagnosis after the controller ended on 2026-11-21 at 21:14 CET. The checks used the sealed rejection receipt, the existing per-rank stage, private-work and timer rows, and the source that handles the deadline and rejection. The diagnosis completed at 21:16 CET. [Driver failure](../G5/build627/S1/failed_attempt.json) · [First rejected event](../G5/build627/S1/full627_run1/first_rejected_event.json)

## First failed standing gate

Event 1 reported `complete=0` on all six ranks, against 1. Its maximum native time was 5,430.853544005 s on rank 5, exceeding 5,400 s by 30.853544005 s. The existing controller stopped its verified process group after the first rejected adaptation. The wrapper recorded solver group exit -15 and controller exit 1. It finished in 5,523.111 s with all recorded handles terminal and with source, binary and inputs pinned.

Final named coverage was at least 99.736% and every final rank ledger showed `fixedstorage=4120`. The reported job peak equalled the sum of the process peaks at 13,358,960,640 B. The remaining workflow gates require an accepted adaptation. There was no accepted S1 event for mesh publication, transfer, continued CFD on that adapted mesh or isolated restart. Event 2 and S2 are unevaluated, and repeat payload identity is unevaluated. [Timer receipt](../G5/build627/S1/full627_run1/f3b_event1_timer_gate.json) · [Controller status](../G5/build627/S1/full627_run1/status.json)

## Cause supported by the existing counters

The corrected surface steps completed on all six ranks. Stage 0 accepted 18,120 splits, and the collective state build installed 85,194 faces, 42,599 points and 748 traces. This preceded entry into native engine quality repair. The final exclusive quality time ranged from 5,122.492 to 5,122.583 s per rank, with exclusive construction at 230.102 to 230.151 s. The final repair window counts plateaued by 20:10:48 CET and stayed at these values through the 21:13:27 exit census.

| Rank | q failures | J failures | Size failures | Geometry failures |
| --- | ---: | ---: | ---: | ---: |
| 0 | 0 | 0 | 0 | 0 |
| 1 | 1 | 1 | 0 | 4 |
| 2 | 3 | 0 | 0 | 2 |
| 3 | 0 | 0 | 0 | 2 |
| 4 | 0 | 0 | 0 | 0 |
| 5 | 1 | 0 | 0 | 11 |
| Sum | 5 | 1 | 0 | 19 |

The last private-work rounds still reported rank 5 making one selection per round while the other ranks often reported none. The unresolved counts remained steady for at least 3,759 s. The rejection names exhaustion of the elapsed-time budget during geometry/private work and retains the curved atlas deadline message. It also reports that the last local rejection on rank 0 reduced neither frozen size excess nor complete measured shape debt. The existing private-round join elects the elapsed deadline, and the geometry vote propagates the cached atlas exhaustion. The evidence identifies a repair plateau followed by the deadline. The specific geometric or topological obstruction behind each residual remains undetermined. The executed logs and source review found no transcription or wiring error to correct. [Per-rank timers and rows](FP2_A_627_C_rows.md)

Part C default qualification stays passed at `36d41fc3c3`. The order closes §11.3 on the rejected S1 attempt. The source implementation is retained and there is no new surface-enabled output reference.
