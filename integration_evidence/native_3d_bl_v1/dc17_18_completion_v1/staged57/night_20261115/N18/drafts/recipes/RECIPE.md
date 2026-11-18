# N18 Part F validation recipe

The workflow adapter reuses the N16 C workflow, the chronologically normalized event-one comparator and the accepted isolated restart runner and validator. They remain separate from production source. The draft ran only Python source checks and read-only checks of historical receipts.

## Use

Run `workflow_f.py prepare --build 580` once to bind the seven N16 C helpers and write their hashes into the fresh `N4/section6_3_after_N11/round_8_5r2_F/G5` workspace. The selected build number can change with `--build`. The runtime location preserves the directory depth relied on by the N16 workflow. The run expected alias is `N4/section6_3_after_N11/build580_aliases.py`.

After D/E and the F scoped gates and safeguards pass, use the actual host:

```sh
/usr/bin/python3 workflow_f.py run --build 580 \
  --scoped /path/to/F/scoped/acceptance_summary.json \
  --safeguards /path/to/F/safeguards/acceptance_summary.json
```

Both scoped receipts use `passed` and `build`. Each long stage calls the accepted actual-host preflight. The numerical launches inherit the existing resource budgets, rank count, bind settings and nice value. The runner verifies the frozen build pins and binary digests before launch, then matches both full-run binary provenance digests against the same build. T0's sealed workflow receipt and six rank-stream hashes are verified before use.

Run one runs the standing physical, time, memory, transfer, continued CFD, timer and proof coverage gates. Its event-one prefix compares against T0 and then it undergoes isolated event-two disk restart and post-validation. Run two performs all standing gates and compares the six output hashes against the observed run-one payloads. The new reference manifest is emitted only on a passed gate 8. A nonzero child result cannot trigger a repeat and the raw rank logs stay in their case directories. Terminal, zero-exit controller receipts can be reused when resuming interrupted outer orchestration. An interrupted stage without a verified terminal wait requires the parent's normal process audit.

## Purpose and measurements

The purpose calculation takes the smaller native maximum for each event independently. The gate uses the two stated event ceilings of 2236.870 and 1416.499 s and the stated combined ceiling of 3288.032 s. These decimal literals come from the user-confirmed rule. The unrounded result of multiplying the stated T0 sum by 0.90 is 3288.0321 s. The exclusive quality sum S stays in the report.

The prefix check ignores the explicit new M28 marker families. The inherited cutoff excludes the enlarged private-work record. The dedicated M28 helper consumes the retained rank logs and receipts after the workflow ends. The workflow leaves the STEP report and CSV to the parent. A rejected case with terminal raw output keeps the available prefix evidence, status and six raw-log digests in `failed_attempt.json`. Gate 8 stays unevaluated after a gates 6 or 7 interruption. The parent determines the gate disposition and any R5 rollback.

## No-launch verification

`check_without_launch.py` checks helper binding in a private temporary directory, Python compilation and the exact gate arithmetic. It confirmed all six T0 and rejected N16 C prefix streams match at 450 / 446 / 446 / 446 / 447 / 446 rows. It also confirmed that the retained N6 restart validator's `source_event_receipt_sha256` is the digest of `full_workflow_receipt.json`. No build, CFD or MPI run was launched.
