# FP2-B continuation 1 · build640

The archived order matches SHA256 `50c3c7a7fc8a62959a424473deadd36b13ada8ca92f27b4a7398c25d87bc8bb9`. The tracked Common, SU2_CFD and UnitTests paths match `ecaed957db`. The live solver and test driver match the build640 hashes, including the recorded source pins. The entry process audit found 378 FP2-B identities and 19 D1 identities terminal. [Order](../inputs/FP2_B_CONTINUE_1.md) · [SHA256](../inputs/FP2_B_CONTINUE_1.sha256) · [Build receipt](../build640/build_status.json)

| Build640 binary | SHA256 |
|---|---|
| SU2_CFD | `ce93af9ef95c3b5aa5295a850bb4f74f92facdb00e93e5ef8fa27cee57b48b07` |
| UnitTests/test_driver | `9319873b17e14ec9956d666798ee76e110e24ae7909b67ea424f886dff7879a4` |

## Default proof

The original build640 receipt has zero new warnings. The correction 3 fixtures and G1 were verified from their receipts, including 375 serial, 114 two-rank and 228 four-rank records. The N1 invariant includes fixture j and reconciles 253 tolerance placements to 253 accepted splits. N1, the invariant, N6 and valid N6P were reused as the hand-over gate. S1 was reused from its passing workflow and isolated restart. The completed build638 scope remains historical evidence. [Sheet fixture](../scoped/build640/sheet/fixture_status.json) · [Fixture and G1 cohort](../scoped/build640/fixtures.json) · [N1](../cases/build640/N1/measurement.json) · [N1 invariant](../cases/build640/N1Invariant/measurement.json) · [N6](../cases/build640/N6/measurement.json) · [N6P](../cases/build640/N6P/measurement.json) · [Prior S1](../G5/build640/S1/run_receipt.json) · [Build638 scope](../scoped/build638/scoped.json)

G2 and GQ passed the exact-row and scheduling comparisons in 237.768 and 656.293 s. G3 passed in 613.799 s with 1,568 accepted splits, a matching common prefix of 1,113, and equal observer-adjusted entry work. G4 passed its 1/2/4-rank cases in a 36.251 s controller. The shared merit481, saved481 and saved_current controllers passed in 121.473, 144.338 and 143.402 s. [G2](../scoped/build640/prefix/G2_comparison.json) · [GQ](../scoped/build640/prefix/GQ_comparison.json) · [Scoped proof](../scoped/build640/scoped.json) · [Shared safeguards](../scoped/build640/safeguards.json)

The initial entry through the generic correction 2 launcher omitted the correction 3 fixture pins. A temporary launcher then resolved a draft wrapper from its own directory. Both preflight attempts stopped at the immutable provenance comparison before running a check. Using the committed correction 3 provenance and Python's safe path option let the existing controllers run with the original gate values and binaries.

### G5 OFF

Both adaptation events and the isolated restart passed with both options disabled. All six on-disk SHA256 values matched the N21 W reference. Native maxima were 1,757.338 and 1,017.567 s. Minimum named timer coverage was 98.9954% and 99.5266%. Job peaks were 14,014,525,440 and 16,700,268,544 B, each equal to the sum of the process peaks. Owned proof faces were 61,906 and 58,260. [Standing-gate receipt](../G5/build640/OFF/run_receipt.json) · [N21 W payload comparison](../G5/build640/OFF/payload_identity.json)

### G5 CORE

CORE used the pinned FP1 config SHA256 `1ec6c26d49d4ec7c62103d5bd096471b5422290cd06c44b2f1073b7489f2b4c1`. Both adaptation events and the isolated restart passed, and the six on-disk SHA256 values matched the accepted FP1 ON reference. Native maxima were 1,509.433 and 371.336 s. Minimum named timer coverage was 99.0551% and 98.6725%. Job peaks were 11,576,315,904 and 11,652,763,648 B, each equal to the sum of the process peaks. Owned proof faces were 68,006 and 64,758. [Standing-gate receipt](../G5/build640/CORE/run_receipt.json) · [FP1 payload comparison](../G5/build640/CORE/payload_identity.json) · [Default proof](../G5/build640/default_proven.json)

## Acceptance

The default-proof checkpoint was pushed as `6a37f55fe7` before S2. S2 passed both adaptation events and the isolated restart, with controller times of 1,411.584 and 167.837 s. I rehashed the six files in each of S1 and S2 from disk. Both sets match the published reference for both options enabled. The surface option defaults to NO. [S2 standing-gate receipt](../G5/build640/S2/run_receipt.json) · [S2 isolated restart](../G5/build640/S2/disk_restart_full640_run1_event2/disk_restart_validation.json) · [S1 and S2 identity receipt](../G5/build640/S2/payload_identity.json) · [Accepted reference](../on_surface_reference.json)

| Mode | Event 1 native maximum (s) | Event 2 native maximum (s) | Minimum named coverage, events 1 / 2 | Event 1 time rule |
|---|---:|---:|---:|---|
| CORE | 1,509.433 | 371.336 | 99.0551% / 98.6725% | Baseline |
| S1 | 649.954 | 446.547 | 98.2433% / 98.9311% | Passed |
| S2 | 649.993 | 444.120 | 98.2632% / 98.9130% | Passed |

The event-1 limit is `1.05 × 1509.433419667 = 1584.90509065035 s`. Both S runs cleared it. Each produced 550,027 cells, 113,007 points and 80,650 owned proof faces in event 1, and 575,733 cells, 116,342 points and 79,274 owned proof faces in event 2. S2 job peaks were 12,621,164,544 and 13,060,841,472 B, equal to the respective sums of process peaks. [Acceptance comparison](../G5/build640/acceptance_summary.json)

## Where the time is

Each event uses the rank with the largest native time in the sealed summaries. Maxima are computed independently over all six ranks, and the maximum sum is the maximum of rank sums. Core surface clocks include their nested sampling and placement work. Core quality includes V rounds when enabled. Surface-state build covers gathering, construction and checking of the state. Ordinary pass setup, exchange and write-back remain in Rest. Engine phases use nonoverlapping elapsed stage spans. The remesh ledger’s final-proof exclusive seconds are moved from the terminal engine phase to the separate proof row. Transfer is the remesh ledger’s transfer phase. Replacement and mesh output have distinct timer roots outside the native total. Zero marks a part that did not run, and — marks unavailable evidence.

### Event 1

| Part | S1 rank 5 | S1 max over ranks | S2 rank 1 | S2 max over ranks | CORE rank 5 | CORE max over ranks |
| --- | --- | --- | --- | --- | --- | --- |
| Core surface length step | 0.023 | 1.309 | 0.026 | 1.326 | 0.000 | 0.000 |
| Core surface distance step | 2.207 | 3.988 | 2.906 | 3.978 | 0.000 | 0.000 |
| Core length work | 2.846 | 20.737 | 5.076 | 20.884 | 2.822 | 13.973 |
| Core quality, including V rounds when enabled | 2.397 | 9.967 | 7.453 | 9.878 | 0.280 | 1.495 |
| Core surface-state build | 32.980 | 32.980 | 32.958 | 32.995 | 0.000 | 0.000 |
| Engine construction | 210.476 | 210.478 | 210.527 | 210.527 | 501.144 | 501.145 |
| Engine quality | 235.301 | 235.302 | 235.038 | 235.040 | 852.831 | 852.836 |
| Engine geometry repair 1 | 24.311 | 24.311 | 24.401 | 24.401 | 0.824 | 0.824 |
| Engine geometry repair 2 | 0.000 | 0.000 | 0.000 | 0.000 | 0.437 | 0.437 |
| Engine geometry repair 3 | 0.000 | 0.000 | 0.000 | 0.000 | 1.941 | 1.941 |
| Engine geometry repair 4 | 0.000 | 0.000 | 0.000 | 0.000 | 0.732 | 0.732 |
| Engine geometry repair 5 | 0.000 | 0.000 | 0.000 | 0.000 | 0.512 | 0.513 |
| Engine size repair 1 | 0.000 | 0.000 | 0.000 | 0.000 | 0.059 | 0.059 |
| Engine final proof | 33.194 | 33.194 | 33.155 | 33.155 | 56.337 | 56.337 |
| Transfer within the remesh root | 31.877 | 31.880 | 31.849 | 31.852 | 30.569 | 30.569 |
| Sum | 575.612 | 597.719 | 583.389 | 597.946 | 1448.489 | 1459.497 |
| Rest | 74.343 | 74.343 | 66.605 | 74.667 | 60.944 | 60.944 |
| Native total | 649.954 | 649.954 | 649.993 | 649.993 | 1509.433 | 1509.433 |


### Event 2

| Part | S1 rank 1 | S1 max over ranks | S2 rank 1 | S2 max over ranks | CORE rank 3 | CORE max over ranks |
| --- | --- | --- | --- | --- | --- | --- |
| Core surface length step | 0.292 | 0.292 | 0.287 | 0.287 | 0.000 | 0.000 |
| Core surface distance step | 1.672 | 2.768 | 1.657 | 2.769 | 0.000 | 0.000 |
| Core length work | 10.529 | 10.529 | 10.408 | 10.408 | 7.189 | 8.760 |
| Core quality, including V rounds when enabled | 1.427 | 1.935 | 1.416 | 1.939 | 0.274 | 0.485 |
| Core surface-state build | 32.650 | 32.724 | 32.724 | 32.733 | 0.000 | 0.000 |
| Engine construction | 212.901 | 212.902 | 212.000 | 212.000 | 200.824 | 200.825 |
| Engine quality | 112.428 | 112.428 | 112.189 | 112.190 | 73.242 | 73.245 |
| Engine geometry repair 1 | 0.000 | 0.000 | 0.000 | 0.000 | 0.000 | 0.000 |
| Engine size repair 1 | 0.000 | 0.000 | 0.000 | 0.000 | 16.346 | 16.347 |
| Engine final proof | 24.213 | 24.213 | 24.187 | 24.187 | 27.880 | 27.880 |
| Transfer within the remesh root | 32.808 | 32.811 | 32.040 | 32.040 | 30.400 | 30.400 |
| Sum | 428.920 | 429.773 | 426.909 | 428.076 | 356.156 | 357.726 |
| Rest | 17.627 | 22.015 | 17.211 | 21.585 | 15.180 | 15.715 |
| Native total | 446.547 | 446.547 | 444.120 | 444.120 | 371.336 | 371.336 |


## FP2-A Part C workflow comparison

Each row uses the final remesh timer ledger for that rank. Partial evidence lists only observed values. Times are seconds and memory is bytes.

R4 marks exact engine surface length splits, geometry splits, and first above-tolerance face counts unavailable. The engine rows give all-lane length splits, surface-lane rounds, mixed-edit wall-lane commits, and geometry-invalid cells at the first emitted complete geometry census.

- **CORE** sealed.
- **S1** sealed.
- **S2** sealed. Event 1 time rule passed at 649.99327815 s with a 1584.90509065035 s limit.

### Event 1

| Case | Native max | Cells | Points | Surface faces | Min coverage % | Job peak | Builds observed |
| --- | --- | --- | --- | --- | --- | --- | --- |
| CORE | 1509.433 | 554969 | 111697 | 68006 | 99.0551 (6 ranks) | 11576315904 | 0 |
| S1 | 649.954 | 550027 | 113007 | 80650 | 98.2433 (6 ranks) | 12618448896 | 1 |
| S2 | 649.993 | 550027 | 113007 | 80650 | 98.2632 (6 ranks) | 12621164544 | 1 |

| Case | Rank | Native | preparation | construction | quality | final_proof | transfer | topology | geometry_bound | mobility_policy | communication | Process peak | Builds observed |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| CORE | 0 | 1509.345 | 64.160 | 487.781 | 859.561 | 56.337 | 30.564 | 11.928 | 136.428 | 0.000 | 266.840 | 2042990592 | 0 |
| CORE | 1 | 1509.368 | 64.125 | 487.796 | 859.556 | 56.337 | 30.566 | 12.276 | 76.863 | 0.000 | 337.930 | 1906511872 | 0 |
| CORE | 2 | 1509.344 | 63.984 | 487.832 | 859.561 | 56.337 | 30.568 | 17.881 | 101.584 | 0.000 | 296.929 | 1924472832 | 0 |
| CORE | 3 | 1509.340 | 63.767 | 487.905 | 859.563 | 56.337 | 30.567 | 24.173 | 118.665 | 0.000 | 266.628 | 2118291456 | 0 |
| CORE | 4 | 1509.351 | 64.142 | 487.787 | 859.560 | 56.337 | 30.568 | 11.639 | 97.074 | 0.000 | 317.543 | 1930936320 | 0 |
| CORE | 5 | 1509.433 | 64.119 | 487.759 | 859.558 | 56.337 | 30.569 | 7.652 | 130.834 | 0.000 | 294.709 | 1653112832 | 0 |
| S1 | 0 | 649.954 | 70.936 | 242.082 | 260.119 | 33.194 | 31.878 | 39.834 | 23.186 | 16.777 | 142.458 | 1873203200 | 1 |
| S1 | 1 | 649.954 | 70.964 | 242.106 | 260.118 | 33.194 | 31.878 | 36.694 | 18.833 | 16.780 | 150.566 | 1784995840 | 1 |
| S1 | 2 | 649.954 | 70.796 | 242.077 | 260.120 | 33.194 | 31.878 | 39.405 | 9.747 | 16.828 | 147.750 | 2166013952 | 1 |
| S1 | 3 | 649.954 | 70.444 | 242.084 | 260.121 | 33.194 | 31.880 | 63.535 | 10.540 | 16.759 | 100.051 | 3146559488 | 1 |
| S1 | 4 | 649.952 | 70.971 | 242.082 | 260.120 | 33.194 | 31.877 | 29.510 | 13.303 | 16.794 | 165.065 | 1795010560 | 1 |
| S1 | 5 | 649.954 | 70.977 | 242.058 | 260.121 | 33.194 | 31.877 | 27.655 | 28.668 | 16.826 | 152.118 | 1852665856 | 1 |
| S2 | 0 | 649.927 | 70.899 | 242.539 | 259.960 | 33.155 | 31.852 | 39.674 | 23.099 | 16.775 | 143.272 | 1873289216 | 1 |
| S2 | 1 | 649.993 | 70.847 | 242.566 | 259.958 | 33.155 | 31.849 | 36.831 | 18.845 | 16.794 | 150.752 | 1759989760 | 1 |
| S2 | 2 | 649.898 | 70.784 | 242.561 | 259.960 | 33.155 | 31.851 | 39.380 | 9.760 | 16.798 | 148.236 | 2191917056 | 1 |
| S2 | 3 | 649.925 | 70.394 | 242.542 | 259.961 | 33.155 | 31.850 | 63.843 | 10.547 | 16.779 | 99.794 | 3147374592 | 1 |
| S2 | 4 | 649.896 | 70.957 | 242.540 | 259.961 | 33.155 | 31.849 | 29.532 | 13.280 | 16.790 | 165.545 | 1795440640 | 1 |
| S2 | 5 | 649.992 | 70.878 | 242.518 | 259.960 | 33.155 | 31.849 | 27.590 | 28.625 | 16.791 | 152.656 | 1853153280 | 1 |

| Case | Rank | Length splits by engine exit | Surface rounds by engine exit | Construction wall commits | Quality wall commits | First census stage emitted | Geometry-invalid cells |
| --- | --- | --- | --- | --- | --- | --- | --- |
| CORE | 0 | 7103 | 3236 | 3991 | 2001 | length_collapse_exit | 461 |
| CORE | 1 | 4183 | 3236 | 2517 | 3747 | length_collapse_exit | 411 |
| CORE | 2 | 5198 | 3236 | 3104 | 2160 | length_collapse_exit | 744 |
| CORE | 3 | 6284 | 3236 | 3787 | 2610 | length_collapse_exit | 601 |
| CORE | 4 | 4279 | 3236 | 2529 | 4748 | length_collapse_exit | 339 |
| CORE | 5 | 1681 | 3236 | 2043 | 4478 | length_collapse_exit | 416 |
| S1 | 0 | 22 | 143 | 711 | 1339 | length_collapse_exit | 24 |
| S1 | 1 | 36 | 143 | 607 | 1993 | length_collapse_exit | 10 |
| S1 | 2 | 256 | 143 | 571 | 799 | length_collapse_exit | 0 |
| S1 | 3 | 519 | 143 | 722 | 620 | length_collapse_exit | 17 |
| S1 | 4 | 39 | 143 | 517 | 424 | length_collapse_exit | 32 |
| S1 | 5 | 157 | 143 | 713 | 871 | length_collapse_exit | 217 |
| S2 | 0 | 22 | 143 | 711 | 1339 | length_collapse_exit | 24 |
| S2 | 1 | 36 | 143 | 607 | 1993 | length_collapse_exit | 10 |
| S2 | 2 | 256 | 143 | 571 | 799 | length_collapse_exit | 0 |
| S2 | 3 | 519 | 143 | 722 | 620 | length_collapse_exit | 17 |
| S2 | 4 | 39 | 143 | 517 | 424 | length_collapse_exit | 32 |
| S2 | 5 | 157 | 143 | 713 | 871 | length_collapse_exit | 217 |

#### Surface rows

```text
S1 NATIVE3D_LOCAL_SURFACE rank=0 stage=0 pass=1 applicable=1 faces_in=9423 faces_out=15581 points_in=4894 points_out=7973 splits_length=0 splits_distance=3079 splits_feature=13 sweeps_length=0 sweeps_distance=9 left_long=0 left_border=59 left_distance=377 left_all_feature=0 left_refused=0 left_limit=0 left_short=0 left_neighbor_locked=318 waits_neighbor=1658 path_splits=277 path_max=7 left_long_neighbor=0 waits_length=0 star_max=198 star_gt100=22 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=0 refused_topology=0 refused_memory=0 refused_entity=0 refused_near=0 walk_tol8=82 walk_far=0 walk_no_start=0 walk_exception=0 chord_unknown=0 samples=86956 samples_unconverged=1812 outward_max=9.311180236387248e-07 seconds_length=0.012759650999996097 seconds_distance=2.319466177999999 seconds_samples=0.7501166720008996 seconds_place=0.1016517920001121 seconds_entity=1.2089632729998669 v_rounds=4 v_limit=0 v_collapses=43 v_splits=354 v_changes=c34s315q20,c8s34q11,c1s5q4,c0s0q1 bad_before_v=3114 seconds_v=2.4121471610000071
S1 NATIVE3D_LOCAL_SURFACE rank=0 stage=0 pass=2 applicable=1 faces_in=16493 faces_out=19827 points_in=8512 points_out=10179 splits_length=0 splits_distance=1667 splits_feature=3 sweeps_length=0 sweeps_distance=8 left_long=0 left_border=93 left_distance=272 left_all_feature=0 left_refused=26 left_limit=0 left_short=0 left_neighbor_locked=153 waits_neighbor=1720 path_splits=140 path_max=7 left_long_neighbor=0 waits_length=0 star_max=144 star_gt100=31 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=13 refused_topology=0 refused_memory=0 refused_entity=0 refused_near=0 walk_tol8=50 walk_far=0 walk_no_start=0 walk_exception=0 chord_unknown=0 samples=92644 samples_unconverged=1672 outward_max=6.0167270963416384e-06 seconds_length=0.014343632000006323 seconds_distance=1.6683802959999952 seconds_samples=0.76182327599863697 seconds_place=0.064563750000019127 seconds_entity=0.60358959400045364 v_rounds=5 v_limit=0 v_collapses=13 v_splits=218 v_changes=c7s192q20,c5s21q16,c1s4q5,c0s1q2,c0s0q1 bad_before_v=2210 seconds_v=2.1971049809999954
S1 NATIVE3D_LOCAL_SURFACE_STATE rank=0 stage=0 splits_all=18650 faces=86254 points=43129 traces=753 revision=2 seconds_exchange=0.038296160000015789 seconds_gather=0.19591875099999356 seconds_build=32.702366558999998 seconds_check=0.038954908999983218 modeled_peak=2004863780 rss_before=544067584 rss_after=544133120
S1 NATIVE3D_LOCAL_SURFACE rank=1 stage=0 pass=1 applicable=1 faces_in=9031 faces_out=14851 points_in=4710 points_out=7620 splits_length=0 splits_distance=2910 splits_feature=0 sweeps_length=0 sweeps_distance=8 left_long=0 left_border=18 left_distance=84 left_all_feature=0 left_refused=0 left_limit=0 left_short=0 left_neighbor_locked=66 waits_neighbor=1462 path_splits=328 path_max=5 left_long_neighbor=0 waits_length=0 star_max=138 star_gt100=18 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=0 refused_topology=0 refused_memory=0 refused_entity=0 refused_near=0 walk_tol8=51 walk_far=0 walk_no_start=0 walk_exception=0 chord_unknown=0 samples=82684 samples_unconverged=1903 outward_max=1.4943970271539778e-11 seconds_length=0.010830134000002545 seconds_distance=2.1450827580000009 seconds_samples=0.687891690998228 seconds_place=0.087689943000114567 seconds_entity=1.1635823860004848 v_rounds=5 v_limit=0 v_collapses=52 v_splits=328 v_changes=c32s288q16,c16s31q15,c4s7q8,c0s2q2,c0s0q1 bad_before_v=3464 seconds_v=2.7887371380000019
S1 NATIVE3D_LOCAL_SURFACE rank=1 stage=0 pass=2 applicable=1 faces_in=15204 faces_out=15854 points_in=7837 points_out=8162 splits_length=0 splits_distance=325 splits_feature=0 sweeps_length=0 sweeps_distance=7 left_long=0 left_border=26 left_distance=60 left_all_feature=0 left_refused=2 left_limit=0 left_short=0 left_neighbor_locked=32 waits_neighbor=363 path_splits=33 path_max=6 left_long_neighbor=0 waits_length=0 star_max=154 star_gt100=14 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=1 refused_topology=0 refused_memory=0 refused_entity=0 refused_near=0 walk_tol8=9 walk_far=0 walk_no_start=0 walk_exception=0 chord_unknown=0 samples=66016 samples_unconverged=1512 outward_max=1.5032035086537339e-11 seconds_length=0.013753460999993194 seconds_distance=0.74663302700000145 seconds_samples=0.45428889200054812 seconds_place=0.011425698999843803 seconds_entity=0.12221844800012605 v_rounds=4 v_limit=0 v_collapses=10 v_splits=42 v_changes=c10s35q8,c0s6q5,c0s1q2,c0s0q1 bad_before_v=1289 seconds_v=1.1147646230000134
S1 NATIVE3D_LOCAL_SURFACE_STATE rank=1 stage=0 splits_all=18650 faces=86254 points=43129 traces=753 revision=2 seconds_exchange=0.038272114999998053 seconds_gather=0.19592366700000241 seconds_build=32.644840463000008 seconds_check=0.038956815999995342 modeled_peak=2004429508 rss_before=552497152 rss_after=552497152
S1 NATIVE3D_LOCAL_SURFACE rank=2 stage=0 pass=1 applicable=1 faces_in=6040 faces_out=10372 points_in=3195 points_out=5361 splits_length=321 splits_distance=1845 splits_feature=0 sweeps_length=5 sweeps_distance=9 left_long=27 left_border=60 left_distance=148 left_all_feature=0 left_refused=0 left_limit=0 left_short=0 left_neighbor_locked=101 waits_neighbor=1134 path_splits=55 path_max=21 left_long_neighbor=14 waits_length=36 star_max=128 star_gt100=2 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=0 refused_topology=0 refused_memory=0 refused_entity=0 refused_near=0 walk_tol8=1 walk_far=0 walk_no_start=0 walk_exception=0 chord_unknown=0 samples=56248 samples_unconverged=1414 outward_max=1.0624059606946464e-05 seconds_length=0.24256632300000547 seconds_distance=1.7216890330000041 seconds_samples=0.6173353030001465 seconds_place=0.060160692999986054 seconds_entity=1.0982282509999095 v_rounds=3 v_limit=0 v_collapses=8 v_splits=15 v_changes=c7s14q8,c1s1q11,c0s0q1 bad_before_v=242 seconds_v=0.33267543099999841
S1 NATIVE3D_LOCAL_SURFACE rank=2 stage=0 pass=2 applicable=1 faces_in=11228 faces_out=12230 points_in=5855 points_out=6356 splits_length=52 splits_distance=449 splits_feature=0 sweeps_length=5 sweeps_distance=23 left_long=3 left_border=25 left_distance=39 left_all_feature=0 left_refused=2 left_limit=0 left_short=0 left_neighbor_locked=15 waits_neighbor=437 path_splits=29 path_max=21 left_long_neighbor=0 waits_length=17 star_max=128 star_gt100=2 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=1 refused_topology=0 refused_memory=0 refused_entity=0 refused_near=0 walk_tol8=0 walk_far=0 walk_no_start=0 walk_exception=0 chord_unknown=0 samples=52512 samples_unconverged=1517 outward_max=1.906865257206128e-05 seconds_length=0.04418062800000655 seconds_distance=1.0681991560000057 seconds_samples=0.55739154800146196 seconds_place=0.019389762000102451 seconds_entity=0.2202833760000118 v_rounds=2 v_limit=0 v_collapses=2 v_splits=7 v_changes=c2s7q8,c0s0q1 bad_before_v=160 seconds_v=0.18808582099998716
S1 NATIVE3D_LOCAL_SURFACE_STATE rank=2 stage=0 splits_all=18650 faces=86254 points=43129 traces=753 revision=2 seconds_exchange=0.038262437000000205 seconds_gather=0.19591907100000583 seconds_build=32.744954838000012 seconds_check=0.038958438999998179 modeled_peak=2598624692 rss_before=536686592 rss_after=537231360
S1 NATIVE3D_LOCAL_SURFACE rank=3 stage=0 pass=1 applicable=1 faces_in=6694 faces_out=15830 points_in=3448 points_out=8016 splits_length=1929 splits_distance=2639 splits_feature=35 sweeps_length=9 sweeps_distance=8 left_long=40 left_border=28 left_distance=89 left_all_feature=0 left_refused=0 left_limit=0 left_short=0 left_neighbor_locked=74 waits_neighbor=1096 path_splits=149 path_max=6 left_long_neighbor=27 waits_length=598 star_max=122 star_gt100=13 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=0 refused_topology=0 refused_memory=0 refused_entity=0 refused_near=0 walk_tol8=28 walk_far=0 walk_no_start=0 walk_exception=0 chord_unknown=0 samples=84432 samples_unconverged=1620 outward_max=6.0313230428769143e-06 seconds_length=1.2972871479999952 seconds_distance=2.1620772560000034 seconds_samples=0.74495625399833898 seconds_place=0.15505592200054252 seconds_entity=2.1980241189999958 v_rounds=4 v_limit=0 v_collapses=6 v_splits=150 v_changes=c5s125q20,c1s23q8,c0s2q3,c0s0q1 bad_before_v=1079 seconds_v=1.5001481350000034
S1 NATIVE3D_LOCAL_SURFACE rank=3 stage=0 pass=2 applicable=1 faces_in=15504 faces_out=15800 points_in=7903 points_out=8051 splits_length=0 splits_distance=148 splits_feature=0 sweeps_length=1 sweeps_distance=7 left_long=5 left_border=15 left_distance=26 left_all_feature=0 left_refused=0 left_limit=0 left_short=0 left_neighbor_locked=14 waits_neighbor=161 path_splits=4 path_max=6 left_long_neighbor=2 waits_length=0 star_max=122 star_gt100=3 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=0 refused_topology=0 refused_memory=0 refused_entity=0 refused_near=0 walk_tol8=0 walk_far=0 walk_no_start=0 walk_exception=0 chord_unknown=0 samples=64384 samples_unconverged=1425 outward_max=0 seconds_length=0.012083286999995835 seconds_distance=0.6917376189999942 seconds_samples=0.45680873700021607 seconds_place=0.0057863060001039912 seconds_entity=0.053033309999918288 v_rounds=4 v_limit=0 v_collapses=0 v_splits=15 v_changes=c0s11q20,c0s3q11,c0s1q3,c0s0q1 bad_before_v=435 seconds_v=1.1190219850000034
S1 NATIVE3D_LOCAL_SURFACE_STATE rank=3 stage=0 splits_all=18650 faces=86254 points=43129 traces=753 revision=2 seconds_exchange=0.038343752999992375 seconds_gather=0.19592192100000716 seconds_build=32.558971275000005 seconds_check=0.038956920999993372 modeled_peak=4260274524 rss_before=551534592 rss_after=551600128
S1 NATIVE3D_LOCAL_SURFACE rank=4 stage=0 pass=1 applicable=1 faces_in=9634 faces_out=11688 points_in=4969 points_out=5996 splits_length=0 splits_distance=1027 splits_feature=0 sweeps_length=0 sweeps_distance=23 left_long=0 left_border=53 left_distance=183 left_all_feature=0 left_refused=0 left_limit=0 left_short=0 left_neighbor_locked=130 waits_neighbor=698 path_splits=49 path_max=20 left_long_neighbor=0 waits_length=0 star_max=98 star_gt100=0 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=0 refused_topology=0 refused_memory=0 refused_entity=0 refused_near=0 walk_tol8=0 walk_far=0 walk_no_start=0 walk_exception=0 chord_unknown=0 samples=54968 samples_unconverged=1371 outward_max=2.1320851362276744e-05 seconds_length=0.012591284999999175 seconds_distance=1.276527932999997 seconds_samples=0.41611287000134212 seconds_place=0.028742091999930608 seconds_entity=0.46752559299962115 v_rounds=5 v_limit=0 v_collapses=54 v_splits=51 v_changes=c44s47q10,c7s4q5,c2s0q4,c1s0q1,c0s0q1 bad_before_v=1312 seconds_v=1.047973486999993
S1 NATIVE3D_LOCAL_SURFACE rank=4 stage=0 pass=2 applicable=1 faces_in=10846 faces_out=10846 points_in=5622 points_out=5622 splits_length=0 splits_distance=0 splits_feature=0 sweeps_length=0 sweeps_distance=1 left_long=0 left_border=22 left_distance=48 left_all_feature=0 left_refused=0 left_limit=0 left_short=0 left_neighbor_locked=26 waits_neighbor=0 path_splits=0 path_max=4 left_long_neighbor=0 waits_length=0 star_max=112 star_gt100=1 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=0 refused_topology=0 refused_memory=0 refused_entity=0 refused_near=0 walk_tol8=0 walk_far=0 walk_no_start=0 walk_exception=0 chord_unknown=0 samples=43384 samples_unconverged=1309 outward_max=0 seconds_length=0.01005564500000844 seconds_distance=0.30387630799999954 seconds_samples=0.26738900999910697 seconds_place=0 seconds_entity=0 v_rounds=3 v_limit=0 v_collapses=3 v_splits=0 v_changes=c2s0q4,c1s0q1,c0s0q1 bad_before_v=654 seconds_v=0.35971661400000698
S1 NATIVE3D_LOCAL_SURFACE_STATE rank=4 stage=0 splits_all=18650 faces=86254 points=43129 traces=753 revision=2 seconds_exchange=0.038260053000001903 seconds_gather=0.19591958200000192 seconds_build=32.658267359000007 seconds_check=0.03895884599998567 modeled_peak=2002049608 rss_before=537509888 rss_after=538279936
S1 NATIVE3D_LOCAL_SURFACE rank=5 stage=0 pass=1 applicable=1 faces_in=8132 faces_out=12650 points_in=4257 points_out=6516 splits_length=0 splits_distance=2259 splits_feature=28 sweeps_length=0 sweeps_distance=12 left_long=0 left_border=66 left_distance=393 left_all_feature=0 left_refused=0 left_limit=0 left_short=0 left_neighbor_locked=327 waits_neighbor=1151 path_splits=216 path_max=7 left_long_neighbor=0 waits_length=0 star_max=134 star_gt100=10 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=0 refused_topology=0 refused_memory=0 refused_entity=0 refused_near=0 walk_tol8=41 walk_far=0 walk_no_start=0 walk_exception=0 chord_unknown=0 samples=68672 samples_unconverged=1396 outward_max=6.0167270877548275e-06 seconds_length=0.010757853000001205 seconds_distance=1.8470892799999916 seconds_samples=0.58198082300087606 seconds_place=0.080929507999869088 seconds_entity=0.92018745400011426 v_rounds=5 v_limit=0 v_collapses=20 v_splits=185 v_changes=c14s170q11,c3s13q8,c2s2q3,c1s0q4,c0s0q1 bad_before_v=1641 seconds_v=1.0318739079999943
S1 NATIVE3D_LOCAL_SURFACE rank=5 stage=0 pass=2 applicable=1 faces_in=11697 faces_out=11697 points_in=6102 points_out=6102 splits_length=0 splits_distance=0 splits_feature=0 sweeps_length=0 sweeps_distance=1 left_long=0 left_border=65 left_distance=158 left_all_feature=0 left_refused=0 left_limit=0 left_short=0 left_neighbor_locked=93 waits_neighbor=0 path_splits=0 path_max=5 left_long_neighbor=0 waits_length=0 star_max=108 star_gt100=4 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=0 refused_topology=0 refused_memory=0 refused_entity=0 refused_near=0 walk_tol8=0 walk_far=0 walk_no_start=0 walk_exception=0 chord_unknown=0 samples=46788 samples_unconverged=1054 outward_max=0 seconds_length=0.012309389999998643 seconds_distance=0.35954980400001091 seconds_samples=0.31657973899994829 seconds_place=0 seconds_entity=0 v_rounds=1 v_limit=0 v_collapses=0 v_splits=0 v_changes=c0s0q1 bad_before_v=304 seconds_v=0.060345433999998477
S1 NATIVE3D_LOCAL_SURFACE_STATE rank=5 stage=0 splits_all=18650 faces=86254 points=43129 traces=753 revision=2 seconds_exchange=0.038284997000005205 seconds_gather=0.19591732799999306 seconds_build=32.745598181999981 seconds_check=0.038960105999990446 modeled_peak=1995406684 rss_before=457555968 rss_after=457555968
S2 NATIVE3D_LOCAL_SURFACE rank=0 stage=0 pass=1 applicable=1 faces_in=9423 faces_out=15581 points_in=4894 points_out=7973 splits_length=0 splits_distance=3079 splits_feature=13 sweeps_length=0 sweeps_distance=9 left_long=0 left_border=59 left_distance=377 left_all_feature=0 left_refused=0 left_limit=0 left_short=0 left_neighbor_locked=318 waits_neighbor=1658 path_splits=277 path_max=7 left_long_neighbor=0 waits_length=0 star_max=198 star_gt100=22 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=0 refused_topology=0 refused_memory=0 refused_entity=0 refused_near=0 walk_tol8=82 walk_far=0 walk_no_start=0 walk_exception=0 chord_unknown=0 samples=86956 samples_unconverged=1812 outward_max=9.311180236387248e-07 seconds_length=0.012725683000013532 seconds_distance=2.3172060619999968 seconds_samples=0.7499448760020897 seconds_place=0.10145844800014459 seconds_entity=1.2085264030000218 v_rounds=4 v_limit=0 v_collapses=43 v_splits=354 v_changes=c34s315q20,c8s34q11,c1s5q4,c0s0q1 bad_before_v=3114 seconds_v=2.3783225880000032
S2 NATIVE3D_LOCAL_SURFACE rank=0 stage=0 pass=2 applicable=1 faces_in=16493 faces_out=19827 points_in=8512 points_out=10179 splits_length=0 splits_distance=1667 splits_feature=3 sweeps_length=0 sweeps_distance=8 left_long=0 left_border=93 left_distance=272 left_all_feature=0 left_refused=26 left_limit=0 left_short=0 left_neighbor_locked=153 waits_neighbor=1720 path_splits=140 path_max=7 left_long_neighbor=0 waits_length=0 star_max=144 star_gt100=31 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=13 refused_topology=0 refused_memory=0 refused_entity=0 refused_near=0 walk_tol8=50 walk_far=0 walk_no_start=0 walk_exception=0 chord_unknown=0 samples=92644 samples_unconverged=1672 outward_max=6.0167270963416384e-06 seconds_length=0.01545453700001076 seconds_distance=1.6606131039999923 seconds_samples=0.75601926199915681 seconds_place=0.063811423000032619 seconds_entity=0.59993751300017095 v_rounds=5 v_limit=0 v_collapses=13 v_splits=218 v_changes=c7s192q20,c5s21q16,c1s4q5,c0s1q2,c0s0q1 bad_before_v=2210 seconds_v=2.1847704269999895
S2 NATIVE3D_LOCAL_SURFACE_STATE rank=0 stage=0 splits_all=18650 faces=86254 points=43129 traces=753 revision=2 seconds_exchange=0.038851657999998679 seconds_gather=0.19654365200000257 seconds_build=32.657987406999993 seconds_check=0.040239289999988159 modeled_peak=2004863780 rss_before=543961088 rss_after=544026624
S2 NATIVE3D_LOCAL_SURFACE rank=1 stage=0 pass=1 applicable=1 faces_in=9031 faces_out=14851 points_in=4710 points_out=7620 splits_length=0 splits_distance=2910 splits_feature=0 sweeps_length=0 sweeps_distance=8 left_long=0 left_border=18 left_distance=84 left_all_feature=0 left_refused=0 left_limit=0 left_short=0 left_neighbor_locked=66 waits_neighbor=1462 path_splits=328 path_max=5 left_long_neighbor=0 waits_length=0 star_max=138 star_gt100=18 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=0 refused_topology=0 refused_memory=0 refused_entity=0 refused_near=0 walk_tol8=51 walk_far=0 walk_no_start=0 walk_exception=0 chord_unknown=0 samples=82684 samples_unconverged=1903 outward_max=1.4943970271539778e-11 seconds_length=0.0117465339999967 seconds_distance=2.159296972000007 seconds_samples=0.68989839799995423 seconds_place=0.087959477999987712 seconds_entity=1.1710422480002762 v_rounds=5 v_limit=0 v_collapses=52 v_splits=328 v_changes=c32s288q16,c16s31q15,c4s7q8,c0s2q2,c0s0q1 bad_before_v=3464 seconds_v=2.8006879530000077
S2 NATIVE3D_LOCAL_SURFACE rank=1 stage=0 pass=2 applicable=1 faces_in=15204 faces_out=15854 points_in=7837 points_out=8162 splits_length=0 splits_distance=325 splits_feature=0 sweeps_length=0 sweeps_distance=7 left_long=0 left_border=26 left_distance=60 left_all_feature=0 left_refused=2 left_limit=0 left_short=0 left_neighbor_locked=32 waits_neighbor=363 path_splits=33 path_max=6 left_long_neighbor=0 waits_length=0 star_max=154 star_gt100=14 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=1 refused_topology=0 refused_memory=0 refused_entity=0 refused_near=0 walk_tol8=9 walk_far=0 walk_no_start=0 walk_exception=0 chord_unknown=0 samples=66016 samples_unconverged=1512 outward_max=1.5032035086537339e-11 seconds_length=0.014533842999995272 seconds_distance=0.74639951000000337 seconds_samples=0.45323367299995709 seconds_place=0.011473892000111618 seconds_entity=0.12188271799999484 v_rounds=4 v_limit=0 v_collapses=10 v_splits=42 v_changes=c10s35q8,c0s6q5,c0s1q2,c0s0q1 bad_before_v=1289 seconds_v=1.1172864349999969
S2 NATIVE3D_LOCAL_SURFACE_STATE rank=1 stage=0 splits_all=18650 faces=86254 points=43129 traces=753 revision=2 seconds_exchange=0.038824511999990818 seconds_gather=0.19654580099999919 seconds_build=32.721334928999994 seconds_check=0.040242238000018915 modeled_peak=2004429508 rss_before=552706048 rss_after=552771584
S2 NATIVE3D_LOCAL_SURFACE rank=2 stage=0 pass=1 applicable=1 faces_in=6040 faces_out=10372 points_in=3195 points_out=5361 splits_length=321 splits_distance=1845 splits_feature=0 sweeps_length=5 sweeps_distance=9 left_long=27 left_border=60 left_distance=148 left_all_feature=0 left_refused=0 left_limit=0 left_short=0 left_neighbor_locked=101 waits_neighbor=1134 path_splits=55 path_max=21 left_long_neighbor=14 waits_length=36 star_max=128 star_gt100=2 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=0 refused_topology=0 refused_memory=0 refused_entity=0 refused_near=0 walk_tol8=1 walk_far=0 walk_no_start=0 walk_exception=0 chord_unknown=0 samples=56248 samples_unconverged=1414 outward_max=1.0624059606946464e-05 seconds_length=0.24225346200000786 seconds_distance=1.7235836879999908 seconds_samples=0.61559008900007939 seconds_place=0.059653484999998341 seconds_entity=1.0982372610003779 v_rounds=3 v_limit=0 v_collapses=8 v_splits=15 v_changes=c7s14q8,c1s1q11,c0s0q1 bad_before_v=242 seconds_v=0.33241013900000382
S2 NATIVE3D_LOCAL_SURFACE rank=2 stage=0 pass=2 applicable=1 faces_in=11228 faces_out=12230 points_in=5855 points_out=6356 splits_length=52 splits_distance=449 splits_feature=0 sweeps_length=5 sweeps_distance=23 left_long=3 left_border=25 left_distance=39 left_all_feature=0 left_refused=2 left_limit=0 left_short=0 left_neighbor_locked=15 waits_neighbor=437 path_splits=29 path_max=21 left_long_neighbor=0 waits_length=17 star_max=128 star_gt100=2 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=1 refused_topology=0 refused_memory=0 refused_entity=0 refused_near=0 walk_tol8=0 walk_far=0 walk_no_start=0 walk_exception=0 chord_unknown=0 samples=52512 samples_unconverged=1517 outward_max=1.906865257206128e-05 seconds_length=0.045698887999989779 seconds_distance=1.0935342419999898 seconds_samples=0.55232660300012526 seconds_place=0.019071623000101567 seconds_entity=0.21861756199997728 v_rounds=2 v_limit=0 v_collapses=2 v_splits=7 v_changes=c2s7q8,c0s0q1 bad_before_v=160 seconds_v=0.1877903740000022
S2 NATIVE3D_LOCAL_SURFACE_STATE rank=2 stage=0 splits_all=18650 faces=86254 points=43129 traces=753 revision=2 seconds_exchange=0.038820469999990337 seconds_gather=0.19654500499999017 seconds_build=32.758067029000003 seconds_check=0.040245233999996799 modeled_peak=2598624692 rss_before=536952832 rss_after=537018368
S2 NATIVE3D_LOCAL_SURFACE rank=3 stage=0 pass=1 applicable=1 faces_in=6694 faces_out=15830 points_in=3448 points_out=8016 splits_length=1929 splits_distance=2639 splits_feature=35 sweeps_length=9 sweeps_distance=8 left_long=40 left_border=28 left_distance=89 left_all_feature=0 left_refused=0 left_limit=0 left_short=0 left_neighbor_locked=74 waits_neighbor=1096 path_splits=149 path_max=6 left_long_neighbor=27 waits_length=598 star_max=122 star_gt100=13 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=0 refused_topology=0 refused_memory=0 refused_entity=0 refused_near=0 walk_tol8=28 walk_far=0 walk_no_start=0 walk_exception=0 chord_unknown=0 samples=84432 samples_unconverged=1620 outward_max=6.0313230428769143e-06 seconds_length=1.3130500399999931 seconds_distance=2.1757155070000067 seconds_samples=0.74838590199922805 seconds_place=0.15815475100015419 seconds_entity=2.2166522410005456 v_rounds=4 v_limit=0 v_collapses=6 v_splits=150 v_changes=c5s125q20,c1s23q8,c0s2q3,c0s0q1 bad_before_v=1079 seconds_v=1.5162420559999958
S2 NATIVE3D_LOCAL_SURFACE rank=3 stage=0 pass=2 applicable=1 faces_in=15504 faces_out=15800 points_in=7903 points_out=8051 splits_length=0 splits_distance=148 splits_feature=0 sweeps_length=1 sweeps_distance=7 left_long=5 left_border=15 left_distance=26 left_all_feature=0 left_refused=0 left_limit=0 left_short=0 left_neighbor_locked=14 waits_neighbor=161 path_splits=4 path_max=6 left_long_neighbor=2 waits_length=0 star_max=122 star_gt100=3 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=0 refused_topology=0 refused_memory=0 refused_entity=0 refused_near=0 walk_tol8=0 walk_far=0 walk_no_start=0 walk_exception=0 chord_unknown=0 samples=64384 samples_unconverged=1425 outward_max=0 seconds_length=0.013003714000006994 seconds_distance=0.699613068000005 seconds_samples=0.45603874900005792 seconds_place=0.0058302519999386959 seconds_entity=0.052958968000083928 v_rounds=4 v_limit=0 v_collapses=0 v_splits=15 v_changes=c0s11q20,c0s3q11,c0s1q3,c0s0q1 bad_before_v=435 seconds_v=1.1328755119999983
S2 NATIVE3D_LOCAL_SURFACE_STATE rank=3 stage=0 splits_all=18650 faces=86254 points=43129 traces=753 revision=2 seconds_exchange=0.038885457999995765 seconds_gather=0.19655158399999095 seconds_build=32.741534665000003 seconds_check=0.040245369000018627 modeled_peak=4260274524 rss_before=552386560 rss_after=552452096
S2 NATIVE3D_LOCAL_SURFACE rank=4 stage=0 pass=1 applicable=1 faces_in=9634 faces_out=11688 points_in=4969 points_out=5996 splits_length=0 splits_distance=1027 splits_feature=0 sweeps_length=0 sweeps_distance=23 left_long=0 left_border=53 left_distance=183 left_all_feature=0 left_refused=0 left_limit=0 left_short=0 left_neighbor_locked=130 waits_neighbor=698 path_splits=49 path_max=20 left_long_neighbor=0 waits_length=0 star_max=98 star_gt100=0 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=0 refused_topology=0 refused_memory=0 refused_entity=0 refused_near=0 walk_tol8=0 walk_far=0 walk_no_start=0 walk_exception=0 chord_unknown=0 samples=54968 samples_unconverged=1371 outward_max=2.1320851362276744e-05 seconds_length=0.01255729800000438 seconds_distance=1.2788122620000024 seconds_samples=0.41720550000066225 seconds_place=0.028857770000342953 seconds_entity=0.46970252599990658 v_rounds=5 v_limit=0 v_collapses=54 v_splits=51 v_changes=c44s47q10,c7s4q5,c2s0q4,c1s0q1,c0s0q1 bad_before_v=1312 seconds_v=1.0426560949999981
S2 NATIVE3D_LOCAL_SURFACE rank=4 stage=0 pass=2 applicable=1 faces_in=10846 faces_out=10846 points_in=5622 points_out=5622 splits_length=0 splits_distance=0 splits_feature=0 sweeps_length=0 sweeps_distance=1 left_long=0 left_border=22 left_distance=48 left_all_feature=0 left_refused=0 left_limit=0 left_short=0 left_neighbor_locked=26 waits_neighbor=0 path_splits=0 path_max=4 left_long_neighbor=0 waits_length=0 star_max=112 star_gt100=1 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=0 refused_topology=0 refused_memory=0 refused_entity=0 refused_near=0 walk_tol8=0 walk_far=0 walk_no_start=0 walk_exception=0 chord_unknown=0 samples=43384 samples_unconverged=1309 outward_max=0 seconds_length=0.0082420899999959829 seconds_distance=0.2995458400000075 seconds_samples=0.26569663599980231 seconds_place=0 seconds_entity=0 v_rounds=3 v_limit=0 v_collapses=3 v_splits=0 v_changes=c2s0q4,c1s0q1,c0s0q1 bad_before_v=654 seconds_v=0.35683057999999335
S2 NATIVE3D_LOCAL_SURFACE_STATE rank=4 stage=0 splits_all=18650 faces=86254 points=43129 traces=753 revision=2 seconds_exchange=0.038814142999996193 seconds_gather=0.19654710599999703 seconds_build=32.689175035999995 seconds_check=0.040235382999981084 modeled_peak=2002049608 rss_before=543244288 rss_after=543309824
S2 NATIVE3D_LOCAL_SURFACE rank=5 stage=0 pass=1 applicable=1 faces_in=8132 faces_out=12650 points_in=4257 points_out=6516 splits_length=0 splits_distance=2259 splits_feature=28 sweeps_length=0 sweeps_distance=12 left_long=0 left_border=66 left_distance=393 left_all_feature=0 left_refused=0 left_limit=0 left_short=0 left_neighbor_locked=327 waits_neighbor=1151 path_splits=216 path_max=7 left_long_neighbor=0 waits_length=0 star_max=134 star_gt100=10 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=0 refused_topology=0 refused_memory=0 refused_entity=0 refused_near=0 walk_tol8=41 walk_far=0 walk_no_start=0 walk_exception=0 chord_unknown=0 samples=68672 samples_unconverged=1396 outward_max=6.0167270877548275e-06 seconds_length=0.010243240999997738 seconds_distance=1.8409429009999911 seconds_samples=0.58270317500105762 seconds_place=0.080849669000002677 seconds_entity=0.91936257600048066 v_rounds=5 v_limit=0 v_collapses=20 v_splits=185 v_changes=c14s170q11,c3s13q8,c2s2q3,c1s0q4,c0s0q1 bad_before_v=1641 seconds_v=1.0296547010000126
S2 NATIVE3D_LOCAL_SURFACE rank=5 stage=0 pass=2 applicable=1 faces_in=11697 faces_out=11697 points_in=6102 points_out=6102 splits_length=0 splits_distance=0 splits_feature=0 sweeps_length=0 sweeps_distance=1 left_long=0 left_border=65 left_distance=158 left_all_feature=0 left_refused=0 left_limit=0 left_short=0 left_neighbor_locked=93 waits_neighbor=0 path_splits=0 path_max=5 left_long_neighbor=0 waits_length=0 star_max=108 star_gt100=4 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=0 refused_topology=0 refused_memory=0 refused_entity=0 refused_near=0 walk_tol8=0 walk_far=0 walk_no_start=0 walk_exception=0 chord_unknown=0 samples=46788 samples_unconverged=1054 outward_max=0 seconds_length=0.011010189000003834 seconds_distance=0.35853710400000693 seconds_samples=0.31470087799877433 seconds_place=0 seconds_entity=0 v_rounds=1 v_limit=0 v_collapses=0 v_splits=0 v_changes=c0s0q1 bad_before_v=304 seconds_v=0.059165090999997005
S2 NATIVE3D_LOCAL_SURFACE_STATE rank=5 stage=0 splits_all=18650 faces=86254 points=43129 traces=753 revision=2 seconds_exchange=0.038838847000008059 seconds_gather=0.19655011599999739 seconds_build=32.692757936999996 seconds_check=0.040240638000000217 modeled_peak=1995406684 rss_before=528039936 rss_after=528105472
```

#### S1 summed surface faces by stage and pass

| Stage | Pass | Ranks with rows | Sum of faces_out |
| --- | --- | --- | --- |
| 0 | 1 | 0, 1, 2, 3, 4, 5 | 80972 |
| 0 | 2 | 0, 1, 2, 3, 4, 5 | 86254 |

### Event 2

| Case | Native max | Cells | Points | Surface faces | Min coverage % | Job peak | Builds observed |
| --- | --- | --- | --- | --- | --- | --- | --- |
| CORE | 371.336 | 553652 | 109124 | 64758 | 98.6725 (6 ranks) | 11652763648 | 0 |
| S1 | 446.547 | 575733 | 116342 | 79274 | 98.9311 (6 ranks) | 13027512320 | 1 |
| S2 | 444.120 | 575733 | 116342 | 79274 | 98.9130 (6 ranks) | 13060841472 | 1 |

| Case | Rank | Native | preparation | construction | quality | final_proof | transfer | topology | geometry_bound | mobility_policy | communication | Process peak | Builds observed |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| CORE | 0 | 371.332 | 53.570 | 164.801 | 89.589 | 27.880 | 30.398 | 17.729 | 18.189 | 0.000 | 89.214 | 2042990592 | 0 |
| CORE | 1 | 371.248 | 53.701 | 164.788 | 89.588 | 27.880 | 30.396 | 15.466 | 15.079 | 0.000 | 103.854 | 1906511872 | 0 |
| CORE | 2 | 371.246 | 53.692 | 164.794 | 89.589 | 27.880 | 30.398 | 14.253 | 5.816 | 0.000 | 113.967 | 1924472832 | 0 |
| CORE | 3 | 371.336 | 53.588 | 164.809 | 89.589 | 27.880 | 30.400 | 15.729 | 6.420 | 0.000 | 106.336 | 2118291456 | 0 |
| CORE | 4 | 371.203 | 53.804 | 164.746 | 89.588 | 27.880 | 30.398 | 14.636 | 28.392 | 0.000 | 90.916 | 1930936320 | 0 |
| CORE | 5 | 371.241 | 53.772 | 164.731 | 89.588 | 27.880 | 30.397 | 14.883 | 21.919 | 0.000 | 95.800 | 1729560576 | 0 |
| S1 | 0 | 446.542 | 81.191 | 189.602 | 112.423 | 24.213 | 32.811 | 34.093 | 9.099 | 16.840 | 104.654 | 1873203200 | 1 |
| S1 | 1 | 446.547 | 81.084 | 189.603 | 112.428 | 24.213 | 32.808 | 39.103 | 8.523 | 16.783 | 94.251 | 2033299456 | 1 |
| S1 | 2 | 446.540 | 81.152 | 189.599 | 112.424 | 24.213 | 32.807 | 32.895 | 7.162 | 16.770 | 107.153 | 2166013952 | 1 |
| S1 | 3 | 446.389 | 81.284 | 189.618 | 112.427 | 24.213 | 32.810 | 34.422 | 11.965 | 16.768 | 97.709 | 3146559488 | 1 |
| S1 | 4 | 446.547 | 81.128 | 189.620 | 112.427 | 24.213 | 32.806 | 39.299 | 37.376 | 16.779 | 64.134 | 1955770368 | 1 |
| S1 | 5 | 446.387 | 81.332 | 189.602 | 112.424 | 24.213 | 32.804 | 34.197 | 19.442 | 16.846 | 93.662 | 1852665856 | 1 |
| S2 | 0 | 443.992 | 80.895 | 188.973 | 112.188 | 24.187 | 32.037 | 34.157 | 9.066 | 16.773 | 104.299 | 1873289216 | 1 |
| S2 | 1 | 444.120 | 80.663 | 188.958 | 112.189 | 24.187 | 32.040 | 39.093 | 8.520 | 16.767 | 93.379 | 2038013952 | 1 |
| S2 | 2 | 444.109 | 80.740 | 188.953 | 112.189 | 24.187 | 32.040 | 32.764 | 7.162 | 16.775 | 106.704 | 2191917056 | 1 |
| S2 | 3 | 443.952 | 80.862 | 188.955 | 112.190 | 24.187 | 32.039 | 34.512 | 11.992 | 16.778 | 97.040 | 3147374592 | 1 |
| S2 | 4 | 444.115 | 80.683 | 188.957 | 112.188 | 24.187 | 32.037 | 39.462 | 37.159 | 16.779 | 64.021 | 1957093376 | 1 |
| S2 | 5 | 444.110 | 80.759 | 188.956 | 112.188 | 24.187 | 32.038 | 34.132 | 19.410 | 16.769 | 93.449 | 1853153280 | 1 |

| Case | Rank | Length splits by engine exit | Surface rounds by engine exit | Construction wall commits | Quality wall commits | First census stage emitted | Geometry-invalid cells |
| --- | --- | --- | --- | --- | --- | --- | --- |
| CORE | 0 | 2704 | 422 | 1432 | 278 | length_collapse_exit | 0 |
| CORE | 1 | 45 | 422 | 585 | 270 | length_collapse_exit | 0 |
| CORE | 2 | 247 | 422 | 498 | 175 | length_collapse_exit | 0 |
| CORE | 3 | 2827 | 422 | 1305 | 243 | length_collapse_exit | 0 |
| CORE | 4 | 10 | 422 | 651 | 339 | length_collapse_exit | 0 |
| CORE | 5 | 11 | 422 | 562 | 399 | length_collapse_exit | 0 |
| S1 | 0 | 0 | 0 | 510 | 603 | length_collapse_exit | 0 |
| S1 | 1 | 24 | 0 | 557 | 580 | length_collapse_exit | 0 |
| S1 | 2 | 3 | 0 | 491 | 591 | length_collapse_exit | 0 |
| S1 | 3 | 28 | 0 | 592 | 239 | length_collapse_exit | 0 |
| S1 | 4 | 40 | 0 | 831 | 934 | length_collapse_exit | 0 |
| S1 | 5 | 56 | 0 | 647 | 520 | length_collapse_exit | 0 |
| S2 | 0 | 0 | 0 | 510 | 603 | length_collapse_exit | 0 |
| S2 | 1 | 24 | 0 | 557 | 580 | length_collapse_exit | 0 |
| S2 | 2 | 3 | 0 | 491 | 591 | length_collapse_exit | 0 |
| S2 | 3 | 28 | 0 | 592 | 239 | length_collapse_exit | 0 |
| S2 | 4 | 40 | 0 | 831 | 934 | length_collapse_exit | 0 |
| S2 | 5 | 56 | 0 | 647 | 520 | length_collapse_exit | 0 |

#### Surface rows

```text
S1 NATIVE3D_LOCAL_SURFACE rank=0 stage=0 pass=1 applicable=1 faces_in=11524 faces_out=12152 points_in=6032 points_out=6346 splits_length=36 splits_distance=278 splits_feature=0 sweeps_length=3 sweeps_distance=5 left_long=1 left_border=7 left_distance=18 left_all_feature=0 left_refused=8 left_limit=0 left_short=0 left_neighbor_locked=4 waits_neighbor=155 path_splits=36 path_max=3 left_long_neighbor=0 waits_length=1 star_max=124 star_gt100=5 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=4 refused_topology=0 refused_memory=0 refused_entity=0 refused_near=0 walk_tol8=10 walk_far=0 walk_no_start=0 walk_exception=0 chord_unknown=0 samples=50832 samples_unconverged=634 outward_max=1.6699461355723424e-05 seconds_length=0.050583522000010817 seconds_distance=0.84739987899990865 seconds_samples=0.61494539099874146 seconds_place=0.011924838998766063 seconds_entity=0.10889572000144199 v_rounds=5 v_limit=0 v_collapses=22 v_splits=70 v_changes=c18s58q13,c3s9q7,c1s2q8,c0s1q5,c0s0q1 bad_before_v=811 seconds_v=0.75078597499998523
S1 NATIVE3D_LOCAL_SURFACE rank=0 stage=0 pass=2 applicable=1 faces_in=13727 faces_out=13785 points_in=7276 points_out=7305 splits_length=1 splits_distance=28 splits_feature=0 sweeps_length=1 sweeps_distance=4 left_long=0 left_border=0 left_distance=8 left_all_feature=0 left_refused=8 left_limit=0 left_short=0 left_neighbor_locked=0 waits_neighbor=21 path_splits=6 path_max=3 left_long_neighbor=0 waits_length=0 star_max=126 star_gt100=5 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=4 refused_topology=0 refused_memory=0 refused_entity=0 refused_near=0 walk_tol8=1 walk_far=0 walk_no_start=0 walk_exception=0 chord_unknown=0 samples=55364 samples_unconverged=708 outward_max=4.8924231092930309e-06 seconds_length=0.035273729999971692 seconds_distance=0.84456302600005984 seconds_samples=0.67900107799994203 seconds_place=0.0013691880002397738 seconds_entity=0.0084323789999416476 v_rounds=3 v_limit=0 v_collapses=2 v_splits=20 v_changes=c1s16q6,c1s4q5,c0s0q1 bad_before_v=493 seconds_v=0.38072979700007181
S1 NATIVE3D_LOCAL_SURFACE_STATE rank=0 stage=0 splits_all=2260 faces=85170 points=42587 traces=752 revision=1178 seconds_exchange=0.012881153999956041 seconds_gather=0.27867447700009507 seconds_build=32.397950709000042 seconds_check=0.047001158999933068 modeled_peak=2505711876 rss_before=1386541056 rss_after=1386541056
S1 NATIVE3D_LOCAL_SURFACE rank=1 stage=0 pass=1 applicable=1 faces_in=11934 faces_out=13352 points_in=6177 points_out=6886 splits_length=446 splits_distance=263 splits_feature=0 sweeps_length=5 sweeps_distance=7 left_long=10 left_border=12 left_distance=12 left_all_feature=0 left_refused=0 left_limit=0 left_short=0 left_neighbor_locked=8 waits_neighbor=133 path_splits=32 path_max=5 left_long_neighbor=2 waits_length=38 star_max=126 star_gt100=19 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=0 refused_topology=0 refused_memory=0 refused_entity=0 refused_near=0 walk_tol8=0 walk_far=0 walk_no_start=0 walk_exception=0 chord_unknown=0 samples=55512 samples_unconverged=1168 outward_max=7.1439759082551554e-05 seconds_length=0.26758481299998493 seconds_distance=0.89289111099992624 seconds_samples=0.59774331499590971 seconds_place=0.023003159002541906 seconds_entity=0.26037504499834085 v_rounds=3 v_limit=0 v_collapses=13 v_splits=58 v_changes=c12s52q12,c1s6q4,c0s0q1 bad_before_v=677 seconds_v=0.46972776799998428
S1 NATIVE3D_LOCAL_SURFACE rank=1 stage=0 pass=2 applicable=1 faces_in=13967 faces_out=14037 points_in=7275 points_out=7310 splits_length=11 splits_distance=24 splits_feature=0 sweeps_length=2 sweeps_distance=4 left_long=0 left_border=0 left_distance=2 left_all_feature=0 left_refused=2 left_limit=0 left_short=0 left_neighbor_locked=0 waits_neighbor=27 path_splits=5 path_max=3 left_long_neighbor=0 waits_length=0 star_max=112 star_gt100=6 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=1 refused_topology=0 refused_memory=0 refused_entity=0 refused_near=0 walk_tol8=0 walk_far=0 walk_no_start=0 walk_exception=0 chord_unknown=0 samples=56340 samples_unconverged=1228 outward_max=7.8515498129817315e-06 seconds_length=0.024023994999993192 seconds_distance=0.77957433000005949 seconds_samples=0.61317896000423389 seconds_place=0.0014150150001341899 seconds_entity=0.010220870000011928 v_rounds=2 v_limit=0 v_collapses=0 v_splits=3 v_changes=c0s3q5,c0s0q1 bad_before_v=353 seconds_v=0.19791557100006685
S1 NATIVE3D_LOCAL_SURFACE_STATE rank=1 stage=0 splits_all=2260 faces=85170 points=42587 traces=752 revision=1178 seconds_exchange=0.01288242500004344 seconds_gather=0.27867642600006093 seconds_build=32.32407265300003 seconds_check=0.047002388999999312 modeled_peak=2898651620 rss_before=1297567744 rss_after=1297567744
S1 NATIVE3D_LOCAL_SURFACE rank=2 stage=0 pass=1 applicable=1 faces_in=10630 faces_out=10936 points_in=5500 points_out=5653 splits_length=53 splits_distance=100 splits_feature=0 sweeps_length=2 sweeps_distance=3 left_long=2 left_border=3 left_distance=13 left_all_feature=0 left_refused=8 left_limit=0 left_short=0 left_neighbor_locked=3 waits_neighbor=36 path_splits=2 path_max=2 left_long_neighbor=1 waits_length=0 star_max=120 star_gt100=4 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=4 refused_topology=0 refused_memory=0 refused_entity=0 refused_near=0 walk_tol8=3 walk_far=0 walk_no_start=0 walk_exception=0 chord_unknown=0 samples=44544 samples_unconverged=1432 outward_max=1.8042209461166817e-05 seconds_length=0.054429179999942789 seconds_distance=0.60390168200001426 seconds_samples=0.48887002500259769 seconds_place=0.0067845679998299602 seconds_entity=0.059999181000307544 v_rounds=4 v_limit=0 v_collapses=9 v_splits=38 v_changes=c8s30q7,c0s8q4,c1s0q3,c0s0q1 bad_before_v=552 seconds_v=0.44805412200003047
S1 NATIVE3D_LOCAL_SURFACE rank=2 stage=0 pass=2 applicable=1 faces_in=11701 faces_out=11735 points_in=6115 points_out=6132 splits_length=2 splits_distance=15 splits_feature=0 sweeps_length=2 sweeps_distance=3 left_long=0 left_border=0 left_distance=8 left_all_feature=0 left_refused=8 left_limit=0 left_short=0 left_neighbor_locked=0 waits_neighbor=9 path_splits=0 path_max=2 left_long_neighbor=0 waits_length=0 star_max=106 star_gt100=1 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=4 refused_topology=0 refused_memory=0 refused_entity=0 refused_near=0 walk_tol8=0 walk_far=0 walk_no_start=0 walk_exception=0 chord_unknown=0 samples=47060 samples_unconverged=1525 outward_max=5.4193550010018623e-16 seconds_length=0.02557585600004586 seconds_distance=0.62500274599995009 seconds_samples=0.52515515299842264 seconds_place=0.00093010499961110327 seconds_entity=0.0055997700000034456 v_rounds=3 v_limit=0 v_collapses=0 v_splits=9 v_changes=c0s8q5,c0s1q4,c0s0q1 bad_before_v=329 seconds_v=0.31576042899996537
S1 NATIVE3D_LOCAL_SURFACE_STATE rank=2 stage=0 splits_all=2260 faces=85170 points=42587 traces=752 revision=1178 seconds_exchange=0.012873904999992192 seconds_gather=0.2786699429999544 seconds_build=32.38057312400008 seconds_check=0.046997353000051589 modeled_peak=2362578904 rss_before=1397567488 rss_after=1397567488
S1 NATIVE3D_LOCAL_SURFACE rank=3 stage=0 pass=1 applicable=1 faces_in=12744 faces_out=13322 points_in=6557 points_out=6846 splits_length=215 splits_distance=74 splits_feature=0 sweeps_length=3 sweeps_distance=4 left_long=24 left_border=16 left_distance=1 left_all_feature=0 left_refused=0 left_limit=0 left_short=0 left_neighbor_locked=0 waits_neighbor=16 path_splits=5 path_max=3 left_long_neighbor=9 waits_length=4 star_max=88 star_gt100=0 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=0 refused_topology=0 refused_memory=0 refused_entity=0 refused_near=0 walk_tol8=3 walk_far=0 walk_no_start=0 walk_exception=0 chord_unknown=0 samples=53880 samples_unconverged=1442 outward_max=2.4546945779567109e-05 seconds_length=0.13960898999994242 seconds_distance=1.0057379039999432 seconds_samples=0.83142227498399279 seconds_place=0.010283578999860765 seconds_entity=0.12823175700088996 v_rounds=3 v_limit=0 v_collapses=9 v_splits=25 v_changes=c8s25q5,c1s0q2,c0s0q1 bad_before_v=316 seconds_v=0.21673879100001159
S1 NATIVE3D_LOCAL_SURFACE rank=3 stage=0 pass=2 applicable=1 faces_in=13385 faces_out=13435 points_in=6947 points_out=6972 splits_length=25 splits_distance=0 splits_feature=1 sweeps_length=5 sweeps_distance=0 left_long=0 left_border=0 left_distance=0 left_all_feature=0 left_refused=0 left_limit=0 left_short=0 left_neighbor_locked=0 waits_neighbor=0 path_splits=0 path_max=0 left_long_neighbor=0 waits_length=10 star_max=96 star_gt100=0 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=0 refused_topology=0 refused_memory=0 refused_entity=0 refused_near=0 walk_tol8=0 walk_far=0 walk_no_start=0 walk_exception=0 chord_unknown=0 samples=53740 samples_unconverged=1380 outward_max=4.3388844396421387e-16 seconds_length=0.046709797999938019 seconds_distance=0.87798351300000377 seconds_samples=0.82853833500951168 seconds_place=0.0014929229998870142 seconds_entity=0.0099399820002190609 v_rounds=2 v_limit=0 v_collapses=1 v_splits=0 v_changes=c1s0q1,c0s0q1 bad_before_v=147 seconds_v=0.092944622000004529
S1 NATIVE3D_LOCAL_SURFACE_STATE rank=3 stage=0 splits_all=2260 faces=85170 points=42587 traces=752 revision=1178 seconds_exchange=0.012876621000032173 seconds_gather=0.27866976500001783 seconds_build=32.205931674999988 seconds_check=0.046998964999943382 modeled_peak=2380469860 rss_before=1827852288 rss_after=1827852288
S1 NATIVE3D_LOCAL_SURFACE rank=4 stage=0 pass=1 applicable=1 faces_in=18858 faces_out=19586 points_in=9780 points_out=10144 splits_length=3 splits_distance=361 splits_feature=0 sweeps_length=1 sweeps_distance=5 left_long=0 left_border=4 left_distance=21 left_all_feature=0 left_refused=0 left_limit=0 left_short=0 left_neighbor_locked=17 waits_neighbor=220 path_splits=58 path_max=4 left_long_neighbor=0 waits_length=0 star_max=116 star_gt100=7 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=0 refused_topology=0 refused_memory=0 refused_entity=0 refused_near=0 walk_tol8=2 walk_far=0 walk_no_start=0 walk_exception=0 chord_unknown=0 samples=81232 samples_unconverged=1143 outward_max=1.2916559724783657e-05 seconds_length=0.03609699499997987 seconds_distance=1.587690473000066 seconds_samples=1.1914415299951315 seconds_place=0.017483339998989322 seconds_entity=0.1304798800001663 v_rounds=4 v_limit=0 v_collapses=42 v_splits=111 v_changes=c36s102q9,c5s9q5,c1s0q1,c0s0q1 bad_before_v=1186 seconds_v=0.68764732900001491
S1 NATIVE3D_LOCAL_SURFACE rank=4 stage=0 pass=2 applicable=1 faces_in=17840 faces_out=17840 points_in=9417 points_out=9417 splits_length=0 splits_distance=0 splits_feature=0 sweeps_length=0 sweeps_distance=1 left_long=0 left_border=1 left_distance=2 left_all_feature=0 left_refused=0 left_limit=0 left_short=0 left_neighbor_locked=1 waits_neighbor=0 path_splits=0 path_max=1 left_long_neighbor=0 waits_length=0 star_max=100 star_gt100=0 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=0 refused_topology=0 refused_memory=0 refused_entity=0 refused_near=0 walk_tol8=0 walk_far=0 walk_no_start=0 walk_exception=0 chord_unknown=0 samples=71360 samples_unconverged=989 outward_max=0 seconds_length=0.033891715999970984 seconds_distance=1.1804682870000534 seconds_samples=1.0464141949978512 seconds_place=0 seconds_entity=0 v_rounds=2 v_limit=0 v_collapses=3 v_splits=1 v_changes=c3s1q7,c0s0q1 bad_before_v=423 seconds_v=0.22280554200006009
S1 NATIVE3D_LOCAL_SURFACE_STATE rank=4 stage=0 splits_all=2260 faces=85170 points=42587 traces=752 revision=1178 seconds_exchange=0.01270100299996102 seconds_gather=0.27867532699997355 seconds_build=32.232431499999961 seconds_check=0.047000101000094219 modeled_peak=2377939216 rss_before=1269882880 rss_after=1269882880
S1 NATIVE3D_LOCAL_SURFACE rank=5 stage=0 pass=1 applicable=1 faces_in=14960 faces_out=15608 points_in=7723 points_out=8047 splits_length=172 splits_distance=152 splits_feature=3 sweeps_length=3 sweeps_distance=4 left_long=22 left_border=19 left_distance=13 left_all_feature=0 left_refused=5 left_limit=0 left_short=0 left_neighbor_locked=4 waits_neighbor=65 path_splits=20 path_max=3 left_long_neighbor=7 waits_length=1 star_max=112 star_gt100=3 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=3 refused_topology=0 refused_memory=0 refused_entity=0 refused_near=0 walk_tol8=5 walk_far=0 walk_no_start=0 walk_exception=0 chord_unknown=0 samples=63648 samples_unconverged=1106 outward_max=2.9843938080002266e-07 seconds_length=0.11838518500007922 seconds_distance=1.2703784799999767 seconds_samples=1.0092430039959481 seconds_place=0.014287397000202873 seconds_entity=0.12707039300062206 v_rounds=4 v_limit=0 v_collapses=22 v_splits=65 v_changes=c20s56q10,c1s7q6,c1s2q4,c0s0q1 bad_before_v=626 seconds_v=0.57705350499998076
S1 NATIVE3D_LOCAL_SURFACE rank=5 stage=0 pass=2 applicable=1 faces_in=14336 faces_out=14338 points_in=7469 points_out=7470 splits_length=0 splits_distance=1 splits_feature=0 sweeps_length=0 sweeps_distance=1 left_long=0 left_border=0 left_distance=1 left_all_feature=0 left_refused=1 left_limit=0 left_short=0 left_neighbor_locked=0 waits_neighbor=0 path_splits=0 path_max=0 left_long_neighbor=0 waits_length=0 star_max=114 star_gt100=2 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=1 refused_topology=0 refused_memory=0 refused_entity=0 refused_near=0 walk_tol8=0 walk_far=0 walk_no_start=0 walk_exception=0 chord_unknown=0 samples=57360 samples_unconverged=1022 outward_max=0 seconds_length=0.025970049999955336 seconds_distance=1.0185144590000164 seconds_samples=0.91217539799447422 seconds_place=0.00022000699993895978 seconds_entity=0.00027486700003009901 v_rounds=2 v_limit=0 v_collapses=1 v_splits=0 v_changes=c1s0q1,c0s0q1 bad_before_v=315 seconds_v=0.14025181800002429
S1 NATIVE3D_LOCAL_SURFACE_STATE rank=5 stage=0 splits_all=2260 faces=85170 points=42587 traces=752 revision=1178 seconds_exchange=0.012868995999951949 seconds_gather=0.2786714920000577 seconds_build=32.383242077999967 seconds_check=0.046999304999985725 modeled_peak=2104776904 rss_before=1330630656 rss_after=1330630656
S2 NATIVE3D_LOCAL_SURFACE rank=0 stage=0 pass=1 applicable=1 faces_in=11524 faces_out=12152 points_in=6032 points_out=6346 splits_length=36 splits_distance=278 splits_feature=0 sweeps_length=3 sweeps_distance=5 left_long=1 left_border=7 left_distance=18 left_all_feature=0 left_refused=8 left_limit=0 left_short=0 left_neighbor_locked=4 waits_neighbor=155 path_splits=36 path_max=3 left_long_neighbor=0 waits_length=1 star_max=124 star_gt100=5 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=4 refused_topology=0 refused_memory=0 refused_entity=0 refused_near=0 walk_tol8=10 walk_far=0 walk_no_start=0 walk_exception=0 chord_unknown=0 samples=50832 samples_unconverged=634 outward_max=1.6699461355723424e-05 seconds_length=0.049203778999981296 seconds_distance=0.84586609400002999 seconds_samples=0.61221156199451343 seconds_place=0.011933061999229722 seconds_entity=0.10915765900176666 v_rounds=5 v_limit=0 v_collapses=22 v_splits=70 v_changes=c18s58q13,c3s9q7,c1s2q8,c0s1q5,c0s0q1 bad_before_v=811 seconds_v=0.75161633999994137
S2 NATIVE3D_LOCAL_SURFACE rank=0 stage=0 pass=2 applicable=1 faces_in=13727 faces_out=13785 points_in=7276 points_out=7305 splits_length=1 splits_distance=28 splits_feature=0 sweeps_length=1 sweeps_distance=4 left_long=0 left_border=0 left_distance=8 left_all_feature=0 left_refused=8 left_limit=0 left_short=0 left_neighbor_locked=0 waits_neighbor=21 path_splits=6 path_max=3 left_long_neighbor=0 waits_length=0 star_max=126 star_gt100=5 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=4 refused_topology=0 refused_memory=0 refused_entity=0 refused_near=0 walk_tol8=1 walk_far=0 walk_no_start=0 walk_exception=0 chord_unknown=0 samples=55364 samples_unconverged=708 outward_max=4.8924231092930309e-06 seconds_length=0.03112170999997943 seconds_distance=0.84193404300003749 seconds_samples=0.68169393700623004 seconds_place=0.0013244610001947876 seconds_entity=0.0085145560000228215 v_rounds=3 v_limit=0 v_collapses=2 v_splits=20 v_changes=c1s16q6,c1s4q5,c0s0q1 bad_before_v=493 seconds_v=0.38489775499999723
S2 NATIVE3D_LOCAL_SURFACE_STATE rank=0 stage=0 splits_all=2260 faces=85170 points=42587 traces=752 revision=1178 seconds_exchange=0.012494246000073872 seconds_gather=0.27864579200002026 seconds_build=32.403838359000019 seconds_check=0.050641391999988628 modeled_peak=2505711876 rss_before=1359327232 rss_after=1359327232
S2 NATIVE3D_LOCAL_SURFACE rank=1 stage=0 pass=1 applicable=1 faces_in=11934 faces_out=13352 points_in=6177 points_out=6886 splits_length=446 splits_distance=263 splits_feature=0 sweeps_length=5 sweeps_distance=7 left_long=10 left_border=12 left_distance=12 left_all_feature=0 left_refused=0 left_limit=0 left_short=0 left_neighbor_locked=8 waits_neighbor=133 path_splits=32 path_max=5 left_long_neighbor=2 waits_length=38 star_max=126 star_gt100=19 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=0 refused_topology=0 refused_memory=0 refused_entity=0 refused_near=0 walk_tol8=0 walk_far=0 walk_no_start=0 walk_exception=0 chord_unknown=0 samples=55512 samples_unconverged=1168 outward_max=7.1439759082551554e-05 seconds_length=0.26330662600003052 seconds_distance=0.88301090499999191 seconds_samples=0.59301252299974294 seconds_place=0.022625979001418273 seconds_entity=0.26002806100041198 v_rounds=3 v_limit=0 v_collapses=13 v_splits=58 v_changes=c12s52q12,c1s6q4,c0s0q1 bad_before_v=677 seconds_v=0.46766868700001396
S2 NATIVE3D_LOCAL_SURFACE rank=1 stage=0 pass=2 applicable=1 faces_in=13967 faces_out=14037 points_in=7275 points_out=7310 splits_length=11 splits_distance=24 splits_feature=0 sweeps_length=2 sweeps_distance=4 left_long=0 left_border=0 left_distance=2 left_all_feature=0 left_refused=2 left_limit=0 left_short=0 left_neighbor_locked=0 waits_neighbor=27 path_splits=5 path_max=3 left_long_neighbor=0 waits_length=0 star_max=112 star_gt100=6 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=1 refused_topology=0 refused_memory=0 refused_entity=0 refused_near=0 walk_tol8=0 walk_far=0 walk_no_start=0 walk_exception=0 chord_unknown=0 samples=56340 samples_unconverged=1228 outward_max=7.8515498129817315e-06 seconds_length=0.023950425999942127 seconds_distance=0.77368344100000286 seconds_samples=0.61372225899845034 seconds_place=0.001401719999762463 seconds_entity=0.010321971999815105 v_rounds=2 v_limit=0 v_collapses=0 v_splits=3 v_changes=c0s3q5,c0s0q1 bad_before_v=353 seconds_v=0.19840729899999587
S2 NATIVE3D_LOCAL_SURFACE_STATE rank=1 stage=0 splits_all=2260 faces=85170 points=42587 traces=752 revision=1178 seconds_exchange=0.012501870000050985 seconds_gather=0.27864973900000223 seconds_build=32.394987382999943 seconds_check=0.050642616000004637 modeled_peak=2898651620 rss_before=1370955776 rss_after=1370955776
S2 NATIVE3D_LOCAL_SURFACE rank=2 stage=0 pass=1 applicable=1 faces_in=10630 faces_out=10936 points_in=5500 points_out=5653 splits_length=53 splits_distance=100 splits_feature=0 sweeps_length=2 sweeps_distance=3 left_long=2 left_border=3 left_distance=13 left_all_feature=0 left_refused=8 left_limit=0 left_short=0 left_neighbor_locked=3 waits_neighbor=36 path_splits=2 path_max=2 left_long_neighbor=1 waits_length=0 star_max=120 star_gt100=4 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=4 refused_topology=0 refused_memory=0 refused_entity=0 refused_near=0 walk_tol8=3 walk_far=0 walk_no_start=0 walk_exception=0 chord_unknown=0 samples=44544 samples_unconverged=1432 outward_max=1.8042209461166817e-05 seconds_length=0.053534062000039739 seconds_distance=0.60392957800002023 seconds_samples=0.48804817100540276 seconds_place=0.0066536989996848206 seconds_entity=0.059760522999681598 v_rounds=4 v_limit=0 v_collapses=9 v_splits=38 v_changes=c8s30q7,c0s8q4,c1s0q3,c0s0q1 bad_before_v=552 seconds_v=0.44641784700002063
S2 NATIVE3D_LOCAL_SURFACE rank=2 stage=0 pass=2 applicable=1 faces_in=11701 faces_out=11735 points_in=6115 points_out=6132 splits_length=2 splits_distance=15 splits_feature=0 sweeps_length=2 sweeps_distance=3 left_long=0 left_border=0 left_distance=8 left_all_feature=0 left_refused=8 left_limit=0 left_short=0 left_neighbor_locked=0 waits_neighbor=9 path_splits=0 path_max=2 left_long_neighbor=0 waits_length=0 star_max=106 star_gt100=1 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=4 refused_topology=0 refused_memory=0 refused_entity=0 refused_near=0 walk_tol8=0 walk_far=0 walk_no_start=0 walk_exception=0 chord_unknown=0 samples=47060 samples_unconverged=1525 outward_max=5.4193550010018623e-16 seconds_length=0.022290320000024622 seconds_distance=0.62650470999994923 seconds_samples=0.52688083200109759 seconds_place=0.00090717400007633842 seconds_entity=0.0055944310000768382 v_rounds=3 v_limit=0 v_collapses=0 v_splits=9 v_changes=c0s8q5,c0s1q4,c0s0q1 bad_before_v=329 seconds_v=0.31554312299999765
S2 NATIVE3D_LOCAL_SURFACE_STATE rank=2 stage=0 splits_all=2260 faces=85170 points=42587 traces=752 revision=1178 seconds_exchange=0.012482869999985269 seconds_gather=0.2786396290000539 seconds_build=32.386963781999953 seconds_check=0.050635777000024973 modeled_peak=2362578904 rss_before=1458778112 rss_after=1458778112
S2 NATIVE3D_LOCAL_SURFACE rank=3 stage=0 pass=1 applicable=1 faces_in=12744 faces_out=13322 points_in=6557 points_out=6846 splits_length=215 splits_distance=74 splits_feature=0 sweeps_length=3 sweeps_distance=4 left_long=24 left_border=16 left_distance=1 left_all_feature=0 left_refused=0 left_limit=0 left_short=0 left_neighbor_locked=0 waits_neighbor=16 path_splits=5 path_max=3 left_long_neighbor=9 waits_length=4 star_max=88 star_gt100=0 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=0 refused_topology=0 refused_memory=0 refused_entity=0 refused_near=0 walk_tol8=3 walk_far=0 walk_no_start=0 walk_exception=0 chord_unknown=0 samples=53880 samples_unconverged=1442 outward_max=2.4546945779567109e-05 seconds_length=0.14080151099994964 seconds_distance=1.0066264059998957 seconds_samples=0.83218230999875686 seconds_place=0.0098561100004417312 seconds_entity=0.12830997299897717 v_rounds=3 v_limit=0 v_collapses=9 v_splits=25 v_changes=c8s25q5,c1s0q2,c0s0q1 bad_before_v=316 seconds_v=0.21530051000002004
S2 NATIVE3D_LOCAL_SURFACE rank=3 stage=0 pass=2 applicable=1 faces_in=13385 faces_out=13435 points_in=6947 points_out=6972 splits_length=25 splits_distance=0 splits_feature=1 sweeps_length=5 sweeps_distance=0 left_long=0 left_border=0 left_distance=0 left_all_feature=0 left_refused=0 left_limit=0 left_short=0 left_neighbor_locked=0 waits_neighbor=0 path_splits=0 path_max=0 left_long_neighbor=0 waits_length=10 star_max=96 star_gt100=0 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=0 refused_topology=0 refused_memory=0 refused_entity=0 refused_near=0 walk_tol8=0 walk_far=0 walk_no_start=0 walk_exception=0 chord_unknown=0 samples=53740 samples_unconverged=1380 outward_max=4.3388844396421387e-16 seconds_length=0.042262782000079824 seconds_distance=0.88464363599996432 seconds_samples=0.83320559298931585 seconds_place=0.0013524130000632795 seconds_entity=0.0094454939999195631 v_rounds=2 v_limit=0 v_collapses=1 v_splits=0 v_changes=c1s0q1,c0s0q1 bad_before_v=147 seconds_v=0.093096208000019942
S2 NATIVE3D_LOCAL_SURFACE_STATE rank=3 stage=0 splits_all=2260 faces=85170 points=42587 traces=752 revision=1178 seconds_exchange=0.012486534000004212 seconds_gather=0.27864089799993508 seconds_build=32.368763189999981 seconds_check=0.050637508000022535 modeled_peak=2380469860 rss_before=1782759424 rss_after=1782759424
S2 NATIVE3D_LOCAL_SURFACE rank=4 stage=0 pass=1 applicable=1 faces_in=18858 faces_out=19586 points_in=9780 points_out=10144 splits_length=3 splits_distance=361 splits_feature=0 sweeps_length=1 sweeps_distance=5 left_long=0 left_border=4 left_distance=21 left_all_feature=0 left_refused=0 left_limit=0 left_short=0 left_neighbor_locked=17 waits_neighbor=220 path_splits=58 path_max=4 left_long_neighbor=0 waits_length=0 star_max=116 star_gt100=7 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=0 refused_topology=0 refused_memory=0 refused_entity=0 refused_near=0 walk_tol8=2 walk_far=0 walk_no_start=0 walk_exception=0 chord_unknown=0 samples=81232 samples_unconverged=1143 outward_max=1.2916559724783657e-05 seconds_length=0.035419445999991694 seconds_distance=1.5868494600000531 seconds_samples=1.1856832899990195 seconds_place=0.017116572001214081 seconds_entity=0.12853267400021195 v_rounds=4 v_limit=0 v_collapses=42 v_splits=111 v_changes=c36s102q9,c5s9q5,c1s0q1,c0s0q1 bad_before_v=1186 seconds_v=0.6907659019999528
S2 NATIVE3D_LOCAL_SURFACE rank=4 stage=0 pass=2 applicable=1 faces_in=17840 faces_out=17840 points_in=9417 points_out=9417 splits_length=0 splits_distance=0 splits_feature=0 sweeps_length=0 sweeps_distance=1 left_long=0 left_border=1 left_distance=2 left_all_feature=0 left_refused=0 left_limit=0 left_short=0 left_neighbor_locked=1 waits_neighbor=0 path_splits=0 path_max=1 left_long_neighbor=0 waits_length=0 star_max=100 star_gt100=0 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=0 refused_topology=0 refused_memory=0 refused_entity=0 refused_near=0 walk_tol8=0 walk_far=0 walk_no_start=0 walk_exception=0 chord_unknown=0 samples=71360 samples_unconverged=989 outward_max=0 seconds_length=0.032258366999940336 seconds_distance=1.1816937870000856 seconds_samples=1.0492434919997322 seconds_place=0 seconds_entity=0 v_rounds=2 v_limit=0 v_collapses=3 v_splits=1 v_changes=c3s1q7,c0s0q1 bad_before_v=423 seconds_v=0.22329242800003613
S2 NATIVE3D_LOCAL_SURFACE_STATE rank=4 stage=0 splits_all=2260 faces=85170 points=42587 traces=752 revision=1178 seconds_exchange=0.01249880000000303 seconds_gather=0.27864377100002002 seconds_build=32.373940893000054 seconds_check=0.050640217000022858 modeled_peak=2377939216 rss_before=1269661696 rss_after=1269661696
S2 NATIVE3D_LOCAL_SURFACE rank=5 stage=0 pass=1 applicable=1 faces_in=14960 faces_out=15608 points_in=7723 points_out=8047 splits_length=172 splits_distance=152 splits_feature=3 sweeps_length=3 sweeps_distance=4 left_long=22 left_border=19 left_distance=13 left_all_feature=0 left_refused=5 left_limit=0 left_short=0 left_neighbor_locked=4 waits_neighbor=65 path_splits=20 path_max=3 left_long_neighbor=7 waits_length=1 star_max=112 star_gt100=3 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=3 refused_topology=0 refused_memory=0 refused_entity=0 refused_near=0 walk_tol8=5 walk_far=0 walk_no_start=0 walk_exception=0 chord_unknown=0 samples=63648 samples_unconverged=1106 outward_max=2.9843938080002266e-07 seconds_length=0.1238309540000273 seconds_distance=1.2618369400000802 seconds_samples=1.0032142969994311 seconds_place=0.014380737999999837 seconds_entity=0.12741910300064774 v_rounds=4 v_limit=0 v_collapses=22 v_splits=65 v_changes=c20s56q10,c1s7q6,c1s2q4,c0s0q1 bad_before_v=626 seconds_v=0.5677512749999778
S2 NATIVE3D_LOCAL_SURFACE rank=5 stage=0 pass=2 applicable=1 faces_in=14336 faces_out=14338 points_in=7469 points_out=7470 splits_length=0 splits_distance=1 splits_feature=0 sweeps_length=0 sweeps_distance=1 left_long=0 left_border=0 left_distance=1 left_all_feature=0 left_refused=1 left_limit=0 left_short=0 left_neighbor_locked=0 waits_neighbor=0 path_splits=0 path_max=0 left_long_neighbor=0 waits_length=0 star_max=114 star_gt100=2 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=1 refused_topology=0 refused_memory=0 refused_entity=0 refused_near=0 walk_tol8=0 walk_far=0 walk_no_start=0 walk_exception=0 chord_unknown=0 samples=57360 samples_unconverged=1022 outward_max=0 seconds_length=0.024997622999990199 seconds_distance=1.0146419650000098 seconds_samples=0.90896229099200809 seconds_place=0.00019406700005220046 seconds_entity=0.00027007500000308937 v_rounds=2 v_limit=0 v_collapses=1 v_splits=0 v_changes=c1s0q1,c0s0q1 bad_before_v=315 seconds_v=0.14091272200005278
S2 NATIVE3D_LOCAL_SURFACE_STATE rank=5 stage=0 splits_all=2260 faces=85170 points=42587 traces=752 revision=1178 seconds_exchange=0.012500517000034961 seconds_gather=0.27864154600001712 seconds_build=32.257813039999974 seconds_check=0.050636714999996002 modeled_peak=2104776904 rss_before=1378045952 rss_after=1378045952
```
## Event 1 engine timing and geometry census

Phase spans use each rank’s engine elapsed clock. The first construction span starts with that clock. Later construction spans begin at quality exit, and shape-quality spans end at the next repair boundary or phase exit. The final observed stage bounds an unfinished span.


| Run | Rank | Engine invocation | Phase | From | Through | Seconds |
| --- | --- | --- | --- | --- | --- | --- |
| S1 | 0 | 1 | construction | engine start | quality_entry | 209.271047 |
| S1 | 0 | 1 | quality | quality_entry | geometry_repair_entry | 205.183304 |
| S1 | 0 | 1 | geometry repair | geometry_repair_entry | quality_exit | 24.310840 |
| S1 | 0 | 1 | construction | quality_exit | quality_entry | 1.206634 |
| S1 | 0 | 1 | quality | quality_entry | exit | 63.310698 |
| S1 | 1 | 1 | construction | engine start | quality_entry | 209.271790 |
| S1 | 1 | 1 | quality | quality_entry | geometry_repair_entry | 205.182252 |
| S1 | 1 | 1 | geometry repair | geometry_repair_entry | quality_exit | 24.310923 |
| S1 | 1 | 1 | construction | quality_exit | quality_entry | 1.206611 |
| S1 | 1 | 1 | quality | quality_entry | exit | 63.311019 |
| S1 | 2 | 1 | construction | engine start | quality_entry | 209.270430 |
| S1 | 2 | 1 | quality | quality_entry | geometry_repair_entry | 205.184026 |
| S1 | 2 | 1 | geometry repair | geometry_repair_entry | quality_exit | 24.310844 |
| S1 | 2 | 1 | construction | quality_exit | quality_entry | 1.206403 |
| S1 | 2 | 1 | quality | quality_entry | exit | 63.311106 |
| S1 | 3 | 1 | construction | engine start | quality_entry | 209.268283 |
| S1 | 3 | 1 | quality | quality_entry | geometry_repair_entry | 205.185466 |
| S1 | 3 | 1 | geometry repair | geometry_repair_entry | quality_exit | 24.310726 |
| S1 | 3 | 1 | construction | quality_exit | quality_entry | 1.206575 |
| S1 | 3 | 1 | quality | quality_entry | exit | 63.310348 |
| S1 | 4 | 1 | construction | engine start | quality_entry | 209.270205 |
| S1 | 4 | 1 | quality | quality_entry | geometry_repair_entry | 205.183599 |
| S1 | 4 | 1 | geometry repair | geometry_repair_entry | quality_exit | 24.310729 |
| S1 | 4 | 1 | construction | quality_exit | quality_entry | 1.206591 |
| S1 | 4 | 1 | quality | quality_entry | exit | 63.310657 |
| S1 | 5 | 1 | construction | engine start | quality_entry | 209.268886 |
| S1 | 5 | 1 | quality | quality_entry | geometry_repair_entry | 205.184827 |
| S1 | 5 | 1 | geometry repair | geometry_repair_entry | quality_exit | 24.310701 |
| S1 | 5 | 1 | construction | quality_exit | quality_entry | 1.206745 |
| S1 | 5 | 1 | quality | quality_entry | exit | 63.310426 |
| S2 | 0 | 1 | construction | engine start | quality_entry | 209.311838 |
| S2 | 0 | 1 | quality | quality_entry | geometry_repair_entry | 204.859869 |
| S2 | 0 | 1 | geometry repair | geometry_repair_entry | quality_exit | 24.400904 |
| S2 | 0 | 1 | construction | quality_exit | quality_entry | 1.214123 |
| S2 | 0 | 1 | quality | quality_entry | exit | 63.334594 |
| S2 | 1 | 1 | construction | engine start | quality_entry | 209.312879 |
| S2 | 1 | 1 | quality | quality_entry | geometry_repair_entry | 204.858366 |
| S2 | 1 | 1 | geometry repair | geometry_repair_entry | quality_exit | 24.400884 |
| S2 | 1 | 1 | construction | quality_exit | quality_entry | 1.214193 |
| S2 | 1 | 1 | quality | quality_entry | exit | 63.334361 |
| S2 | 2 | 1 | construction | engine start | quality_entry | 209.311599 |
| S2 | 2 | 1 | quality | quality_entry | geometry_repair_entry | 204.860029 |
| S2 | 2 | 1 | geometry repair | geometry_repair_entry | quality_exit | 24.400831 |
| S2 | 2 | 1 | construction | quality_exit | quality_entry | 1.214054 |
| S2 | 2 | 1 | quality | quality_entry | exit | 63.334889 |
| S2 | 3 | 1 | construction | engine start | quality_entry | 209.309687 |
| S2 | 3 | 1 | quality | quality_entry | geometry_repair_entry | 204.861280 |
| S2 | 3 | 1 | geometry repair | geometry_repair_entry | quality_exit | 24.400696 |
| S2 | 3 | 1 | construction | quality_exit | quality_entry | 1.214154 |
| S2 | 3 | 1 | quality | quality_entry | exit | 63.334121 |
| S2 | 4 | 1 | construction | engine start | quality_entry | 209.310467 |
| S2 | 4 | 1 | quality | quality_entry | geometry_repair_entry | 204.860608 |
| S2 | 4 | 1 | geometry repair | geometry_repair_entry | quality_exit | 24.400702 |
| S2 | 4 | 1 | construction | quality_exit | quality_entry | 1.214447 |
| S2 | 4 | 1 | quality | quality_entry | exit | 63.334346 |
| S2 | 5 | 1 | construction | engine start | quality_entry | 209.310309 |
| S2 | 5 | 1 | quality | quality_entry | geometry_repair_entry | 204.860812 |
| S2 | 5 | 1 | geometry repair | geometry_repair_entry | quality_exit | 24.400837 |
| S2 | 5 | 1 | construction | quality_exit | quality_entry | 1.214653 |
| S2 | 5 | 1 | quality | quality_entry | exit | 63.333435 |
| CORE | 0 | 1 | construction | engine start | quality_entry | 494.520270 |
| CORE | 0 | 1 | quality | quality_entry | size_repair_entry | 821.724445 |
| CORE | 0 | 1 | size repair | size_repair_entry | geometry_repair_entry | 0.059179 |
| CORE | 0 | 1 | geometry repair | geometry_repair_entry | quality_exit | 0.823873 |
| CORE | 0 | 1 | construction | quality_exit | quality_entry | 1.124699 |
| CORE | 0 | 1 | quality | quality_entry | geometry_repair_entry | 5.936395 |
| CORE | 0 | 1 | geometry repair | geometry_repair_entry | quality_exit | 0.437309 |
| CORE | 0 | 1 | construction | quality_exit | quality_entry | 1.104112 |
| CORE | 0 | 1 | quality | quality_entry | geometry_repair_entry | 0.080002 |
| CORE | 0 | 1 | geometry repair | geometry_repair_entry | quality_exit | 1.941223 |
| CORE | 0 | 1 | construction | quality_exit | quality_entry | 1.084915 |
| CORE | 0 | 1 | quality | quality_entry | geometry_repair_entry | 0.057677 |
| CORE | 0 | 1 | geometry repair | geometry_repair_entry | quality_exit | 0.731824 |
| CORE | 0 | 1 | construction | quality_exit | quality_entry | 1.101177 |
| CORE | 0 | 1 | quality | quality_entry | geometry_repair_entry | 0.319042 |
| CORE | 0 | 1 | geometry repair | geometry_repair_entry | quality_exit | 0.512201 |
| CORE | 0 | 1 | construction | quality_exit | quality_entry | 2.206563 |
| CORE | 0 | 1 | quality | quality_entry | exit | 81.052216 |
| CORE | 1 | 1 | construction | engine start | quality_entry | 494.523914 |
| CORE | 1 | 1 | quality | quality_entry | size_repair_entry | 821.721736 |
| CORE | 1 | 1 | size repair | size_repair_entry | geometry_repair_entry | 0.059309 |
| CORE | 1 | 1 | geometry repair | geometry_repair_entry | quality_exit | 0.823783 |
| CORE | 1 | 1 | construction | quality_exit | quality_entry | 1.124552 |
| CORE | 1 | 1 | quality | quality_entry | geometry_repair_entry | 5.936329 |
| CORE | 1 | 1 | geometry repair | geometry_repair_entry | quality_exit | 0.437401 |
| CORE | 1 | 1 | construction | quality_exit | quality_entry | 1.104136 |
| CORE | 1 | 1 | quality | quality_entry | geometry_repair_entry | 0.079769 |
| CORE | 1 | 1 | geometry repair | geometry_repair_entry | quality_exit | 1.941454 |
| CORE | 1 | 1 | construction | quality_exit | quality_entry | 1.084875 |
| CORE | 1 | 1 | quality | quality_entry | geometry_repair_entry | 0.057962 |
| CORE | 1 | 1 | geometry repair | geometry_repair_entry | quality_exit | 0.731583 |
| CORE | 1 | 1 | construction | quality_exit | quality_entry | 1.101117 |
| CORE | 1 | 1 | quality | quality_entry | geometry_repair_entry | 0.318909 |
| CORE | 1 | 1 | geometry repair | geometry_repair_entry | quality_exit | 0.512343 |
| CORE | 1 | 1 | construction | quality_exit | quality_entry | 2.206423 |
| CORE | 1 | 1 | quality | quality_entry | exit | 81.053386 |
| CORE | 2 | 1 | construction | engine start | quality_entry | 494.520171 |
| CORE | 2 | 1 | quality | quality_entry | size_repair_entry | 821.724967 |
| CORE | 2 | 1 | size repair | size_repair_entry | geometry_repair_entry | 0.059232 |
| CORE | 2 | 1 | geometry repair | geometry_repair_entry | quality_exit | 0.823813 |
| CORE | 2 | 1 | construction | quality_exit | quality_entry | 1.124638 |
| CORE | 2 | 1 | quality | quality_entry | geometry_repair_entry | 5.936229 |
| CORE | 2 | 1 | geometry repair | geometry_repair_entry | quality_exit | 0.437335 |
| CORE | 2 | 1 | construction | quality_exit | quality_entry | 1.104062 |
| CORE | 2 | 1 | quality | quality_entry | geometry_repair_entry | 0.079995 |
| CORE | 2 | 1 | geometry repair | geometry_repair_entry | quality_exit | 1.941257 |
| CORE | 2 | 1 | construction | quality_exit | quality_entry | 1.084975 |
| CORE | 2 | 1 | quality | quality_entry | geometry_repair_entry | 0.057767 |
| CORE | 2 | 1 | geometry repair | geometry_repair_entry | quality_exit | 0.731733 |
| CORE | 2 | 1 | construction | quality_exit | quality_entry | 1.101222 |
| CORE | 2 | 1 | quality | quality_entry | geometry_repair_entry | 0.318841 |
| CORE | 2 | 1 | geometry repair | geometry_repair_entry | quality_exit | 0.512266 |
| CORE | 2 | 1 | construction | quality_exit | quality_entry | 2.206583 |
| CORE | 2 | 1 | quality | quality_entry | exit | 81.052333 |
| CORE | 3 | 1 | construction | engine start | quality_entry | 494.520785 |
| CORE | 3 | 1 | quality | quality_entry | size_repair_entry | 821.723908 |
| CORE | 3 | 1 | size repair | size_repair_entry | geometry_repair_entry | 0.059107 |
| CORE | 3 | 1 | geometry repair | geometry_repair_entry | quality_exit | 0.823691 |
| CORE | 3 | 1 | construction | quality_exit | quality_entry | 1.124365 |
| CORE | 3 | 1 | quality | quality_entry | geometry_repair_entry | 5.938534 |
| CORE | 3 | 1 | geometry repair | geometry_repair_entry | quality_exit | 0.435735 |
| CORE | 3 | 1 | construction | quality_exit | quality_entry | 1.103758 |
| CORE | 3 | 1 | quality | quality_entry | geometry_repair_entry | 0.080365 |
| CORE | 3 | 1 | geometry repair | geometry_repair_entry | quality_exit | 1.941130 |
| CORE | 3 | 1 | construction | quality_exit | quality_entry | 1.084827 |
| CORE | 3 | 1 | quality | quality_entry | geometry_repair_entry | 0.058511 |
| CORE | 3 | 1 | geometry repair | geometry_repair_entry | quality_exit | 0.731189 |
| CORE | 3 | 1 | construction | quality_exit | quality_entry | 1.101129 |
| CORE | 3 | 1 | quality | quality_entry | geometry_repair_entry | 0.319196 |
| CORE | 3 | 1 | geometry repair | geometry_repair_entry | quality_exit | 0.512093 |
| CORE | 3 | 1 | construction | quality_exit | quality_entry | 2.206229 |
| CORE | 3 | 1 | quality | quality_entry | exit | 81.052555 |
| CORE | 4 | 1 | construction | engine start | quality_entry | 494.523341 |
| CORE | 4 | 1 | quality | quality_entry | size_repair_entry | 821.720952 |
| CORE | 4 | 1 | size repair | size_repair_entry | geometry_repair_entry | 0.059176 |
| CORE | 4 | 1 | geometry repair | geometry_repair_entry | quality_exit | 0.824033 |
| CORE | 4 | 1 | construction | quality_exit | quality_entry | 1.124790 |
| CORE | 4 | 1 | quality | quality_entry | geometry_repair_entry | 5.936291 |
| CORE | 4 | 1 | geometry repair | geometry_repair_entry | quality_exit | 0.437237 |
| CORE | 4 | 1 | construction | quality_exit | quality_entry | 1.104091 |
| CORE | 4 | 1 | quality | quality_entry | geometry_repair_entry | 0.079816 |
| CORE | 4 | 1 | geometry repair | geometry_repair_entry | quality_exit | 1.941366 |
| CORE | 4 | 1 | construction | quality_exit | quality_entry | 1.084837 |
| CORE | 4 | 1 | quality | quality_entry | geometry_repair_entry | 0.058097 |
| CORE | 4 | 1 | geometry repair | geometry_repair_entry | quality_exit | 0.731589 |
| CORE | 4 | 1 | construction | quality_exit | quality_entry | 1.100825 |
| CORE | 4 | 1 | quality | quality_entry | geometry_repair_entry | 0.319015 |
| CORE | 4 | 1 | geometry repair | geometry_repair_entry | quality_exit | 0.512547 |
| CORE | 4 | 1 | construction | quality_exit | quality_entry | 2.206533 |
| CORE | 4 | 1 | quality | quality_entry | exit | 81.052215 |
| CORE | 5 | 1 | construction | engine start | quality_entry | 494.523172 |
| CORE | 5 | 1 | quality | quality_entry | size_repair_entry | 821.721804 |
| CORE | 5 | 1 | size repair | size_repair_entry | geometry_repair_entry | 0.059198 |
| CORE | 5 | 1 | geometry repair | geometry_repair_entry | quality_exit | 0.823977 |
| CORE | 5 | 1 | construction | quality_exit | quality_entry | 1.124544 |
| CORE | 5 | 1 | quality | quality_entry | geometry_repair_entry | 5.936433 |
| CORE | 5 | 1 | geometry repair | geometry_repair_entry | quality_exit | 0.437388 |
| CORE | 5 | 1 | construction | quality_exit | quality_entry | 1.103954 |
| CORE | 5 | 1 | quality | quality_entry | geometry_repair_entry | 0.080011 |
| CORE | 5 | 1 | geometry repair | geometry_repair_entry | quality_exit | 1.941392 |
| CORE | 5 | 1 | construction | quality_exit | quality_entry | 1.084808 |
| CORE | 5 | 1 | quality | quality_entry | geometry_repair_entry | 0.057875 |
| CORE | 5 | 1 | geometry repair | geometry_repair_entry | quality_exit | 0.731780 |
| CORE | 5 | 1 | construction | quality_exit | quality_entry | 1.101005 |
| CORE | 5 | 1 | quality | quality_entry | geometry_repair_entry | 0.318991 |
| CORE | 5 | 1 | geometry repair | geometry_repair_entry | quality_exit | 0.512366 |
| CORE | 5 | 1 | construction | quality_exit | quality_entry | 2.206415 |
| CORE | 5 | 1 | quality | quality_entry | exit | 81.052636 |
| S1 build627 | 0 | 1 | construction | engine start | quality_entry | 201.230645 |
| S1 build627 | 0 | 1 | quality | quality_entry | exit | 5122.422152 |
| S1 build627 | 1 | 1 | construction | engine start | quality_entry | 201.232986 |
| S1 build627 | 1 | 1 | quality | quality_entry | exit | 5122.435189 |
| S1 build627 | 2 | 1 | construction | engine start | quality_entry | 201.230002 |
| S1 build627 | 2 | 1 | quality | quality_entry | exit | 5122.440006 |
| S1 build627 | 3 | 1 | construction | engine start | quality_entry | 201.227171 |
| S1 build627 | 3 | 1 | quality | quality_entry | exit | 5122.426209 |
| S1 build627 | 4 | 1 | construction | engine start | quality_entry | 201.230478 |
| S1 build627 | 4 | 1 | quality | quality_entry | exit | 5122.422752 |
| S1 build627 | 5 | 1 | construction | engine start | quality_entry | 201.228869 |
| S1 build627 | 5 | 1 | quality | quality_entry | exit | 5122.424780 |

Geometry bins count geometry-bad cells by worst relative excess, using boundaries 0.01, 0.1, 1 and 10, with a sixth bin for nonfinite or unresolved values. The engine emits histograms for complete improvement censuses. The length-collapse exit has counts without bins.

| Run | Rank | Stage | Engine seconds | Geometry bad | q bad | J bad | Geometry bins |
| --- | --- | --- | --- | --- | --- | --- | --- |
| S1 | 0 | length_collapse_exit | 160.94331091199999 | 24 | — | — | — |
| S1 | 0 | quality_entry | 209.27104659 | 47 | 834 | 750 | 4,4,27,12,0,0 |
| S1 | 0 | geometry_repair_entry | 414.45435047500001 | 34 | 0 | 0 | 6,2,23,3,0,0 |
| S1 | 0 | quality_entry | 439.97182413899998 | 0 | 3 | 3 | 0,0,0,0,0,0 |
| S1 | 1 | length_collapse_exit | 160.943338936 | 10 | — | — | — |
| S1 | 1 | quality_entry | 209.271790028 | 3 | 984 | 938 | 0,0,2,1,0,0 |
| S1 | 1 | geometry_repair_entry | 414.454041539 | 5 | 0 | 0 | 2,0,3,0,0,0 |
| S1 | 1 | quality_entry | 439.97157575400001 | 0 | 0 | 0 | 0,0,0,0,0,0 |
| S1 | 2 | length_collapse_exit | 160.94331497300001 | 0 | — | — | — |
| S1 | 2 | quality_entry | 209.27042964099999 | 18 | 505 | 588 | 0,7,5,6,0,0 |
| S1 | 2 | geometry_repair_entry | 414.45445565 | 12 | 0 | 0 | 0,7,3,2,0,0 |
| S1 | 2 | quality_entry | 439.97170335800001 | 0 | 0 | 0 | 0,0,0,0,0,0 |
| S1 | 3 | length_collapse_exit | 160.943299104 | 17 | — | — | — |
| S1 | 3 | quality_entry | 209.268282949 | 43 | 415 | 443 | 0,6,28,9,0,0 |
| S1 | 3 | geometry_repair_entry | 414.45374911099998 | 27 | 0 | 0 | 0,5,17,5,0,0 |
| S1 | 3 | quality_entry | 439.97105053500002 | 2 | 2 | 2 | 0,2,0,0,0,0 |
| S1 | 4 | length_collapse_exit | 160.94331680299999 | 32 | — | — | — |
| S1 | 4 | quality_entry | 209.27020467400001 | 4 | 479 | 446 | 0,0,2,2,0,0 |
| S1 | 4 | geometry_repair_entry | 414.453803843 | 4 | 0 | 0 | 0,0,2,2,0,0 |
| S1 | 4 | quality_entry | 439.97112445400001 | 0 | 0 | 0 | 0,0,0,0,0,0 |
| S1 | 5 | length_collapse_exit | 160.943295686 | 217 | — | — | — |
| S1 | 5 | quality_entry | 209.268886471 | 67 | 479 | 557 | 0,3,58,6,0,0 |
| S1 | 5 | geometry_repair_entry | 414.45371321699997 | 58 | 0 | 0 | 0,4,48,6,0,0 |
| S1 | 5 | quality_entry | 439.97115954100002 | 0 | 3 | 6 | 0,0,0,0,0,0 |
| S2 | 0 | length_collapse_exit | 160.68009347399999 | 24 | — | — | — |
| S2 | 0 | quality_entry | 209.31183813199999 | 47 | 834 | 750 | 4,4,27,12,0,0 |
| S2 | 0 | geometry_repair_entry | 414.171707223 | 34 | 0 | 0 | 6,2,23,3,0,0 |
| S2 | 0 | quality_entry | 439.78673392399998 | 0 | 3 | 3 | 0,0,0,0,0,0 |
| S2 | 1 | length_collapse_exit | 160.68009414400001 | 10 | — | — | — |
| S2 | 1 | quality_entry | 209.312879288 | 3 | 984 | 938 | 0,0,2,1,0,0 |
| S2 | 1 | geometry_repair_entry | 414.17124481600001 | 5 | 0 | 0 | 2,0,3,0,0,0 |
| S2 | 1 | quality_entry | 439.78632140899998 | 0 | 0 | 0 | 0,0,0,0,0,0 |
| S2 | 2 | length_collapse_exit | 160.68013953600001 | 0 | — | — | — |
| S2 | 2 | quality_entry | 209.31159862800001 | 18 | 505 | 588 | 0,7,5,6,0,0 |
| S2 | 2 | geometry_repair_entry | 414.17162764800003 | 12 | 0 | 0 | 0,7,3,2,0,0 |
| S2 | 2 | quality_entry | 439.78651276699998 | 0 | 0 | 0 | 0,0,0,0,0,0 |
| S2 | 3 | length_collapse_exit | 160.68014013499999 | 17 | — | — | — |
| S2 | 3 | quality_entry | 209.30968696299999 | 43 | 415 | 443 | 0,6,28,9,0,0 |
| S2 | 3 | geometry_repair_entry | 414.17096681800001 | 27 | 0 | 0 | 0,5,17,5,0,0 |
| S2 | 3 | quality_entry | 439.78581607400002 | 2 | 2 | 2 | 0,2,0,0,0,0 |
| S2 | 4 | length_collapse_exit | 160.68012864799999 | 32 | — | — | — |
| S2 | 4 | quality_entry | 209.31046660600001 | 4 | 479 | 446 | 0,0,2,2,0,0 |
| S2 | 4 | geometry_repair_entry | 414.17107501700002 | 4 | 0 | 0 | 0,0,2,2,0,0 |
| S2 | 4 | quality_entry | 439.78622417899999 | 0 | 0 | 0 | 0,0,0,0,0,0 |
| S2 | 5 | length_collapse_exit | 160.680178775 | 217 | — | — | — |
| S2 | 5 | quality_entry | 209.310308945 | 67 | 479 | 557 | 0,3,58,6,0,0 |
| S2 | 5 | geometry_repair_entry | 414.17112140099999 | 58 | 0 | 0 | 0,4,48,6,0,0 |
| S2 | 5 | quality_entry | 439.78661137199998 | 0 | 3 | 6 | 0,0,0,0,0,0 |
| CORE | 0 | length_collapse_exit | 171.616185153 | 461 | — | — | — |
| CORE | 0 | quality_entry | 494.52027001099998 | 8 | 1836 | 2012 | 0,1,7,0,0,0 |
| CORE | 0 | geometry_repair_entry | 1316.30389461 | 1 | 0 | 0 | 0,0,1,0,0,0 |
| CORE | 0 | quality_entry | 1318.252466424 | 1 | 1 | 1 | 0,0,1,0,0,0 |
| CORE | 0 | quality_entry | 1325.730282043 | 2 | 1 | 1 | 0,0,2,0,0,0 |
| CORE | 0 | quality_entry | 1328.83642203 | 2 | 0 | 0 | 0,0,2,0,0,0 |
| CORE | 0 | quality_entry | 1330.727101061 | 2 | 1 | 1 | 0,0,2,0,0,0 |
| CORE | 0 | quality_entry | 1333.7649074850001 | 0 | 3 | 1 | 0,0,0,0,0,0 |
| CORE | 1 | length_collapse_exit | 171.61617103699999 | 411 | — | — | — |
| CORE | 1 | quality_entry | 494.52391438900003 | 3 | 2500 | 2820 | 0,3,0,0,0,0 |
| CORE | 1 | geometry_repair_entry | 1316.3049585809999 | 1 | 0 | 0 | 0,1,0,0,0,0 |
| CORE | 1 | quality_entry | 1318.25329293 | 0 | 0 | 0 | 0,0,0,0,0,0 |
| CORE | 1 | quality_entry | 1325.7311592860001 | 0 | 0 | 0 | 0,0,0,0,0,0 |
| CORE | 1 | quality_entry | 1328.8372571519999 | 0 | 0 | 0 | 0,0,0,0,0,0 |
| CORE | 1 | quality_entry | 1330.7279198819999 | 0 | 0 | 0 | 0,0,0,0,0,0 |
| CORE | 1 | quality_entry | 1333.7655957060001 | 0 | 0 | 0 | 0,0,0,0,0,0 |
| CORE | 2 | length_collapse_exit | 171.61610838499999 | 744 | — | — | — |
| CORE | 2 | quality_entry | 494.52017057299997 | 10 | 1636 | 1801 | 2,1,7,0,0,0 |
| CORE | 2 | geometry_repair_entry | 1316.304369426 | 1 | 0 | 0 | 0,0,1,0,0,0 |
| CORE | 2 | quality_entry | 1318.252820034 | 1 | 1 | 1 | 0,0,1,0,0,0 |
| CORE | 2 | quality_entry | 1325.7304464409999 | 0 | 0 | 0 | 0,0,0,0,0,0 |
| CORE | 2 | quality_entry | 1328.836673836 | 0 | 0 | 0 | 0,0,0,0,0,0 |
| CORE | 2 | quality_entry | 1330.7273961139999 | 0 | 0 | 0 | 0,0,0,0,0,0 |
| CORE | 2 | quality_entry | 1333.7650866179999 | 1 | 1 | 0 | 0,0,1,0,0,0 |
| CORE | 3 | length_collapse_exit | 171.61610988300001 | 601 | — | — | — |
| CORE | 3 | quality_entry | 494.52078515300002 | 10 | 1783 | 2000 | 0,5,5,0,0,0 |
| CORE | 3 | geometry_repair_entry | 1316.303799795 | 1 | 0 | 0 | 0,1,0,0,0,0 |
| CORE | 3 | quality_entry | 1318.251855662 | 0 | 0 | 0 | 0,0,0,0,0,0 |
| CORE | 3 | quality_entry | 1325.729882528 | 0 | 0 | 0 | 0,0,0,0,0,0 |
| CORE | 3 | quality_entry | 1328.8362042629999 | 0 | 0 | 0 | 0,0,0,0,0,0 |
| CORE | 3 | quality_entry | 1330.727033545 | 0 | 0 | 0 | 0,0,0,0,0,0 |
| CORE | 3 | quality_entry | 1333.7645513079999 | 0 | 0 | 0 | 0,0,0,0,0,0 |
| CORE | 4 | length_collapse_exit | 171.61614730900001 | 339 | — | — | — |
| CORE | 4 | quality_entry | 494.52334140099998 | 1 | 2807 | 3476 | 0,0,1,0,0,0 |
| CORE | 4 | geometry_repair_entry | 1316.303469789 | 1 | 0 | 0 | 0,0,1,0,0,0 |
| CORE | 4 | quality_entry | 1318.2522922329999 | 1 | 9 | 7 | 0,0,1,0,0,0 |
| CORE | 4 | quality_entry | 1325.7299111560001 | 0 | 0 | 0 | 0,0,0,0,0,0 |
| CORE | 4 | quality_entry | 1328.83593034 | 0 | 0 | 0 | 0,0,0,0,0,0 |
| CORE | 4 | quality_entry | 1330.726441673 | 0 | 0 | 0 | 0,0,0,0,0,0 |
| CORE | 4 | quality_entry | 1333.7645363829999 | 1 | 1 | 1 | 0,0,0,1,0,0 |
| CORE | 5 | length_collapse_exit | 171.61613899 | 416 | — | — | — |
| CORE | 5 | quality_entry | 494.52317209199998 | 0 | 2573 | 2922 | 0,0,0,0,0,0 |
| CORE | 5 | geometry_repair_entry | 1316.3041747029999 | 0 | 0 | 0 | 0,0,0,0,0,0 |
| CORE | 5 | quality_entry | 1318.2526959689999 | 0 | 0 | 0 | 0,0,0,0,0,0 |
| CORE | 5 | quality_entry | 1325.7304708669999 | 0 | 0 | 0 | 0,0,0,0,0,0 |
| CORE | 5 | quality_entry | 1328.8366814230001 | 0 | 0 | 0 | 0,0,0,0,0,0 |
| CORE | 5 | quality_entry | 1330.7273406679999 | 0 | 0 | 0 | 0,0,0,0,0,0 |
| CORE | 5 | quality_entry | 1333.7651124500001 | 0 | 0 | 0 | 0,0,0,0,0,0 |
| S1 build627 | 0 | length_collapse_exit | 146.534471086 | 26 | — | — | — |
| S1 build627 | 0 | quality_entry | 201.23064484 | 28 | 2624 | 2795 | 0,1,6,21,0,0 |
| S1 build627 | 1 | length_collapse_exit | 146.534478963 | 7 | — | — | — |
| S1 build627 | 1 | quality_entry | 201.23298634099999 | 6 | 3359 | 3620 | 0,0,1,5,0,0 |
| S1 build627 | 2 | length_collapse_exit | 146.534531367 | 5 | — | — | — |
| S1 build627 | 2 | quality_entry | 201.23000159399999 | 16 | 2411 | 2743 | 0,0,2,14,0,0 |
| S1 build627 | 3 | length_collapse_exit | 146.534470984 | 8 | — | — | — |
| S1 build627 | 3 | quality_entry | 201.227171164 | 20 | 2414 | 2582 | 0,0,3,17,0,0 |
| S1 build627 | 4 | length_collapse_exit | 146.534529112 | 3 | — | — | — |
| S1 build627 | 4 | quality_entry | 201.23047834299999 | 4 | 2499 | 2788 | 0,0,1,3,0,0 |
| S1 build627 | 5 | length_collapse_exit | 146.53453798199999 | 11 | — | — | — |
| S1 build627 | 5 | quality_entry | 201.22886935700001 | 15 | 2505 | 2911 | 1,0,1,13,0,0 |

| Run | Rank | First LENGTH minimum | Length collapses | Committed ownership rows |
| --- | --- | --- | --- | --- |
| S1 | 0 | 0.020372955551597037 | 745 | 1 |
| S1 | 1 | 0.026253971902465189 | 673 | 1 |
| S1 | 2 | 0.046970869981073777 | 803 | 1 |
| S1 | 3 | 0.027261040654623701 | 1410 | 1 |
| S1 | 4 | 0.042235282858877916 | 573 | 1 |
| S1 | 5 | 0.042003012336116365 | 629 | 1 |
| S2 | 0 | 0.020372955551597037 | 745 | 1 |
| S2 | 1 | 0.026253971902465189 | 673 | 1 |
| S2 | 2 | 0.046970869981073777 | 803 | 1 |
| S2 | 3 | 0.027261040654623701 | 1410 | 1 |
| S2 | 4 | 0.042235282858877916 | 573 | 1 |
| S2 | 5 | 0.042003012336116365 | 629 | 1 |
| CORE | 0 | 0.057871110721358392 | 465 | 2 |
| CORE | 1 | 0.038592715927605226 | 459 | 2 |
| CORE | 2 | 0.047282540091361975 | 428 | 2 |
| CORE | 3 | 0.049945221461337114 | 573 | 2 |
| CORE | 4 | 0.042235282858877916 | 451 | 2 |
| CORE | 5 | 0.056263897611721882 | 445 | 2 |
| S1 build627 | 0 | 1.763040275738747e-05 | 603 | 11 |
| S1 build627 | 1 | 0.0068544928561841776 | 577 | 11 |
| S1 build627 | 2 | 5.4527443514081955e-10 | 780 | 11 |
| S1 build627 | 3 | 0.003646617255834537 | 1394 | 11 |
| S1 build627 | 4 | 0.018250379906111053 | 555 | 11 |
| S1 build627 | 5 | 0.0086529686067382194 | 578 | 11 |

| Run | Rank | Stage | Pass | Short out | Wall-near points |
| --- | --- | --- | --- | --- | --- |
| S1 | 0 | 0 | 1 | 57392 | 3145 |
| S1 | 0 | 0 | 2 | 70317 | 3646 |
| S1 | 1 | 0 | 1 | 54167 | 2601 |
| S1 | 1 | 0 | 2 | 57307 | 2704 |
| S1 | 2 | 0 | 1 | 44607 | 3717 |
| S1 | 2 | 0 | 2 | 53125 | 4503 |
| S1 | 3 | 0 | 1 | 76905 | 12018 |
| S1 | 3 | 0 | 2 | 73940 | 11768 |
| S1 | 4 | 0 | 1 | 43264 | 2259 |
| S1 | 4 | 0 | 2 | 38770 | 1980 |
| S1 | 5 | 0 | 1 | 48523 | 2658 |
| S1 | 5 | 0 | 2 | 43464 | 2213 |
| S2 | 0 | 0 | 1 | 57392 | 3145 |
| S2 | 0 | 0 | 2 | 70317 | 3646 |
| S2 | 1 | 0 | 1 | 54167 | 2601 |
| S2 | 1 | 0 | 2 | 57307 | 2704 |
| S2 | 2 | 0 | 1 | 44607 | 3717 |
| S2 | 2 | 0 | 2 | 53125 | 4503 |
| S2 | 3 | 0 | 1 | 76905 | 12018 |
| S2 | 3 | 0 | 2 | 73940 | 11768 |
| S2 | 4 | 0 | 1 | 43264 | 2259 |
| S2 | 4 | 0 | 2 | 38770 | 1980 |
| S2 | 5 | 0 | 1 | 48523 | 2658 |
| S2 | 5 | 0 | 2 | 43464 | 2213 |
| CORE | 0 | 0 | 1 | 40982 | 2846 |
| CORE | 0 | 0 | 2 | 43897 | 3153 |
| CORE | 1 | 0 | 1 | 38541 | 2337 |
| CORE | 1 | 0 | 2 | 39835 | 2408 |
| CORE | 2 | 0 | 1 | 32414 | 2400 |
| CORE | 2 | 0 | 2 | 36316 | 2652 |
| CORE | 3 | 0 | 1 | 33086 | 3545 |
| CORE | 3 | 0 | 2 | 30729 | 3328 |
| CORE | 4 | 0 | 1 | 38452 | 2263 |
| CORE | 4 | 0 | 2 | 34448 | 1993 |
| CORE | 5 | 0 | 1 | 36996 | 2496 |
| CORE | 5 | 0 | 2 | 32216 | 2063 |
| S1 build627 | 0 | 0 | 1 | 57397 | 2845 |
| S1 build627 | 0 | 0 | 2 | 63408 | 3147 |
| S1 build627 | 1 | 0 | 1 | 50100 | 2331 |
| S1 build627 | 1 | 0 | 2 | 51822 | 2400 |
| S1 build627 | 2 | 0 | 1 | 45830 | 3803 |
| S1 build627 | 2 | 0 | 2 | 52739 | 4405 |
| S1 build627 | 3 | 0 | 1 | 77696 | 12242 |
| S1 build627 | 3 | 0 | 2 | 73885 | 11948 |
| S1 build627 | 4 | 0 | 1 | 43927 | 2263 |
| S1 build627 | 4 | 0 | 2 | 38881 | 1994 |
| S1 build627 | 5 | 0 | 1 | 50648 | 2495 |
| S1 build627 | 5 | 0 | 2 | 44549 | 2060 |

## Closure

FP2-B is accepted on build640. Both binaries and their tracked source tree were retained from `ecaed957db`. The source and binary verification and terminal process audit precede the closing checkpoint. [Terminal audit](../terminal_audit_continue_1.json)
