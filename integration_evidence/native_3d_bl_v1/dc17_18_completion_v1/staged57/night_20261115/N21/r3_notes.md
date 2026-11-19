# N21 test and evidence helper corrections

- The entry preflight path was first looked up under the phase runner or the
  staging root. The accepted host preflight is in
  `end_B_preparation/primary_control_archive` under the phase workspace. The
  corrected command ran before the first source edit and found no owned job.
- The W adapter's initial no-launch check tried to reuse an exclusive name
  for the inherited helper pins. The new adapter records its own pins in
  `adapter_pins.json` and retains the inherited manifest unchanged.
- The initial proof report adapter looked for both the proof rows and timers
  in the historical stdout stream. Final proof and certificate lines are on
  rank stderr, while timers and M28/M29/fine rows stay on stdout. Both helpers
  now read those channels separately. No-launch checks cover exact stderr
  event grouping, gzip replay and incomplete records.
- The event-one interim clock probe assumed a rank log contained a single
  native summary. Rank output includes the aggregate after all six ranks
  finish. The corrected read-only probe used the six rank IDs in rank 0's
  aggregate and took their maximum. Sealed workflow validation uses its
  existing rank-aware comparator.
- The wide-run interim certification probe first read the ledger's decimal
  strings as text. It now converts each rank's seconds to a number before
  taking the maximum. The sealed purpose comparator already uses numeric
  decimal values.
- The report writer originally treated an existing native-rejection dump as
  a conflicting report output. It now links the dump sealed by the workflow,
  or writes one only when a rejected event has no existing dump. The report
  formatter self-check passed.
- T4 requires two or four MPI ranks. The initial case entered the default
  Catch selection because the MPI tag alone does not hide it. The fixture now
  uses the repository's hidden tag. Build589 excluded it from default
  selection, repeated T1–T3 with 925 assertions and T4 with 103 assertions per
  rank at 2 and 4 ranks, and verified the build588 CFD solver hash exactly.
