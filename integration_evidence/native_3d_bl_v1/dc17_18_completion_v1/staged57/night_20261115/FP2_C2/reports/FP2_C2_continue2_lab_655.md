# FP2-C2 continuation 2 lab, build655

**FP2-C2: lab passed; default path not yet proven**

Build655 uses the build654 solver and unchanged Common and SU2_CFD source pins. The corrected fixture verified the real Bound refusal and three unchanged snapshots, then exercised the removals-only state exchange through the test wrapper. The option-off cohort matched build652, all 18 FP1 cohorts passed, and G1 matched 375 / 114 / 228 records. Fixture evidence was pushed as `4494472267`. [Corrected order](../inputs/FP2_C2_CONTINUE_2.md) · [Fixture acceptance](../fixtures/build655/c2_acceptance.json)

All six lab trials passed. Each case matched exactly across its paired trials on the required fields. The comparator initially compared a tuple with its serialized list after writing the acceptance. Its writer now uses a list. The finalized receipt uses the completed trials and preserves all prior checks. [Lab acceptance](../cases/build655/lab_acceptance.json)

| Case | Trial 1 / 2 elapsed s | C collapses per trial | Bound refusals per trial | Strict faces checked | Above tolerance with D reasons | Unexplained | Model gap entities |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| N1 | 525.297 / 441.984 | 5,930 | 3 | 75,954 | 0 | 0 | 0 |
| N6 | 569.022 / 547.965 | 7,089 | 5 | 73,618 | 4 | 0 | 0 |
| N6P | 383.921 / 385.153 | 7,027 | 2 | 72,208 | 324 | 0 | 0 |

The overlay answers matched the census on every face in all trials, including the surviving collapse children. No rank/pass conservative storage sum exceeded the modeled ceiling (0 observed). Each rank/pass report includes its admitted bytes, cumulative overlay peak and largest query scratch. The N1 build652 comparison retains its failed proof, and entity-face counts for build653 are unavailable. The JSON report includes the final counts by entity for each new trial and the false Bound answers beside accepted collapses per sweep. [Per-rank and per-entity report](FP2_C2_lab_655.json)

S is the next ordered gate.
