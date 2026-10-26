# Frozen M6 MPI attribution — diagnostic55

The bounded run measured MPI-call duration, not pure communication time or complete
useful computation. [Decision and numerical table](M6_DIAGNOSTIC_DECISION.md).

## Method

`diagnostic55/mpi_calls.c` is an optional LD_PRELOAD PMPI collector. Weak symbols
in the hidden frozen-field test enable/disable it around `backend->Remesh()` only.
It wraps eighteen blocking/nonblocking/communicator routines used by the path,
records count/type width/reduction/communicator size and direct return address,
and aggregates count/duration/maximum in a bounded 4,096-entry local table.
Six ranks report zero overflow. Nonblocking initiation and Waitall durations are
separate; they do not measure asynchronous progress outside those calls.

There are no added collectives, barriers, clock-alignment messages or sampling
threads. Durations use each rank's own PMPI_Wtime; absolute timestamps are not
compared across ranks. Output occurs after disabling measurement. Direct callsites
are resolved against the exact pinned ELF symbols, not reconstructed call stacks.
The test binary and PMPI library are pinned; no production numerical source changed.

The two-rank collector self-check verifies ten Allreduce calls plus one broadcast,
their values, counts, datatype widths and elapsed bounds on each rank. Both M6
replays complete with launcher exit zero and native work-budget rejection at 2,048.
Ten selected per-rank work/state counters agree. This is not a bytewise mesh check
and does not establish identical candidate sequences or execution trajectories.

## Observations

- Approximately 670,000 wrapped MPI calls per rank over the full remesher event.
  Initialization, source setup, weight evaluation and demand seeding are included;
  dividing this count by accepted edits would misattribute fixed setup work.
- The direct `CPassiveComm::AllreduceMax(unsigned long, communicator)` callsite
  executes 465,080 times per rank, taking 17.23–22.73 seconds. It reduces one
  unsigned-long value. Payload width is not network traffic volume.
- This helper is used by `ElectFailure` (`CDistributedSearch.cpp:440–458`) and
  other maxima. The trace does not identify which outer caller issued each call.
  The prior short perf sample suggests failure agreement is significant, but it
  does not justify assigning every helper call to failure agreement.
- Total wrapped MPI durations: 25.909, 25.996, 18.739, 24.251, 22.822, 24.044 s
  on ranks 0–5, respectively. Event elapsed: 62.723 s maximum. Outside wrapped
  MPI: 36.814, 36.668, 43.925, 38.413, 39.842, 38.620 s. These residuals include
  local computation, scheduling, diagnostics, bookkeeping and unwrapped MPI work.
  They are not a useful-compute workload measure.
- Existing code-native tracked collective timers cover only some calls and are
  nested in phase timings. They cannot replace the PMPI totals or be added to them.
- Separate source/import/donor/reconstruction scopes still include synchronization;
  complete MPI attribution per subphase and dependency request/service is unavailable.

The observed run's per-rank reconstruction CPU scopes are 3.894–4.156 s, compared
with 4.119–4.380 s in the plain run. Nested callbacks/source work and other useful
work are not fully classified; no whole-event useful-compute imbalance is reported.
The full validation54 CPU scopes of approximately 330–344 s are a different run.

## Overhead and conditional accounting

Matched plain/observed remesher durations are 89.999 / 62.723 s, delta -30.3%.
On a shared host, with one run of each and no exclusive CPU allocation, this does
not estimate true profiling overhead. The difference must not be reported as an
optimization or zero-overhead proof. All runs use six ranks, `--bind-to none`,
nice 19 and single-threaded numerical libraries; inputs and operation counters match.

For this fixed observed run only, subtracting every wrapped MPI duration while
keeping each rank's remaining elapsed work unchanged gives a maximum residual of
43.925 s and the quotient 62.723 / 43.925 = 1.428. This is a hypothetical accounting
illustration, not an engineering speedup ceiling or forecast. Removing waits can
change scheduling and subsequent work, invalidating the fixed-residual assumption.
It does not bound batching, repartitioning or full-M6 performance improvements.

The validation54 five-second profile (~74% inclusive MPI_Allreduce samples) is a
CPU sample from a different stage, not contradictory to these full-prefix wall
durations and not a valid substitute for them.

## Files and remaining attribution

`diagnostic55/m6_mpi_callsites.csv` contains per-rank aggregated direct-callsite
measurements; `observed/mpi_calls_rank*.csv` retains count/type/communicator details.
`mpi_analysis.json` contains fractions, residuals and the original conditional-bound
field labels; those historical receipts retain their hashes. Interpret that quotient
with the accounting limitations above.
`bounded_records.json` retains per-phase native logs with raw line numbers;
`m6_rank_phase_timings.csv` is the independently reconciled full-run baseline.

No global clock alignment, causal waiting graph, pure network cost, regional
useful-work labels or complete accepted-cycle performance has been obtained.
Further timing should target the construction transaction named in the decision,
including its outer failure-vote/source call sites, rather than another scaling matrix.
