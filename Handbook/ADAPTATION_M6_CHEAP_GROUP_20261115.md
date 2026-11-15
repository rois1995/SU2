# B1/B2/B3/A2c — 2026-11-15

Numerical checks all PASS once, with source/binary/runtime/input pins unchanged. Same-v2 profileA passes; profileB is active. No new standalone fullM6 run. Curved BL/RANS capability remains open.
R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`; D=`R/purpose_query_fix_20261114/cheap_group_20261115`.
Reviewer update2.0 accepts the source-gap report: B4/B6 withdrawn, A2 closed/replaced by A2c. No query-purpose, pool/quota, eviction, budget, quality, height/progression or target semantics change.

Implementation: exact x87 zero/subnormal/smallest-normal adjacent stepping; synchronous preparation guidance/strip memo with exact point/binding/sensor keys and target invalidation; distinct facet-resolution count in CurvedSearch/stage output. Reuses the preserved drafts and existing admission. Frozen build499/source checkpoint0b56ad8f67,1540pins. CFD87d6d83baddd1999347b6d332ee78c8ac0ee4b73b4644a95c607cd301f0a7241; test a787f09b2bd5401e2997ff911c64ce36720987f043309e8b872dbf88588cda4c.

| Ordered check | Result | Wall seconds |
|---|---|---:|
| Build499 | terminal0/all1540pins unchanged; nice19/j4/libs1/4TBscratch |660.202|
| Units |79cases/20580assertions, bit-exact stepping and guidance samples |196.655|
| Six-rank first collapse |exact90809869upper work, counts480/473/346/263/469/464, zero refinements; prescribed boundary stop/all9terminal |1272.214outer;933.125maxrank native|
| Serial C492 BL |462splits; first336exactlyP6c;304988positive private tets; zero refinements |547.142outer;390.199API;389.200engine|
|481merit |56cases/5943assertions |123.752|
| Saved current target |611527assertions/1case; old pins not edited |151.553|
| Native2D1/2/4ranks |39cases/rank, all pass |6.896/6.844/6.761|

BL first336 are identical; record336 is an additional edit after the reference ends, not a changed prefix. Geometry is valid but surface_prepared=false, engine_valid=false, census incomplete. No accepted BL mesh or exact scheduler resume claim. Output payloads stay local, SHA-sealed in D/BL_receipt.json. Memo94931guidance hits,0strip hits,16830144live bytes. Different amount of completed work; no isolated speedup claim.

A2c at first-collapse boundary:1resolution each rank; facet misses13026/12382/7734/6477/12001/13785. Requested raw lower-limit formula gives negative values there (no positive rebuild lower bound). Serial BL exit:2resolutions,6186misses, raw6186−2×48954=−91722; not one resolution despite zero refinements. Full-event numbers await profileB. No quota change.

Host contention is explicit: unrelated Lean work shared CPU with nice19MPI6, load reached~16; first-collapse work/partition/counts match P6c. Do not attribute slower wall time to a code regression or add overlapping counters as exclusive CPU.

20GB decimal gate: accepted historical P6c native peak sums13,969,158,144/17,700,634,624B pass. New serial BL uses exact GNUtime process peak, not sampledRSS; other serial/2D checks also record conservative peak bounds in final_checks_sequence.json. The intentionally terminated collapse run has no final native summary; VmHWM history is retained, not claimed as a final native peak sum. Profiles retain native sums when present and labeled per-rank GNUtime descendant bounds otherwise. The12288MiB model and query quotas remain unchanged; reviewer owns the total-memory design.

Historical v2 sample text is losslessly gzip archived931,661,039→68,713,365B with full decompression SHA verification/restoration manifest D/historical_profile_text_compression.json. This keeps the unchanged global2GB profiler cap. Raw perf data and analysis tables retained. No evidence content deleted.

ProfileA: terminal0/allowned terminal/source1540+binary+runtime/input pins unchanged,557.165s. Catch1case916815assertions;467splits/305000positive,0refinements,2facet resolutions. Preparation119.849549s/11634samples; public native393.819145s/38538samples. Both optional LBR captures decode0 with5786/5934stacks; raw LBR retained. Serial conservative peak bound3323449344B<20GB; no native-summary peak row claimed. Source v2 helpers and methods frozen in profile provenance.

Next: finish exactly one same-v2 A/B profile, put new tables beside v2, then F3 exclusive timer tree/95%coverage. Reviewer design remains read-only; no8.2–8.7 implementation before user acceptance.
