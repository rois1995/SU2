# F3b — reviewer correction, 2026-11-15

Input: source856b89ccb4/build503, correct ledger with failed complete-event coverage68.357–74.357%. Reviewer section2.0b identifies the unscoped repair join and clearance service. The source has78 scalar Allreduce invocations on77 source lines (one line contains two); replace every direct call except the unique private_work_used join, as required by the proposal.

Output: exactly two new operation names (JoinWait,Clearance); direct scalar collective wrapper without World-counter side effects; scoped join/clearance/contact/progress/candidate outer work. No algorithm, collective order/value, cache quota, memory option, budget or acceptance change. A5/B5 withdrawn, no work on either.

Acceptance, one check each, stop first failure: (a) one build and nine timer unit cases, including one case for both names; (b) six-rank first collapse work90809869, counts480/473/346/263/469/464 and each rankcoverage>=95%; (c) one full two-event milestone, each event/rankcoverage>=95%, JoinWait versus sum of legacy join_call_seconds within1%, all output mesh/ref/restart hashes equalP6c, native<=5400s/event, sum reported rank process peakRSS<=20000000000B/event. Report predictions (quality unattributed15–25s/rank; coverage96.5–97%) alongside measured values, without tuning to them.

Retain503 C492/481/current-target/2D evidence only if its executed objects have equal hashes. Snapshot422objects before build; compare after build. If changed, repeat affected checks once after(b) and before the full milestone. Reuse prior runners/physical validators; do not repin/overwrite prior receipts. One owned heavy job, nice19, at most6heavy ranks, libraries1, buildj4,4TBcompile scratch, actual host/RAM check before sustained work. Freeze all compiled sources during build/validation.

No join-wait optimization or catch-all timer. If a check fails, preserve the exact per-rank/phase counters and stop without another change/run. Readability: new functions<=80lines, small lambda<=15, comments and repository formatting; narrow repair leaves legacy bodies unchanged. Maintain current handoffs/timings and push clearly scoped checkpoints.
