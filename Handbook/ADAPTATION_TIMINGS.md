# Adaptation timings and computational work

## Fast path stage 2

### FP2-P2 vertex witness, build672

The same-build N1 control and closure-on D1 trial used identical inputs. Step C fell from 913.506 to 419.957 s and nested Bound time fell from 811.483 to 319.598 s. Classification work fell from 463,538,756 to 156,105,696 units, with traversal decreasing from 416,021,808 to 77,469,988 and scope building increasing from 6,651,129 to 37,769,889. The first-collapse work sums were 90,809,869 with closure off and 76,652,506 with closure on, against the 108,459,426 ceiling.

| Closure-on lab case | Trial 1 s | Trial 2 s |
| --- | ---: | ---: |
| N1 | 640.378 | 645.077 |
| N6P | 494.160 | 536.812 |
| N6 | 742.555 | 792.215 |

All three lab pairs matched exactly. Traced N1 qualified in 680.214 s. It recorded 8 invalid footprint groups across 68,858 plane rows, beside 11,860 across 68,809 for FP2-P1. The trace clock includes collection overhead.

| S event | Build664 native s | Build672 native s | Build672 minimum timer coverage | Build672 job peak bytes |
| --- | ---: | ---: | ---: | ---: |
| 1 | 951.321 | 655.988 | 98.015% | 9,586,192,384 |
| 2 | 689.037 | 350.407 | 99.000% | 9,686,908,928 |

The single opt-in S observation passed both native events, continued CFD and isolated restart (56.881 s). The report gives the per-rank exclusive clocks for geometry bound, communication plus join wait, and other operations. S clocks compare separate runs. All six payloads differ from build664. [FP2-P2 report](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FP2_P2/reports/FP2_P2_672.md)

### FP2-P1 conforming witness, build670

All six closure-on lab trials qualified with exact same-binary comparisons. The closure-off N1 trial matched build664 in 1,121.645 s. The pass and Bound clocks use the same projection as the stage-2 lab report.

| Case | Build664 trial 1 s | Build670 closure-on trial 1 s | Build670 closure-on trial 2 s |
| --- | ---: | ---: | ---: |
| N6P | 858.019 | 1,283.069 | 1,283.655 |
| N1 | 1,086.701 | 1,529.068 | 1,527.192 |
| N6 | 1,507.171 | 2,746.051 | 2,250.350 |

ANATOMY qualified in 1,996.941 s and matched the trace-off N1 projection. Over the 70,362 grouped miss and bypass forwards, the root-enough count rose from 51,772 to 52,249. Result work rose from 480,614,358 to 703,296,684 and prepared nodes rose from 7,552,490 to 50,438,362. These are measured trace comparisons with FP2-M2. The trace clock includes collection overhead.

The closure-on first-collapse check exceeded the 108,459,426 work ceiling. The existing counters attribute the increase from 90,809,869 to 111,921,030 units to scope building (+10,787,910) and traversal (+10,323,251). Both settings built 12,494 scopes and issued 22,889 upper calls. S timings and isolated restart are unevaluated. [FP2-P1 report](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FP2_P1/reports/FP2_P1_670.md)

### FP2-M3 footprint trace, build668

N1 trace-off and trace-on took 1,126.088 and 1,206.416 s. FP2-M2's baseline and anatomy took 1,180.061 and 1,253.027 s. These trial times include different observation costs. Both M3 projections matched build664 exactly, and the M2-format counts of forwards, prepared nodes, root-enough forwards and result work matched M2 in every bin. The provenance stream was 136,934,750 compressed bytes. Offline analysis took 113.726 s.

Of 70,482 uncached step-C forwards, 68,853 had a usable witness batch and 1,629 had no batch. In the 20 mm-and-above cohort with a usable batch, 4,887 of the 6,451 forwards needing more than the root had an inside T-junction. The analyzer found 14,844 intra-facet and 19,004 inter-facet pairs in that cohort. The 11,625 root-enough forwards in the same bin had no classified inside edges. The classes describe the measured witnesses and establish an association. [FP2-M3 report and tables](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FP2_M3/reports/FP2_M3_668.md)

### FP2-M2 lab, build667

The serial baseline took 1,180.061 s and the instrumented ANATOMY run took 1,253.027 s. The sealed build664 run took 1,086.701 s. Both exact comparisons passed. On trace-off non-direct faces with longest edge at least 20 mm, physical traversal used 86.8% of upper-bound time and scope building used 11.0%. On those faces ANATOMY counted a median of 1 prepared node, a 90th percentile of 1,049, and a maximum of 20,297. The root was enough for 64.3% of these forwards. The traced elapsed time includes collection overhead.

| Lab tolerance | Trial s | Forward calls | Mean forward ms | Cells out | Hand-over faces | Inherited evaluation |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| 1E-4 | 1,180.061 | 72,564 | 11.418 | 373,889 | 26,920 | Qualified |
| 2E-4 | 809.411 | 62,373 | 8.177 | 364,922 | 22,208 | Altitude finding |
| 5E-4 | 485.966 | 50,232 | 4.399 | 370,213 | 25,146 | Altitude finding |
| 1E-3 | 369.420 | 41,964 | 2.760 | 375,293 | 28,892 | Altitude finding |

The larger-tolerance runs measured altitude 7.7325745101917715E-5 on rank 0, pass 1, below their respective minima 1.4E-4, 3.5E-4 and 7E-4. Each rebuilt proof qualified and accepted at its trial tolerance, with zero faces classified above it. The inherited step clocks include the observer. The frozen sensor supplies the same input tensors, and the measured tolerances move several geometry thresholds together. [FP2-M2 tables and trace sample](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FP2_M2/reports/FP2_M2_667.md)

### FP2-M1 lab, build666

Build664 S took 951.321 / 689.037 native seconds versus 550.220 / 330.735 for build660. Across event-1 ranks the geometry-bound exclusive time rose from 80–188 to 197–450 s while calls fell from 166–286k to 148–277k. The mean call cost rose from 0.4–1.0 ms to 0.9–3.0 ms. Event-2 geometry-bound time rose from 35–130 to 72–392 s. In event 1, communication plus join wait rose from 211–308 to 369–613 s. All other exclusive rows together fell from 140–204 to 120–192 s. The hand-over shrank to 26,480 / 21,308 physical faces from 57,640 / 51,808. Build664 N1 averaged 1.81 ms below 2 mm and 22.96 ms at 20 mm and above, with 772.852 of 775.167 s on the non-direct branch.

| Case and pass | Mean full / small support facets, stage 1 | Same-run full forwards, rank-s | Stage 1 replay, rank-s | Stage 1+2 replay, rank-s |
| --- | ---: | ---: | ---: | ---: |
| N6P pass 1 | 238.59 / 20.47 | 817.166 | 771.714 | 772.026 |
| N6P pass 2 | 440.69 / 28.76 | 206.529 | 201.818 | 201.818 |
| N1 pass 1 | 239.39 / 20.46 | 924.039 | 882.769 | 883.066 |

The replay uses each run’s cache history and full-support equality information. Stage 2 added zero proofs. The observed lab elapsed times were 1,160.010 / 2,238.47 s for N6P / N1 versus 858.019 / 1,086.701 s on build664. Both exact projections matched. N1's supervisor stopped sampling before the child exited, so the final job peak is available only as a lower bound of 4,446,085,120 bytes from GNU time's serial-process RSS. [FP2-M1 measurement and receipts](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FP2_M1/reports/FP2_M1_666.md)

### FP2-C3 continuation 1 S, build664

Native maxima were 951.321 and 689.037 s, up from build660’s 550.220 and 330.735 s. Minimum timer coverage was 98.891% and 99.599%. At the slowest native rank of event 1, step C took 253.148 s, including 238.667 s in geometry-bound queries and 5.582 s in contact tests. The engine removed 687 and 522 surface points after hand-over, compared with 2,202 and 2,031 at build660. Isolated restart passed in 46.276 s after the sampler correction. The elapsed phase tables exclude the lab-only observer work. [S timing tables](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FP2_C3/reports/FP2_C3_664.md) · [Lab query breakdown](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FP2_C3/reports/FP2_C3_lab_664.md)

### FP2-C3 lab, build663

N1 removed 21,960 surface points versus 5,682 at FP2-D S0. Its exact repeat matched. The first pass took 950.332 s and step C took 877.460 s, including 784.810 s in geometry-bound queries and 41.132 s in contact tests. The serial end check took 4.105 s after a 0.065 s gather. The independent embedding check is separate and took 34.799 s. N6 matched in both trials. N6P took 858.792 s and failed the metric-edge floor after pass 2, so C3 has no S timing. [FP2-C3 lab comparison and diagnosis](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FP2_C3/reports/FP2_C3_663.md)

### FP2-D continuation 2 S, build660

Native maxima were 550.220 and 330.735 s, with minimum timer coverage 97.892% and 98.800%. The remeshed outputs had 424,564 and 407,937 cells, and isolated restart took 83.336 s. The S0 native maxima were 598.347 and 416.655 s. [S comparison](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FP2_D/reports/FP2_D_660.md)

### FP2-D continuation 1 lab, build657

N1 completed its two matching trials in 365.029 and 361.054 s. The first pass took 175.827 s at a starting one-minute load of 1.21. Step D completed seven marked sweeps with 8,636 splits, down from 40 marked sweeps in build656. N6 took 509.264 s and failed the metric-edge gate after pass 2. Its slowest pass took 48.238 s. S remains unevaluated. [FP2-D continuation 1 report](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FP2_D/reports/FP2_D_657.md)

### FP2-D lab, build656

N1 trial 1 took 322.010 s end to end at a starting one-minute host load of 1.82. The production pass took 155.507 s. Step D used 40 marked sweeps and made 3,926 splits. The S0 pass used 23 sweeps and made 17,123 splits. Build656 failed the sweep gate and stopped before S. [N1 evidence and diagnosis](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FP2_D/reports/FP2_D_656.md)

### FP2-C2 S, build655

Run S passed both remesh events in 598.347 and 416.655 native seconds, with 446,509 and 435,746 cells. The corresponding build647 S events were 606.213 and 387.779 s. The per-rank report includes the Bound and modeled scratch observations. Isolated restart took 112.807 s. [S timings and receipts](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FP2_C2/reports/FP2_C2_655.md)

### FP2-C2 continuation 1

The test-only build653 N1 diagnostic ran for 567.114 s at a one-minute starting host load of 7.00. The initial geometry proof and the extra facet localization are both included in the elapsed time. Build654 closed at the MPI removals-only fixture, so there is no new lab or S timing. [Continuation report](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FP2_C2/reports/FP2_C2_continue1_654.md)

### FP2-C2 inside-only surface collapse, build652 N1

Step C accepted 5,937 surface collapses in 25.504 s, including 5.324 s of forward samples and 3.777 s of entity checks. The N1 pass took 176.668 s and the lab ended in 441.308 s at a starting host load of 3.87. Build647 on the same frozen input took 120.920 s for the pass and 332.144 s for the lab. The strict proof logged four unexplained faces above tolerance. S is unevaluated. [Fixture and lab evidence](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FP2_C2/reports/FP2_C2_652.md)

### RCFD2 metric settings, build647

R-SM completed events 1 and 2 in 648.383 and 363.454 native seconds. R-SG reached native exit for event 2 in 506.927 s with incomplete quality. Its second CFD cycle and isolated restart are unavailable. Metric logs record three computations for R-SM and two for R-SG. The first metric took 59 gradation sweeps in R-SM and 48 in R-SG. [RCFD2 report](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/RCFD1/reports/RCFD2.md)

### RCFD1 real CFD, build647

| Mode | Native seconds, events 1 / 2 | Solve seconds, cycles 0 / 1 / 2 | Adapt seconds, cycles 0 / 1 |
| --- | ---: | ---: | ---: |
| R-S | 732.093 / 488.784 | 35.062 / 267.748 / 235.075 | 789.996 / 540.600 |
| R-OFF | 1,961.595 / 1,372.908 | 35.699 / 320.953 / 503.145 | 2,013.050 / 1,433.120 |

Solve includes metric generation. Mean history time per nonzero iteration was 0.443 / 0.359 s on the two R-S meshes and 0.532 / 0.782 s on the two R-OFF meshes. The minimum named coverage across the four remeshes was 97.94%. These were sequential full workflows on build647 with cycle counts of 1 / 500 / 500. The two modes produced different mesh sizes and the results are measurements. [RCFD1 report](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/RCFD1/reports/RCFD1.md)

### FP2-C1 wall-near length collapse, build647 S

| Event | FX2 → C1 native s | FX2 → C1 final cells | FX2 → C1 engine construction s | FX2 → C1 engine quality s |
|---|---:|---:|---:|---:|
| 1 | 614.463 → 606.213 | 550,027 → 467,139 | 210.259 → 201.092 | 267.667 → 257.030 |
| 2 | 410.522 → 387.779 | 575,733 → 464,565 | 212.803 → 187.658 | 136.275 → 134.143 |

The engine phase clocks are the maximum per-rank sums of its measured spans. The collapse boundary clock includes setup and lies within construction. Wall-near length collapse committed 16,970 and 10,221 core edits. The engine accepted 3,936 and 3,884 collapses. Minimum named coverage was 98.092% and 98.785%. The default proof and S2 passed. S2 matched all six S payloads and measured 1,029.687 and 498.925 native seconds, with one-minute host load 6.43 at launch and 6.32–19.67 in the run samples. The user accepted the timing flag on 2026-11-23. [Continuation timings](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FP2_C1/reports/FP2_C1_647_continue_1.md) [Per-rank stage and repair clocks](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FP2_C1/G5/build647/S/S_report.md)

### FX2 mobility-policy replay, build645

| G5 mode | Event 1 native s, build640 → build645 | Event 2 native s, build640 → build645 | Build645 minimum coverage, events 1 / 2 |
|---|---:|---:|---:|
| OFF | 1,757.338 → 1,735.398 | 1,017.567 → 1,016.311 | 99.006% / 99.527% |
| S | 649.954 → 614.463 | 446.547 → 410.522 | 98.158% / 98.853% |

The M6 fixture constructor took 10.285 s versus 25.968 s with the same modeled peak, reservation work and serialized state. N1 surface-state construction took 14.439 s versus 29.858 s. N6P measured 16.166–16.361 s per rank versus 32.821–32.875 s.

Transfer mobility replay took at most 7.804 µs in OFF and 2.728 µs in S. Construction mobility replay in S took at most 8.633 µs. The first preparation build still took about 17 s, and rank 0 spent about 18 s per event building the fresh output policy. These operation clocks are exclusive. The total native clocks also reflect the other phases and run-to-run variation. Timer coverage, phase clocks and unattributed time remain in the per-rank receipts. [FX2 timing and validation evidence](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FX2/reports/FX2_645.md)

### FP2-B accepted surface path, build640

| G5 mode | Event 1 native (s) | Event 2 native (s) | Minimum timer coverage, events 1 / 2 |
| --- | ---: | ---: | ---: |
| OFF | 1,757.338 | 1,017.567 | 98.9954% / 99.5266% |
| CORE | 1,509.433 | 371.336 | 99.0551% / 98.6725% |
| S1 | 649.954 | 446.547 | 98.2433% / 98.8869% |
| S2 | 649.993 | 444.120 | 98.2632% / 98.9130% |

S1 and S2 published the same six files and passed the event-1 timing rule of 1,584.905 s. On each event-1 maximum-time rank, S2 spent 210.527 s in engine construction and 235.038 s in engine quality, compared with 501.144 and 852.831 s for CORE. The first S2 engine geometry repair took 24.401 s. Event 2 took 444.120 s in S2 and 371.336 s in CORE. These are elapsed comparisons on the same binary. [Continuation report and per-rank phase accounting](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FP2_B/reports/FP2_B_640_continue_1.md)

FP2-B's `seconds_quality` includes the V rounds when they run. FP2-A's field covers its ordinary core quality work. The timing breakdown counts final proof separately by subtracting its exclusive ledger time from the terminal engine span. Surface sampling and placement are included in the surface-step clocks.

### FP2-A corrected surface steps and default proof, build627

| G5 mode | Event 1 native (s) | Event 2 native (s) | Minimum timer coverage, events 1 / 2 |
| --- | ---: | ---: | ---: |
| OFF | 1,779.827 | 1,023.870 | 99.004% / 99.530% |
| CORE | 1,512.056 | 369.654 | 99.045% / 98.654% |

Serial LAB-S spent 1.472 s in surface length and 10.797 s in surface distance, accepting 2,416 and 15,158 splits. Strict proof took 105.313 s mesh-to-model and 42.131 s model-to-mesh. K1 and K3 finished in 126.387 s and 114.542 s, with maximum post-surface stars of 168 and no alarm under Correction 4. S1 event 1 ended incomplete at 5,430.854 native seconds after the repair counts plateaued. S2 is unevaluated. [FP2-A report](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FP2_A/reports/FP2_A_627.md) · [Per-rank Part C timings](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FP2_A/reports/FP2_A_627_C_rows.md)

### FP2-0b feature-edge split measurement, build620

| Measurement | Seconds | Result |
| --- | ---: | --- |
| Length and distance split steps | 1.658 / 12.944 | 2,415 / 15,347 splits, including 35 / 22 feature splits |
| IN1 and new PA strict Classify | 47.712 / 74.964 | 48,954 / 40,734 faces queried |
| PA bulk constructor | 28.722 | Accepted 84,478 faces and 731 traces |
| PA mesh-to-model and model-to-mesh proof | 13.662 / 42.486 | All four entities within 1e-4 |

The serial measurement peaked at 2,034,744 KiB RSS. P4 used 57,865,510 work units, with separate aggregate direction timings and per-entity bounds. Feature proposal and trace reconstruction times are nested in the split steps. [FP2-0b report](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FP2_0B/reports/FP2_0B_620.md)

### FP2-0 surface measurements, build619

| Measurement | Seconds | Result |
| --- | ---: | --- |
| OUT1 bulk constructor and solo mobility policy | 25.209 / 15.480 | Snapshot equal |
| OUT1 float wing scan | 3.334 | 210,332 samples |
| IN1 length and distance split steps | 0.659 / 3.445 | 2,719 / 12,873 splits |
| IN1 and new PD Classify | 47.075 / 79.643 | 48,954 / 36,189 faces queried |
| PD bulk constructor | 27.087 | Rejected on entity/orientation |

The split case peaked at 2,742,700 KiB. The strict census marked 46 PD faces, including 16 whose four-sample value was within tolerance. The two-sided proof is conditional on the bulk constructor accepting PD. [FP2-0 report](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FP2_0/reports/FP2_0_619.md)

## Fast path stage 1

### FX1 Part M, build617

| Run | Event | Native maximum | Transfer maximum | Transfer mobility maximum | Rank 0 output root | Rank 0 output mobility |
| --- | --- | --- | --- | --- | --- | --- |
| N22 C OFF | 1 | 2,018.805 | 29.640 | 16.953 | 35.225 | 18.013 |
| N22 C OFF | 2 | 1,083.551 | 31.151 | 17.858 | 35.340 | 18.617 |
| Build615 OFF | 1 | 1,778.627 | 29.558 | 16.937 | 34.106 | 17.853 |
| Build615 OFF | 2 | 1,029.081 | 28.890 | 16.911 | 33.409 | 17.792 |
| Build617 OFF | 1 | 1,783.323 | 29.383 | 17.034 | 34.184 | 17.810 |
| Build617 OFF | 2 | 1,051.580 | 29.207 | 17.159 | 34.516 | 18.062 |
| Build616 ON1 | 1 | 1,499.541 | 31.576 | 17.020 | 36.216 | 18.145 |
| Build616 ON1 | 2 | 386.884 | 31.072 | 17.002 | 36.028 | 17.999 |
| Build616 ON2 | 1 | 1,500.864 | 30.799 | 16.875 | 35.496 | 17.841 |
| Build616 ON2 | 2 | 373.634 | 30.547 | 16.874 | 35.530 | 17.879 |
| Build617 ON | 1 | 1,552.606 | 30.641 | 16.838 | 35.519 | 17.922 |
| Build617 ON | 2 | 378.535 | 30.236 | 16.734 | 35.037 | 17.779 |

| Run | Metric computation | Target composition max (s) | Nested gradation max (s) | Trials | Sweeps | Visits | Updates |
| --- | --- | --- | --- | --- | --- | --- | --- |
| N22 C OFF | Event 1 | 39.408 | 39.167 | 7 | 800 | 14,153,877 | 9,853,577 |
| N22 C OFF | Event 2 | 161.847 | 161.166 | 8 | 1478 | 43,004,068 | 30,146,553 |
| N22 C OFF | After event 2 | 129.573 | 128.824 | 8 | 1199 | 41,122,066 | 28,821,833 |
| Build615 OFF | Event 1 | 38.394 | 38.153 | 7 | 800 | 14,153,877 | 9,853,577 |
| Build615 OFF | Event 2 | 146.205 | 145.609 | 8 | 1478 | 43,004,068 | 30,146,553 |
| Build615 OFF | After event 2 | 127.417 | 126.723 | 8 | 1199 | 41,122,066 | 28,821,833 |
| Build617 OFF | Event 1 | 33.608 | 33.368 | 7 | 800 | 14,153,877 | 9,853,577 |
| Build617 OFF | Event 2 | 130.076 | 129.457 | 8 | 1478 | 43,004,068 | 30,146,553 |
| Build617 OFF | After event 2 | 113.713 | 113.006 | 8 | 1199 | 41,122,066 | 28,821,833 |
| Build616 ON1 | Event 1 | 38.678 | 38.439 | 7 | 800 | 14,153,877 | 9,853,577 |
| Build616 ON1 | Event 2 | 68.233 | 67.762 | 8 | 1056 | 28,977,415 | 21,001,471 |
| Build616 ON1 | After event 2 | 91.056 | 90.575 | 8 | 1246 | 28,674,333 | 20,088,842 |
| Build616 ON2 | Event 1 | 39.845 | 39.597 | 7 | 800 | 14,153,877 | 9,853,577 |
| Build616 ON2 | Event 2 | 68.269 | 67.798 | 8 | 1056 | 28,977,415 | 21,001,471 |
| Build616 ON2 | After event 2 | 89.052 | 88.577 | 8 | 1246 | 28,674,333 | 20,088,842 |
| Build617 ON | Event 1 | 34.112 | 33.873 | 7 | 800 | 14,153,877 | 9,853,577 |
| Build617 ON | Event 2 | 60.603 | 60.125 | 8 | 1056 | 28,977,415 | 21,001,471 |
| Build617 ON | After event 2 | 77.166 | 76.695 | 8 | 1246 | 28,674,333 | 20,088,842 |

The two mobility-policy builds remain in their original transfer and output paths. Part M skips 3D gradation work for unchanged pairs. Both G5 modes passed with their exact reference files and gradation counts. The Part N and S timer names provide coverage margin, with minima of 99.008% / 99.529% OFF and 99.065% / 98.738% ON against 95%. The timing rows are measured rank maxima and nested clocks. [FX1 report](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FX1/reports/FX1_617.md)

### Resume 6 Part S, build616

| Run | Event 1 / event 2 native max (s) | Minimum named coverage |
|---|---:|---:|
| Build615 OFF | 1,778.627 / 1,029.081 | 97.467% / 98.019% |
| Build616 ON1 | 1,499.541 / 386.884 | 99.041% / 98.658% |
| Build616 ON2 | 1,500.864 / 373.634 | 99.043% / 98.650% |

Each phase, core and shift clock is a rank maximum in seconds. Engine construction excludes the core and shift within each rank. These maxima are nonadditive.

| Event | Run | Preparation | Engine construction | Core | Shift | Quality | Final proof |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | Build615 OFF | 63.022 | 460.190 | 0.000 | 0.000 | 1,150.640 | 63.140 |
| 1 | ON1 | 64.431 | 464.264 | 15.047 | 0.592 | 855.544 | 57.016 |
| 1 | ON2 | 64.371 | 465.799 | 15.185 | 0.601 | 856.888 | 56.233 |
| 2 | Build615 OFF | 83.436 | 202.218 | 0.000 | 0.000 | 602.798 | 105.126 |
| 2 | ON1 | 54.316 | 157.551 | 9.930 | 0.751 | 99.501 | 28.825 |
| 2 | ON2 | 53.270 | 156.628 | 9.670 | 0.733 | 90.169 | 27.780 |

Part S assigned generation, queue rebuild and census work to Scheduling. The ON pair passed all standing gates and matched all six payloads. Both ON runs passed the native time comparisons against build615 OFF. The Part S G5 OFF proof passed in FX1. [Resume 6 report](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FP1/reports/FP1_616_resume6.md)

### Resume 5 Part N, build615

| Mode | Event 1 native maximum (s) | Event 2 native maximum (s) | Named coverage minima |
|---|---:|---:|---:|
| Build608 OFF | 1,818.319 | 1,039.725 | 95.257% / 95.333% |
| Build615 OFF | 1,778.627 | 1,029.081 | 97.467% / 98.019% |
| Build615 ON1 | 1,521.154 | 383.071 | 96.952% / 95.110% |
| Build615 ON2 | 1,519.720 | 393.250 | 96.978% / 94.842% (failed) |

Part N assigned the specified ownership and partition work to Communication and private quality work to Candidate. Each OFF rank improved in both events. The build615 OFF maxima define the ON comparison limits. ON1 passed both events and the isolated restart. ON2 event 2 failed the 95% named coverage gate on rank 0 before publication. Rank 0 unattributed time rose from 18.734 to 20.287 s, mostly in construction. Resume 5 ended after read-only diagnosis. [Resume 5 report](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FP1/reports/FP1_615_resume5.md)

Each phase, local core and shift clock is the maximum over ranks in seconds. Engine construction excludes the core and shift clocks within each rank. The maxima are nonadditive.

| Event | Mode | Preparation | Engine construction | Core | Shift | Quality | Final proof |
|---:|---|---:|---:|---:|---:|---:|---:|
| 1 | OFF | 63.022 | 460.190 | 0.000 | 0.000 | 1,150.640 | 63.140 |
| 1 | ON1 | 64.961 | 470.098 | 15.931 | 0.613 | 868.808 | 58.117 |
| 1 | ON2 | 65.108 | 470.470 | 16.304 | 0.614 | 868.417 | 57.325 |
| 2 | OFF | 83.436 | 202.218 | 0.000 | 0.000 | 602.798 | 105.126 |
| 2 | ON1 | 55.866 | 161.225 | 10.276 | 0.753 | 90.986 | 28.655 |
| 2 | ON2 | 55.121 | 163.652 | 10.085 | 0.751 | 96.553 | 30.405 |

### Resume 4 H6, build614

| Check | Pass or native wall (s) | Evidence |
|---|---:|---|
| LAB1 V2 | 43.571 | 422,857 exact-positive output cells |
| LAB2 V2 | 51.102 | 558,310 exact-positive output cells |
| LAB1 V5 | 47.670 | Surface blocking off. 479,791 exact-positive output cells |
| LAB2 V3 | 26.658 | Lower short-edge variant. 644,771 exact-positive output cells |
| Six-rank frozen event 1 replay | 1,568.109 | Accepted. Stage-0 core and shifts max 16.806 s |

LAB1 V2 split work fell from 515.057 s on H to 7.826 s on H6, and peak live cells in sweep rows fell from 1,127,484 to 564,602. The frozen replay used zero CFD steps and reached 95.055% minimum timer coverage. G5 ON1 native maxima were 1,572.119 and 383.244 s against build608 OFF at 1,818.319 and 1,039.725 s. The second event failed named coverage at 93.688% minimum against 95%. Rank 0 retained 24.192 s of unattributed time, including 19.011 s in construction. The run stopped before event 2 publication and the repeat. [Resume 4 report](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FP1/reports/FP1_614_resume4.md)

### Resume 3 lab, build613

| Pair | Pass wall (s) | Outcome |
|---|---:|---|
| LAB2 V2 | 88.835 | Exact and strict audits completed |
| LAB1 V2 | 1,072.121 | Exact and strict audits completed. The 600 s threshold triggered the stop |
| LAB2 V3 | 60.969 | Lower short-edge variant, exact and strict audits completed |
| LAB1 V1 | — | 1,200 s cutoff during quality. Owned process group stopped at 1,292.141 s after SIGTERM did not end the core |
| LAB2 V1 | 66.450 | Exact audit passed. Strict metric audit stopped at cell 732833 |
| LAB2 V0 | 162.988 | Exact audit passed. Strict metric audit stopped at cell 729907 |

Pass wall time covers array construction through write-back. LAB1 V2 consumed 265.073 s for collapse, 515.057 s for split and 290.467 s for quality. Its 696 sweep rows reached a peak of 1,127,484 live cells. Build613 OFF prefixes captured G2 in 246.756 s and GQ in 685.827 s with exact adopted comparisons. The six-rank H replay and H workflow timing are unevaluated. [Resume 3 report](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FP1/reports/FP1_613_resume3.md)

### Build608 workflow reference

Build608 passed the fresh Part C OFF G2 prefix in 246.927 s with 90,809,869 work units, matching the exact rows of the adopted reference. Rank 0 recorded 250 preparation and 333,799 construction communication calls. Work order 3 correction 1 designated those counters as report rows. [FP1 report](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FP1/reports/FP1_608.md)

The Part F single-rank refusal-memory fixture skipped 1,134 evaluations and produced identical owned bytes. The modeled local-core charge increased from 9,492 to 10,620 bytes per cell. Both MPI fixture layouts exercised all five local edit types and both border shifts. Strict refinement returned valid. [FP1 fixture evidence](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FP1/reports/FP1_608.md)

| Stage | OFF event 1 (s) | OFF event 2 (s) | ON1 event 1 |
|---|---:|---:|---|
| Preparation | 64.113 | 80.215 | Reached the first local pass |
| Local core | 0.000 | 0.000 | First pass rejected an old metric donor |
| Border shifts | 0.000 | 0.000 | 0 shift rows |
| Engine construction | 460.024 | 209.275 | Unevaluated |
| Quality | 1,183.856 | 607.086 | Unevaluated |
| Final proof | 65.801 | 103.523 | Unevaluated |
| Whole native event | 1,818.319 | 1,039.725 | Incomplete |

The phase seconds are maxima over ranks. OFF peak memory was 13,381.543 / 15,980.539 MiB and minimum coverage was 95.257% / 95.333%. In the incomplete ON1 event, rank 0 recorded 413.263 s inclusive in the first local topology call, of which 376.998 s was nested communication. The attempt printed one normalization stage and no completed pass or shifts. The purpose timing comparison and ON event 2 memory remain unevaluated.

## Step 8.5u, build591

[Step 8.5u report](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N22/reports/STEP_8_5U_591.md)

The user accepted both N22 purpose timing flags on 2026-11-19. The table records the clocks for both parts.

| Run | Event | Maximum native time (s) |
| --- | --- | --- |
| M reference | 1 | 1,981.678851 |
| M reference | 2 | 1,348.621272 |
| C | 1 | 2,018.805346 |
| C | 2 | 1,083.551456 |

Each phase row is a mean over the six final remesh ledgers. The native clock has a wider scope.

| Run | Event | Phase | Exclusive phase (s) | Communication calls | Exclusive communication (s) | Inclusive census (s) |
| --- | --- | --- | --- | --- | --- | --- |
| M reference | 1 | preparation | 134.840312 | 34,008.000000 | 78.242747 | 193.016460 |
| M reference | 1 | construction | 467.296132 | 4,863,428.000000 | 273.045850 | 218.595881 |
| M reference | 1 | quality | 1,270.450288 | 719,469.000000 | 378.061554 | 163.570729 |
| M reference | 1 | final_proof | 64.326104 | 16.000000 | 29.442327 | 0.000000 |
| M reference | 2 | preparation | 281.941315 | 76,905.000000 | 223.542345 | 538.204121 |
| M reference | 2 | construction | 227.409014 | 1,803,980.000000 | 140.563290 | 51.518775 |
| M reference | 2 | quality | 683.837923 | 132,206.000000 | 137.576668 | 85.406936 |
| M reference | 2 | final_proof | 110.915879 | 16.000000 | 32.644446 | 0.000000 |
| C | 1 | preparation | 62.659080 | 272.000000 | 7.242190 | 50.971052 |
| C | 1 | construction | 464.642995 | 4,618,782.000000 | 268.160545 | 216.360631 |
| C | 1 | quality | 1,382.236449 | 606,909.000000 | 365.435680 | 120.349887 |
| C | 1 | final_proof | 65.697836 | 16.000000 | 29.585848 | 0.000000 |
| C | 2 | preparation | 89.843659 | 269.000000 | 32.224620 | 152.706708 |
| C | 2 | construction | 220.295926 | 1,642,210.000000 | 134.343384 | 43.587377 |
| C | 2 | quality | 616.671926 | 54,372.000000 | 93.103384 | 49.247299 |
| C | 2 | final_proof | 113.277466 | 16.000000 | 33.712187 | 0.000000 |

## Step 8.5t, Part P build587 and Part W build588

Part P reduced the maximum rank-exclusive final certification from 90.407 / 91.040 s to 19.217 / 21.517 s across events 1 and 2. The six P6c/P7 payloads matched.
[Step 8.5t report](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N21/reports/STEP_8_5T_588.md)

## Workflow measurements

| Run | Event | Native max (s) | Process-peak sum (B) | Minimum timer coverage | Proof faces |
|---|---:|---:|---:|---:|---:|
| T0 | 1 | 2236.869538535 | 14,007,504,896 | 96.1996% | 67,170 |
| T0 | 2 | 1416.499470597 | 17,363,812,352 | 96.7275% | 62,912 |
| T1 | 1 | 2094.502498638 | 14,003,933,184 | 96.2532% | 67,170 |
| T1 | 2 | 1417.104791628 | 17,352,138,752 | 96.6015% | 62,912 |
| run1 | 1 | 2048.285885933 | 14,055,071,744 | 95.2382% | 61,906 |
| run1 | 2 | 1379.449004216 | 16,693,104,640 | 96.1063% | 58,260 |
| run2 | 1 | 2741.581855733 | 14,014,971,904 | 96.1846% | 61,906 |
| run2 | 2 | 1278.700341572 | 16,775,467,008 | 96.1185% | 58,260 |

## Mean rank-exclusive seconds by phase

Each mean uses the six final remesh ledgers. The whole-event wall clock has a wider scope.

| Run | Event | Preparation | Construction | Quality | Final proof |
|---|---:|---:|---:|---:|---:|
| T1 | 1 | 135.627946 | 475.363796 | 1386.850431 | 52.318934 |
| T1 | 2 | 207.377299 | 203.119927 | 882.211175 | 78.756776 |
| run1 | 1 | 140.104302 | 513.510297 | 1281.016546 | 66.178124 |
| run1 | 2 | 272.304431 | 240.280538 | 714.701431 | 111.295533 |

The 0.95 rule selected 2,048.286 s from run one for event one and 1,278.700 s from run two for event two. Their sum was 3,326.986 s against a 3,336.027 s ceiling. The second run had higher host contention during event-one preparation. The adoption uses the per-event minima.

## N6 — build578

G5 passed in 4041.079 s, with native maxima 2173.311 / 1434.914 s and minimum timer coverage 96.4000% / 96.7297%. The isolated restart took 234.819 s. The six payloads, old work and timer calls matched the full build574 reference. [N6 report](ADAPTATION_CUT_N6_20261118.md)

| Final ledger phase | Event 1 exclusive s, min–max | Event 2 exclusive s, min–max | Event 1 calls, sum | Event 2 calls, sum |
|---|---:|---:|---:|---:|
| preparation | 137.102–137.292 | 209.673–210.154 | 1,181,839 | 2,891,851 |
| metric | 4.512–4.520 | 5.349–5.357 | 267,804 | 335,124 |
| construction | 472.955–472.979 | 202.374–202.391 | 32,416,907 | 12,690,268 |
| surface | 0.000–0.000 | 0.000–0.000 | 0 | 0 |
| quality | 1395.891–1395.913 | 842.814–842.827 | 7,211,139 | 4,257,051 |
| final proof | 122.844–122.844 | 138.810–138.810 | 31,682 | 35,358 |
| transfer | 30.229–30.236 | 30.091–30.101 | 150 | 150 |
| output | 0.000–0.000 | 0.000–0.000 | 0 | 0 |

Maximum unattributed time was 78.241 / 46.927 s. Each rank contributes one set of exclusive times. Event and remesh-root clocks are nested, and the build comparisons include host variation. [Per-event timing and work](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/phase_N6/G5/acceptance_summary.json)

## Step 8.5r — build574 T0

T0 passed both events in 4089.709 s. Native event maxima were 2236.870 and 1416.499 s. Each event had 1,274 and 801 private quality rounds per rank. The six-rank exclusive join, communication and clearance sums were 5546.566 and 3420.686 s. The 64-wide build575 rejected event one with one q-bad cell after 716 quality rounds. Its 4576.262 s sum is a failed-run observation. [Gate and failure report](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N16/reports/STEP_8_5R_575.md)

## Phase D — build572

The same six files and old integer and timer rows matched in 4140.805 s of workflow time. The isolated restart passed in 233.893 s. Native event maxima were 2188.708 and 1505.656 s. Minimum timer coverage was 96.4580% and 96.3409%. [Phase D report](ADAPTATION_CUT_PHASE_D_20261118.md)

| Final ledger phase | Event 1 exclusive s, min–max | Event 2 exclusive s, min–max | Event 1 calls, sum | Event 2 calls, sum |
|---|---:|---:|---:|---:|
| preparation | 135.178–135.301 | 223.316–223.743 | 1,181,839 | 2,891,851 |
| metric | 5.076–5.087 | 5.005–5.012 | 267,804 | 335,124 |
| construction | 462.744–462.759 | 227.735–227.756 | 32,416,907 | 12,690,268 |
| surface | 0.000–0.000 | 0.000–0.000 | 0 | 0 |
| quality | 1409.521–1409.535 | 874.701–874.721 | 7,211,139 | 4,257,051 |
| final proof | 131.620–131.620 | 139.396–139.396 | 31,682 | 35,358 |
| transfer | 34.425–34.434 | 29.814–29.821 | 150 | 150 |
| output | 0.000–0.000 | 0.000–0.000 | 0 | 0 |

The maximum final unattributed time was 77.528 s in event one and 55.095 s in event two. The ranges use each rank’s exclusive ledger once. Event and root clocks are nested. The between-run elapsed difference includes different host conditions. [Per-event ledger and work](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/phase_D/G5/acceptance_summary.json)

## Phase C — build570

The full two-event workflow passed in 4058.303 s. The isolated event-two restart passed in 225.848 s. Native event maxima were 2181.430 and 1448.062 s, and the six payloads matched their reference hashes. Minimum timer coverage was 96.5141% and 96.7599%. G2 matched 90,809,869 units and six accepted-collapse counts. GQ and the full run matched the required integer and timer-call rows. [Phase C qualification](ADAPTATION_CUT_PHASE_C_20261118.md)

| Final ledger phase | Event 1 exclusive s, min–max | Event 2 exclusive s, min–max | Event 1 calls, sum | Event 2 calls, sum |
|---|---:|---:|---:|---:|
| preparation | 135.513–135.703 | 212.582–213.043 | 1,181,839 | 2,891,851 |
| metric | 4.102–4.106 | 5.508–5.520 | 267,804 | 335,124 |
| construction | 465.842–465.853 | 202.619–202.637 | 32,416,907 | 12,690,268 |
| surface | 0.000–0.000 | 0.000–0.000 | 0 | 0 |
| quality | 1412.024–1412.034 | 850.710–850.724 | 7,211,139 | 4,257,051 |
| final proof | 124.009–124.009 | 140.909–140.909 | 31,682 | 35,358 |
| transfer | 30.235–30.241 | 29.951–29.966 | 150 | 150 |
| output | 0.000–0.000 | 0.000–0.000 | 0 | 0 |

The maximum final unattributed time was 76.045 s in event one and 46.921 s in event two. The ranges use each rank’s exclusive ledger once. Native event and ledger root clocks are nested, and rank ranges describe different ranks across phases. The elapsed comparison with N14 includes different host and run conditions. The M27 query clocks measure the existing union calls inside this ledger. [Per-event ledger and work evidence](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/phase_C/G5/acceptance_summary.json)

## Earlier timings

Phase B completed on558: G3 617.688712s/1170splits/1113commonedits exact/adjustedentry5113; G4 6.492431/6.504021/6.514544s; GQ1083.237551stop/1091.436227sealed/all6thirdwindows/18strictsnapshotrows. Corrected integratedG2 316.078817stop/323.937712sealed/work90,809,869/counts480/473/346/263/469/464. Prefix HWM sums6,440,038,400/13,999,366,144B are notfullP8peaks. EarlierB6/B7/B8G2 timings below are numerical observations from runs with an overlooked seventh owned draft-generator process; they do NOT qualify R1. The connected558 state was requalified after terminating the orphan and fixing ownership accounting. G2/GQ controllers were recovered on the SAME numerical runs, no solver restart; see phase-B report. No controlled efficiency gain is claimed. Build552 was the full-workflow reference at the phase B stop.


Historical N14/N15 qualification (build552) PASSED and pushed0c2cb5c460. The ONE full two-event workflow sealed4738.250709s (stop4728.829883s); Native event maxima2568.277632/1672.244847s. Both events passed physical/quality/size/transfer/continued-CFD/resource/timer gates, all six payloads exact against accepted references. Exact P8 process-peak sums14,054,744,064/17,397,911,552B<19,999,490,048B; minimum timer coverage96.0177948%/96.0351953%. Root-remesh event1 maximum2568.341873s has a slightly different clock from Native; clocks are nested/nonadditive. Isolated disk restart passed236.822017s,168797points/855828cells,zero coordinate error,positive finite fields changed after one further iteration (no convergence claim). M26 report uses the same full run,35900 quality local-wall commits/3002.832616 committed-slot busy seconds; class intervals and local Prepare clocks are not global wall time. No controlled speed claim. Evidence: night_20261115/N14/section6_12/{acceptance_summary.json,REPORT.md,reports/M26_full552.md}.

B6 Ownership extraction PASSED scoped build556/G1/G2. Build 70.159541s/1779 pins; G1 serial/MPI2/MPI4 796.648625/16.245114/32.141896s, zero changed cases; strict G2 stop/seal 422.579359/431.041191s, all old integer/model/timer rows exact against adopted552. All actual handles and pins sealed. Prefix HWM sum 5,229,854,720B is not a full-workflow P8 peak. No numerical repeat or efficiency gain claim.

CUT B8/build558 PASSED scoped gates: build 80.122834s/1781 pins; G1 serial/MPI2/MPI4 782.588145/13.597257/15.629537s, zero changed cases; strict G2 stop/seal 364.021415/372.503794s, all old integer/model/timer rows exact against adopted552. All actual handles and pins sealed. Prefix HWM sum 6,337,683,456B is not a full-workflow P8 peak. No numerical repeat or efficiency gain claim.

CUT B7/build557 PASSED scoped gates: build 90.137656s/1780 pins; G1 serial/MPI2/MPI4 799.466036/16.047393/17.009736s, zero changed cases; strict G2 stop/seal 358.986603/367.124557s, all old integer/model/timer rows exact against adopted552. All actual handles and pins sealed. Prefix HWM sum 6,083,198,976B is not a full-workflow P8 peak. No numerical repeat or efficiency gain claim.

CUTB5/build555 PASSEDscopedgates: build70.163345s/1778pins; G1serial766.075553/MPI2 14.503078/MPI4 14.556237s, zerochangedcases. MPI2launcher10vsref9 isreport-onlyperC2.1; exacttablepassed. StrictG2stop317.596742/seal325.356386s, alloldinteger/model/timerrows exact vsadopted552. ObservedprefixHWMsum6,052,069,376B, notfullP8peak. Allactualhandles/pinssealed. No computational efficiency gain claimed.

CUTB4/build554 PASSEDscopedgates: build70.160484s/1777pins; G1serial765.089244/MPI2 13.584711/MPI4 13.561882s, zerochangedcases. StrictG2stop317.442030/seal325.206035s, alloldinteger/model/timerrows exact vsadopted552. ObservedprefixHWMsum6,057,889,792B, notfullP8peak. Allactualhandles/pinssealed. No efficiency gain claimed; code movement only.

Current cut B3/build553 passed70.160315s/four actions/1776unchanged pins; actual build handles sealed. ScopedG1PASSED780.160772/13.539927/14.536724s at1/2/4ranks, zerochangedcases; strictG2PASSED317.640179sstop/325.481453ssealed, alloldinteger/model/timerrows exact. PrefixHWMsum7,946,518,528B is notfullP8peak. Actualhandles/pinssealed. No computational efficiency gain claimed.

Completed build552 stages: build120.173935s/1775pins; focused4.862019s/113assertions12cases; G1serial798.622486/MPI2 13.881759/MPI4 16.963543s, zero case changes; G2stop358.613009/seal367.124284s and GQstop1234.610042/seal1242.824580s with all old integers/model rows exact and exact census-call deltas. G2/GQ observed prefix rank-HWM sums7,961,542,656/14,030,856,192B are not full-workflow P8 peaks. G3first/second624.866412/625.957396s with1113/1099splits (both>=689), all1099common edits exact; stage elapsed includes about75.9s post-budget exit-law census, trial work about388.9s under unchanged388.8s configured stage budget. Nested clocks must not be added. Six-rank wall report577.391424s/459splits, process-VmHWM sum5,483,991,040B, report only. Safeguards merit481/saved481/current127.197434/157.009231/154.506960s; 2D1/2/4ranks7.213273/7.014112/7.095061s. All completed-case pins and actual terminal handles are sealed. M26 is print-only within existing scopes, fixedstorage4120 unchanged; no8.5ab or accepted BL claim. Evidence: night_20261115/N14/section6_12/REPORT.md, validation/event1_completed_observation.json and reports_G3_pair/g3_pair_audit.md.

CUTB2 passed/pushed76b2ad61db: build55190.159432s/G1serial793.680386/MPI2 13.930448/MPI4 15.946141s; G2strict368.626787stop/376.929529seal. All required old integer/model/timer/law rows identical. Prefix sampled HWMsum7,895,732,224B, not full job peak. Code movement alone supports no computational improvement claim. Detailed history follows.

CUTB1/build550:build90.150096s; G1serial791.819377/MPI2 14.076732/MPI4 17.073050s, all C2.1case/failure tables exact. Successful-assert totals shifted by18 acrossMPI4ranks; own extra wrapper condition corrected post-only to writtenC2.1 (numbers/rawfalse receipt retained), no numericalrepeat. StrictG2passed358.315913stop/366.571845seal; all old integer/model/timer/law rows exact; observedprefixHWMsum5,342,928,896B notfullP8peak. Allpins/actualhandlessealed. No efficiency gain claimed. Evidence night_20261115/N4/section6_3_after_N11/B1/REPORT.md.

N13/M25(test-only):final549build70.148795s; finalcase440.655953s sealed/435.120s Catchduration,2408edges/2354complete/all2408exactprefixselfchecks/all4816parent+9632childbounds/54supplements/0refusals. GNUtime1859240KiB/sampletestVmHWM1,904,635,904B (notwholejobpeak). Earlierfixture-only setup/coverage corrections preserved; CFDbyte-identical545 and noG1–G5run. Evidence night_20261115/N13/section6_10/REPORT.md. No production efficiency claim.

Phase-A qualification on545: G3 passed618.164501s total,386accepted splits versus375fresh536, all375common edits exact, adjustedentrywork5113; physical304784positive+sensors/zeroUpperRefinements/model11,190,581,249B. Stageconfigured388.8s; reported461.173047s includes exitcensus; lawentry71.2821s/exit71.416430s are nested. Clock-dependent counts are reports under6.6; no efficiency/acceptedBL claim. G4 passed7.839087/6.904820/6.919375s at1/2/4ranks. GQ passed1228.127955s to planned stop/1236.253216s sealed, all6thirdwindows/oldintegers/model/timer/lawfields exact; observedprefixHWMsum14,031,200,256B (notfullP8peak). G5 PASSED: fullsealed4796.320843s, nativeevents2583.997544/1727.465975s, process-peak sums14,000,402,432/17,305,853,952B; sixpayloads byte-identical and alloldinteger/per-rowtimer rows exact againstaccepted534. IsolatedrestartPASSED246.458883s,coorderror0. Qualifiedcheckpoint5ec465ca1aPUSHED. Compared to534native2529.592735/1684.219472s, the timing difference is an observation under varying contention; no efficiency gain is claimed. Nested timers retain their existing scopes. Two own post-only helper repairs preserved the completed numerical cases without repeats. Evidence candidate_full_preparation/{G5_summary.json,G5_report_helper_repair.json,restart545/R3_validator_log_binding.json,restart545/primary_final_terminal_proof.json}. Evidence: N4/section6_3_after_N11/candidate_g3_preparation/G3_summary.json and candidate_g4_preparation/g4_case_paths.json.

A4 (build545) passed scoped G1/G2: build90.152649s; serial/MPI2/MPI4 790.765579/15.002979/16.232606s; G2 stop/seal368.050631/376.307212s,strict alloldints/model/timer identity. Prefix observed HWMsum7,960,436,736B is not a full-workflow peak. All1915casepins/actualterminal identities verified. No numericalrepeat or efficiency claim. Evidence: night_20261115/N4/section6_3_after_N11/A4/acceptance_summary.json.

A3 Ownership (build544) passed scoped G1/G2: build80.143433s; serial/MPI2/MPI4 794.785246/15.160131/15.404183s; G2 stop/seal368.251504/376.777587s. All old integer/model/timer calls matched and pins/terminal handles verified. Prefix observed HWMsum5,223,751,680B is not a full-workflow peak. State grouping adds no measured efficiency claim. Evidence: night_20261115/N4/section6_3_after_N11/A3_Ownership/acceptance_summary.json.

Earlier phase-A state timings: A1/A2/Control/Census/Queues/Exchange/Commit allscopedG1/G2PASSED/PUSHED; Ownership/A4 and current qualification are recorded above. Builds539–54380.117992/80.149112/80.138963/80.133215/90.142698s,4actions each. ControlG1serial/MPI2/MPI4 789.847011/13.926688/16.181808s,G2strict363.020346stop/371.248518seal; CensusG1 789.755303/15.061827/15.091666s,G2 368.016698/376.343898; QueuesG1 791.689938/14.954960/16.126801s,G2 368.064862/376.312930; ExchangeG1 792.799754/16.067293/17.237838s,G2 362.974876/371.425439; CommitG1 793.810234/15.058899/17.186484s,G2 373.214264/381.353821. ZEROchangedG1cases/reportertotalsidentical; ALLoldG2integer/model/perrowTIMERcalls/lawcountsexact,90,809,869work/counts480/473/346/263/469/464. Observedprefix/sealSUMrankHWMControl6,418,857,984B/Census6,355,832,832B/Queues7,903,481,856B/Exchange6,056,386,560B/Commit7,959,818,240B<19,999,490,048B; notfullP8peaks. No speedclaim under unequalhostconditions. Allpins/actualterminalidentities verified. PhaseA/N4unfinished; newestfull-qualifiedf8c7b01085/build534.

Fresh cut reference536 captured BEFORE firstcutedit, allpassed/terminal: G1 identity375serial/57perrankMPI2/4,794.971271/14.883794/17.237846s. G2strictidentity363.186728stop/371.571871seal,90,809,869work/counts480/473/346/263/469/464. G3PASS623.153178s/375commonN1edits/first336P6c/adjustedentry5113/304757positive+sensors/model11,190,581,249B; lawentry73.8598s/exit75.030967s nested. GQPASS1212.527148stop/1220.753742seal/all6thirdwindows/embeddedG2exact. No controlled efficiencyclaim, no acceptedBLclaim. Evidence night_20261115/N4/section6_3_after_N11.

N11 FAILED first G3 purpose and reverted0d9d627645 (pushed): geometry guidance1046/1301=.8039969254 versus required<.70. Physical gates passed; no second G3 or full N11 run. Qualified G2 took353.063082s to stop/361.351210s sealed; GQ1238.571465/1246.812545s; G3 normal exit622.150845s,459splits/304983positive+sensors/model11,197,749,225B. Its exit law74.060795s and stage/API463.895586s are nested; no controlled speedup. One readonly diagnosis found proposal cause. Failed535 binaries archived. Restored accepted534 source rebuilt as536 in90.125806s/1598unchangedpins/12terminalIDs, binaries byte-identical accepted534. Fresh cut references completed before A1; see current scoped substep status above. Evidence R/night_20261115/N11 and N4/section6_3_after_N11.

Accepted pre-cut Section6.8 reference: G1identityPASS375serial/57perrankMPI2/4,0casechanges,839.004931/15.181993/20.688848controllerseconds. G2identityPASSalloldintegers/bytes/nonCensuscalls,12timerboundaries+6lawrows/exactCensusdelta;ONEsolverrun398.804320s untilplannedstop/408.513817s terminalseal,work90809869/counts480/473/346/263/469/464. Scope includesCFD/import/metric/setup; unlike olderN9/6.1prefixcontrollers it stopsatfirstcollapse anddoesnotincludeextra constructionguard, so no speedclaim. GQPASS1357.238631s untilplannedstop/1367.993514s sealed;18timerboundaries/oldints exact. G3PASS621.012663s (savedload+30s observer+388.8s stage+entry/exitlaw),384splits/304779positivecells; lawentry73.3382s/exit73.63954s are nested in the whole fixture, not extra independent work. G4PASS1/2/4ranks; geometrymerit124.021300s; corrected savedtarget152.297794s (controller160.078255s), currenttarget159.141062controllerseconds. Full534 PASSED4697.519889s with6exactN1hashes/allstandingworkflowgates/pinsunchanged: native2529.592735/1684.219472s, exactSUMprocesspeaks14,021,578,752/17,419,386,880B. Isolatedrestart PASSED248.353069s; group534ACCEPTEDf8c7b01085/allhandles terminal. Fresh549finaltimerrows besideF3b/N1 in section6_8/reports/full534_vs_F3b_N1_timer_rows.csv; inclusive rowsnested/nonadditive. M24qualitylocalinteriorbusyshare2.53025%/4.23719% acrosssixrankbusyintervals (not CPU/eventwalltime), commitshare9.90584%/20.95503%; compatiblecommits/workreconcileallranks. Only postprocessing/schema helpers repaired underR3; numericalG2notrepeated. Evidence section6_8/G1_summary.json,G2_summary.json.


Section6.8 combinedgroup: build534 passed110.165477s/7actions,1588source/helper/policy/configpinsunchanged,16actual-host build identities terminal. Focused requirements21assertions/1case4.750761s, lane21/3 4.718268s, law10/1 4.870562s, diagnostics26/3 5.404644s. No efficiency claim; geometry/ordered prerequisites passed, fullgroup acceptedf8c7b01085. Build533 focused/M6b history remains valid but repeats on534 per latestuserorder. Evidence R/night_20261115/N8/A2_A3_N12/section6_8.


N9 section6.6 PASSED: build529100.145415s; correctedC492546.644725s,457splits/all457commonN1/adjustedentry5113,GNUserialpeak3,339,386,880B/physicalPASS. Operatorreason904=360retainedoccurrences+544overflow. G2exact90,809,869/counts/ALLoldintegersPASS; samelegacyjob continuesextra constructionguard,751.169940s untilplannedstop/759.371902s sealed (notcollapse-only). Allownedhandlesactualhostterminal/pinsunchanged. Noequal-work speedclaim andnoextra fullN9run. Evidence R/night_20261115/N9/section6_6/REPORT.md.

Section6.6/6.7 continuation:shared accepted-base G1 reference complete on528:375serial792.252073s,57perrankMPI2 14.745597s/MPI4 15.756263s; failedexpressions24/19/37 are baseline table evidence, not candidatepasses. No test/tolerancechanges. RestoredN9+boundedreasoncounter build529100.145415s/6actions,all1562pinsunchanged/13actual-host buildIDs terminal; standalonecounterfixture0.504s. CorrectedG3 commonprefix/net-entrywork and G2 pending. N12/M24 joinslaterN8A2/A3samebuild/fullrun; noextra run/no8.5. Evidence N9/section6_6 andN10/G1_shared_reference_manifest.json.

N9 print-only observations (2026-11-16): build527 PASSED100.162575s/6actions,1562pins/17actual-host build IDs terminal; standalone fixture compiled/ran0.482s. C492547.162650s,459splits/304981positivecells/first336PASS, GNUtime serialpeak3,340,496,896B. ALL-old-integer identity FAILED entryatlas_work11,497,909 vs10,631,585; exactly before30sobserverworkdelta866,324 (subtracting gives5113both, diagnostic only; raw gate unchanged). All459common edits identical N1;388.8s stage ends atdifferentcount459vs486. R5readonly202.5s foundnoauthorizedwiringfix; no retry/G2/full. Counter sums agree; all6ownedIDsactualhostterminal/all1804casehelperpinsunchanged before productionrevert. N9reverted; noaccepted efficiencyclaim. User programs observationonly. Evidence R/night_20261115/N9.

B7 section6.2 (2026-11-16): item0 prediction17.70e9B<=19.0e9B; connected5path change failed and was reverted after the C492 gate below. Build523failed10.138870s due declaration transcription; preserved/correctedonce. Correctedbuild524PASSED910.995026s/47actions,CFDc6187c58e829/test82be0e6dc3f7,1561pinsunchanged,134actualhostbuildIDsterminal. A late sample~5.1GB is not whole-build peak RSS. Correctedbuild52520.137106s, units51/1903pass130.175983s;exactcollapse+extra guard842.021957ssealed (not collapse-only). C492FAILED166.764834s,0splits; modeledpeak13,061,193,021B>12GiB. B7reverted9831c787e7; no full B7 event timing exists. AcceptedN1rebuild526610.262174s,CFD949/test6de byte-identical522; N9sourcecheckbeforefirsteditcomplete. Evidence R/night_20261115/section6_2.

Current section6.1 (2026-11-16): P7/P8 restoration atop accepted N5 checkpoint3d28237f03, build522880.347348s/347actions, CFD949359f28cd4/test6de087f99ba0. Serial43cases1697assertions119.558236s; MPI2memory/subset4.527206s; exactcollapse90,809,869work/counts+extra constructionguard626.694995suntilplannedstop/634.619724ssealed. This legacy prefix runner continues past the early collapse boundary; its whole duration is not a collapse-only timing. C492542.492544s,486splits/first336identical, unchanged388.8sconfiguredstage; no accepted BL mesh. 481122.016101s/56cases5947assertions; current492target149.018121s/611527assertions;2D1/2/4 11.007315/12.007212/11.006833s,39cases/rank. All standing precheck resource/pin gates pass; old helper import failure occurred before any numerical case and was repaired without rebuilding. Fullrun1 PASSED normal0/4286.332645s, native2161.697077/1634.828953s, SUMpeaks14028087296/17349582848B; all standing gates/event1P6c/event2P7 match. Isolated restart245.402971s under two-CPU allocation for six MPI ranks; not a like-for-like timing comparison to the131.149530s earlier allocation. Fresh549timerrows beside F3b/firstN1,192qualitywindows; scopes nested/overlapping, wall-time not CPU. Evidence R/night_20261115/section6_1.

N5 observations (2026-11-16, qualified full two-event repeat PASSED): build521 CFD861737e1/test382dc714; exact G2 317.258592s boundary/326.130305s sealed versus317.194/326.125 reference,90,809,869work; GQ1049.623115/1058.351788s versus1064.175/1072.976, all6thirdwindows/oldintegers, SUMHWM13,933,174,784B. Clean full native2581.028493454/1907.499019169s, SUM6processpeaks13,952,311,296/17,729,433,600B<19,999,490,048B, all6P6c hashes and physical/timer gates pass. Fresh204qualitywindows/180qualitytimerrows/549allphase rows and F3b side-by-side: `R/night_20261115/N5/reports/current_profile*`; busy markers are wall-time, join/collective intervals overlap, inclusive tree rows are nested/nonadditive. M16 clean216rows/12rankevents in `reports/m16_d9_counters.*`. First full5544.074028s passed numerical gates (native2947.7362/2158.0587s; peaks13,926,658,048/17,698,131,968B) but FAILED R1: two abandoned owned regex scanners beside six ranks (8>6). Their verified cleanup and one clean repeat are preserved, never an accepted speedup. Corrected geometry293/492/input wrappers341.925/293.261/25.601s; locality2/4/6 wrappers54.691/47.719/47.824s, peak sums5,394,911,232/5,973,905,408/6,684,585,984B. Test-only builds515–521 left CFD byte-identical; build52180.134s. Counter175assertions4.499s; five stack/channel wrappers4.448/4.431/4.508/6.439/7.426s and bounded operators55.716s. Partial BL fixtures retain quality debt and establish no accepted BL capability. No controlled efficiency claim: host conditions and observations differ. Latest user section6 requires P7/P8 again, conditionalB7, then correctedG1 cut reference; N7 is not final yet.

Historical first unattended N4 cut (2026-11-16): [source check and ledger](ADAPTATION_ADAPT_CUT_20261115.md). N1 failed/reverted; N2 B7 not started; N3 source check completed. Accepted F3b rebuild:870.265s/347actions, current source/build pins unchanged; sampled build-tree RSS is sampled, not whole-job HWM. Before any cut edit, new references passed: G2 317.194s to boundary/326.125s sealed, exactly90,809,869work and480/473/346/263/469/464 collapses, peak sum5,201,391,616B; G3 C492543.547s outer/unchanged388.8s stage budget,473splits/305015positivecells/first336equalP6c, serial GNU-time peak3,323,441,152B; GQ1064.175s to boundary/1072.976s sealed, third quality window on all6ranks and embedded exactG2, sampled rank-HWM sum13,917,859,840B. Reference hashes/rows/resources: R/night_20261115/N4/reference_G2, G3_reference_receipt.json, reference_GQ. G3's fresh473-split count differs from historical F3b462 under the time-bound fixture; no equal-work timing gain is claimed. A1 build510 passed70.128s; G1 serial failed first metric assertion209.9357526597 versus656.1522579418 and timed out603.814s. MPI2/4/G2 did not start. One read-only diagnosis found no transcription fix; failed ec0e8f3ef5 is preserved and A1 source reverted. N4 has no accepted performance improvement or completed cut yet; outer/engine/nested timers remain nonadditive.

Historical first N1 FAILED event2 output identity; connected production/config/tests reverted to F3b bytes. [Report](ADAPTATION_P7_P8_R3_20261115.md), R/night_20261115/progress.md. All prechecks passed, with fixture repairs/provenance retained. Full tool28458 exited0, allhandles terminal,4117.532s; independent physical/memory/timer gates pass. Native events2220.3398/1450.9665s versus F3b2683.4115/1977.2148s; exact jobpeak sums14007463936/17343528960B versus13950185472/17696780288B. Minimum remesh namedcoverage96.3632%/96.6216%; complete per-rank tree comparisonCSV is in p7p8_r3_20261115/failed_N1_vs_F3b_timer_rows.csv. Root clocks are about0.05s wider than native totals; inclusive/nested clocks and rank-summed seconds are nonadditive with job wall. No accepted speedup claim: event2 has+68cells/+7points, all3payloads differ; event1all3match. R5 bounded read-only diagnosis found retained-cache pieces entering geometry partition weights, no proposal transcription. No numerical retry or restart; B7 NOT STARTED because N1failed. N3 is now complete; current N4 reference/cut work is above.

Prior P7/P8 fixture correction2.0e-r2 (2026-11-15): [report](ADAPTATION_P7_P8_R2_20261115.md). Both configurations received the four required time-domain options; production CFD hash unchanged. Fixture rebuild80.115s passes1,544pins. Resumed serial check(a)4.543s wrapper/0.28s process fails again in the same fixture: initial1MiB MemoryBudget refusal emitted with job/process peak33,402,880B, then the allowed-memory ImportGeometry branch aborts on face/physical-label manifold incidence. History-array abort resolved; no second fix/retry. Later units/MPI2/collapse/C492/481/target/2D/full workflow NOT RUN. No event2M18/B7 predicted peak available; B7 not implemented. All owned handles terminal, no new performance or accepted-mesh claim.

Initial connected P7/P8/M18/M20/M21 (2026-11-15, prior to reviewer correction): [report](ADAPTATION_P7_P8_CONNECTED_20261115.md). Whole-item source review complete after2.0d-r/e-r. Compilation invocations40.102s failed(bool MPI type),910.140s passed(full),80.113s passed(final static fixture/storage corrections), all pinned; no numerical tuning. Serial sandbox launch4.693s failed socket EPERM before MPI_Init completed/no tests. Executed host unit check5.471s wrapper/1.38s process aborts in the first new P8 fixture before Remesh: steady configuration lacks time-history arrays read by State. One assertion passed, fatal bounds assertion follows; no memory refusal/retention qualification. Stop/no repair or retry; all sampled owned handles terminal. MPI2/collapse/C492/481/target/2D/full workflow NOT RUN. No P7 speedup or resource/counter result claimed. Build505 is unvalidated; existing P6c/F3b results below are historical qualified evidence.

Current F3b (2026-11-15): [report](ADAPTATION_F3B_20261115.md). Implemented reviewer scopes only; no join optimization. Build504 490.164 s; timer units 9 cases / 25 assertions, 4.509 s. Exact collapse work/counts pass, 330.164 s outer, coverage 96.465–96.687%. Changed objects required conditional repeats once: C492 543.473 s outer / 462 positive splits / first336 exactly P6c; 481/target/2D1/2/4 wrappers 124.530/155.802/8.298/8.139/8.119 s, all pass. Full numerical process 5082.303 s, controller 5091.350 s; native events 2683.412/1977.215 s, nested engine 2596.907/1926.176 s; reported rank lifetime peak sums 13,950,185,472/17,696,780,288 B. Complete-event coverage 96.911–97.586% / 97.460–97.942%; join difference <=0.000159%. Quality unattributed 13.397–30.518 / 3.518–13.677 s versus approximate 15–25 s prediction; discrepancies retained. Clearance and old-cavity exclusive per-rank costs are in the report; overlapping inclusive clocks are nonadditive. Host load 2.33–9.96; no controlled speedup claim. Numerical/physical/event gates pass, but final aggregate assessor raises duplicate-root TypeError at line40 before the all-six P6c identity check. Original stop preserved; reviewer2.0b-r helper-only correction now passes once, including all-six P6c payload identity. No build/numerical repeat; F3/F3b complete. P8 section2.0e-r resolves the prior subset collective issue. P7/P8/counters remain stopped before build/run at P7's query-call versus wall-face equality:31,622recorded queries versus52,583wingfaces in retained event1. [Source report](ADAPTATION_P7_P8_SOURCE_CHECK_20261115.md). A5/B5 withdrawn. Historical F3 failure below remains preserved.

F3 step3 (2026-11-15) **failed complete-event coverage**, see [report](ADAPTATION_F3_EXCLUSIVE_TIMERS_20261115.md). Native event1 accepted2659.995s; rank-peakRSS sum13,923,971,072B; mesh/ref exactlyP6c. Ledger sums reconcile but named coverage68.357–74.357% misses95%. Quality unattributed619.987–773.172s/rank, construction52.156–58.582s, root9.852–10.015s. Old-cavity node0.168–0.333s/rank/178067–333516calls includes old edge/field work, not pureq/J. Bounded collapse95.699–96.108% and BL98.356% did not establish complete-event coverage. The one full milestone was deliberately stopped at this first failure before event2; all handles terminal, no retry/fix loop. Compiles500failed40.148s,501passed500.128s,502passed70.145s,503passed20.146s; all failure/scope-completion evidence retained. Exact ledger/caller scope and attribution gaps remain explicit; no isolated speed claim.

Current connected B1/B2/B3/A2c (2026-11-15): [report](ADAPTATION_M6_CHEAP_GROUP_20261115.md). Build499660.202s, units196.655s/79cases20580assertions. Six-rank collapse work EXACT90809869 and counts P6c; native933.125s, outer1272.214s under unrelated Lean contention/load~16. No seconds-based regression claim. BL462splits/304988positive in unchanged388.8sconfigured budget; API390.199s, engine389.200s, outer547.142s; first336edits EXACTP6c. Observed throughput only, unequal completed work/host; surface not ready.481merit123.752s/56cases5943assertions; saved-current151.553s/611527assertions;2D1/2/4ranks6.896/6.844/6.761s,39cases/rank. Counter:1facet resolution each rank at collapse,2at serial BL exit despite0refinements. Full-event A2c is reported in the profile tables; rank0 event2 has 246144 misses at one resolution, giving lower bound 197190 repeated builds. Same-v2 profileA complete557.165s: preparation119.850s/public native393.819s,467splits; profileB complete: total profile wall6920.110s, native events2697.000/3684.252s, nested engine maxima2605.604/2419.099s. Both independently accepted and all six payloads match P6c. Rank peak-RSS sums13,933,756,416/17,657,745,408B pass20GB. Engine time and outer time overlap; event2 post-engine LBR exposes EdgeNormal/BuildMobilityPolicy work, not an exclusive timing partition. External observer143 and bracketed engine epochs are explicit limitations. Historical profile status: F3 was then pending; current F3b results and remaining final-assessment failure are above.


Completed P6c baseline (2026-11-15): reviewer section1.7 supplied after the preserved P6 failure.
Same-fixture baseline scan:31gaps, zero refinements (does not reproduce M6);61.483s,
unchanged P6 production CFD SHA b7c78fdb145684243e43522095233a1b85bb396057545be56efdb3a15bc280ab.
Scan harness first compile failed10.145s on missing test-only standard headers;
headers corrected, build30.137s, scan run once and passed. Both receipts retained.
P6c build passed570.142s/1539inputs unchanged; check1 passed77cases/8710assertions210.677s.
Checks2/3 pass: exact90809869collapse work/predicted counts; collapse max267.270s, construction634.158s, phase366.887s.
All6zero refinements, required rank5/gen1 and rank2/gen4 completions observed. Rank0facet339060/13328=25.439work/miss.
Load1m3.96–10.37, outer750.796s, intentional boundary stop/all9owned terminal; sampled solver RSS sum max7066300KiB, not whole-job peak.
Behavior differs after collapse; no equal-work speed claim. Nested timers remain overlapping. Check4 passes336splits/304663positive in the same388.8sconfigured budget, zero refinements; outer566.603s, API390.280s.
First edit difference59; changed bounded output allowed by P6, no equal-work speed claim or accepted BL. Check5 passes:481merit149.628s/56cases5940assertions, saved-current175.501s/611527assertions;2D1/2/4ranks6.503/6.486/6.518s,39cases each rank. Full6 and isolated restart pass; details below.
Evidence: staged57/purpose_query_fix_20261114/p6c_20261115.

Completed P6c full workflow: event1 collapse265.358→construction637.180→quality674.853→engine exit2839.815s, wholeNative2941.587s; event2 collapse300.343→construction346.921→quality391.515→engine exit2070.980s, wholeNative2128.892s. Actual points/cells146676/726442 and168790/855760; all strict final bad counts0. Native sum5070.478s, numerical process wall5493.181s, controller+validation5502.371s; unchanged configured5400s Native budget per event. Overlapping clocks must not be added. No controlled equal-work speedup claim.
Full host load1m4.04–10.62; modeled per-rank peak7880819872B<12GiB; sampled concurrent solverRSS13069084KiB, individual2293808KiB, neither a measured physical whole-job peak. One-sided refinements0/0, deferred completions20/0. Raw format-only validator failure preserved; read-only publication-label correction passes all original numerical gates without solver rerun. Fresh event2 disk restart106.211s passes unchanged source/runtime/solution gates; all owned handles terminal. Historical P6c attribution was pending; current F3b scope and final-assessment failure are above.

Historical P6 (2026-11-15) stopped on check3 failure; see [P6 report](ADAPTATION_M6_P6_20261115.md).
Isolated build496500.164s/1539inputs unchanged; check1 passed77cases/8584assertions in163.569s.
Check2 passed90809869work<=108459426, predicted480/473/346/263/469/464, zero refinements;
maxrank collapse elapsed266.185s (P5 max426.100s; seconds have different contention).
Same run's surface splits later recorded4upper_refinements on each of ranks2/5, contradicting P6.
Latest gen5 split-boundary elapsed442.921s; deliberately stopped before construction, outer547.210s.
Host load1m2.56–10.14; sampled concurrent solver RSS max6372224KiB, not whole-job peak.
Per-purpose work/calls, facet work/misses, nested update times and first-refinement brackets saved in
staged57/purpose_query_fix_20261114/p6_20261115/nobl_to_construction. No complete phase comparison
or capability acceptance; checks4–6 not run. Superseded by the reviewer P6c correction above; no independent fix loop.

Updated2026-11-14: P5 attempt7 built in40.085s and passed focused validation after the terminal attempt6 workflow; [current validation status](ADAPTATION_M6_PURPOSE_QUERY_FIX_20261114.md). Build293 Euler success/timings below are historical. Build494's unprofiled full workflow rejected both adaptations at the unchanged time budget; see [clean-check work/timing tables](ADAPTATION_M6_NOBL_CLEAN_CHECK_20261114.md). The diagnostic old-lambda build reduced first-collapse upper work from1,034,769,513 to78,172,576 but undid the BL magnitude correction; it is not a production fix. Build495 attempt3 first-collapse upper work117,163,715 (88.68% below494) still failed the108,459,426 gate; attempt5 after A2 allocation116,641,480, all six counts equal494, inner max426.763s. A2 removed all memo evictions but reduced upper work only0.45%; no main-cost attribution to eviction pressure is supported. Whole query work and wall remain distinct; the host was busy. Attempt6 work116,540,211 still misses by7.45%. Its full event1 rejected at5401.986s whole-native/5345.664s nested refine; event2 was not evaluated. The quality phase completed only9 batch rounds after entry at2418.792s; rank3 upper-bound timer3561.998s includes nested guidance728.256s. Full first-event upper work620,573,401 versus293 accepted-event2,423,102,562 compares unequal progress (P5 still has24359 q/24622 J/356 geometry failures), not equal-work performance. Host load2.97–15.03; max modeled per-rank storage10.186GiB, reported max individual RSS5.972GiB, sampled concurrent six-solver RSS12.760GiB. These RSS observations are distinct from modeled per-rank12GiB admission and do not establish whole-job peak. Attempt7 retains certified outside receipts in the existing memo; first-collapse work remains116,540,211 (zero saving), with identical six-rank counts and426.100s native. BL300-split output stays byte-identical to v2 A; API389.774s, outer565.547s, upper work78,851,436 versus attempt6 79,308,240 (0.58% lower). Units76/8537, merit55/5763, saved611527 assertions and2D1/2/4 pass; none establishes the missing full workflow. Full workflow and BL acceptance remain unvalidated. The complete curved-BL/RANS goal remains open.

Production492: configured388.8s/233220work/12GiB unchanged; actual389.572522s native/566.441118s outer,305splits,1.277287s native per accepted split (1.857184s including setup/observations/capture). Census31308/39853=78.5587%, incomplete.305new nodes/770new tets give61855nodes/304593positive tets, provisional.1014geometry-guidance evaluations/705refusals=69.5266%;1288private failed checks and448top-level rejected/deferred selections are different scopes, not interchangeable trial denominators. Native2974globalwork/73841533geometrywork, CPU389.204810s. Native reserved-work maximum10098980728B<12GiB. Largest sampled individual RSS3194220544B is NOT whole-job peak; process-tree samples are retained. Actual prelaunch46,286,671,872B available/load1.50; nice19/libraries1, one owned heavy job throughout.

Before/after fixed-face observations30.001276/30.000979s used7877612/11238584geometrywork respectively, charged to the shared ledger outside the native timer. Each exhausted its first full-entity ErrorMagnitude query:0/1799before and0/1926after faces completed, no two-halving/deviation conclusion. Local-time budget copies do not poison the production deadline. Approximately116.866s of wrapper time remains unsplit across loading/setup/import/capture/verification. Upper/scope/facet geometry counters include observers plus native work and overlap; do not add their timers or attribute235.269s upper/16.793s scope exclusively to native construction. Exact split ancestry/output topology agree. Read-only cubic-direction analysis took.565s at ancestor centers and.718s at exact saved bindings; forward roundoff<=6.66e-16, all36spanwise margins>=.1023/principal gaps>=.6502. These are diagnostics, not geometric acceptance.

Stage2 producer523.101s outer (518.48GNUwall,3072.69user+5.60system summed inclMPIpolling). Nested metric439.928s, construction388.186s, geometrysetup51.354s, Hessian.392s maxrank; checkpointwrite/join42.048s nested. All1756pins/actualhandles verified;12payloads303788608B/all6same47601773Bauthority. Reload364.939s outer (359.22GNUwall,2124.46user+4.16system summed inclMPIpolling),1769pins/10actualidentities verified;61330/61330owned tensors and chords bit-identical. Seed sensor export1.721s, core complexity140142.106, independentMacherror6.66e-16. New seed-target492586.451 with empirical indicator49430.288/tangent.28717458875/core.00472635759;500k nominal, evolved complexity unmeasured.

Qualification costs: build492 failed241.845s (new test interfaces); build493 passed448.057s; focused493121.558s (8cases pass, only incomplete new checkpoint fixture fails); fixture-only build494 passed313.760s/2tasks; checkpoint494 passed3cases/60assertions/4.506s (.32GNUwall). All9unique focused cases covered with explicit relevant-source/object/runtime reuse. Failed states/snapshots remain preserved.

No controlled computational speedup is established: corrected target and saved input differ from historical484, and host contention differs from441. Historical484 had86splits/389.675s, so throughput improved observationally, not an isolated algorithmic gain. Curvature propagation reuses differential values already computed by mapped projection/association; no added closest-point search, and normalization still reports40506geometry queries. Final accepted curved-BL/MPI/transfer/CFD/restart cost remains unknown. No changing-count or pre-BL q/J progress claim.

Post-run mathematical self-check proves a joint-envelope gap in the independent principal-cap extension (R/joint_curvature492_debt.json). No second native run/build followed; individual-axis and reload evidence does not establish mixed-direction envelope correctness. Wider doubly-curved qualification and corrected target semantics remain required before another cycle.

Historical measurements below retain their original scope; currenthandoff owns state.
[Current handoff](../HANDOFF_Codex.md) owns source/build/handle state;
[stage contract](ADAPTATION_CONNECTED_PATCH_TRAJECTORY_20261113.md) owns acceptance/stalls.
R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.


##486 data-only comparison and inspection export — complete

Same485build/target/profile/currentrootmove; three contexts26/23/20complete endpoint
unions, ALL24constructors.178.773souter/4954765geometrywork;1788pins/fiveactualidentities
verifiedterminal. IndividualsampledRSS1,324,937,216B is notwholejobpeak. Zero active
feasible/publication; outputOwned ANDparameters EXACT485input. Tippositivecollapse
6→4geometryscreens but upperexcess+4.50%; free-sidecollapse7→6 but excess+141%.
Both geometry-admission refusals, not proved physical violations. Cleancontrolprivate
feasible. Nominatedreverse rootedge never generated; no operator-inability conclusion.
No comparable walltime speedup claimed across different cavity nominations. Pertrial
CPU/wall/work retained in actual_bl_cavity_planning.jsonl; no totalwait attribution.
R/reverse_surface486_assessment.json owns coverage/inputidentity checks.

Requestedinspection export uses no native target queries/build: wing19727points/
39243triangles, allboundaries24700/49396; allVTUarrays roundtrip checked. Sensor tensor
only, not fullcomposedtarget; no fabricated globalquality. R/inspection_surface485/.
Next487decision is exactdirected3candidate coverage; test-only buildPASS465.248s/2tasks/1529pins/10actualterminalidentities, CFDunchanged. Actual3candidate comparison TERMINAL173.451s (casecurved487_directed_surface_corrections_serial_20261113T131300_3203305),1794pins/fiveactualidentities verified.2067261geometrywork; zeroactivefeasible/publication; Owned/paramsEXACT485input. Rootconstructor exactpositivity fails266950; tipgeometryadmissionfails; cleancontrolfeasible. Trialwork/wall149829/1.582763s,692/.042882s,154428/.606005s. GNUtime164.88user+1.02system/166.22wall; outerwrapper173.451. Restoration/halo/capture remainder notsplit, no waitclaim or speedup. IndividualRSS1,404,796,928B notwholepeak. Build488PASS329.814s/2tasks/1530pins/10actualidentities, CFDunchanged; actual488TERMINAL183.618s/1799pins/fiveactualidentities.2513378geometrywork; roothelper63009work/1step/.012637s, no growth20→20; positivefinalReconnectCollapse20→21 takes385264work/1.621721s includingmeasurements/admission. q20→18,J19→16,geometry7→6 butexcess+57.793%refuses, nopublication/Ownedparamsunchanged. Tip149829work/1.580353s,control154428/.570115s. Finalcalls3/internaltotalupper9of24. GNUtime173.50user+1.48system/176.31wall versus183.618outer; attributionremainderunsplit. IndividualRSS1,375,707,136B notwholepeak. No timinggain/MPIwaitclaim; no ownedheavyjob.

##485 phase-faithful residual candidate comparison — complete, no netfailureclosure

Same484output303823tets/61550nodes/fullparams/currenttransport and441target/stageh.
Only diagnosticbinding changed: nonfixturecount, activeGeometryRepaircollapse versus
stricteligibleCoarsencontrol, publication/admission distinguished; no productionchange.
24slots across33/10/20currentstars; guard/admission/memory/phase reviewedbeforecompile.
Launch47.616GBavailable/load4.70/2.42/1.99,nice19/j3/libs1. Laterhostload rose~9.8 with
availableRAM~42GB; no walltimecomparison/sourceedit whilebuildlive. BuildPASS484.384s/2tasks/1528pins/10actualterminalidentities, cleanNinja/bothbinaries
verified; CFDhashUNCHANGED484, onlytestbinarychanged. Actualmatrix TERMINAL211.218s/24constructors/all3contexts,1782pins/5actualidentities
verifiedterminal.4419766geometrywork;4activefeasiblemoves, root6→6pubexcess−8.9243%
BUTgeometrybad4→4,q6→6,J5→5;66cellhalo unchangedcounts16geometry/52q/49J. No netclosure
or timinggain. SampleindividualRSS1,319,981,056B notwholepeak. Casepertrialwork/CPU/wall
retained.485output303823tets/61550nodes,67retired64065077new, allothersensorsunchanged.
486data-only/SAMEbuild follows evidence ofwrong collapse direction: constrainedroot→
offcarrier refused, reverse-side capability untested. Newcontexts26/23/20starunions,
preserve rootmove;24slots/300geometry/600outer/12GiB unchanged. No source/build/norm/
sensor/constraint changes; no timing comparison acrossdifferentnominatedcavities.

##484 connected required-demand discovery — physical edits, incomplete closure

483monolithic census did notenableconstruction. Revisedinitialentry repeats existing
16-row batches onlyuntil globalrequired deficit or completecursor, then existing
handoff/refill/completefinalreadiness. No newstate/config/scheduler/gate/budget.
Same303606tets/61464nodes;483outputOwned+params EXACTinput, fulltarget/stage unchanged.
BuildPASS85.661s/5tasks/1527pins/18actualterminalidentities, cleanNinja/bothbinaries
verified. Launch47.984GBavailable/load2.84/3.56/4.57,nice19/j3/libs1. Focusedserial
PASS7.616s/14cases/2621assertions/1732pins/5actualterminalidentities.
Actualsame303606inputserial TERMINAL511.504souter/389.675sAPI/388.880nativeexit
vs388.8safe-pointclock,1785pins/5actualidentities verified. Requiredhandoff42.213s/
6384of39071wallrows then86splitpubs/834kernelwork, nooptionalcollapse. Output303823tets/
61550nodes,86newpoints, physicalfaces172removed/344new, allsurviving303389cellrows
byte-identical; no survivingnodalcoord/tensorchange.24370/39243wallmeasured, no readiness
or wholefieldbad-removalrate, q/J/heightsunknown. No surface_repair_entry.
Geometry33350938work; upper5984queries/31750741work/219.053s; guidance203.630s overlaps.
Nestedfacet preparation151.745s, scope9.916s, traversal57.054s: not additive toupper.
Facet3577misses/28131hits,3577retainedentries; scope1690misses/1167hits; prepared
1,988,502,144Bof3,204,054,800B,0evictions. Exactunionmemo3202hits/2782misses;
missesalone cannot distinguish nearbypositions/new queries/previouslyevicted identical
queries (16437memo evictions). Validupper>tau4933,0unionexceptions; notresourcefailure.
No new profiler campaign or cache mechanism selected from these counters alone.
CPU388.425s; accountedpeak9,937,462,744B/sampleindividualRSS3,195,957,248B, notwholepeak.
Serial/MPI mechanism qualification: MPI6ALL14cases perrankPASS4.666s,1732pins verified;
2sampledactualidentities terminal, fastworkers notcaptured forRSS/CPU, sixworker summaries
and normalMPIexit0. No worker-resource or speedupclaim. Reuses prior unaffectedgeometry
checks; not dedicatedfullBLMPIqualification. R/surface_discovery484_outcome.json.
Relative to483sameinput: moves work frommonolithic measurement to86real edits, but
DEFERSpartofcensus. Lowergeometrywork is notprovedtotalworksaving; no finalaccepted
adaptation timingestablished. Nexttarget: useful surface repair/certification power,
notanother blindsplit/censusretry.
Physical construction demonstrated; netclosure and complete-workflow speedup unestablished.
Decisionandcriteria in
ADAPTATION_SURFACE_DISCOVERY_484_20261113.md; R/surface_discovery484_fidelity.json.

##483 connected requested-wall census decision — measurement bound, no construction

Reuse current482valid303606tets/61464nodes, SAME441target/authority/stageh6.4e-5.
Only seed predicate enables existing full entry measurement for empty-composition
surface preparation; existing16cell imports/admission/deadlines retained. Composed
volume remains incremental; no operator/config/gate/budget change. Need complete wall
coverage ANDuseful required geometry/tangential progress, not merely measurement.
Build PASS105.973s/5tasks/1526pins/18terminalactualidentities; cleanNinja/bothbinaries
verified. Launch49.983GBavailable/load2.97/3.82/6.52,nice19/j3/libs1. Focusedserial
PASS9.361s/13cases/2611assertions,1731pins/5terminalactualidentities. Actual483serial
TERMINAL from progressed482state; all1782pins/5actual identities verified.
Outer522.326s/API397.916s/nativeexit397.165s vs388.8s clock; full entryseeding
36930/39071wall rows,0work/commits/trials. Input/outputOwned+params byte-identical.
68399194geometrywork;6786upperqueries203.070s/67492516work,0unionexceptions.
5624upper_refusals is VALIDupper>tau, notresourceexception/physicalviolationproof.
Nestedfacet56.830s/scope17.892s/traversal127.765s; do notsumoverlap. CPU396.501s,
maxaccounted8,835,972,848B/sampleindividualRSS2,563,235,840B, notwholepeak.
2794geometryscreens/30sizebad in known subset;2141wallunknown, q/J/heights unknown.
No readiness, no speedup/usefulconstruction claim; monolithic entry path not practical.
Nextdecision: yield discovery to existing requiredhandoff oncefirstdeficit known,
retain censuscursor and complete finalcoverage requirement. No newbaseline/profiler.
R/surface_census483_fidelity.json; revised decision executed as484above.
No timing/work improvement or readiness established; current input differs from482entry.

##482 connected production-stage continuation — serial complete, MPI timeout

New shared bound reuses already computed point-distance upper plus outward physical
childdiameter for magnitude physical-source only. Original failed256MiBcheck unchanged.
New binding exercises public PrepareSurface from current304585tets/61840nodes with
transported metrics/parameters and SAME441policy/authority/stageheight. No upstream
CFD/geometry/metric recomputation. Actual driver wallh/growth/thickness forwarded;
old fixture omitted region controls and reset input, so its timings are NOTa matching
current-state baseline. Existing388.8s/233220orlower/12GiB, parent5400s/outer600s.
Stage timers include safe-point return/capture; record overruns and attribution gaps.
CPU includes MPI polling; tracked collective time is not total wait.

Build482PASS371.770s/5tasks, nice19/j3/libs1; launch50.178GBavailable/load1.33/1.41/2.18.
All1525pins/18actual identities verified, cleanNinja. Focused53cases/5716assertions
PASS79.719s/1730pins/5terminal identities. Same256MiBbodycheck unchanged; magnitude
multi-querycase14.448svs48189.860sFAIL. No query-isolated orM6walltimegain claim:
setup/contention and case scope differ. Actual serial stage is terminal; work savings,
netbadcounts/readiness and MPIqualification remain unestablished. FullTarget files
all6verified SAME47,601,773Bauthority; sourceinput shared coords/tensors consistent.
R/surface_stage482_fidelity.json and saved_production_surface482_inputs.json.

Serial actual production stage:518.229s outer,389.952s APIreturn,388.895s nativeexit
(388.8s clock; safe-point/return/capture overruns explicit),369.814s processCPU.
377commits(376collapses/1split),49913work,17,962,582geometrywork;
303606positive tets/61464nodes. Only7448/39071requiredwall rows measured: no net
whole-surface geometry/q/J closure rate or directional conformity established.
Existing collapse-correction timer170.313s/4661constructed-support-corrections/18281added-support-cells/35353work is a major
measured cost (~44%of APIwall). The4661counter is successful support correction
construction, not all timer invocations; totalcallcount is not isolated. Nested upper-query15.645s/3067calls/7130550work,
scope1.354s, guidance13.620s, update8.539s overlap; DO NOTadd them.
Tracked collective .896s plus election .194s are call timers, not totalMPIwait.
Maximum accounted workbytes10,077,652,824; sampled individual RSS2,177,744,896B,
not summed-job peak. No speedup claim or comparison to resetting old fixture.
All1781pins/5actual handles verified. R/saved_production_surface482_serial_assessment.json.
MPI6 same-input attempt TERMINAL124/600souter (608.145s wrapper), all1781pins and
10actual owned handles verified. Last common window1856/native306.324s:314commits,
303786observedcells,3437/39194wall measured, initiallyemptyrank5 aligned. No exit
capture or qualified MPIcheckpoint; timeoutSIGTERM is not a failed geometry invariant.
Max sampled process-tree RSS4,044,648,448B (sampled, not exact peak); rank CPU ticks
and partial per-rank windows retained in R/saved_production_surface482_mpi6_timeout.json.
Pre-census time lacks target-restore versus engine-setup attribution. No additive timer,
CPU-usefulness or speedup claim. No upstream CFD/target solve repeated.

##Older qualification context (historical, not current source/status)

Build476642.982s under contention; current-input trajectory228.411s/609828assertions,
legacy regression264.200s/610217assertions, both terminal/pins/actualhandles verified.
That trajectory had5pubs/24calls, no readiness. Hostlibc changed5pinnedlibraries;
historical manifests preserved, runtime_20261113_474explicitly bound. No walltime
comparison across runtime/contention changes. Build473305.914s; focused62.658s/
52cases/5688assertions; MPI185.914s/41cases everyrank; source35e173b043 pushed.
Its surface integration was NOTREADY; these are retained historical qualifications,
not a claim that current484source/CFD equals473or476.

##481 active connected correction — unqualified

Build481PASS917.563s,47Ninja tasks, nice19/j3/libs1; launch52.335GBavailable,
load12.47/10.39/7.13. All165actual identities terminal, source/binary pins and clean
Ninja verified. Focusedserial FAIL151.562s:52/53cases,5715/5716assertionspass; new magnitude
separate-body case89.860s fails256MiBstorage. No incorrect acceptance observed.
Actual fixedM6 comparison PASS183.704s/1773pins/5terminal identities.24calls across
all3contexts;9active candidates admitted, one MoveVertex published15→15cells. Geometry
excess−29.0024%BUTbad6→7,q/J15→15each; strictheightFALSE.5399586geometrywork
versus4782261666, no efficiency or netclosure gain. First connected entry FAIL157.239s/1775pins before anyconstructor/work: stale retired
vertex descriptor, NOTproduction failure; mesh/params unchanged. Corrected continuation PASS276.745s/1777pins/5terminal identities.24splitcalls/24pubs,
R4nested114.166740wall/111.374853CPU/3187979geometrywork; R5.702123wall/.701898CPU/
143855work,0calls. Total3331834work; retain nested timers separately. Geometrybad
localdelta+4,q/J+64each despite~89%excess decline. Scope27→121includes30old additions;
rawgeometry10→31 is not churn. Shortplanning edges<.5grow2/23→72/128. No geometry
closure/firstrow/heightcoverage or useful-progress/s improvement established;
no timing, work-saving or useful-construction improvement claimed. Exact sourcepins
and CPU/RSS samples captured by existing build helper. Preserve frozen sources.
Shared fixed magnitude-query purpose replaces473's2tauheuristic, keeping physicaltau,
ordinary classification, final bidirectional certificates and every budget/gate.
Need common old/fresh complete-patch results and connected useful progress;480's
asymmetric counterfactual alone is insufficient. Source review and test scope in
R/geometry_magnitude481_fidelity.json. Reuse current476coherentstate/441target/height.

##478–480 completed construction comparison and certification decision

Reuses476coherent304521tet state and SAME441target/profile/stageheight. Prepared
reference embedded in441checkpoint is extracted, then native decoding must check
exact authority agreement. No CFD geometry/mobility/metric construction repeated.
Three contexts each8distinct slots; total24/300s/600s/12GiB unchanged. Input endpoint
starunions27/10/20cells. 478buildPASS355.909s; actualcomparison182.460s completes24slots before diagnostic
postpublication staleOLDview failure. All3contextscovered; noactivegeometry-admitted
replacement; onlycontrolcollapse10→8published.304511untouched cells bit-identical.
479reuses that output for proof only,0constructors/0repeatedcomparisontrials. No speedup claim.
479build357.852s; zero-constructor proof192.915s/1774pins, all5actual handles terminal.
The already published control is semantically rechecked; no repeated trials, no active
geometry closure. 480exact-query wrapper47.552s/1731pins, all5actual handles terminal;
nested query.029949181s/1557work, traversal.000840165s/395work. Do not add these timers.
Same48support/child coordinates: upper.004238912→.000353533 at decision ceiling
.001207578, physical gate1e-4 unchanged. Complete fresh counterfactual excess
9.45926e-8 vs preserved old1.25704e-6; BOTHsides under a common tighter policy remain
untested. No speedup, accepted mesh, net geometry retirement or MPI capability claim.
478trial sums: R4 5.123800s/450778geometrywork;R5 2.348240s/992908;control
2.182837s/275065. Overall2,261,666geometrywork includes observation/publication setup;
these are not additive exclusive phase totals. SurfacePlanning lengths are endpoint/
strip guidance, not full composed-edge integration. Current measured trajectory still
does not reach tangential/firstrow readiness. See R/geometry_progress480_assessment.json.
The already-frozen477trace completed253.679swrapper with SAME24decisions/geometrywork
and bit-identical workingOwned/parameters as476.1134queries=104misses/1030hits.
19refusals repeat FOUR unchanged split candidates. Trace overhead is not a baseline.
Upper/lower query gaps leave relative geometry regression unresolved.

##473 active cycle — comparable fitting bounds

Parent-face replay54.988s wrapper, four original faces through production guidance,
exact prescribed input bindings validated; original complete two-parent scores agree
up to native longdouble versus Python float summation's last bit. Cold/warm/uncached
primitive results agree. Five actual host identities terminal before continuing.

Eight MATCHED old/fresh same-support certificate queries at2e-4 took95.376s wrapper,
including repeated process/authority startup. Each used existing200000work/8GiB cap,
shared300s;1725unique pins and33actual identities verified terminal. Geometry
acceptance remained1e-4. Old direct correspondence bounds remain tighter than the
supplemental query bounds; two candidate cross-scope bounds tighten substantially.
These are certificate results, not native recomputed complete production merit,
trajectory improvement or a speed benchmark. Native complete-candidate and
regression checks subsequently passed in the scopes below. No net bad-cell removal/s
claim is possible from the incomplete baseline census.

Build473 freezes one production and three affected test/fixture files. Its purpose
is safer/more comparable progress assessment, not an established timing improvement.
Extra certificate work must count against the SAME limits; tighter upper bounds alone
are not end-to-end success. No CFD/target recalculation, new baseline/perf/GDB or
changed construction/height policy. Build PASS305.914s, focused52cases/5688assertions PASS62.658s. Current M6preflight
PASS148.615s:4new residualprivate-feasible candidates, geometrybad2→2 and q/J2→4or8.
Geometrywork484520→1015819 (includes changed mappedcontrol and search/cache effects).
Per-candidate R4split19862→35154work, R5split70009→74752; usefulclosure UNPROVEN.
The complete saved451transaction regression PASS247.696swrapper:24calls/22pubs,
SAME36→110observation,geometry16→24/localdelta+2,qJ+63/+63 andfirstrowunreached.
Trajectory72.709656wall/72.696812CPU/1650973work, vs47074.886533/74.866628/1590930.
No timinggain from these fewseconds; work is up3.77% and mesh progress unchanged.
MPI41cases/everyrank PASS185.914s;3642–124194assertions/rank. Actual fullsurface
integration TERMINAL497.751swrapper/389.166snative,1743pins/10actualidentities.
Requested-wall census100%(39760/39760), vs47077.95%(30500/39128).481commits/4804work,
geometrywork51.310Mvs30.089M. No speedup or rawbad-count retirement/churn comparison:
mesh and coverage differ. Geometry-screen failures3102,sizebad34; no geometry closure,
tangential readiness or firstrow. q/J/heights are unmeasured globally.
Rank5 guidance rejection481/622 vs1124/1157; this is improved admission yield, not
bad-cell retirement. Upper-query seconds77.344 include scope19.276/facet17.879/
traversal39.652; these timers overlap guidance62.014 and other wrappers. Do notsum.
Rank4 upper80.284 includes scope22.279/facet21.008/traversal36.382;470rank4 exitcounters
are unavailable due interleaving. Newrank1 exitcounters also interleaved/discarded.
R4/R5 hold84.3%of measured remaininggeometry-screen failures; R2holds15.7%.
CPU includes polling; election/join is not totalMPIwait. SampledtreeRSS4.642GB/
individual1.006GB are not peaks. Next investigate actual coupled geometry-closure
yield on the COMPLETEcurrent wall field, not a speculative timing tweak.

##476 complete current-input trajectory — contact unblocked, readiness stalled

Build642.982s under hostload~18–20; no compiler-speed comparison. ProductionCFD SHA
unchanged. Current trajectory228.411swrapper/1783pins/all5actualterminal handles.
R4:27.276542wall/27.100779CPU/933080geometrywork,24constructors/5pubs; R5measurement
.608298wall/.608067CPU/49678work,0constructors because shared24exhausted. Contact
passes24/24 after persistent map accounting correction. Collision admission peak
9,614,873,560B<12GiB; each actual map estimate136,419,584B, retained3,829,441,888B,
held3,965,861,472B; capture reserve134,283,264B. This is reduced conservative
reservation, NOT a demonstrated allocated-memory reduction or speedup.
Coherent working mesh304521tets; original304508rows unchanged. R4scope17→38(+8old),
geometry10→11/localdelta−4,q/Jlocaldelta+13each. Below-.5edges0/19→10/44; directional
need unresolved. No whole-field retirement/s, strictsurface/firstrow/BL acceptance.
19guidance-descent refusals; no demonstrated physical violation from upper bounds.
Trajectory MUST be reconsidered before another variant/fullretry; downstreamunreached.
R/connected_trajectory476_outcome.json preserves the complete bounded outcome.
Legacy regression PASS264.200swrapper/1782pins/all5actualterminal; exact16step replay,
24calls/22pubs, same36→110scope/localgeometry+2/qJ+63each,firstrowUNREACHED.
78.018044wall/77.896780CPU/1663296geometrywork. Canonical exact checks pass; no new
MPI/BLcapability or speedup from this historical regression.

##474 current-input trajectory — stalled, no mesh progress

Same saved473mesh/parameters,441target and physical stageheight. No legacy451precursor.
191.814s wrapper includes authority/target reconstruction; current R4trajectory7.982927s
wall/7.980734CPU/643727geometrywork and R5trajectory5.990949s/5.989344CPU/449132work.
ONEshared24limit:13+10constructors,23constructed,0published,1callunused. Geometrybad
10of17and6of10observed cells unchanged; q/J17/17and10/9unchanged; planningmax1.23655
and.91876. Whole-field churn and finalgeometry/height/shape are NOT measured here.
All304508original records byte-identical; last coherent private mesh/parameter state
saved. Control6070has zero native guidanceexcess, not a firstrow certificate.
Four candidates lower complete-patch excess, but all23collision protocol calls fail;
reason text was overwritten by later height checks. No physical collision/height
obstruction conclusion.475separates these reasons and adds missing current-input
production GeometryRefine progress admission. Legacy replay remains a regression,
not proof of identical production admission. No production source/operator change.
FirstrowUNREACHED; this is a stalled geometry prefix, not successful stage output.

## Latest bounded diagnosis (not a performance baseline)

472test-only build297.837s, current-residual replay147.641s wrapper/608081assertions/
1771verified pins, original303757Owned records unchanged.24high-level constructors
share existing work/time/memory bounds; one clean-control collapse private-feasible,
no residual publication. Same replay with EXISTING query trace147.639s; all24
nontiming decisions/measurements exactly match. Trace is diagnostic overhead, not
another baseline or a speed comparison. Per-query preparation and traversal timers
are nested; do not add them to wrapper time.

Eight exact child-face query replays total95.479s wrapper, including repeated authority
load/test process startup. Native queries .012–.341s each. Strict replay bounds match
captured production queries exactly; one derived certificate ceiling supplies tighter
valid bounds for each failed child. Complete-patch geometry upper-bound merit changes;
no construction trajectory or accepted mesh was accelerated. Traversal work per query
is preserved in residual472_query_replay_completed.json. Fewer work units on some
queries do not establish a speedup: their stopping purposes differ. Extra refinement
can also cost more work. No new perf/GDB campaign or raised resource bound.

Geometry/tangential closure FAILED and firstrow UNREACHED in the already-tested470
connected trajectory. Do not report time-to-ready or bad-cell removal/s for a stage
that never reached readiness. Next correction must compare complete old/fresh merit
and repeat the whole connected trajectory, not optimize an isolated query count.

## Current per-stage status

| Stage | Latest measured time/work | Useful output / status |
|---|---|---|
| Saved CFD and geometry |492 reused the ramp5 saved CFD seed, all6targets contain the same immutable authority; no flow advance. | Historical473 load45.271s/import.684s are earlier setup measurements. Euler293 preservation evidence remains unchanged. |
| Composed target/normalization |492 correction focused coverage complete; new6rank producer523.101s and reload364.939s complete/fidelity qualified; saved CFD reused, no flow advance.492 seed complexity492586.451±empirical49430.288,tangent.28717458875; nominal500k. | Complete target rebuilt/reload-qualified before stage3; evolved-mesh complexity unmeasured. |
| Coupled surface preparation |492 serial389.573s native/566.441s outer;305splits/1.277s per split,1014guidance evaluations/705refusals,73841533geometrywork. | NOT READY;31308/39853wall rows measured. Saved61855nodes/304593positive tets plus current parameters/transport/full target.36/65nose splits spanwise rejects size-only sufficiency; fixed geometry bound comparison unresolved. |
| Directions / firstrow | Actual connected trajectory does not reach readiness; no credible fullstage output yet. | Local strict/provisional checks pass; do not substitute them for physical height/alignment/coverage on generated M6. |
| Successive growth | Not reached by current credible stage sequence. | Actual progression, coverage, collisions and preservation remain unproven. |
| Outer transition / strict closure | Not reached. | All finalgeometry/q/J/size/height gates remain mandatory. |
| RANS transfer / continuedCFD / second adaptation / diskrestart | Not reached. | Euler293 historical full workflow completed; RANS obligations still open. |

470qualification costs, separate from stage runtime: build580.205s (header-restoration
rebuild), focused21cases/4077assertions7.713swrapper; MPI32cases/everyrank155.969s.
Saved actual cavity replay251.852swrapper, nested trajectory74.886533wall/74.866628CPU/
1590930geometrywork.24calls/22commits; no surface/firstrow closure. Do not add nested
trajectory/setup/geometry/MPI timers to obtain a new total.

470surface native seed wrappers~90.5s/rank, reconstruction wrappers~164s/rank include
synchronization; rank4/5metric wrappers30.822/28.026s. Rank5guidance45.740s, upper-query
48.476s, scopeprep15.038s, facetprep13.877s, traversal19.073s overlap. Directoryelection
rank0~339.328s/rank5~192.886s includes wait; trackedcollective counters~.8–.9s do not
cover totalMPIwait. Interleaved exit lines prevent safe attribution on several ranks.
No exclusive useful-work/wait decomposition or totalMPIwait claim. Sampled treeRSS
4.432GB/individual.925GB are not whole-job peaks. Full wrapper minus native stage
~109.779s includes setup/hash/load/cleanup; it is not one separately timed operation.

## Evidence-backed efficiency changes and limits

| Change | Demonstrated computational effect | Limits |
|---|---|---|
|393 immutable region invariants +394 existing interior KKT stop|Original difficult edge now converges in147.517s/6375evaluations, where393exhausted its300s atlas deadline after16831successfulsamples. Refinement samples12960→2190.|Same seed/target policy/constraints/limits; numerical witness/cut trajectory changes. No accepted-mesh or workflow timing claim. More expensive region cache records remain bounded.|
|329 bounded optional cubic-cache maintenance|At most64optional cache work units per closest query, preserving original certification cap. Previously captured328query now completes in5791units;325control1314units. Both retain original2e-12query error and16384cap.|Primitive work/correctness gain; not a full-target timing comparison. See [complexity evidence](ADAPTATION_CURVED_BL_COMPLEXITY_20261110.md).|
|360–362 unique-node surface guidance and incremental seeding|Cached measurement coverage17540→49346cells(2.81x) within matching391.6s window; enabled14commits instead of none.|Connected changes, so gains are not apportioned between them. Readiness still failed; no final q/J improvement claimed.|
|364 existing-batch planning reuse|Versus362:5.13%more seeded cells,5.16%more cached cells,4.77%less seed wall time; requests203168→181894(−10.47%), edge evaluations296298→284869(−3.86%).|Full metric work/target IDENTICAL; surface trajectory differs. Modest wall differences remain contention-sensitive. Surface readiness still failed.|
|364 bounded matching patch controls|Root20-cell seed0.55455→0.37551s; smooth14-cell0.30381→0.11588s; exact cached/single-cell edges and flags agree.|Controls, not global speedup. Related failed363fixture and source are preserved; compare engine seeding, not differently grouped direct calls. See `R/seed364_assessment.json`.|


451scoped position-only thin-strip reuse remains qualified:39,140actual pairs,
bit-identical ordered tensors/normals/rules,10,880,920derived avoided predicates;
paired local guidance3.70–5.79x (warm-order limits). Full451vs449 at389s measured
wall coverage13779vs5890, but neither stage completed. No accepted-mesh speedup.

470is a phase-consistency capability correction, not a demonstrated speed optimization.
It admits one actual previously blocked3→10replacement with complete guards, local
geometrybad3→3/excess~5%lower but q/J3→10 and heightdebt. Same-limit surface coverage
30500vs45113779(2.21x), commits164vs107; partialknowngeometry1458vs486 and measured
sets/trajectories differ. No like-for-like closure/churn attribution from these raw
counts. Fullstage CPU2296.593vs2284.099s and wall389.138vs389.125s are budget-limited,
not a measured timing gain. Intervening production changes/source pins are recorded;
no sole-function attribution for the fullstage coverage increase.

473makes four actual previously refused private replacements admissible through
valid tighter geometry bounds; it is not a computational-speed optimization. The
same-budget fullstage completes requested-wall measurement100%vs47077.95%, with
more admissions, but geometrywork is70.5%higher. No accepted-mesh timing, netgeometry
retirement or q/J closure improvement is established. Keep those claims separate.

Authoritative latest evidence: R/surface_stage473_assessment.json,
R/surface_phase473_outcome.json and R/surface_phase473_qualification.json.
The next demonstrated issue is incomplete geometry-preserving surface construction,
especially remaining rank4/5 refusals; NOT proven shape-repair weakness (q/J unmeasured).

Detailed earlier phase timings, improvements, failed hypotheses, source receipts and
comparison/maintenance rules are preserved in the
[historical timing ledger](ADAPTATION_TIMINGS_HISTORY_THROUGH_473_20261113.md).
Old RUNNING labels there do not describe current workload state.

##489 face-admission attribution and requested density view — pendingnativequalification

Offline1134saved477queries matchedONEoldrootface/NOcurrentfreshfaces. Allchangedfaces
areWing/noaffine; root2unchangedfacescyclicallyrotated. No actualorderartifact yetproved.
Test-onlydeferred3context faceprobe/math/scope/memory reviewed. Build489failed61.791s
privateCurvedBoundsaccess,1532pins/9actualterminalidentities. No nativecase ran. Exact
failedsources preserved; retry490usespublicErrorBound foranchorrealizationuncertainty,
samehypothesis/limits. Currentbuild490LIVE/FROZEN,exec39603, no secondheavyjob.
FocusedMagnitudeInterval test plus actualsame-target comparison pending.

Userfluidrhoexport completed via savedseed parametricboundary sampling, no native/CFD
run.24700points(dim0/1/2=24101/121/478),23roundoffweightnormalizations;24479original
referencevertexcoords exactlymatchrestart. Allrho finitepositive, allVTUexistingarrays/
geometry readbackunchanged. Visualizationonly, no conservativetransfer claim; rawunits.
Exportwalltime notseparatelyrecorded, no efficiencyclaim.

Build490PASS367.858s, sourcepins1533/10actualterminalidentities; focusedflatfixture
refused zerodeclaration beforequery4.382souter, notintervalfailure. Fixedfixture491
compiled/linked butwrapper143beforefinalreceipt. Entireownedsessionterminal/pins1534
verified; normalno-workNinja completion0. .025033sONLYcompletioncheck, originalcompile
wallUNKNOWN. Focused491PASS1case/22assertions4.956souter/.001sCatch. Actualexperiment489
underbuild491 nowLIVE/FROZEN,exec99533; node/target/geometry/height/budgets unchanged.

2026-11-16 night update: N9section6.6 build529100.145415s/6actions; corrected G3 fixture546.644725s incl saved-state load +30s observer +388.8s wall-stage budget,457accepted splits; G2samejob751.169940s until planned extra construction boundary /759.371902s sealed, NOT collapse-only. Sampled serial GNUtimepeak3,339,386,880B is not a sum of controller peaks. A1launcher53010.138423s failed before compilation (Ninja PATH setup); compile53140.163326s failed missing defining header, generated buildgraph refreshed. Neither establishes law/census speed or an efficiency gain. Corrected build532 in progress with4workers,nice19; stage census seconds remain pending. All rawattempts/corrections in R/night_20261115/N8/A1.

2026-11-16 A1completefailure: build532470.250586s/50actions,pins1565unchanged; lawunit4.526925s/10assertions; firstEuler293measurement344.990330s,82,559edges/7,521needunchangedbutCSVbitidentityFAILED. One<1minreadonlydiagnosispoints to strictlibraryvsoldfastmath arithmeticcontext; reverted03fffc52e8, no furtherA1checks orfullrun. A2/A3+N12connectedbuild533PASSED490.230314s/52actions,pins1575unchanged/141actualhostIDsterminal; Req/Censusouter4.541926/4.519411s,21assertionseach. M6bruntimeunqualifieduntilfixtureends. Noefficiencyimprovementclaim; newN12callclockscopeexcludesroundsetup/postprocessing andincludesnestedcallwait, nottotalengine/CPUtime.

2026-11-17 CUTB2: build55190.159432s, G1serial793.680386/MPI2 13.930448/MPI4 15.946141s; strictG2stop368.626787/seal376.929529s. Prefixsampled rankHWMsum7,895,732,224B (not full job peak). G1/G2rows identical; code movement establishes no computational improvement. Evidence N4/section6_3_after_N11/B2/acceptance_summary.json.
