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
