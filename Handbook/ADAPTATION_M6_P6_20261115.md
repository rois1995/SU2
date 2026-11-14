# Reviewer P6 implementation — 2026-11-15

**Stopped on check3 failure; wait for a corrected reviewer proposal. No follow-up repair.**
P6 is isolated from the uncompiled cheap group. Source/build inputs1539 unchanged; build496500.164s.
CFD SHA b7c78fdb145684243e43522095233a1b85bb396057545be56efdb3a15bc280ab.
R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`; D=`R/purpose_query_fix_20261114/p6_20261115`.

| Check | Result |
|---|---|
| 1 affine/backend/entity units | PASS:77cases/8584assertions,163.569s; near-tolerance no-refinement fixture passes. |
| 2 six-rank first collapse | PASS:90809869work <=108459426; zero refinements all ranks;480/473/346/263/469/464, prediction exact. |
| 3 same run to construction | FAIL before boundary: rank2/5 each reach4refinements. No construction-entry row. |
| 4 bounded C492 BL | NOT RUN after first failure. |
| 5 build481/2D1/2/4 | NOT RUN after first failure. |
| 6 full no-BL two adaptations | NOT RUN after first failure. |

P6 source: gap-only magnitude stop; conservative base classification; ExactClassification/witness memo reuse removed;
per-purpose calls/work and upper_refinements printed. No caller need, tolerance, budget, fixture pin or quota change.
EntityFaceError tests live in CNativeGeometryBackend3D_tests.cpp, not the separately named file in the proposal.

Collapse boundary (cumulative per rank; C=classification, M=magnitude, L=limit):

| Rank | Collapses | Elapsed s | C calls/work | M calls/work | L calls/work |
|---|---:|---:|---:|---:|---:|
| 0 | 480 | 266.184 | 3498/10415996 | 459/8332523 | 0/0 |
| 1 | 473 | 266.184 | 3325/8170655 | 410/5802344 | 0/0 |
| 2 | 346 | 266.185 | 3428/8127225 | 739/5145632 | 0/0 |
| 3 | 263 | 266.184 | 2794/4556409 | 602/6444821 | 0/0 |
| 4 | 469 | 266.184 | 3812/8871372 | 347/3590762 | 0/0 |
| 5 | 464 | 266.184 | 3048/14010714 | 427/7341416 | 0/0 |

First failing intervals are surface splits: rank5 generation1 at297.576s (1refinement), rank2 generation4 at417.115s (1).
Their interval C/M/L work deltas are4491382/2530127/0 and1463256/979446/0 respectively.
Latest complete rows, all length_split_exit generation5 (partial run, not construction acceptance):

| Rank | Elapsed s | Refinements | C/M/L work | Facet work/misses | Update s |
|---|---:|---:|---:|---:|---:|
| 0 | 442.918 | 0 | 23419639/15951005/0 | 330784/13264 | 138.961 |
| 1 | 442.915 | 0 | 23061296/13590209/0 | 247266/13423 | 138.964 |
| 2 | 442.921 | 4 | 14224038/10897341/0 | 265916/7916 | 138.999 |
| 3 | 442.919 | 0 | 17536433/14155240/0 | 243154/8365 | 138.955 |
| 4 | 442.921 | 0 | 16623742/14936764/0 | 256598/12445 | 138.947 |
| 5 | 442.920 | 4 | 32418344/18718619/0 | 372476/14676 | 138.960 |

Counter increments at Common/src/adaptation/CNativeAffineReference.cpp:2148; guard throws at2386 despite gap stop2374.
This contradicts the P6 no-cascade prediction on the actual input; the new unit fixture alone did not establish it.
Individual Eng/Bnd call-site and failing-face/gap/diameter attribution is unavailable: P6 permits purpose counters only.
Raw rows/first-failure brackets/purpose deltas: D/nobl_to_construction/p6_check3_failure_assessment.json.
Authoritative manual stop: manual_stop_p6_check3_failure.json. Controller generic stop label is misleading;
it says exited-before-collapse although all6 collapse rows exist. Raw status is retained; -15 is intentional termination.
All9owned identities terminal; source/binary/input/runtime pins unchanged. Outer547.210s; load1m2.56–10.14.
Sampled peak concurrent solver RSS6372224KiB is a sampled sum, not whole-job peak.
F1 build494 counts remain unchanged. No mesh, BL preservation, transfer or full-workflow acceptance claimed.
Cheap-group drafts/provenance are preserved under D/cheap_group_set_aside and the earlier handoff_alignment_20261115.
Next action: reviewer correction of failed P6; do not rerun or design another fix.
