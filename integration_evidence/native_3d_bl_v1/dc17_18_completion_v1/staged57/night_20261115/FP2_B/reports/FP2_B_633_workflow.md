# FP2-B workflow comparison for build633

Each row uses the final remesh timer ledger for that rank. Partial evidence lists only observed values. Times are seconds and memory is bytes.

R4 marks exact engine surface length splits, geometry splits, and first above-tolerance face counts unavailable. The engine rows give all-lane length splits, surface-lane rounds, mixed-edit wall-lane commits, and geometry-invalid cells at the first emitted complete geometry census.

- **CORE** not observed.
- **S1** failed. Stage full event gates. full run did not end normally with exit 0 (stop=f3b_event2_timer_gate_failed, exit=-15)
- **S2** not observed.

## Event 1

| Case | Native max | Cells | Points | Surface faces | Min coverage % | Job peak | Builds observed |
| --- | --- | --- | --- | --- | --- | --- | --- |
| CORE | — | — | — | — | — | — | — |
| S1 | 1667.908 | — | — | — | 99.2429 (6 ranks) | 13032607744 | 1 |
| S2 | — | — | — | — | — | — | — |

| Case | Rank | Native | preparation | construction | quality | final_proof | transfer | topology | geometry_bound | mobility_policy | communication | Process peak | Builds observed |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| S1 | 0 | 1667.854 | 91.266 | 323.519 | 1147.559 | 63.052 | 31.000 | 30.683 | 62.816 | 16.802 | 183.032 | 1984294912 | 1 |
| S1 | 1 | 1667.831 | 91.306 | 323.544 | 1147.558 | 63.052 | 31.002 | 29.536 | 47.268 | 16.827 | 201.620 | 1960472576 | 1 |
| S1 | 2 | 1667.835 | 91.109 | 323.517 | 1147.561 | 63.052 | 31.006 | 42.039 | 10.008 | 16.843 | 213.396 | 2174361600 | 1 |
| S1 | 3 | 1667.830 | 90.740 | 323.522 | 1147.559 | 63.052 | 31.005 | 61.054 | 19.168 | 16.793 | 153.535 | 3145752576 | 1 |
| S1 | 4 | 1667.853 | 91.264 | 323.519 | 1147.559 | 63.052 | 31.001 | 27.961 | 13.863 | 16.784 | 240.493 | 1905041408 | 1 |
| S1 | 5 | 1667.908 | 91.225 | 323.517 | 1147.561 | 63.052 | 31.005 | 24.769 | 45.025 | 16.871 | 211.269 | 1862684672 | 1 |

| Case | Rank | Length splits by engine exit | Surface rounds by engine exit | Construction wall commits | Quality wall commits | First census stage emitted | Geometry-invalid cells |
| --- | --- | --- | --- | --- | --- | --- | --- |
| S1 | 0 | 1155 | 1379 | 1711 | 2619 | length_collapse_exit | 535 |
| S1 | 1 | 782 | 1379 | 1256 | 2609 | length_collapse_exit | 352 |
| S1 | 2 | 1201 | 1379 | 852 | 3839 | length_collapse_exit | 0 |
| S1 | 3 | 751 | 1379 | 927 | 3430 | length_collapse_exit | 112 |
| S1 | 4 | 74 | 1379 | 539 | 1864 | length_collapse_exit | 32 |
| S1 | 5 | 645 | 1379 | 1134 | 5065 | length_collapse_exit | 313 |

### Surface rows

```text
S1 NATIVE3D_LOCAL_SURFACE rank=0 stage=0 pass=1 applicable=1 faces_in=9423 faces_out=12725 points_in=4894 points_out=6545 splits_length=0 splits_distance=1651 splits_feature=13 sweeps_length=0 sweeps_distance=9 left_long=0 left_border=55 left_distance=771 left_all_feature=0 left_refused=84 left_limit=0 left_short=0 left_neighbor_locked=632 waits_neighbor=885 path_splits=146 path_max=7 star_max=132 star_gt100=2 star_gt1000=0 refused_walk=72 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=0 refused_topology=0 refused_memory=0 refused_entity=0 chord_unknown=0 samples=64108 samples_unconverged=1701 outward_max=9.311180236387248e-07 seconds_length=0.01462410700000305 seconds_distance=1.4877020710000011 seconds_samples=0.5069871780003723 seconds_place=0.057176059999633821 seconds_entity=0.71030456800021113 v_rounds=4 v_limit=0 v_collapses=43 v_splits=142 v_changes=c35s131q20,c7s9q5,c1s2q4,c0s0q1 bad_before_v=1423 seconds_v=1.0507777010000012
S1 NATIVE3D_LOCAL_SURFACE rank=0 stage=0 pass=2 applicable=1 faces_in=13629 faces_out=15187 points_in=7066 points_out=7845 splits_length=0 splits_distance=779 splits_feature=3 sweeps_length=0 sweeps_distance=8 left_long=0 left_border=66 left_distance=1006 left_all_feature=0 left_refused=119 left_limit=0 left_short=0 left_neighbor_locked=821 waits_neighbor=1378 path_splits=48 path_max=7 star_max=136 star_gt100=9 star_gt1000=0 refused_walk=114 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=5 refused_topology=0 refused_memory=0 refused_entity=0 chord_unknown=0 samples=66980 samples_unconverged=1691 outward_max=6.0167270963416384e-06 seconds_length=0.015050114000004555 seconds_distance=1.0491858209999947 seconds_samples=0.50330937400099174 seconds_place=0.032164999000130479 seconds_entity=0.29536460599985048 v_rounds=4 v_limit=0 v_collapses=8 v_splits=77 v_changes=c3s69q16,c3s8q7,c2s0q3,c0s0q1 bad_before_v=1012 seconds_v=0.93205791600000509
S1 NATIVE3D_LOCAL_SURFACE_STATE rank=0 stage=0 splits_all=13993 faces=76940 points=38472 traces=753 revision=2 seconds_exchange=0.028991299999987064 seconds_gather=0.18365169700000195 seconds_build=31.619096882000008 seconds_check=0.036903560000013158 modeled_peak=1907631436 rss_before=570527744 rss_after=570593280
S1 NATIVE3D_LOCAL_SURFACE rank=1 stage=0 pass=1 applicable=1 faces_in=9031 faces_out=11563 points_in=4710 points_out=5976 splits_length=0 splits_distance=1266 splits_feature=0 sweeps_length=0 sweeps_distance=7 left_long=0 left_border=17 left_distance=642 left_all_feature=0 left_refused=92 left_limit=0 left_short=0 left_neighbor_locked=533 waits_neighbor=648 path_splits=187 path_max=5 star_max=88 star_gt100=0 star_gt1000=0 refused_walk=51 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=0 refused_topology=0 refused_memory=0 refused_entity=0 chord_unknown=0 samples=56380 samples_unconverged=1745 outward_max=1.4943970271539778e-11 seconds_length=0.012768366000003084 seconds_distance=1.2079259079999929 seconds_samples=0.41842250600014097 seconds_place=0.039555799000197567 seconds_entity=0.59828590800015036 v_rounds=5 v_limit=0 v_collapses=44 v_splits=83 v_changes=c32s74q12,c9s8q7,c3s0q8,c0s1q1,c0s0q1 bad_before_v=1490 seconds_v=1.2076440250000076
S1 NATIVE3D_LOCAL_SURFACE rank=1 stage=0 pass=2 applicable=1 faces_in=11915 faces_out=12109 points_in=6189 points_out=6286 splits_length=0 splits_distance=97 splits_feature=0 sweeps_length=0 sweeps_distance=7 left_long=0 left_border=7 left_distance=686 left_all_feature=0 left_refused=106 left_limit=0 left_short=0 left_neighbor_locked=573 waits_neighbor=659 path_splits=12 path_max=6 star_max=130 star_gt100=1 star_gt1000=0 refused_walk=62 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=0 refused_topology=0 refused_memory=0 refused_entity=0 chord_unknown=0 samples=49212 samples_unconverged=1532 outward_max=1.5032035086537339e-11 seconds_length=0.013623280000004456 seconds_distance=0.48248302300000034 seconds_samples=0.30330396300054474 seconds_place=0.004334894999971084 seconds_entity=0.041079787000001033 v_rounds=3 v_limit=0 v_collapses=11 v_splits=13 v_changes=c11s8q19,c0s5q3,c0s0q1 bad_before_v=746 seconds_v=0.5423218670000125
S1 NATIVE3D_LOCAL_SURFACE_STATE rank=1 stage=0 splits_all=13993 faces=76940 points=38472 traces=753 revision=2 seconds_exchange=0.028975084999999012 seconds_gather=0.18365035300000443 seconds_build=31.699233575999997 seconds_check=0.036906557000008888 modeled_peak=1908428844 rss_before=523808768 rss_after=523874304
S1 NATIVE3D_LOCAL_SURFACE rank=2 stage=0 pass=1 applicable=1 faces_in=6040 faces_out=10840 points_in=3195 points_out=5595 splits_length=557 splits_distance=1843 splits_feature=0 sweeps_length=35 sweeps_distance=9 left_long=20 left_border=60 left_distance=150 left_all_feature=0 left_refused=1 left_limit=0 left_short=0 left_neighbor_locked=102 waits_neighbor=1134 path_splits=55 path_max=21 star_max=157 star_gt100=7 star_gt1000=0 refused_walk=8 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=0 refused_topology=0 refused_memory=0 refused_entity=0 chord_unknown=0 samples=58104 samples_unconverged=1449 outward_max=1.0624059606946464e-05 seconds_length=0.37616245799999604 seconds_distance=1.7126577469999944 seconds_samples=0.61265140500000825 seconds_place=0.067021243999818125 seconds_entity=1.1972651040003086 v_rounds=3 v_limit=0 v_collapses=9 v_splits=15 v_changes=c8s14q8,c1s1q11,c0s0q1 bad_before_v=683 seconds_v=0.37153801299999145
S1 NATIVE3D_LOCAL_SURFACE rank=2 stage=0 pass=2 applicable=1 faces_in=11995 faces_out=12925 points_in=6244 points_out=6709 splits_length=16 splits_distance=449 splits_feature=0 sweeps_length=2 sweeps_distance=23 left_long=12 left_border=22 left_distance=41 left_all_feature=0 left_refused=3 left_limit=0 left_short=0 left_neighbor_locked=16 waits_neighbor=438 path_splits=29 path_max=21 star_max=326 star_gt100=8 star_gt1000=0 refused_walk=13 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=1 refused_topology=0 refused_memory=0 refused_entity=0 chord_unknown=0 samples=55292 samples_unconverged=1632 outward_max=1.906865257206128e-05 seconds_length=0.026119648999994638 seconds_distance=1.1300352549999957 seconds_samples=0.55291304000010655 seconds_place=0.018266878999909864 seconds_entity=0.20114080799987732 v_rounds=10 v_limit=1 v_collapses=34 v_splits=88 v_changes=c13s9q11,c6s1q8,c4s1q19,c3s1q7,c2s4q9,c2s4q7,c2s8q3,c1s3q15,c0s20q3,c1s37q6 bad_before_v=1271 seconds_v=4.0976185649999906
S1 NATIVE3D_LOCAL_SURFACE_STATE rank=2 stage=0 splits_all=13993 faces=76940 points=38472 traces=753 revision=2 seconds_exchange=0.028977316999998948 seconds_gather=0.18365662600000121 seconds_build=31.656720811999989 seconds_check=0.03690767099999448 modeled_peak=2624364100 rss_before=536694784 rss_after=537243648
S1 NATIVE3D_LOCAL_SURFACE rank=3 stage=0 pass=1 applicable=1 faces_in=6694 faces_out=15558 points_in=3448 points_out=7880 splits_length=2214 splits_distance=2218 splits_feature=35 sweeps_length=36 sweeps_distance=8 left_long=18 left_border=27 left_distance=225 left_all_feature=0 left_refused=19 left_limit=0 left_short=0 left_neighbor_locked=192 waits_neighbor=963 path_splits=104 path_max=5 star_max=124 star_gt100=11 star_gt1000=0 refused_walk=33 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=0 refused_topology=0 refused_memory=0 refused_entity=0 chord_unknown=0 samples=79976 samples_unconverged=1622 outward_max=6.0313230428769143e-06 seconds_length=1.424705760000009 seconds_distance=1.9167607199999992 seconds_samples=0.67441059600008657 seconds_place=0.15192084900036207 seconds_entity=2.1579793229995801 v_rounds=4 v_limit=0 v_collapses=12 v_splits=103 v_changes=c10s87q11,c2s14q20,c0s2q4,c0s0q1 bad_before_v=1119 seconds_v=1.4095390419999916
S1 NATIVE3D_LOCAL_SURFACE rank=3 stage=0 pass=2 applicable=1 faces_in=14934 faces_out=15230 points_in=7619 points_out=7767 splits_length=0 splits_distance=148 splits_feature=0 sweeps_length=0 sweeps_distance=7 left_long=0 left_border=13 left_distance=162 left_all_feature=0 left_refused=17 left_limit=0 left_short=0 left_neighbor_locked=132 waits_neighbor=282 path_splits=4 path_max=6 star_max=122 star_gt100=4 star_gt1000=0 refused_walk=26 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=0 refused_topology=0 refused_memory=0 refused_entity=0 chord_unknown=0 samples=62104 samples_unconverged=1450 outward_max=0 seconds_length=0.014498832999990441 seconds_distance=0.67124612800000705 seconds_samples=0.42341529000053413 seconds_place=0.0059330110000388459 seconds_entity=0.053803049000094916 v_rounds=4 v_limit=0 v_collapses=0 v_splits=15 v_changes=c0s11q20,c0s3q11,c0s1q3,c0s0q1 bad_before_v=368 seconds_v=1.1274383899999947
S1 NATIVE3D_LOCAL_SURFACE_STATE rank=3 stage=0 splits_all=13993 faces=76940 points=38472 traces=753 revision=2 seconds_exchange=0.029060354999984384 seconds_gather=0.1836533390000028 seconds_build=31.612167207999988 seconds_check=0.036909989000008636 modeled_peak=4284647000 rss_before=544174080 rss_after=544239616
S1 NATIVE3D_LOCAL_SURFACE rank=4 stage=0 pass=1 applicable=1 faces_in=9634 faces_out=11688 points_in=4969 points_out=5996 splits_length=0 splits_distance=1027 splits_feature=0 sweeps_length=0 sweeps_distance=23 left_long=0 left_border=53 left_distance=183 left_all_feature=0 left_refused=0 left_limit=0 left_short=0 left_neighbor_locked=130 waits_neighbor=698 path_splits=49 path_max=20 star_max=98 star_gt100=0 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=0 refused_topology=0 refused_memory=0 refused_entity=0 chord_unknown=0 samples=54968 samples_unconverged=1371 outward_max=2.1320851362276744e-05 seconds_length=0.014996906999996895 seconds_distance=1.2896436019999982 seconds_samples=0.41418947100109449 seconds_place=0.029139587999949867 seconds_entity=0.46922357299966677 v_rounds=5 v_limit=0 v_collapses=54 v_splits=51 v_changes=c44s47q10,c7s4q5,c2s0q4,c1s0q1,c0s0q1 bad_before_v=1312 seconds_v=1.0344921140000025
S1 NATIVE3D_LOCAL_SURFACE rank=4 stage=0 pass=2 applicable=1 faces_in=10846 faces_out=10846 points_in=5622 points_out=5622 splits_length=0 splits_distance=0 splits_feature=0 sweeps_length=0 sweeps_distance=1 left_long=0 left_border=22 left_distance=48 left_all_feature=0 left_refused=0 left_limit=0 left_short=0 left_neighbor_locked=26 waits_neighbor=0 path_splits=0 path_max=4 star_max=112 star_gt100=1 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=0 refused_topology=0 refused_memory=0 refused_entity=0 chord_unknown=0 samples=43384 samples_unconverged=1309 outward_max=0 seconds_length=0.011706422999992583 seconds_distance=0.30646957700000144 seconds_samples=0.26583918100013193 seconds_place=0 seconds_entity=0 v_rounds=3 v_limit=0 v_collapses=3 v_splits=0 v_changes=c2s0q4,c1s0q1,c0s0q1 bad_before_v=654 seconds_v=0.358525293999989
S1 NATIVE3D_LOCAL_SURFACE_STATE rank=4 stage=0 splits_all=13993 faces=76940 points=38472 traces=753 revision=2 seconds_exchange=0.028969639999985475 seconds_gather=0.18364822099999856 seconds_build=31.586824882999991 seconds_check=0.036907096999982514 modeled_peak=1908946864 rss_before=541720576 rss_after=541786112
S1 NATIVE3D_LOCAL_SURFACE rank=5 stage=0 pass=1 applicable=1 faces_in=8132 faces_out=11588 points_in=4257 points_out=5985 splits_length=0 splits_distance=1728 splits_feature=28 sweeps_length=0 sweeps_distance=12 left_long=0 left_border=64 left_distance=551 left_all_feature=0 left_refused=36 left_limit=0 left_short=0 left_neighbor_locked=451 waits_neighbor=883 path_splits=157 path_max=7 star_max=134 star_gt100=6 star_gt1000=0 refused_walk=41 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=0 refused_topology=0 refused_memory=0 refused_entity=0 chord_unknown=0 samples=60176 samples_unconverged=1342 outward_max=6.0167270877548275e-06 seconds_length=0.011375780999998142 seconds_distance=1.5374933100000021 seconds_samples=0.49181323200048155 seconds_place=0.065143099999801279 seconds_entity=0.73086104200019975 v_rounds=5 v_limit=0 v_collapses=19 v_splits=122 v_changes=c14s116q11,c2s6q3,c2s0q3,c1s0q4,c0s0q1 bad_before_v=1148 seconds_v=0.65925179799999967
S1 NATIVE3D_LOCAL_SURFACE rank=5 stage=0 pass=2 applicable=1 faces_in=10643 faces_out=10643 points_in=5566 points_out=5566 splits_length=0 splits_distance=0 splits_feature=0 sweeps_length=0 sweeps_distance=1 left_long=0 left_border=51 left_distance=321 left_all_feature=0 left_refused=27 left_limit=0 left_short=0 left_neighbor_locked=243 waits_neighbor=159 path_splits=0 path_max=5 star_max=108 star_gt100=3 star_gt1000=0 refused_walk=35 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=0 refused_topology=0 refused_memory=0 refused_entity=0 chord_unknown=0 samples=42572 samples_unconverged=1084 outward_max=0 seconds_length=0.01203753400000096 seconds_distance=0.33477753600000426 seconds_samples=0.28630611900013037 seconds_place=0.00043184800000517498 seconds_entity=0 v_rounds=1 v_limit=0 v_collapses=0 v_splits=0 v_changes=c0s0q1 bad_before_v=209 seconds_v=0.046129569999990849
S1 NATIVE3D_LOCAL_SURFACE_STATE rank=5 stage=0 splits_all=13993 faces=76940 points=38472 traces=753 revision=2 seconds_exchange=0.028996657000007531 seconds_gather=0.18364796299999853 seconds_build=31.725224205000004 seconds_check=0.036914099000000533 modeled_peak=1901325700 rss_before=494501888 rss_after=494567424
```

### S1 summed surface faces by stage and pass

| Stage | Pass | Ranks with rows | Sum of faces_out |
| --- | --- | --- | --- |
| 0 | 1 | 0, 1, 2, 3, 4, 5 | 73962 |
| 0 | 2 | 0, 1, 2, 3, 4, 5 | 76940 |

## Event 2

| Case | Native max | Cells | Points | Surface faces | Min coverage % | Job peak | Builds observed |
| --- | --- | --- | --- | --- | --- | --- | --- |
| CORE | — | — | — | — | — | — | — |
| S1 | 260.408 | — | — | — | 98.3510 (6 ranks) | 13032607744 | 1 |
| S2 | — | — | — | — | — | — | — |

| Case | Rank | Native | preparation | construction | quality | final_proof | transfer | topology | geometry_bound | mobility_policy | communication | Process peak | Builds observed |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| S1 | 0 | 260.311 | 55.582 | 202.986 | 0.000 | 0.000 | 0.000 | 35.599 | 7.067 | 16.706 | 109.355 | 1984294912 | 1 |
| S1 | 1 | 260.312 | 55.640 | 203.004 | 0.000 | 0.000 | 0.000 | 36.163 | 28.627 | 16.715 | 75.929 | 1960472576 | 1 |
| S1 | 2 | 260.318 | 55.581 | 202.985 | 0.000 | 0.000 | 0.000 | 36.713 | 23.681 | 16.719 | 82.292 | 2174361600 | 1 |
| S1 | 3 | 260.408 | 55.576 | 203.004 | 0.000 | 0.000 | 0.000 | 29.442 | 13.302 | 16.760 | 106.898 | 3145752576 | 1 |
| S1 | 4 | 260.310 | 55.640 | 203.005 | 0.000 | 0.000 | 0.000 | 33.944 | 21.648 | 16.705 | 89.572 | 1905041408 | 1 |
| S1 | 5 | 260.315 | 55.626 | 202.986 | 0.000 | 0.000 | 0.000 | 29.841 | 5.635 | 16.797 | 112.268 | 1862684672 | 1 |

| Case | Rank | Length splits by engine exit | Surface rounds by engine exit | Construction wall commits | Quality wall commits | First census stage emitted | Geometry-invalid cells |
| --- | --- | --- | --- | --- | --- | --- | --- |
| S1 | 0 | 8 | 1 | 424 | 0 | length_collapse_exit | 0 |
| S1 | 1 | 8 | 1 | 749 | 0 | length_collapse_exit | 0 |
| S1 | 2 | 35 | 1 | 702 | 0 | length_collapse_exit | 0 |
| S1 | 3 | 6 | 1 | 568 | 0 | length_collapse_exit | 0 |
| S1 | 4 | 33 | 1 | 685 | 0 | length_collapse_exit | 0 |
| S1 | 5 | 14 | 1 | 500 | 0 | length_collapse_exit | 0 |

### Surface rows

```text
S1 NATIVE3D_LOCAL_SURFACE rank=0 stage=0 pass=1 applicable=1 faces_in=8490 faces_out=9940 points_in=4440 points_out=5165 splits_length=456 splits_distance=269 splits_feature=0 sweeps_length=32 sweeps_distance=7 left_long=16 left_border=23 left_distance=33 left_all_feature=0 left_refused=8 left_limit=0 left_short=0 left_neighbor_locked=17 waits_neighbor=261 path_splits=69 path_max=5 star_max=146 star_gt100=8 star_gt1000=0 refused_walk=7 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=3 refused_topology=0 refused_memory=0 refused_entity=0 chord_unknown=0 samples=41912 samples_unconverged=1161 outward_max=8.3590994680782365e-06 seconds_length=0.25548992900007761 seconds_distance=0.67415815799995471 seconds_samples=0.4558510699935141 seconds_place=0.025508782000997599 seconds_entity=0.26356540299957487 v_rounds=4 v_limit=0 v_collapses=12 v_splits=56 v_changes=c10s46q9,c2s8q5,c0s2q2,c0s0q1 bad_before_v=554 seconds_v=0.43020302900004026
S1 NATIVE3D_LOCAL_SURFACE rank=0 stage=0 pass=2 applicable=1 faces_in=11097 faces_out=11223 points_in=5862 points_out=5925 splits_length=17 splits_distance=46 splits_feature=0 sweeps_length=2 sweeps_distance=7 left_long=1 left_border=0 left_distance=15 left_all_feature=0 left_refused=6 left_limit=0 left_short=0 left_neighbor_locked=9 waits_neighbor=58 path_splits=15 path_max=5 star_max=102 star_gt100=1 star_gt1000=0 refused_walk=8 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=2 refused_topology=0 refused_memory=0 refused_entity=0 chord_unknown=0 samples=45260 samples_unconverged=1254 outward_max=4.8324236768826673e-12 seconds_length=0.026824870000154988 seconds_distance=0.66006688300012684 seconds_samples=0.49080237799284987 seconds_place=0.0025822990003234736 seconds_entity=0.021060472000499431 v_rounds=3 v_limit=0 v_collapses=2 v_splits=10 v_changes=c2s8q3,c0s2q5,c0s0q1 bad_before_v=286 seconds_v=0.24936943399984557
S1 NATIVE3D_LOCAL_SURFACE_STATE rank=0 stage=0 splits_all=4648 faces=85422 points=42713 traces=757 revision=5213 seconds_exchange=0.025882772999921144 seconds_gather=0.29661860500004877 seconds_build=32.031947768000009 seconds_check=0.045774595000011686 modeled_peak=2891832140 rss_before=1528872960 rss_after=1528872960
S1 NATIVE3D_LOCAL_SURFACE rank=1 stage=0 pass=1 applicable=1 faces_in=15690 faces_out=18024 points_in=8096 points_out=9263 splits_length=0 splits_distance=1167 splits_feature=1 sweeps_length=0 sweeps_distance=8 left_long=0 left_border=10 left_distance=115 left_all_feature=0 left_refused=11 left_limit=0 left_short=0 left_neighbor_locked=94 waits_neighbor=1382 path_splits=312 path_max=7 star_max=148 star_gt100=21 star_gt1000=0 refused_walk=33 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=5 refused_topology=0 refused_memory=0 refused_entity=0 chord_unknown=0 samples=81432 samples_unconverged=1295 outward_max=1.3263851131155408e-05 seconds_length=0.027737259999867092 seconds_distance=2.0364717909999399 seconds_samples=1.272603684001524 seconds_place=0.049107089005246962 seconds_entity=0.36138951599946267 v_rounds=3 v_limit=0 v_collapses=25 v_splits=99 v_changes=c24s89q13,c1s10q6,c0s0q1 bad_before_v=998 seconds_v=0.73383178599988241
S1 NATIVE3D_LOCAL_SURFACE rank=1 stage=0 pass=2 applicable=1 faces_in=18806 faces_out=18840 points_in=9780 points_out=9797 splits_length=0 splits_distance=17 splits_feature=0 sweeps_length=0 sweeps_distance=5 left_long=0 left_border=1 left_distance=93 left_all_feature=0 left_refused=11 left_limit=0 left_short=0 left_neighbor_locked=81 waits_neighbor=93 path_splits=4 path_max=5 star_max=110 star_gt100=3 star_gt1000=0 refused_walk=35 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=5 refused_topology=0 refused_memory=0 refused_entity=0 chord_unknown=0 samples=75496 samples_unconverged=1293 outward_max=0 seconds_length=0.029357306000065364 seconds_distance=1.4346973210001579 seconds_samples=1.1831980939987261 seconds_place=0.0016690680001829605 seconds_entity=0.0050373650003621151 v_rounds=3 v_limit=0 v_collapses=3 v_splits=9 v_changes=c2s7q4,c1s2q2,c0s0q1 bad_before_v=601 seconds_v=0.39541691699992043
S1 NATIVE3D_LOCAL_SURFACE_STATE rank=1 stage=0 splits_all=4648 faces=85422 points=42713 traces=757 revision=5213 seconds_exchange=0.025887777000207279 seconds_gather=0.29662108100001205 seconds_build=32.031283450000046 seconds_check=0.045780252999975346 modeled_peak=2558568004 rss_before=1427320832 rss_after=1427320832
S1 NATIVE3D_LOCAL_SURFACE rank=2 stage=0 pass=1 applicable=1 faces_in=14277 faces_out=17417 points_in=7395 points_out=8965 splits_length=270 splits_distance=1300 splits_feature=2 sweeps_length=4 sweeps_distance=9 left_long=16 left_border=25 left_distance=117 left_all_feature=0 left_refused=9 left_limit=0 left_short=0 left_neighbor_locked=99 waits_neighbor=1580 path_splits=420 path_max=7 star_max=200 star_gt100=30 star_gt1000=0 refused_walk=47 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=3 refused_topology=0 refused_memory=0 refused_entity=0 chord_unknown=0 samples=80068 samples_unconverged=789 outward_max=9.7785811878972098e-06 seconds_length=0.17075047100001939 seconds_distance=1.8095052399999076 seconds_samples=1.000349184013885 seconds_place=0.065492296997945232 seconds_entity=0.48715120600968476 v_rounds=4 v_limit=0 v_collapses=28 v_splits=101 v_changes=c19s86q10,c8s12q6,c1s3q3,c0s0q1 bad_before_v=846 seconds_v=0.67071093699996709
S1 NATIVE3D_LOCAL_SURFACE rank=2 stage=0 pass=2 applicable=1 faces_in=17756 faces_out=17818 points_in=9255 points_out=9286 splits_length=8 splits_distance=23 splits_feature=0 sweeps_length=1 sweeps_distance=5 left_long=0 left_border=1 left_distance=103 left_all_feature=0 left_refused=5 left_limit=0 left_short=0 left_neighbor_locked=97 waits_neighbor=115 path_splits=6 path_max=3 star_max=136 star_gt100=7 star_gt1000=0 refused_walk=47 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=1 refused_topology=0 refused_memory=0 refused_entity=0 chord_unknown=0 samples=71456 samples_unconverged=772 outward_max=7.3869662091454316e-06 seconds_length=0.034582503000137876 seconds_distance=1.1155976899999587 seconds_samples=0.87311352699225608 seconds_place=0.0019090339992544614 seconds_entity=0.0094517709990213916 v_rounds=2 v_limit=0 v_collapses=0 v_splits=4 v_changes=c0s4q2,c0s0q1 bad_before_v=361 seconds_v=0.18635008399996877
S1 NATIVE3D_LOCAL_SURFACE_STATE rank=2 stage=0 splits_all=4648 faces=85422 points=42713 traces=757 revision=5213 seconds_exchange=0.025883964999820819 seconds_gather=0.29661821699983193 seconds_build=32.05249882399994 seconds_check=0.045776876999980232 modeled_peak=2672735012 rss_before=1442557952 rss_after=1442557952
S1 NATIVE3D_LOCAL_SURFACE rank=3 stage=0 pass=1 applicable=1 faces_in=12452 faces_out=12638 points_in=6408 points_out=6501 splits_length=21 splits_distance=72 splits_feature=0 sweeps_length=2 sweeps_distance=4 left_long=0 left_border=0 left_distance=7 left_all_feature=0 left_refused=6 left_limit=0 left_short=0 left_neighbor_locked=1 waits_neighbor=20 path_splits=5 path_max=3 star_max=86 star_gt100=0 star_gt1000=0 refused_walk=4 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=1 refused_topology=0 refused_memory=0 refused_entity=0 chord_unknown=0 samples=51128 samples_unconverged=1684 outward_max=2.4546945779567109e-05 seconds_length=0.028326910000032512 seconds_distance=1.0192332030001126 seconds_samples=0.85717061001241746 seconds_place=0.0043078790001800371 seconds_entity=0.045184716001131164 v_rounds=2 v_limit=0 v_collapses=9 v_splits=21 v_changes=c9s21q6,c0s0q1 bad_before_v=262 seconds_v=0.14150313499999356
S1 NATIVE3D_LOCAL_SURFACE rank=3 stage=0 pass=2 applicable=1 faces_in=12280 faces_out=12280 points_in=6393 points_out=6393 splits_length=0 splits_distance=0 splits_feature=0 sweeps_length=0 sweeps_distance=1 left_long=0 left_border=1 left_distance=7 left_all_feature=0 left_refused=5 left_limit=0 left_short=0 left_neighbor_locked=1 waits_neighbor=1 path_splits=0 path_max=1 star_max=92 star_gt100=0 star_gt1000=0 refused_walk=3 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=1 refused_topology=0 refused_memory=0 refused_entity=0 chord_unknown=0 samples=49120 samples_unconverged=1579 outward_max=0 seconds_length=0.022303887000134637 seconds_distance=0.88323485700016136 seconds_samples=0.81455883701005405 seconds_place=0.00017325600015283271 seconds_entity=0 v_rounds=2 v_limit=0 v_collapses=0 v_splits=3 v_changes=c0s3q3,c0s0q1 bad_before_v=158 seconds_v=0.083394868999903338
S1 NATIVE3D_LOCAL_SURFACE_STATE rank=3 stage=0 splits_all=4648 faces=85422 points=42713 traces=757 revision=5213 seconds_exchange=0.025880888000074265 seconds_gather=0.29661557500003255 seconds_build=32.122985413000151 seconds_check=0.045785132999981215 modeled_peak=2205436788 rss_before=1897185280 rss_after=1897185280
S1 NATIVE3D_LOCAL_SURFACE rank=4 stage=0 pass=1 applicable=1 faces_in=14459 faces_out=16095 points_in=7489 points_out=8307 splits_length=273 splits_distance=545 splits_feature=6 sweeps_length=3 sweeps_distance=8 left_long=18 left_border=26 left_distance=26 left_all_feature=0 left_refused=4 left_limit=0 left_short=0 left_neighbor_locked=14 waits_neighbor=493 path_splits=136 path_max=6 star_max=136 star_gt100=12 star_gt1000=0 refused_walk=4 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=1 refused_topology=0 refused_memory=0 refused_entity=0 chord_unknown=0 samples=68740 samples_unconverged=720 outward_max=7.1439759082551554e-05 seconds_length=0.17218646899982559 seconds_distance=1.4004315970000789 seconds_samples=0.89350269800479509 seconds_place=0.035943506007470205 seconds_entity=0.27687457799970616 v_rounds=3 v_limit=0 v_collapses=21 v_splits=41 v_changes=c20s37q9,c1s4q4,c0s0q1 bad_before_v=526 seconds_v=0.37803638400009731
S1 NATIVE3D_LOCAL_SURFACE rank=4 stage=0 pass=2 applicable=1 faces_in=15835 faces_out=15855 points_in=8293 points_out=8303 splits_length=3 splits_distance=7 splits_feature=0 sweeps_length=1 sweeps_distance=2 left_long=0 left_border=0 left_distance=14 left_all_feature=0 left_refused=4 left_limit=0 left_short=0 left_neighbor_locked=10 waits_neighbor=11 path_splits=0 path_max=4 star_max=106 star_gt100=2 star_gt1000=0 refused_walk=4 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=1 refused_topology=0 refused_memory=0 refused_entity=0 chord_unknown=0 samples=63476 samples_unconverged=730 outward_max=5.3633436560696219e-16 seconds_length=0.039974581000024045 seconds_distance=0.93395156299993687 seconds_samples=0.8070795309974983 seconds_place=0.00055981300033636217 seconds_entity=0.004032065000046714 v_rounds=2 v_limit=0 v_collapses=3 v_splits=3 v_changes=c3s3q3,c0s0q1 bad_before_v=346 seconds_v=0.14743300500003897
S1 NATIVE3D_LOCAL_SURFACE_STATE rank=4 stage=0 splits_all=4648 faces=85422 points=42713 traces=757 revision=5213 seconds_exchange=0.025882228000000396 seconds_gather=0.29661732699992172 seconds_build=31.979042457000105 seconds_check=0.045777197000006709 modeled_peak=2269493628 rss_before=1455824896 rss_after=1455824896
S1 NATIVE3D_LOCAL_SURFACE rank=5 stage=0 pass=1 applicable=1 faces_in=10758 faces_out=11066 points_in=5632 points_out=5786 splits_length=77 splits_distance=77 splits_feature=0 sweeps_length=3 sweeps_distance=4 left_long=7 left_border=15 left_distance=24 left_all_feature=0 left_refused=2 left_limit=0 left_short=0 left_neighbor_locked=14 waits_neighbor=42 path_splits=7 path_max=3 star_max=114 star_gt100=1 star_gt1000=0 refused_walk=2 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=1 refused_topology=0 refused_memory=0 refused_entity=0 chord_unknown=0 samples=44880 samples_unconverged=1297 outward_max=1.8042209424109698e-05 seconds_length=0.066893379000021014 seconds_distance=0.63034621299993887 seconds_samples=0.49327864300744295 seconds_place=0.0056059869987166167 seconds_entity=0.065506862999654913 v_rounds=3 v_limit=0 v_collapses=10 v_splits=14 v_changes=c9s14q8,c1s0q2,c0s0q1 bad_before_v=246 seconds_v=0.18403322199992544
S1 NATIVE3D_LOCAL_SURFACE rank=5 stage=0 pass=2 applicable=1 faces_in=9406 faces_out=9406 points_in=5059 points_out=5059 splits_length=0 splits_distance=0 splits_feature=0 sweeps_length=0 sweeps_distance=1 left_long=0 left_border=2 left_distance=6 left_all_feature=0 left_refused=2 left_limit=0 left_short=0 left_neighbor_locked=2 waits_neighbor=0 path_splits=0 path_max=1 star_max=76 star_gt100=0 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=1 refused_topology=0 refused_memory=0 refused_entity=0 chord_unknown=0 samples=37624 samples_unconverged=1237 outward_max=0 seconds_length=0.01963683700000729 seconds_distance=0.45785799600002974 seconds_samples=0.4027514180036178 seconds_place=4.0105999914885615e-05 seconds_entity=0 v_rounds=1 v_limit=0 v_collapses=0 v_splits=0 v_changes=c0s0q1 bad_before_v=39 seconds_v=0.027132005000112258
S1 NATIVE3D_LOCAL_SURFACE_STATE rank=5 stage=0 splits_all=4648 faces=85422 points=42713 traces=757 revision=5213 seconds_exchange=0.025873939999883078 seconds_gather=0.29661461399996369 seconds_build=32.116144139000198 seconds_check=0.045783081000081438 modeled_peak=2351930308 rss_before=1442095104 rss_after=1442095104
```

## Event 1 engine and core observations

Construction seconds use the engine clock at quality_entry. Improvement seconds use the elapsed difference from quality_entry to exit. Open invocations show the last observed clock. Committed ownership rows count the same collective operation on each participating rank. q and J are counts at the reported shape census. The history preserves measured-complete flags.

[Event 1 stage history](FP2_B_633_engine_event1.json)

| case | rank | engine_call | construction_to_quality_entry_seconds | improvement_to_exit_seconds | last_stage | last_elapsed | quality_entry_q_bad | quality_entry_j_bad | length_collapse_exit_geometry_bad | length_collapses_last_observed |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| S1 | 0 | 1 | 311.80339654199997 | 1210.61105203400003 | exit | 1522.414448576 | 2186 | 2226 | 535 | 612 |
| S1 | 1 | 1 | 311.804715483 | 1210.6104346840001 | exit | 1522.4151501670001 | 2342 | 2479 | 352 | 577 |
| S1 | 2 | 1 | 311.802172429 | 1210.6130955739999 | exit | 1522.4152680029999 | 2265 | 2362 | 0 | 792 |
| S1 | 3 | 1 | 311.80337087499998 | 1210.61131828800002 | exit | 1522.414689163 | 2114 | 2174 | 112 | 1414 |
| S1 | 4 | 1 | 311.80308536400003 | 1210.61100309599987 | exit | 1522.4140884599999 | 2266 | 2250 | 32 | 560 |
| S1 | 5 | 1 | 311.80049394999998 | 1210.61326488400002 | exit | 1522.413758834 | 1956 | 2154 | 313 | 566 |
| CORE-627 | 0 | 1 | 490.01012692199998 | 927.66193540800002 | exit | 1417.67206233 | 1836 | 2012 | 461 | 465 |
| CORE-627 | 1 | 1 | 490.01359709500002 | 927.66027950500008 | exit | 1417.6738766000001 | 2500 | 2820 | 411 | 459 |
| CORE-627 | 2 | 1 | 490.01076090800001 | 927.66215646000009 | exit | 1417.6729173680001 | 1636 | 1801 | 744 | 428 |
| CORE-627 | 3 | 1 | 490.00729254499998 | 927.66473491500002 | exit | 1417.67202746 | 1783 | 2000 | 601 | 573 |
| CORE-627 | 4 | 1 | 490.01120365000003 | 927.66076950899987 | exit | 1417.6719731589999 | 2807 | 3476 | 339 | 451 |
| CORE-627 | 5 | 1 | 490.012671141 | 927.6601867360001 | exit | 1417.6728578770001 | 2573 | 2922 | 416 | 445 |
| S1-627 | 0 | 1 | 201.23064484 | 5122.4221516539996 | exit | 5323.6527964939996 | 2624 | 2795 | 26 | 603 |
| S1-627 | 1 | 1 | 201.23298634099999 | 5122.43518860799991 | exit | 5323.6681749489999 | 3359 | 3620 | 7 | 577 |
| S1-627 | 2 | 1 | 201.23000159399999 | 5122.44000617199981 | exit | 5323.6700077659998 | 2411 | 2743 | 5 | 780 |
| S1-627 | 3 | 1 | 201.227171164 | 5122.4262090750004 | exit | 5323.6533802390004 | 2414 | 2582 | 8 | 1394 |
| S1-627 | 4 | 1 | 201.23047834299999 | 5122.42275237600001 | exit | 5323.653230719 | 2499 | 2788 | 3 | 555 |
| S1-627 | 5 | 1 | 201.22886935700001 | 5122.42478035599999 | exit | 5323.653649713 | 2505 | 2911 | 11 | 578 |

| case | rank | first_minimum_metric_length | committed_ownership_rows |
| --- | --- | --- | --- |
| S1 | 0 | 0.04147892841687132 | 5 |
| S1 | 1 | 0.035361853871773443 | 5 |
| S1 | 2 | 5.4527443514081955e-10 | 5 |
| S1 | 3 | 0.027261040654623701 | 5 |
| S1 | 4 | 0.042235282858877916 | 5 |
| S1 | 5 | 0.042003012336116365 | 5 |
| CORE-627 | 0 | 0.057871110721358392 | 2 |
| CORE-627 | 1 | 0.038592715927605226 | 2 |
| CORE-627 | 2 | 0.047282540091361975 | 2 |
| CORE-627 | 3 | 0.049945221461337114 | 2 |
| CORE-627 | 4 | 0.042235282858877916 | 2 |
| CORE-627 | 5 | 0.056263897611721882 | 2 |
| S1-627 | 0 | 1.763040275738747e-05 | 11 |
| S1-627 | 1 | 0.0068544928561841776 | 11 |
| S1-627 | 2 | 5.4527443514081955e-10 | 11 |
| S1-627 | 3 | 0.003646617255834537 | 11 |
| S1-627 | 4 | 0.018250379906111053 | 11 |
| S1-627 | 5 | 0.0086529686067382194 | 11 |

### Event 1 core work by rank, stage and pass

| case | rank | stage | pass | short_out | wall_near_points | seconds_quality |
| --- | --- | --- | --- | --- | --- | --- |
| S1 | 0 | 0 | 1 | 49126 | 2941 | 2.6223489160000071 |
| S1 | 0 | 0 | 2 | 56768 | 3314 | 1.5716828629999924 |
| S1 | 1 | 0 | 1 | 44775 | 2374 | 2.6070960400000018 |
| S1 | 1 | 0 | 2 | 46628 | 2450 | 0.72360110399999655 |
| S1 | 2 | 0 | 1 | 45076 | 3810 | 0.67407567499999743 |
| S1 | 2 | 0 | 2 | 53775 | 4466 | 4.7587730549999918 |
| S1 | 3 | 0 | 1 | 76880 | 12334 | 2.6909271640000014 |
| S1 | 3 | 0 | 2 | 73618 | 12054 | 1.6854680929999972 |
| S1 | 4 | 0 | 1 | 43264 | 2259 | 2.2915016319999921 |
| S1 | 4 | 0 | 2 | 38770 | 1980 | 0.43495314900000892 |
| S1 | 5 | 0 | 1 | 45630 | 2599 | 1.3135056819999988 |
| S1 | 5 | 0 | 2 | 40587 | 2155 | 0.069752854999990177 |
| CORE-627 | 0 | 0 | 1 | 40982 | 2846 | 0.94933366099999716 |
| CORE-627 | 0 | 0 | 2 | 43897 | 3153 | 0.1597140789999969 |
| CORE-627 | 1 | 0 | 1 | 38541 | 2337 | 1.2509122649999966 |
| CORE-627 | 1 | 0 | 2 | 39835 | 2408 | 0.24235175100000106 |
| CORE-627 | 2 | 0 | 1 | 32414 | 2400 | 0.12137029299999824 |
| CORE-627 | 2 | 0 | 2 | 36316 | 2652 | 0.08934424600001023 |
| CORE-627 | 3 | 0 | 1 | 33086 | 3545 | 0.099823833000002082 |
| CORE-627 | 3 | 0 | 2 | 30729 | 3328 | 0.042536155000007625 |
| CORE-627 | 4 | 0 | 1 | 38452 | 2263 | 1.1374600479999941 |
| CORE-627 | 4 | 0 | 2 | 34448 | 1993 | 0.18222393099999579 |
| CORE-627 | 5 | 0 | 1 | 36996 | 2496 | 0.25560105999998939 |
| CORE-627 | 5 | 0 | 2 | 32216 | 2063 | 0.021896229999995853 |
| S1-627 | 0 | 0 | 1 | 57397 | 2845 | 3.3832843649999944 |
| S1-627 | 0 | 0 | 2 | 63408 | 3147 | 2.6593313219999999 |
| S1-627 | 1 | 0 | 1 | 50100 | 2331 | 2.5681426130000062 |
| S1-627 | 1 | 0 | 2 | 51822 | 2400 | 0.7343211489999959 |
| S1-627 | 2 | 0 | 1 | 45830 | 3803 | 0.29340300299999456 |
| S1-627 | 2 | 0 | 2 | 52739 | 4405 | 0.71804918800000905 |
| S1-627 | 3 | 0 | 1 | 77696 | 12242 | 1.045282628999999 |
| S1-627 | 3 | 0 | 2 | 73885 | 11948 | 0.70212506800000085 |
| S1-627 | 4 | 0 | 1 | 43927 | 2263 | 1.3807178109999967 |
| S1-627 | 4 | 0 | 2 | 38881 | 1994 | 0.17084504299999992 |
| S1-627 | 5 | 0 | 1 | 50648 | 2495 | 1.7072039960000041 |
| S1-627 | 5 | 0 | 2 | 44549 | 2060 | 0.11320060900000328 |
