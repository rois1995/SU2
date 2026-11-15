# Night work progress

Updated 2026-11-15. Latest order: NIGHT_WORK_ORDER.md (SHA-sealed input_manifest.json). Section2 copied verbatim into AGENTS.md as latest user order. No questions or reviewer waits.

| Item | State | Commit / gates | Next |
|---|---|---|---|
| N1 P7/P8/counters | FULL RUN ACTIVE; do not stop/restart | Production0aaa321b2a; fixture corrections uncommitted; serialP8/MPI2/counterfixtures pass; exactcollapse90809869 andcounts pass; C492461splits/first336exactP6c;481/target/2Dpass | Observe existing fullworkflow only |
| N2 B7 | NOT STARTED, conditional | Need N1 fullpass/event2M18 | Computecondition afterN1pass |
| N3 design source check | NOT STARTED | independent;3hourlimit | afterN1/N2 |
| N4 Adapt cut8.2 | NOT STARTED; user acceptance received | ReadC/inventories atstart; recordG2/G3/GQreferencebeforeedit | acceptedN1/N2base, orF3bafterfailed/revertedN1 |
| N5 all design measurements | NOT STARTED | use newestaccepted commit | afterN4 |
| N6 further identical cuts | NOT STARTED | requirescompletedN4+G5 | Remesh,MeshOptimizer,Selection |
| N7 morning report | NOT STARTED | always | finalclose/terminalhandles |

Live owned job: tool28458, R/p7p8_r3_20261115/full_workflow/status.json. AppPID1509949/start_ticks26495566/PGID1509949. Compiled C++ and pinned helpers FROZEN. Read this progress and NIGHT_WORK_ORDER.md after every compaction; verify existing handles before launches/edits.

R5 governs resultfailures: preservefirstnumber; one<=60min readonlydiagnosis; onlytranscription/wiringagainstwrittenproposal permitsonecorrection+onefailedcheckrepeat; else revertitem, markFAILED andcontinueindependentnextitem. R3ownharnessrepairs<=90min each, no gate/productionchanges. Do not implementdesign8.3-8.7,A5/B5 or job-derivedquota.

N1 interim update: in-run first-collapse gate PASS again (90,809,869work, expectedcounts,0refinements). Fullrun remains active; construction intermediate deficits are not final acceptance failures. AllprecedingchecksPASS aslistedabove; no newproductionedit. Currentaccepted reference remainsF3b untilN1fullclosure.
