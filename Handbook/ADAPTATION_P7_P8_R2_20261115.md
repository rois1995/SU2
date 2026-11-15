# P7/P8 resumed validation and conditional B7

Current: **STOPPED at resumed check(a): the same P8 fixture failed again after its one authorized correction.** The time-history abort is resolved. The1MiB branch emits the expected MemoryBudget refusal; the later allowed-memory branch aborts in ImportGeometry's face/physical-label incidence gate. No second fixture correction/retry or later check. Evidence R/p7p8_r2_20261115; R=integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57. Production code and CFD binary are unchanged from0aaa321b2a.

Cause of the previous abort: State(mesh) reads current/time_n/time_n1 arrays, while the fixture was steady. Correction: TIME_DOMAIN YES, TIME_STEP .001, TIME_ITER10 and TIME_MARCHING DUAL_TIME_STEPPING-2ND_ORDER added to both configurations. This is the one authorized correction of that fixture. Same fixture failure again stops.

New user rule: one correction of an item-local fixture/script/report error is permitted; record cause/correction and continue. It is not a production contradiction or a failed result gate. Production/result-gate failures remain stop conditions; no independent design variants. Original failure/source/compile evidence remains in p7p8_20261115.

Sequence: fixture build; serial affected/new units; MPI2 subset; exact MPI6 collapse; affected C492/481/target/2D1/2/4 safeguards; one full two-event workflow with strict P7/P8, physical, transfer, continuedCFD, time, memory, timer and six-output identity gates. Print-only query counts and diagnostic counter differences are reported separately. Source/binary/runtime/input pins and owned terminal handles govern reuse.

After full success, reread reviewer2.0c. Compute need_r=event2distinct_facets_r*prepared_facet_mean_bytes_r; predicted_peak=17.70e9+SUMmax(0,need_r-175.4e6). If <=19.0e9 implement B7 as written after whole-source review, otherwise report per-rank values and total. Do not change the cache before this condition is established.

## Resumed ordered execution

| Step | Result | Seconds |
|---|---|---:|
| Fixture rebuild | PASS;1,544pins unchanged; production CFD SHA unchanged |80.115|
| (a) Serial connected units | **FAIL AGAIN in same fixture; MPI_ABORT exit1; stopped** |4.543controller /0.28process|
| Remaining serial unit cases / MPI2 subset |NOT RUN|—|
| (b) Exact collapse / C492 /481/current-target/2D safeguards |NOT RUN|—|
| (c) Full two-event workflow |NOT RUN|—|

Case: `R/cases/p7p8_r2_units_serial_20261115T190817_1380050`. Initial refusal prints process_peak_rss=job_peak_rss=33,402,880B, limit1MiB and stop=memory_budget; complete=0, accepted CFD mesh/solution retained. The unit reaches the later allowed-memory Remesh call. No earlier failed assertion is logged; this is partial evidence, not a passing case or MPI qualification.

Failure: `Native3D face/physical-label incidence is not an oriented conforming manifold.` Source: Common/src/adaptation/CNativeImport3D.cpp:1230–1239 requires either one volume+one physical label or two oppositely oriented volumes+no physical label. The rejected group's face/counts are not printed, so no new numbers are invented.

Readonly diagnosis: the fixture constructs MeshSolution with `config` but calls the allowed-memory branch with a separately constructed `enabled` config against the same geometry (ImportTests2875–2876,2889–2903). MeshSolution/physical-geometry setup initializes runtime marker tables on its construction config (TransferTestCase88–117); import reads tags from its supplied config at1063–1065 and physical labels at1184–1190. This is a likely configuration/geometry setup mismatch; exact rejected group was not independently reconstructed. No further diagnostic execution or fixture change followed the explicit same-fixture-stop rule.

Build/testpins unchanged. CFD SHA4657124b61fb6e0c0bf731c0086851fe0020cb562067d6d811d04e698db450f5; corrected test SHAdacf8d5348481ce8a606dc59a691c865841879668e7b236eb44dd2b31bc01c5d. The owned unit session and case directory have no remaining live processes; terminal_handles.json records the host check. Unrelated processes were untouched.

## Conditional B7

NOT evaluated or implemented. The full P7/P8 run did not pass or run; no event2 M18 rows exist for this resumed item. Per-rank need and predicted peak cannot be computed from the required run. The formula/threshold above is preserved, but no previous run or guessed distinct count is substituted. Denied-event default zero counters are not full-event M18 measurements. Event2 distinct facets, mean prepared-facet bytes, per-rank need and predicted peak are all unavailable. B7 waits on the required full successful run.
