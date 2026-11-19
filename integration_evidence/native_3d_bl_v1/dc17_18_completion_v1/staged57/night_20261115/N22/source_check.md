# N22 whole-item source check

Base is N21 checkpoint 4fe083d460 and active build589, with the qualified
wide solver SHA and six accepted payload hashes in N21/accepted_reference.json.
The pinned N22 text and the user amendment govern the gates. The N21-wide GQ
run is captured on this unchanged binary before any production edit.

| Required check | Source finding |
| --- | --- |
| M (a) counts and span | EngineCensus::MeasureCell increments initial_cells for seed work and updated_cells for update work, outside all timing guards. EngineTrial and EngineQueues increment attempted in the same EngineResult::stats. EngineAdapt, EngineCensus and EngineOwnership borrow this object throughout the event. Migration moves cached census records and changes no count normalization. NativeRemeshRun's separately named report stats are not the object read by ShouldMigrate. |
| M (b) removed seconds | Decision readers of wall time, the estimate/rebuild/warming values and prior ownership duration are confined to ShouldMigrate. Initialization, seed and ownership duration remain observations. Other readers are reports or timer accounting. Existing WithinTime and atlas deadline checks are outside the eligibility formula. |
| M (c) asserted migrations | Existing ownership fixtures that assert changes or movement select DiagnosticAlways or Disabled. The exact forced path and the final collective participation decision stay in place. A new Automatic fixture can prove it evaluated the formula by a positive accumulated predicted cost. |
| M (d) clock independence | The eligibility helper consumes only imbalance, demand, global cells, measured cells and attempts. The caller records rejected evaluation time after the helper returns false. This leaves ShouldMigrate itself free of clock reads while preserving the rejected-evaluation statistic. |
| C (a) calls inside a chunk | The only collective sites on a normal no-surface chunk are the time maximum, two preparation votes, the current-cavity import election, the admission vote and the final vote. The CavityMetricSource import, MetricEdgeRequests, FrozenField construction, metric and height checks, and row caching call local kernels. Sticky failures can add a second election in the existing Vote; the same election entry localizes both when deferred. |
| C (b) private repair | EngineAdapt sets private_repair only around private trial evaluation. It restores the flag before publication and before census/refill. SeedNext, phase entry, batch commit and slot commit all enter Measure with the flag false. Retain the proposed guard. |
| C (c) false ends event | EngineAdapt seeding invokes Finish on false. Refill and phase-entry callers short-circuit to FinishRound. RepairBatchCommit calls FinishRound and SlotCommit clears ready and calls FinishSlot. None uses the partial census after a false result. EngineControl::Finish assigns Failure when ordinary refusal leaves the stop code unset. |

## Contradictions and readings used

1. I8U 2.2 keeps ownership_seconds while the literal check in 5.1(d) forbids a clock read in ShouldMigrate. Preserve that statistic at the immediate caller on a declined exchange. The private signature no longer carries a clock parameter.
2. The six-call census count describes a normal successful chunk with a configured deadline. Vote can double its election on a sticky atlas failure, and a run with no deadline has no chunk time maximum. The deferred election entry also covers the sticky branch, and no deadline maximum is added when maximum_seconds is zero.
3. ImportCells skips its election only for the new deferred census scope. Its prior valid/reason behavior, work and accounting stay on every other route. Keep an error from a refused final one-cell import in the ordinary local failure, since that branch returns without a vote.
4. The GQ identity reference is the new N21-wide capture per user order. G2 keeps the existing N14 first-collapse anchor. G5 uses N21 W payloads and the explicit 2048.286 / 1278.700 s times until M seals its result. Part C only retains an otherwise passing purpose miss as OPEN under the user amendment.
5. The pinned Part C text preserves the seed/update budget reason. The legacy no-surface timeout fixture checks that reason and the stop code. The joined timeout uses that common reason after the original failure vote, so a recorded failure has precedence.

The source supports all required result readings. No production scope under
reviewer_handoff is involved.
