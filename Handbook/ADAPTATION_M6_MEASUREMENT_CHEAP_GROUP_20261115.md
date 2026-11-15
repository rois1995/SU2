# M6 same-v2 follow-up — B1/B2/B3/A2c, 2026-11-15

ProfileA complete; profileB active. [Historical v2 tables](ADAPTATION_M6_MEASUREMENT_V2_20261114.md) remain unchanged. New source is build499/checkpoint0b56ad8f67; v2 used build494. P6c lies between them. Exact frozen485/C492 inputs/config are retained, but completed work and host contention differ: these are observations, not isolated speedup attribution.

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

Exact-predicate share16.828% is material for conditionalA5 source review; no filter has been implemented. Mobility/preparation attribution remains in the LBR caller data; no lazy-mobility design or arithmetic change is introduced. F3 still owes exclusive timing/95%coverage.

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

## T2–T6 and full-event A2c

Pending the single running six-rank profileB. Report every rank/event exit, native peak-RSS sum<=20GB, facet_resolutions and misses−resolutions×48954; keep nested timers and contention separate. No further numerical run or quota/model change.
