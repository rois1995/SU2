# Section 6.1 accepted P7/P8 and counters

PASSED on build522 (source restoration7c56bab282 atop accepted N5 3d28237f03). One full run, no full repeat needed. Production conflict resolution retained both the Runtime/D9 includes and the N5 Finish observer around the P7 owned proof; the test append retained both fixtures. See source_check.md/restoration_receipt.json.

| Check | Result |
|---|---|
| Build |880.347348s,347 actions;1560 source pins unchanged|
| Units |serial43cases/1697assertions119.558236s;MPI2P8 4.527206s/13assertions per rank;GeometryStats included|
| Collapse |90,809,869work;480/473/346/263/469/464accepted;0refinements. Legacy controller also reached construction guard;634.619724s sealed is not collapse-only|
| C492 |542.492544s;486splits;first336 identical;all private cells positive;unchanged388.8s stage. No accepted BL claim|
| Safeguards |48156cases/5947assertions122.016101s;current492target611527assertions149.018121s;2D1/2/4 39cases/rank11.007315/12.007212/11.006833s|
| Full |normal0;4286.332645s;physical/transfer/continuedCFD/time/memory/timers pass;all handles terminal|
| Isolated event2 restart |normal0;245.402971s;168797points/855828cells;coordinates exact;positive finite flow;one continued iteration;rmsrho-2.410902,CL.27025,CD.015528;all11identities actual-host terminal|

| Event | Accepted native s | F3b s | First N1 s | Accepted peak SUM B | F3b peak SUM B | Proof SUM faces | Min coverage |
|---|---:|---:|---:|---:|---:|---:|---:|
|1|2161.697076749|2683.411540761|2220.339770577|14028087296|13950185472|67170|.9634940740207923|
|2|1634.828953158|1977.214813802|1450.966461726|17349582848|17696780288|62912|.9677704413546446|

Both job peaks exactly equal the sum of six process peaks; all lower bounds are at most those final peaks and both are below19,999,490,048B. Event1 has146676points/726442cells;event2 has168797points/855828cells. Event1 matches P6c and event2 matches the supplied original P7 reference, proving repeated output without another full run. Six accepted hashes are in accepted_reference_payloads.json. P7 query calls31622/35298 are diagnostic; event2 differs by7 from F3b35291 because its mesh differs, not a gate failure.

All549 timer rows are side-by-side with F3b and first N1 under reports/current_profile_vs_reference_timer_rows.csv and current_vs_first_N1_vs_reference_timer_rows.csv. Current quality windows192. These are wall times; inclusive scopes are nested, join/collective intervals may overlap. No additive CPU claim. Qualified M18/M20/M21 are in accepted_counters.json and the adjacent concise exports; absent M21 records remain null, never zero.

The first isolated restart remains preserved (normal0/131.149530s), with its older R1 monitor reporting max8 including browsers and a short-lived Python inspector. Latest user clarification excludes user programs and requires own scripts to sustain>half core>60s. Browser activity alone is no failure/repeat reason. No retrospective claim of an owned sustained violation from short samples. The already-completed corrected restart used two CPUs for six MPI ranks and inspector; max3 busy over67windows, sticky rank HWM sum1,864,327,168B. Its timing is not directly comparable to the first restart's CPU allocation.

Every own helper setup correction is in harness_review_corrections.json, restart_helper_corrections.json and validator_loader_receipt.json; the validator namespace repairs repeated only the report validator. No production gate or tolerance changed. The previous first N1 failure/revert becb4927c9 remains historical in Handbook/ADAPTATION_P7_P8_R3_20261115.md; section6.1 explicitly supersedes its old event2=P6c gate.

Next6.2: reread/archive the updated Night section6, then compute B7 condition from these accepted M18 rows. N7 is not finalized.
