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
  native summary. Ranking output includes the aggregate after all six ranks
  finish. The corrected read-only probe used the six rank IDs in rank 0's
  aggregate and took their maximum. Sealed workflow validation uses its
  existing rank-aware comparator.
