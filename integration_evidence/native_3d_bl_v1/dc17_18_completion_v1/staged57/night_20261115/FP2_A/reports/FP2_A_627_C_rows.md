# FP2-A Part C workflow comparison

Each row uses the final remesh timer ledger for that rank. Partial evidence lists only observed values. Times are seconds and memory is bytes.

R4 marks exact engine surface length splits, geometry splits, and first above-tolerance face counts unavailable. The engine rows give all-lane length splits, surface-lane rounds, mixed-edit wall-lane commits, and geometry-invalid cells at the first emitted complete geometry census.

- **CORE** sealed.
- **S1** failed. Stage full event gates. full run did not end normally with exit 0 (stop=adaptation_rejected_stop_first_failure, exit=-15)
- **S2** not observed.

## Event 1

| Case | Native max | Cells | Points | Surface faces | Min coverage % | Job peak | Builds observed |
| --- | --- | --- | --- | --- | --- | --- | --- |
| CORE | 1512.056 | 554969 | 111697 | 68006 | 99.0454 (6 ranks) | 11576320000 | 0 |
| S1 | 5430.854 | — | — | — | 99.7365 (6 ranks) | 13358960640 | 1 |
| S2 | — | — | — | — | — | — | — |

| Case | Rank | Native | preparation | construction | quality | final_proof | transfer | topology | geometry_bound | mobility_policy | communication | Process peak | Builds observed |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| CORE | 0 | 1512.056 | 64.390 | 482.496 | 866.938 | 56.339 | 30.778 | 11.482 | 135.688 | 0.000 | 265.450 | 2037018624 | 0 |
| CORE | 1 | 1511.973 | 64.471 | 482.506 | 866.929 | 56.339 | 30.780 | 11.868 | 76.764 | 0.000 | 335.065 | 1906176000 | 0 |
| CORE | 2 | 1511.969 | 64.351 | 482.508 | 866.933 | 56.339 | 30.779 | 17.038 | 101.327 | 0.000 | 295.211 | 1930702848 | 0 |
| CORE | 3 | 1511.969 | 64.207 | 482.501 | 866.941 | 56.339 | 30.778 | 23.143 | 118.517 | 0.000 | 264.533 | 2118578176 | 0 |
| CORE | 4 | 1511.972 | 64.471 | 482.496 | 866.934 | 56.339 | 30.780 | 11.151 | 97.338 | 0.000 | 314.765 | 1930121216 | 0 |
| CORE | 5 | 1511.972 | 64.494 | 482.498 | 866.931 | 56.339 | 30.780 | 7.248 | 131.265 | 0.000 | 291.373 | 1653723136 | 0 |
| S1 | 0 | 5430.811 | 68.662 | 230.127 | 5122.496 | 0.000 | 0.000 | 34.952 | 21.121 | 16.798 | 136.502 | 2054377472 | 1 |
| S1 | 1 | 5430.802 | 68.692 | 230.151 | 5122.542 | 0.000 | 0.000 | 31.495 | 16.956 | 16.801 | 145.913 | 2077523968 | 1 |
| S1 | 2 | 5430.812 | 68.497 | 230.124 | 5122.583 | 0.000 | 0.000 | 38.250 | 9.085 | 16.794 | 134.750 | 2196049920 | 1 |
| S1 | 3 | 5430.822 | 68.131 | 230.145 | 5122.497 | 0.000 | 0.000 | 58.826 | 10.163 | 16.772 | 89.739 | 3136860160 | 1 |
| S1 | 4 | 5430.686 | 68.796 | 230.127 | 5122.498 | 0.000 | 0.000 | 28.010 | 11.517 | 16.803 | 155.706 | 1930719232 | 1 |
| S1 | 5 | 5430.854 | 68.650 | 230.102 | 5122.492 | 0.000 | 0.000 | 27.406 | 22.455 | 16.803 | 146.177 | 1963429888 | 1 |

| Case | Rank | Length splits by engine exit | Surface rounds by engine exit | Construction wall commits | Quality wall commits | First census stage emitted | Geometry-invalid cells |
| --- | --- | --- | --- | --- | --- | --- | --- |
| CORE | 0 | 7103 | 3236 | 3991 | 2001 | length_collapse_exit | 461 |
| CORE | 1 | 4183 | 3236 | 2517 | 3747 | length_collapse_exit | 411 |
| CORE | 2 | 5198 | 3236 | 3104 | 2160 | length_collapse_exit | 744 |
| CORE | 3 | 6284 | 3236 | 3787 | 2610 | length_collapse_exit | 601 |
| CORE | 4 | 4279 | 3236 | 2529 | 4748 | length_collapse_exit | 339 |
| CORE | 5 | 1681 | 3236 | 2043 | 4478 | length_collapse_exit | 416 |
| S1 | 0 | 43 | 220 | 587 | 4652 | length_collapse_exit | 26 |
| S1 | 1 | 39 | 220 | 511 | 5564 | length_collapse_exit | 7 |
| S1 | 2 | 1228 | 220 | 850 | 3934 | length_collapse_exit | 5 |
| S1 | 3 | 529 | 220 | 678 | 5922 | length_collapse_exit | 8 |
| S1 | 4 | 24 | 220 | 482 | 2487 | length_collapse_exit | 3 |
| S1 | 5 | 35 | 220 | 539 | 16781 | length_collapse_exit | 11 |

### Surface rows

```text
S1 NATIVE3D_LOCAL_SURFACE rank=0 stage=0 pass=1 applicable=1 faces_in=9423 faces_out=16777 points_in=4894 points_out=8571 splits_length=0 splits_distance=3677 splits_feature=11 sweeps_length=0 sweeps_distance=20 left_long=0 left_border=63 left_distance=63 left_all_feature=0 left_refused=32 left_limit=0 left_short=0 star_max=168 star_gt100=20 star_gt1000=0 refused_walk=48 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=20 refused_topology=0 refused_memory=0 refused_entity=12 chord_unknown=0 samples=96524 samples_unconverged=1901 outward_max=9.311180236387248e-07 seconds_length=0.012492895000008275 seconds_distance=2.8987121380000076 seconds_samples=0.76936984999952074 seconds_place=0.11845514899957266 seconds_entity=1.5008786729993915
S1 NATIVE3D_LOCAL_SURFACE rank=0 stage=0 pass=2 applicable=1 faces_in=18284 faces_out=18538 points_in=9551 points_out=9678 splits_length=0 splits_distance=127 splits_feature=0 sweeps_length=0 sweeps_distance=11 left_long=0 left_border=1 left_distance=43 left_all_feature=0 left_refused=42 left_limit=0 left_short=0 star_max=164 star_gt100=48 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=22 refused_topology=0 refused_memory=0 refused_entity=9 chord_unknown=0 samples=75168 samples_unconverged=1550 outward_max=0 seconds_length=0.014553638000009528 seconds_distance=0.78821599600000525 seconds_samples=0.46231872899986115 seconds_place=0.0053334920000054353 seconds_entity=0.054020360999999184
S1 NATIVE3D_LOCAL_SURFACE_STATE rank=0 stage=0 splits_all=18120 faces=85194 points=42599 traces=748 revision=2 seconds_exchange=0.037769600000004289 seconds_gather=0.19358625399999596 seconds_build=32.90332192599999 seconds_check=0.039910122999998521 modeled_peak=1991950392 rss_before=517214208 rss_after=517279744
S1 NATIVE3D_LOCAL_SURFACE rank=1 stage=0 pass=1 applicable=1 faces_in=9031 faces_out=14127 points_in=4710 points_out=7258 splits_length=0 splits_distance=2548 splits_feature=0 sweeps_length=0 sweeps_distance=12 left_long=0 left_border=20 left_distance=28 left_all_feature=0 left_refused=12 left_limit=0 left_short=0 star_max=118 star_gt100=9 star_gt1000=0 refused_walk=48 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=8 refused_topology=0 refused_memory=0 refused_entity=4 chord_unknown=0 samples=76892 samples_unconverged=1637 outward_max=6.1822605883810054e-12 seconds_length=0.011583313000002704 seconds_distance=1.9751656589999982 seconds_samples=0.56076243599997611 seconds_place=0.075348230999381371 seconds_entity=1.0873168260000625
S1 NATIVE3D_LOCAL_SURFACE rank=1 stage=0 pass=2 applicable=1 faces_in=14526 faces_out=14592 points_in=7533 points_out=7566 splits_length=0 splits_distance=33 splits_feature=0 sweeps_length=0 sweeps_distance=3 left_long=0 left_border=0 left_distance=12 left_all_feature=0 left_refused=12 left_limit=0 left_short=0 star_max=150 star_gt100=15 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=6 refused_topology=0 refused_memory=0 refused_entity=3 chord_unknown=0 samples=58632 samples_unconverged=1308 outward_max=0 seconds_length=0.012771567999990907 seconds_distance=0.41692833400000495 seconds_samples=0.32943183099996531 seconds_place=0.0012927639999986695 seconds_entity=0.014303739999988352
S1 NATIVE3D_LOCAL_SURFACE_STATE rank=1 stage=0 splits_all=18120 faces=85194 points=42599 traces=748 revision=2 seconds_exchange=0.037742350000002034 seconds_gather=0.19359113299999819 seconds_build=32.928876404000007 seconds_check=0.039913812000008875 modeled_peak=1992200312 rss_before=524857344 rss_after=524857344
S1 NATIVE3D_LOCAL_SURFACE rank=2 stage=0 pass=1 applicable=1 faces_in=6040 faces_out=11202 points_in=3195 points_out=5776 splits_length=557 splits_distance=2024 splits_feature=0 sweeps_length=35 sweeps_distance=13 left_long=20 left_border=61 left_distance=48 left_all_feature=0 left_refused=11 left_limit=0 left_short=0 star_max=157 star_gt100=7 star_gt1000=0 refused_walk=8 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=9 refused_topology=0 refused_memory=0 refused_entity=2 chord_unknown=0 samples=61000 samples_unconverged=1450 outward_max=1.0624059606946464e-05 seconds_length=0.38458819299999902 seconds_distance=1.9180353449999927 seconds_samples=0.64752809399892897 seconds_place=0.072845659000165597 seconds_entity=1.3104266459996836
S1 NATIVE3D_LOCAL_SURFACE rank=2 stage=0 pass=2 applicable=1 faces_in=12549 faces_out=12729 points_in=6542 points_out=6632 splits_length=16 splits_distance=74 splits_feature=0 sweeps_length=2 sweeps_distance=4 left_long=12 left_border=1 left_distance=8 left_all_feature=0 left_refused=7 left_limit=0 left_short=0 star_max=330 star_gt100=8 star_gt1000=0 refused_walk=12 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=3 refused_topology=0 refused_memory=0 refused_entity=2 chord_unknown=0 samples=51508 samples_unconverged=1639 outward_max=1.5623570641474437e-16 seconds_length=0.026008654999998271 seconds_distance=0.5867627670000104 seconds_samples=0.47150943800070877 seconds_place=0.0035410450001194249 seconds_entity=0.042986097000010659
S1 NATIVE3D_LOCAL_SURFACE_STATE rank=2 stage=0 splits_all=18120 faces=85194 points=42599 traces=748 revision=2 seconds_exchange=0.037740967000004844 seconds_gather=0.19359093500000313 seconds_build=32.861079144999991 seconds_check=0.039911027000016475 modeled_peak=2637247792 rss_before=535478272 rss_after=537038848
S1 NATIVE3D_LOCAL_SURFACE rank=3 stage=0 pass=1 applicable=1 faces_in=6694 faces_out=16250 points_in=3448 points_out=8226 splits_length=2214 splits_distance=2564 splits_feature=35 sweeps_length=36 sweeps_distance=9 left_long=18 left_border=34 left_distance=32 left_all_feature=0 left_refused=11 left_limit=0 left_short=0 star_max=124 star_gt100=9 star_gt1000=0 refused_walk=22 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=7 refused_topology=0 refused_memory=0 refused_entity=4 chord_unknown=0 samples=85512 samples_unconverged=1657 outward_max=6.0320100053939684e-06 seconds_length=1.466783802000009 seconds_distance=2.1603576659999959 seconds_samples=0.70999833199935836 seconds_place=0.16471282400051734 seconds_entity=2.3770301060005181
S1 NATIVE3D_LOCAL_SURFACE rank=3 stage=0 pass=2 applicable=1 faces_in=15685 faces_out=15709 points_in=8020 points_out=8032 splits_length=0 splits_distance=12 splits_feature=0 sweeps_length=0 sweeps_distance=3 left_long=0 left_border=1 left_distance=13 left_all_feature=0 left_refused=12 left_limit=0 left_short=0 star_max=138 star_gt100=6 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=6 refused_topology=0 refused_memory=0 refused_entity=3 chord_unknown=0 samples=62932 samples_unconverged=1437 outward_max=0 seconds_length=0.012358837999997263 seconds_distance=0.51565554299999405 seconds_samples=0.40742968499993992 seconds_place=0.00082461000003775098 seconds_entity=0.006473251999977947
S1 NATIVE3D_LOCAL_SURFACE_STATE rank=3 stage=0 splits_all=18120 faces=85194 points=42599 traces=748 revision=2 seconds_exchange=0.03779928100000518 seconds_gather=0.19359202900000128 seconds_build=32.903587777999988 seconds_check=0.039915019999995138 modeled_peak=4291088432 rss_before=544690176 rss_after=544694272
S1 NATIVE3D_LOCAL_SURFACE rank=4 stage=0 pass=1 applicable=1 faces_in=9634 faces_out=12098 points_in=4969 points_out=6201 splits_length=0 splits_distance=1232 splits_feature=0 sweeps_length=0 sweeps_distance=11 left_long=0 left_border=56 left_distance=50 left_all_feature=0 left_refused=10 left_limit=0 left_short=0 star_max=98 star_gt100=0 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=8 refused_topology=0 refused_memory=0 refused_entity=2 chord_unknown=0 samples=58248 samples_unconverged=1373 outward_max=2.1320851362276744e-05 seconds_length=0.013071913999993967 seconds_distance=1.2734088899999989 seconds_samples=0.45345729999986872 seconds_place=0.035391605000128834 seconds_entity=0.56424639500002627
S1 NATIVE3D_LOCAL_SURFACE rank=4 stage=0 pass=2 applicable=1 faces_in=10956 faces_out=10956 points_in=5708 points_out=5708 splits_length=0 splits_distance=0 splits_feature=0 sweeps_length=0 sweeps_distance=0 left_long=0 left_border=0 left_distance=0 left_all_feature=0 left_refused=0 left_limit=0 left_short=0 star_max=112 star_gt100=1 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=0 refused_topology=0 refused_memory=0 refused_entity=0 chord_unknown=0 samples=43824 samples_unconverged=1309 outward_max=0 seconds_length=0.0086777140000009467 seconds_distance=0.29306848100000593 seconds_samples=0.26807278999876871 seconds_place=0 seconds_entity=0
S1 NATIVE3D_LOCAL_SURFACE_STATE rank=4 stage=0 splits_all=18120 faces=85194 points=42599 traces=748 revision=2 seconds_exchange=0.037737510999988899 seconds_gather=0.19357440199999587 seconds_build=32.945412578000017 seconds_check=0.039917110000004641 modeled_peak=1991491452 rss_before=538697728 rss_after=538763264
S1 NATIVE3D_LOCAL_SURFACE rank=5 stage=0 pass=1 applicable=1 faces_in=8132 faces_out=14216 points_in=4257 points_out=7299 splits_length=0 splits_distance=3042 splits_feature=28 sweeps_length=0 sweeps_distance=20 left_long=0 left_border=71 left_distance=72 left_all_feature=0 left_refused=39 left_limit=0 left_short=0 star_max=132 star_gt100=14 star_gt1000=0 refused_walk=22 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=34 refused_topology=0 refused_memory=0 refused_entity=6 chord_unknown=0 samples=81200 samples_unconverged=1426 outward_max=6.0167270877548275e-06 seconds_length=0.010802126000001522 seconds_distance=2.4704910490000032 seconds_samples=0.65950401900026634 seconds_place=0.10881830599990394 seconds_entity=1.2647366300000158
S1 NATIVE3D_LOCAL_SURFACE rank=5 stage=0 pass=2 applicable=1 faces_in=12670 faces_out=12670 points_in=6745 points_out=6745 splits_length=0 splits_distance=0 splits_feature=0 sweeps_length=0 sweeps_distance=1 left_long=0 left_border=1 left_distance=1 left_all_feature=0 left_refused=0 left_limit=0 left_short=0 star_max=128 star_gt100=12 star_gt1000=0 refused_walk=0 refused_protected=0 refused_feature=0 refused_trace=0 refused_duplicate=0 refused_outward=0 refused_size=0 refused_valid=0 refused_topology=0 refused_memory=0 refused_entity=0 chord_unknown=0 samples=50680 samples_unconverged=1021 outward_max=0 seconds_length=0.010050348999996572 seconds_distance=0.36367032399999744 seconds_samples=0.32170825799950364 seconds_place=0 seconds_entity=0
S1 NATIVE3D_LOCAL_SURFACE_STATE rank=5 stage=0 splits_all=18120 faces=85194 points=42599 traces=748 revision=2 seconds_exchange=0.037760491999989654 seconds_gather=0.19358055600000057 seconds_build=32.900829648999988 seconds_check=0.039917105999990099 modeled_peak=1984761168 rss_before=499781632 rss_after=499847168
```

### S1 summed surface faces by stage and pass

| Stage | Pass | Ranks with rows | Sum of faces_out |
| --- | --- | --- | --- |
| 0 | 1 | 0, 1, 2, 3, 4, 5 | 84670 |
| 0 | 2 | 0, 1, 2, 3, 4, 5 | 85194 |

## Event 2

| Case | Native max | Cells | Points | Surface faces | Min coverage % | Job peak | Builds observed |
| --- | --- | --- | --- | --- | --- | --- | --- |
| CORE | 369.654 | 553652 | 109124 | 64758 | 98.6541 (6 ranks) | 11655073792 | 0 |
| S1 | — | — | — | — | — | — | 0 |
| S2 | — | — | — | — | — | — | — |

| Case | Rank | Native | preparation | construction | quality | final_proof | transfer | topology | geometry_bound | mobility_policy | communication | Process peak | Builds observed |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| CORE | 0 | 369.654 | 53.198 | 163.311 | 89.720 | 27.771 | 30.569 | 16.815 | 18.070 | 0.000 | 88.689 | 2037018624 | 0 |
| CORE | 1 | 369.576 | 53.315 | 163.299 | 89.719 | 27.771 | 30.567 | 14.501 | 14.914 | 0.000 | 103.438 | 1906176000 | 0 |
| CORE | 2 | 369.577 | 53.312 | 163.298 | 89.720 | 27.771 | 30.563 | 13.345 | 5.787 | 0.000 | 113.536 | 1930702848 | 0 |
| CORE | 3 | 369.581 | 53.311 | 163.300 | 89.719 | 27.771 | 30.566 | 14.801 | 6.405 | 0.000 | 105.905 | 2118578176 | 0 |
| CORE | 4 | 369.577 | 53.330 | 163.298 | 89.718 | 27.771 | 30.568 | 14.056 | 28.462 | 0.000 | 89.606 | 1930121216 | 0 |
| CORE | 5 | 369.572 | 53.327 | 163.297 | 89.719 | 27.771 | 30.565 | 14.264 | 21.977 | 0.000 | 94.929 | 1732476928 | 0 |
| S1 | 0 | — | — | — | — | — | — | — | — | — | — | — | 0 |

| Case | Rank | Length splits by engine exit | Surface rounds by engine exit | Construction wall commits | Quality wall commits | First census stage emitted | Geometry-invalid cells |
| --- | --- | --- | --- | --- | --- | --- | --- |
| CORE | 0 | 2704 | 422 | 1432 | 278 | length_collapse_exit | 0 |
| CORE | 1 | 45 | 422 | 585 | 270 | length_collapse_exit | 0 |
| CORE | 2 | 247 | 422 | 498 | 175 | length_collapse_exit | 0 |
| CORE | 3 | 2827 | 422 | 1305 | 243 | length_collapse_exit | 0 |
| CORE | 4 | 10 | 422 | 651 | 339 | length_collapse_exit | 0 |
| CORE | 5 | 11 | 422 | 562 | 399 | length_collapse_exit | 0 |
| S1 | 0 | — | — | — | — | — | — |
