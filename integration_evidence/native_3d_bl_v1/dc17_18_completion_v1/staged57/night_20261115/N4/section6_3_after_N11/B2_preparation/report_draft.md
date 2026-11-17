# CUT B2 report draft — exchange binding extraction

**Status:** B2 source extraction is applied and build551 passed. G1 MPI4 and G2 remain pending in the active sequence; this report does not claim phase-B completion or acceptance.

## Change and source proof

B2 moved six existing `Adapt` lambdas into `EngineExchange`: `elect_trial_failure`, `vote`, `bytes`, `real_bytes`, `admit`, and `import_source`. The extraction routes each current call directly to the member and borrows the canonical live `EngineContext` state. It adds no storage fields, timer scopes, catches, forwarding closures, or policy decisions. `import_source` continues to ignore its existing `requests` argument.

The generated binding review records whole-file byte reversal, token-equivalent method bodies, preserved captures and constness, live reads of `private_repair` and atlas geometry, and unchanged accounting operands/order. Call counts recorded by that review are 131 `vote`, 54 `bytes` in the main body (plus its internal helper uses), 5 `real_bytes`, 40 `admit`, and 2 `import_source`; `elect_trial_failure` is called internally twice. The callback parameter was also removed from the three B1 report helpers, with the two accounting calls routed directly through `EngineExchange::Bytes`.

The scoped findings and source bindings are documented in `B2_preparation/B2_exchange_binding_inventory.md` and `B2_preparation/primary_binding_plan.md`. The actual-apply receipt says all five base/proposal hashes matched, byte reversal passed, and the accounting sequence was preserved. Its initial pending list was build, G1, and G2; build and serial/MPI2 G1 are now resolved.

## Build and completed G1 evidence

Build551 completed with exit code 0 in 90.159 seconds, verified its source pins, and reported no changed pins. The sealed binary hashes are CFD `0c53e51e7935aa6f268a1b87803e210fcd53e01a1a371050053af677da31bb4b` and test driver `e07ecd0e632c48e5766d30a1be852194ce86779ee0de3ce3d9e9462aa9129dc9`. See `build551/build_status.json`, `build551/primary_terminal_receipt.json`, and `B2/validation/build551_status.json`.

The serial G1 exact comparison passed all 375 case records with zero changed cases, zero exceptions, unchanged case pins, and equal reporter totals. MPI2 passed the same comparison across 114 records with zero changed cases or exceptions and unchanged pins. The successful-assert totals are report-only. Terminal receipts verify the associated owned handles exited normally. See `B2/validation/G1_C2_1_exact_serial.json`, `G1_C2_1_exact_mpi2.json`, and their primary terminal receipts.

## Still required

- **G1 MPI4:** the primary has an MPI4 run active. Its result and terminal seal are pending; this draft intentionally does not read or summarize its live receipt.
- **G2:** not started. Its required first-collapse comparison and terminal evidence remain pending.
- **Phase-B boundary:** continue through the latest required N14 and N15 work and make that committed boundary before the remaining phase-B items. Do not describe N14 as released or validated unless its own evidence establishes that.
- Stop after the defined end-of-phase-B gates, then commit and push that boundary. Phase C is not authorized in this sequence.

## Limits of this report

The method-token proof and build are not substitutes for the outstanding MPI4 and G2 gates. No full workflow or phase-B acceptance is claimed here. This is a report draft; active receipts were left untouched.
