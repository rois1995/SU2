# FP2-C3 continuation 1 lab, build664

All six trials passed individually. The exact comparisons matched for every case, including the four new surface fields and the inherited contact rows and cell digests. Across 50 rank-pass checks there were zero contact failures and zero embedding violations. All three intermediate counts and every final out count were zero. [Lab acceptance](../cases/build664/lab_acceptance.json) · [Per-rank and pass comparisons](FP2_C3_lab_664.json)

| Case | Elapsed s, trial 1 / 2 | Strict faces | Step C removals, build660 / 663 / 664 | Metric directions refused per trial |
|---|---:|---:|---:|---:|
| N6P | 858.019 / 853.561 | 26,482 | 6,827 / 22,411 / 22,410 | 6 |
| N1 | 1,086.701 / 1,116.495 | 26,920 | 5,682 / 21,960 / 21,960 | 0 |
| N6 | 1,507.171 / 1,259.155 | 24,530 | 6,889 / 23,184 / 23,184 | 0 |

N6P's rank-0 pass-2 minimum rose from 0.0090487725598145056 to 0.010048002855871579. The floor refused four directions there and one each on ranks 1 and 2. N6P's remote screen refused 9,388 of 33,473 outward directions. N1's corresponding counts were 0 and 23,061. In the previous S0 production run the engine removed 2,202 and 2,031 surface points after the core, for events 1 and 2 respectively. [Build663 failed case and attribution](FP2_C3_663.md)

N1's passes took 944.469 / 970.973 s, above the 600 s reporting threshold. In trial 1, step C took 868.289 s, with 775.925 s in the inclusive Bound clock. The observer timed 72,564 forwarded geometry calls at 775.167 s in total. One wing entity (body 0, sheet 10154, marker 2) accounted for 64,408 calls and 755.072 s. Calls at or above 100 ms used 183.443 s across 1,199 calls. Non-direct calls used 772.852 s versus 2.315 s for direct calls. The calls associated with surviving face keys used 313.012 s, committed children that did not survive used 461.866 s, and the remaining one call used 0.289 s. Observer bookkeeping took 0.796 s and accounted for 11,616,929 scratch bytes. Lab step C clocks include observer work. The tables contain available classification work and mark complete query work unavailable.

N6P trial 1 captured every Refused face from the final step-D censuses, 111 in pass 1 and 137 in pass 2, with available cached samples for all. A face edge matched a metric-floor event on 45 and 64 of those faces, respectively. The rank, oriented face data, refusal ordinals, wing bins, and the counts outside the bins are preserved in the detailed report. [Detailed observer rows](FP2_C3_lab_664.json)

N6 trial 2's first launch was held by preflight because it saw a sleeping MPI launcher. The launcher exited before the campaign resumed. No test had started and the five earlier receipts were reused. All pair comparisons then passed. R3 corrected the report writer's numeric event identifiers to strings for JSON round-trip equality. The report digest stayed `46a888e9e9c65acf221152b98bf014fe956ec82dee620296f0113d50b8b2c093`, and all six trial receipts were reused. [Preflight observation](../lab_campaign664/N6_repeat2_preflight_block.json)
