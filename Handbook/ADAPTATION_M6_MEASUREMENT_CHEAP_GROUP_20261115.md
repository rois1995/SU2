# M6 same-v2 follow-up — B1/B2/B3/A2c, 2026-11-15

The single requested ProfileA/B campaign is complete. Both flat profiles pass; B also passes independent physical/transfer/continued-CFD validation. [Historical v2 tables](ADAPTATION_M6_MEASUREMENT_V2_20261114.md) remain unchanged. New source is build499/checkpoint0b56ad8f67; v2 used build494. P6c lies between them. Exact frozen485/C492 inputs/config are retained, but completed work and host contention differ: these are observations, not isolated speedup attribution.

## T0 — serial public clocks

| Window | v2/build494 seconds; samples | build499 seconds; samples |
|---|---:|---:|
|preparation|142.716039;13883|119.849549;11634|
|native|393.694166;38549|393.819145;38538|

New process wall557.165s. Native engine exit388.955s is nested within public PrepareSurface393.819s; clock/profiler/output overhead is not added to nested timers. New467splits/305000positive private tets; v2 had300/304578. Neither is an accepted BL mesh. Native test1case/916815assertions passed. New conservative per-rank GNUtime peak bound3323449344B<20GB; native-summary-only peak row is absent and is labeled absent.

## T1 — selected flat leaf groups (% of each full window)

| Window / leaf group | v2 % (samples) | build499 % (samples) |
|---|---:|---:|
|preparation/libm_nextafter|22.358 (3104)|0.645 (75)|
|preparation/interval|56.198 (7802)|72.288 (8410)|
|preparation/InStrip|0.000 (0)|0.000 (0)|
|preparation/Guidance_lambda|0.000 (0)|0.000 (0)|
|native/libm_nextafter|8.529 (3288)|0.970 (374)|
|native/interval|35.936 (13853)|45.900 (17689)|
|native/InStrip|16.322 (6292)|3.804 (1466)|
|native/Guidance_lambda|12.802 (4935)|2.896 (1116)|

Counts below100 are unresolved under v2. Zero observed samples do not prove zero calls. Interval group retains the v2 Multiply/Add/Adjacent/Blossom symbol rule, including relocated detail::Adjacent. Selected groups and broad inherited groups have different scopes; do not add overlapping interpretations.

| Native broad exclusive group | v2 % | build499 % |
|---|---:|---:|
|wall_proof|1.785|3.991|
|exact_predicates|8.807|16.828|
|target_BL|29.306|6.835|
|tree|1.502|2.125|
|memory|2.130|3.739|
|MPI|0.005|0.023|
|pack_unpack|0.145|0.174|
|cavity_topology|0.215|0.182|
|other|56.105|66.104|

Broad exact-predicate share16.828% does not establish cost of A5’s particular SurfaceNormalSign: only28/5934LBR samples expose its ancestry (below100resolved threshold). No recorded M6 operand/gain corpus or filter is claimed. B5 has material global EdgeNormal/BuildMobilityPolicy ancestry (2322/4116interval stacks), but unchanged full-policy consumers contradict a touched-point-only implementation. Source lines and scope limits are in D/conditional_A5_B5_source_review.md; no replacement design is introduced. F3 still owes exclusive timing/95%coverage.

## T7 — optional conditional caller evidence

| LBR window / leaf group | v2 denominator | build499 denominator |
|---|---:|---:|
|preparation/interval_arithmetic|3178|4116|
|preparation/libm_nextafter|1355|36|
|native_surface/boundary_guidance_lambda|1634|700|
|native_surface/in_strip|2301|1004|
|native_surface/interval_arithmetic|808|2061|
|native_surface/libm_nextafter|98|33|

Both new60s LBR captures decode0 (5786preparation/5934native stacks); raw captures retained. Denominators are selected sampled stacks, not invocations. Truncated/unknown ancestry remains explicit; no exact calls per guidance evaluation can be inferred. Full nearest caller prefixes and engine-step categories are in the linked evidence.

Evidence: R/purpose_query_fix_20261114/cheap_group_20261115/profile/run_A/{split_profiles.json,samples_summary.json,caller_summary.json,nearest_callers.json,requested_profile_receipts.json,analysis_provenance.json}; P/A_side_by_side_measurements.json owns exact table values. All1540source pins, binary/runtime/config/input/helper hashes verified before/after; all A identities terminal. Historical sample text is losslessly gzip archived with restoration hashes; raw perf data and old tables are retained.

## T2 — Native event time and mesh

| Run/event | Native elapsed | Mesh points | Mesh cells | Q/J/size/geom bad | Acceptance/work scope |
|---|---:|---:|---:|---|---|
| Historical v2 `improvement` stage exit | 5344.548s elapsed | unavailable | 697,183 stage-exit cell sum | 19,811/19,523/not recorded/525 | legacy stage output; not current strict workflow receipt |
| Historical v2 `construction` stage exit | 5324.467s elapsed | unavailable | 685,450 stage-exit cell sum | 0/0/not recorded/1,482 | legacy stage output; not current strict workflow receipt |
| Current B event 1 | whole event max rank 2697.000s; native total max rank 2697.000s; refine detail 2605.604s | 146,676 | 726,442 | 0/0/0/0 | independent physical workflow accepted; P6c payload match |
| Current B event 2 | whole event max rank 3684.252s; native total max rank 3684.252s; refine detail 2419.099s | 168,790 | 855,760 | 0/0/0/0 | independent physical workflow accepted; P6c payload match |

Historical v2 logged one construction and one improvement exit stage; the rows carry legacy stage timers and cell sums, not global point totals or the current independent workflow receipt. They are not paired as equivalent completed events to current B. Current B’s six rank-local initial/final cell counts are in the JSON receipt; reported global mesh counts above are the independent validator totals.

## T3 — Maximum phase duration and slow rank

The table lists profiler phase labels as emitted; repeated intervals with the same label are summed per rank before taking the maximum. Sample shares are weighted across assigned samples for that phase, not wall-wait or slowest-rank causal attribution. Bracket ambiguity is excluded.

| Run/epoch | Phase label | Max duration | Slow rank | Assigned samples (all ranks) | MPI / tree / memory sample share |
|---|---|---:|---:|---:|---|
| Historical v2 e1 | initialization_collapse | 1423.519s | 5 | 828,622 | 80.11%/0.40%/0.43% |
| Historical v2 e1 | interior | 59.010s | 5 | 28,865 | 51.90%/11.02%/8.39% |
| Historical v2 e1 | phase_setup_repartition_staging | 65.328s | 1 | 32,742 | 42.71%/10.27%/5.90% |
| Historical v2 e1 | quality | 1240.774s | 5 | 676,109 | 78.69%/0.28%/0.77% |
| Historical v2 e1 | quality_until_failed_return | 510.245s | 0 | 287,616 | 79.89%/0.17%/0.60% |
| Historical v2 e1 | repartition_commit | 0.081s | 2 | 0 | n/a/n/a/n/a |
| Historical v2 e1 | surface | 1.519s | 4 | 94 | 41.49%/5.32%/2.13% |
| Historical v2 e1 | surface_with_setup | 2042.834s | 4 | 1,156,211 | 79.09%/0.59%/0.73% |
| Historical v2 e1 | transition | 1.502s | 1 | 496 | 0.60%/31.85%/35.28% |
| Historical v2 e2 | initialization_collapse | 1521.328s | 1 | 853,952 | 80.38%/0.40%/0.46% |
| Historical v2 e2 | interior | 1058.662s | 0 | 351,215 | 94.48%/1.46%/0.95% |
| Historical v2 e2 | phase_setup_repartition_staging | 4.682s | 0 | 2,464 | 13.88%/10.43%/13.92% |
| Historical v2 e2 | surface | 2.473s | 2 | 1,248 | 34.05%/19.79%/13.54% |
| Historical v2 e2 | surface_with_setup | 2730.301s | 1 | 1,342,275 | 81.82%/0.57%/0.68% |
| Historical v2 e2 | surface_with_setup_until_failed_return | 7.085s | 2 | 4,081 | 67.36%/5.29%/2.08% |
| Historical v2 e2 | transition | 0.164s | 3 | 0 | n/a/n/a/n/a |
| Current B e1 | initialization_collapse | 236.176s | 5 | 137,132 | 72.48%/2.25%/1.79% |
| Current B e1 | interior | 55.831s | 2 | 27,782 | 52.63%/10.77%/8.14% |
| Current B e1 | phase_setup_repartition_staging | 50.965s | 1 | 27,900 | 23.24%/11.96%/8.98% |
| Current B e1 | quality | 1317.431s | 5 | 762,009 | 55.35%/2.46%/3.73% |
| Current B e1 | quality_until_failed_return | 543.946s | 3 | 315,928 | 4.64%/1.26%/4.83% |
| Current B e1 | repartition_commit | 0.135s | 4 | 0 | n/a/n/a/n/a |
| Current B e1 | surface | 1.069s | 0 | 68 | 38.24%/8.82%/5.88% |
| Current B e1 | surface_with_setup | 265.587s | 1 | 150,520 | 49.72%/4.12%/3.08% |
| Current B e1 | transition | 135.219s | 2 | 77,934 | 56.94%/2.25%/3.62% |
| Current B e2 | initialization_collapse | 286.105s | 2 | 164,814 | 73.05%/4.88%/1.85% |
| Current B e2 | interior | 24.426s | 4 | 13,661 | 50.76%/14.63%/10.91% |
| Current B e2 | phase_setup_repartition_staging | 56.133s | 5 | 31,308 | 26.75%/12.58%/8.23% |
| Current B e2 | quality | 987.887s | 5 | 550,550 | 61.61%/2.25%/3.19% |
| Current B e2 | quality_until_failed_return | 1047.067s | 2 | 414,817 | 3.13%/1.38%/4.95% |
| Current B e2 | repartition_commit | 0.081s | 0 | 32 | 6.25%/50.00%/0.00% |
| Current B e2 | surface_with_setup | 16.257s | 0 | 9,143 | 26.19%/25.08%/8.67% |
| Current B e2 | transition | 1.267s | 3 | 682 | 3.37%/32.26%/34.02% |

The requested collapse/surface split/interior split/repartition/quality/alternation/final-finish labels are not all separately observable in these receipts. The inherited analyzer label `quality_until_failed_return` names an interval ending at its return breakpoint; it is not a failed current workflow, which independently passed. `initialization_collapse`, `surface_with_setup`, `interior`, `phase_setup_repartition_staging`, `repartition_commit`, `quality`, `quality_until_failed_return`, and `transition` are preserved as their actual analyzer labels; transitions are not relabeled as pure repartition or alternation.

## T4 — Whole-lifetime and phase MPI/tree/memory sample shares

| Run | Whole-lifetime MPI | Whole-lifetime tree | Whole-lifetime memory |
|---|---:|---:|---:|
| Historical v2 | 79.19% | 0.71% | 0.85% |
| Current B | 48.81% | 2.42% | 3.48% |

Whole-lifetime shares are all-rank sample-weighted on-CPU samples. Their six-rank averages/ranges and counts are in the JSON. They do not measure wall wait. Phase shares appear in T3.

## T5 — Peak RSS and A2c facet resolutions

| Evidence | Peak sum | 20 GB gate | Notes |
|---|---:|---|---|
| Historical v2 | 14.713 GB observed max `process_peak_rss` per rank (stage rows) | indicative only; not event-separated | no v2 event receipt for this gate |
| Current B event 1 (receipt index 0) | 13.934 GB (13,933,756,416 B) | pass (limit 20,000,000,000 B) | native per-rank process peak summary |
| Current B event 2 (receipt index 1) | 17.658 GB (17,657,745,408 B) | pass (limit 20,000,000,000 B) | native per-rank process peak summary |

| Current B receipt event index / rank | Misses | Resolutions | `misses − resolutions × 48,954` | Residual / misses |
|---|---:|---:|---:|---:|
| 0 / 0 | 108,640 | 1 | 59,686 | 54.94% |
| 0 / 1 | 115,091 | 1 | 66,137 | 57.46% |
| 0 / 2 | 104,394 | 1 | 55,440 | 53.11% |
| 0 / 3 | 110,003 | 1 | 61,049 | 55.50% |
| 0 / 4 | 103,420 | 1 | 54,466 | 52.66% |
| 0 / 5 | 102,299 | 1 | 53,345 | 52.15% |
| 1 / 0 | 246,144 | 1 | 197,190 | 80.11% |
| 1 / 1 | 236,237 | 1 | 187,283 | 79.28% |
| 1 / 2 | 235,662 | 1 | 186,708 | 79.23% |
| 1 / 3 | 227,240 | 1 | 178,286 | 78.46% |
| 1 / 4 | 235,322 | 1 | 186,368 | 79.20% |
| 1 / 5 | 231,394 | 1 | 182,440 | 78.84% |

Receipt event indices0/1 mean workflow events1/2. Historical v2 has no matching per-rank `facet_resolutions` values, so A2c is unevaluated there. No query-pool, cache-quota or memory-model change was made.

## T6 — Post-native profile and evidence limitations

Optional event-2 post-native LBR has 5,339 weighted stacks: visible `EdgeNormal`/`BuildMobilityPolicy` ancestry 1,893 (35.46%), visible MPI ancestry 812 (15.21%). These are stack sample weights, overlapping ancestry labels, not invocation counts or exclusive time.

The optional observer tool terminated with exit143 mid-event2. The main runner exited0 after6920.110s and all six flat recorders/parsers exited0; all owned identities are terminal. All1540source pins and binary/runtime/config/input/helper hashes match before/after. This does not invalidate flat sampling: raw program logs and GDB clock brackets completed, the analyzer uses inherited guaranteed epoch brackets (direct GDB timer epochs are unavailable), and the optional post-native LBR decoded successfully. The workflow receipt is a read-only physical/numerical validation of the requested profile B outputs; it claims no fresh isolated restart run.

Current B and P6c outputs match byte-for-byte for both events’ mesh, native reference, and restart files. Historical v2’s profiled exit stage data has different bad-cell/work outcomes, so runtime differences are descriptive only; no causal speedup is inferred.

The epoch bracket is `AdaptMetric_entry <= actual start <= Adapt_timer_epoch_unavailable`; direct `_M_d` member lookup failed. Rank0 widths are0.322841879s (event1) and0.014010531s (event2). Samples are assigned only to guaranteed intervals; boundary ambiguity is excluded, not silently attributed. Event1/2 native times2696.999720/3684.251621s each pass the5400s limit; their6381.251341s sum and6920.110s profile wall time are different scopes. Legacy timers overlap and must not be added as exclusive time. F3 remains pending.

## Provenance and output identity

Build499/checkpoint`0b56ad8f67`; CFD SHA256`87d6d83baddd1999347b6d332ee78c8ac0ee4b73b4644a95c607cd301f0a7241`; test SHA256`a787f09b2bd5401e2997ff911c64ce36720987f043309e8b872dbf88588cda4c`. B's runtime manifest SHA256 is`e2daaed3b3b33a648db277ce13f2172d51eb919f5f36e1c95456fe42af6357d7`; full runtime mappings and all unchanged-input checks are retained in `D/profile/run_B/status.json`. The current validator changes only log placement through recorded symlinks; strict numerical, physical, resource and continued-CFD checks remain intact.

| Workflow event | Mesh SHA256 | Reference SHA256 | Restart SHA256 |
|---|---|---|---|
|1|`2aea6a341b0de7c3175aef7c9671c9f42a17a767c5acc35478a8d89bef87eca7`|`9bddc9a6317e82cf732adb13ab157c690c46ee7fd820eebf62bf672553e129b9`|`b77362d32483541b97c9ead50bc52ff33e90a0593db3c5d363977a61331c4091`|
|2|`11f7b7422ef5d0c17d85666ae265ea7a590bcaf729daeb2df56030d39a660be0`|`bc49c1af6f6d691e1c58927470b04b4336336d21f043684a035a6d11394f30db`|`3be66436bd544fca0be3fcc2850c20f6b7e26a9cf7d0e777437a59f1c9272afd`|

Each hash matches P6c. No new isolated restart is claimed. Historical v2 and current B complete different work, so these tables establish observations and result identity to P6c, not an isolated cheap-group speedup.

B evidence: `D/B_measurement_summary.json`, `D/B_measurement_tables.md`, and `D/profile/run_B/{status.json,requested_profile_receipts.json,independent_workflow_receipt.json,P6c_payload_identity.json,run_B_phase_samples.json,samples_summary.json,timers_summary.json}`. `D` is `R/purpose_query_fix_20261114/cheap_group_20261115`. The full runtime/source provenance is in the original receipts; historical receipts/tables remain unchanged.
