# FP1 resume 2, build609

[Verified order](../inputs/FP1_RESUME_2.md) · [Whole-order source check](../resume2/source_check.md) · [Previous OFF acceptance and ON failure](FP1_608.md)

The resume order SHA256 is `92b1eaa6a01b3c053b0a877f6cbc5ba9974b03050d6be827f6d4efc56b91750c`. The prior 179 recorded process identities were terminal before editing.

The first-pass point move now gives the existing sampler only the donors with computable, finite affine weights, in the old order. If none can be used, the trial step receives a Numeric refusal and the next step can run. The shared sampler and the refusal cache rules remain as written. Timing and evaluation counts are report-only.

## Scope reading

- Section 5.1 names `PrintLocalPass` as the NRR edit. Build, writeback and the election are in `RunLocalCore`. Section 2 needs the two phase clocks and section 4 needs completed peer rows after a failed vote. The R4 reading uses report-only clock assignments in the two existing operations and emits rows for locally finished ranks after the same vote. The pass total retains its inclusive meaning, including meeting, admission and peer wait. The question and chosen reading are in the R13 ledger.

- Reporting compatibility. The archived resume 1 G5 reporter treats every extra local-pass field except `seconds` as an integer. The new replay reader retains the round and sweep lists as text and parses the component clocks. The earlier G5 helper remains pinned to its historical receipts.

## Build and fixtures

Build609 compiled and linked in 134.787 s with zero new warnings. Solver SHA256 `df69bd05cf65c12184da379ddb3b2a1eae4fecf5965fa7c8ba60fec411f0eb8a`. Test driver SHA256 `a27061ab00c4ee21ba041881586791d1fa720c1fa92d975d8cd85d787d9a6098`. [Build receipt](../resume2/build609/build_status.json)

The new fixture passed all 12 assertions. It established exact positive orientation for the pinned flat tetrahedron and the prescribed affine-weight exception. Mixed donors returned the analytically expected P1 tensor. An all-flat list refused the step and preserved the output tensor. All-normal input matched the shared sampler bit for bit.

All inherited fixtures passed at their previous rank counts with identical assertion totals and wired edit rows. Numeric refusals stayed at zero in those wired fixtures. The new per-round and per-sweep lists reconciled with the operation and sweep totals, clocks were nonnegative, and evaluated counts bounded the corresponding accepted edits. [Fixture parity](../resume2/fixture_parity.json)

The code scope check against `f36b66eadd` passed under the recorded R4 reading. Outside the local core and new test, the remesher diff is confined to the pass report fields, their two guarded clock measurements and the finished-rank print. [Code scope](../resume2/code_scope.json)

G1 matched all 375 / 114 / 228 adopted records at 1 / 2 / 4 ranks in 784.477 / 10.448 / 10.411 s. [G1 evidence](../resume2/G1_609/checkpoint_paths.json)

OFF G2 passed in 257.098 s to cutoff. Exact first-collapse work was 90,809,869, with counts 480 / 473 / 346 / 263 / 469 / 464. All exact rows, modeled bytes, and decisions matched under the inherited Part C comparator, with `fixedstorage=4120`. The report-only communication fields and counters remain in the comparison receipt. [G2 evidence](../resume2/prefix/G2_comparison.json)

The GQ host sampling saw a separate compiler workload in an unrelated workspace around 10:42. The owned tree remained the six low-priority GQ ranks. The observation is part of the runtime context.

OFF GQ passed in 906.877 s to cutoff. All exact rows matched the N22 capture, including modeled storage, ownership decisions and `fixedstorage=4120`. Build608's accepted OFF G5 receipt continues to qualify the six adopted files. [GQ evidence](../resume2/prefix/GQ_comparison.json)

## Replay provenance

Event 1 uses the adopted FP0 B geometry, original authority and frozen sensor derived from `restart_flow_adap_00000.dat`. The effective config has the original timings setting and the sole added line `ADAP_NATIVE_LOCAL_CORE= YES`. SHA256 `1ec6c26d49d4ec7c62103d5bd096471b5422290cd06c44b2f1073b7489f2b4c1`. Both failed preflights stopped before a fixture process was launched because an unrelated compile was active. Preserved the attempted commands. The third preflight cleared and spawned the single owned six-rank replay. [Pinned inputs and controller](../resume2/event1/pins.json)

Two two-second CPU samples were collected before the first pass printed. At about 689 s, ranks 2 and 3 had sampled quality stacks, while ranks 0, 1, 4 and 5 were sampled in MPI. At about 2,057 s, 65 of rank 3's 92 samples resolved to the quality sweep, including point-move affine weights. Every sampled stack on the other five ranks included MPI. These are stack samples and do not replace the pass clocks. [Rank sample counts](../resume2/event1/cpu_sample_ranks.json)

R3 reporting check. A helper dry-run exposed that an interrupted fixture can have a null native receipt. The summary now uses the replay disposition and leaves unavailable native fields empty. Verified with a timeout-only sample against the existing OFF comparators. No solver run or gate changed.

Pass 1 and the lower shift completed on all six ranks. Pass 1 took 4,074.199 s inclusive. Rank 3 spent 3,677.323 s in quality, 308.308 s in collapse sweeps and 84.586 s in split sweeps. It accepted 357,235 splits and 1,059,690 moves and finished with 1,112,091 local cells from 51,202. Rank 2 spent 1,060.993 s in quality. Rank 0 spent 1.806 s in quality. Each rank reported zero all-unusable Numeric step refusals. The six partitions and lower shift each totaled 1,642,036 cells. This is private core output before engine entry.

The later pass 2 sample resolved rank 2 work to the quality sweep and rank 3 work to split sweeps. The other four ranks were sampled in MPI. The sampled peak sum at 4,613.659 s was 5,863,540 KiB across six ranks. [Pass 2 stack counts](../resume2/event1/cpu_sample_pass2_ranks.json) · [Host RSS samples](../resume2/event1/host_samples.jsonl.gz)

Resume 3 §0 authorized ending the live replay. The verified timeout group received SIGTERM at elapsed 5,301 s. Its wrapper terminated at 5,303.001 s. Rank 3 was still sampled in the second-pass quality sweep near elapsed 5,251 s. All five peers were in MPI during that sample. Pass 2 and the higher shift produced zero completion rows. The native remesher did not publish an event result. The nonzero fixture exit reflects the authorized stop. No optional event 2 was started. [Stop receipt](../resume2/event1/resume3_authorized_stop.json) · [Near-stop rank stacks](../resume2/event1/cpu_sample_near_deadline.json)

The build, fixtures, G1 and exact OFF prefix gates all passed. Resume 3 retains Part G and directs the next implementation. FP1 remains open with the accepted OFF proof and the ON correction in work.
## Frozen event 1

| Measure | Core ON replay | Morning OFF G5 | FP0 OFF replay |
|---|---|---|---|
| Outcome | Rejected | Accepted | Accepted |
| Recorded comparison wall s | — | 1,818.319 | 1,991.772 |
| Native maximum rank wall s | — | 1,818.319 | 1,991.707 |
| Timer root maximum rank s | — | 1,818.373 | 1,991.772 |
| Minimum timer coverage | — | 95.257% | 95.554% |
| Job peak bytes | — | 14,031,564,800 | 13,996,642,304 |
| Rank peak sum bytes | — | 14,031,564,800 | 13,996,642,304 |
| Engine accepted collapses | — | 9,709 | 9,718 |
| Engine accepted splits | — | 82,036 | 82,020 |
| Normalization stages | — | 1 | 1 |
| Engine entry cells / points | — / — | 303,359 / 61,330 | 303,359 / 61,330 |
| Accepted exit cells / points | — / — | 694,248 / 138,661 | 695,524 / 138,878 |
| Engine rounds by rank | — | 25852 / 25852 / 25852 / 25852 / 25852 / 25852 | 25670 / 25670 / 25670 / 25670 / 25670 / 25670 |


| Max exclusive phase s | Core ON replay | Morning OFF G5 | FP0 OFF replay |
|---|---|---|---|
| metric | — | 4.263 | 4.436 |
| preparation | — | 64.113 | 135.964 |
| construction | — | 460.024 | 473.723 |
| quality | — | 1,183.856 | 1,271.258 |
| final_proof | — | 65.801 | 65.278 |
| transfer | — | 30.608 | 31.508 |


| Rank | Stage | Pass | Inclusive s | Build s | Collapse s | Split s | Quality s | Check s | Write s | Other s |
|---|---|---|---|---|---|---|---|---|---|---|
| 0 | 0 | 1 | 4,074.199 | 0.058 | 18.068 | 15.365 | 1.806 | 0.081 | 0.022 | 4,038.800 |
| 1 | 0 | 1 | 4,074.199 | 0.061 | 27.150 | 18.779 | 2.995 | 0.057 | 0.021 | 4,025.136 |
| 2 | 0 | 1 | 4,074.199 | 0.067 | 147.868 | 125.903 | 1,060.993 | 0.752 | 0.088 | 2,738.529 |
| 3 | 0 | 1 | 4,074.199 | 0.055 | 308.308 | 84.586 | 3,677.323 | 3.372 | 0.483 | 0.073 |
| 4 | 0 | 1 | 4,074.199 | 0.059 | 18.495 | 15.387 | 2.379 | 0.061 | 0.024 | 4,037.794 |
| 5 | 0 | 1 | 4,074.199 | 0.054 | 11.613 | 15.671 | 0.828 | 0.057 | 0.017 | 4,045.960 |

[Complete rank rows and comparison data](../resume2/event1_comparison.json)

