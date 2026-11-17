# Night work — current progress

Updated 2026-11-17. Read the complete latest `NIGHT_WORK_ORDER.md` after every compaction, then this file.

## Current boundary — N14/N15 PASSED, no owned heavy job

N14/N15 qualification is COMPLETE and PASSED; `N14/section6_12/acceptance_summary.json`. Full5524738.250709s, all6payloads EXACT E1P6c/E2P7, all standing gates. Full primary receipt sealed10actual handles/1775source+212input+25helper pins. Isolated restart PASSED236.822017s,168797points/855828cells, coordinate error0, finite positive changed conservative fields;11actual handles/2022case+10helper pins sealed. Restart exec1603/validator8245exit0. The numeric status deliberately remains disk_restart_validated=false; separate disk_restart_validation.json is the authoritative passed validation (the primary sealer initially assumed a mutable flag and was corrected without rerun).

M26 report PASSED, both events6ranks,48prepare rows each, all96timer summaries fixedstorage4120; commits22278/13622 agree with N12 and class sums. `reports/M26_full552.md` and JSON/CSV. Post-helper timer grouping initially rejected non-remesh summaries after final; corrected to validate all summaries but group only remesh roots, synthetic+actual report passed. No production code, gate or tolerance changed and no numerical case repeated.

Fresh actual qualification-boundary preflight found NO owned heavy job. Exact qualified552 CFD/test binaries (1,352,928,592B) copied to build552/qualified_binaries with independent inodes/hash manifest; DO NOT Git-stage them. Qualified references adopted in validation/successor_reference_selection.json, G1newmanifest under validation/adopted_reference. Changed G1wallcases: NONE in all3cohorts. G2/GQ552cases and firstN14G3 are the new cut references, with no extra numerical reference run.

NEXT: commit/push the qualified N14/N15 evidence/reference boundary, then rebind/review/apply B3 as three source paths and build553. No production B3 edit has happened yet. All earlier check details and correction history remain below.

## Latest order and stop point

Latest order: N14 released under Night 6.12, using the strict **split-point lift**, with N15 in the same build. AGENTS was amended at the committed/pushed B2 terminal boundary before N14's first production edit. Full 825-line order SHA256: `9cd597fdaab45fdb7b6010034e2144c6d912c4a53167650a919a1e0a9db9dbb1`; archived in `N14/section6_12/correction_1010`.

If N14 passes, its G1/G2/GQ/G3 become cut references without another reference run. The corrected 10:10 bullet ends P6c first-336 edit identity for later cut G3 checks and uses the **first N14 G3 run**; at least 336 splits and all other physical rows remain. If N14 is reverted, retain old G3 references.

Finish N14/N15, then B3–B8 and end-B G3/G4/GQ. After passed end gates and commit/push, update progress/HANDOFF, verify no owned job, record terminal handles, **STOP AND WAIT FOR USER. DO NOT START PHASE C.** If phase B cannot finish under R5, stop after its required revert is committed/pushed. N10 is withdrawn; do not implement 8.5ab. N6/N7/C/D are deferred at this planned stop.

## Accepted state and pending checkpoint

- Newest accepted cut: **76b2ad61db**, B2 scoped G1/G2 passed and pushed.
- Newest fully qualified workflow: **5ec465ca1a**, phase A, build545; passed full/restart and all end-A gates.
- N14/N15 implementation checkpoint **93ea0f93e8** was preserved as pending; its production code is now qualified by completed build552 gates. Evidence/docs b09fa8046d/cb5f79ee86/213fb7af05/30c74c7a67 are pushed. Acceptance summary adopts the current code-equivalent source parent30c74c7a67; the qualification commit follows.
- Shared build: `integration_evidence/build-integrated-v3-p5-20261114`; N14 receipts in `N14/section6_12/build552`.
- CFD SHA256: `9ef34ecc72357e5deb31256e4c5739fedca2170f80f304996a5284e3a280ca35`.
- Test SHA256: `efbfd37bb53d947f1b194860b76c76c9e29bf1b27feae19ecea8808017feac8a`.
- Engine source SHA256: `7545edb4cea518fffc7e0e5cd4f763891fbe2a002b2d26b177b04b76e0d3b848`.
- Whole connected source check: `N14/section6_12/source_check.before_first_edit.md` (SHA c8655c756c5e73702fd285d14442c650616847ae70335e4d097b0162f8ba9edb), canonical `source_check.md`, separate diagnostic-scope clarifications. No blocking result contradiction.

## N14/N15 completed checks — do not repeat

All entries below passed on their first numerical attempts. Actual terminal identities and pins are sealed in the corresponding primary receipts under `N14/section6_12/validation`.

| Check | Time, seconds | Result |
|---|---:|---|
| Build552 | 120.173935 | 1775 unchanged pins; 20 observed actual IDs terminal; exec69638 |
| Focused | 4.862019 | 113 assertions / 12 cases; 1982 pins; 3 IDs; exec70279 |
| G1 serial / MPI2 / MPI4 | 798.622486 / 13.881759 / 16.963543 | 375 / 114 / 228 records; zero case changes including wall cases; 1989 pins each; 6 / 7 / 9 IDs; exec47941 / 47646 / 17371 |
| G2 stop / sealed | 358.613009 / 367.124284 | Work 90,809,869; counts480/473/346/263/469/464; all old integer/model rows exact; 12 timer snapshots satisfy exact census delta; exec91130 |
| GQ stop / sealed | 1234.610042 / 1242.824580 | All six third windows; all old integer/model rows exact; 18 timer snapshots exact census delta; exec68268 |
| G3 first | 624.866412 | 1113 splits >=689; all physical gates; 6 IDs; exec35565 |
| G3 second | 625.957396 | 1099 splits >=689; all physical gates; 6 IDs; exec40663 |
| G3 pair | post-only | All 1099 common edits identical; `n14_g3_pair_comparison.json` |
| Six-rank wall report | 577.391424 | Normal exit, splits82/91/108/76/102/0 (459 total); 10 IDs; exec51178; report only |
| Merit481 | 127.197434 | 1982 pins; 5 IDs; exec71159 |
| Saved481 | 157.009231 | 2043 pins; 5 IDs; exec25307 |
| Saved current | 154.506960 | 2043 pins; 5 IDs; exec71554 |
| 2D 1 / 2 / 4 ranks | 7.213273 / 7.014112 / 7.095061 | 1979 pins each; 5 / 6 / 8 IDs; exec43347 / 60801 / 74493 |

G2/GQ observed prefix rank-HWM sums: 7,961,542,656 / 14,030,856,192 bytes, **not full-workflow P8 peaks**. MPI wall conservative process-VmHWM sum: 5,483,991,040 bytes. G3 modeled max: 11,215,009,617 bytes <12GiB; positive/sensor cells306462 /306427; zero upper refinements. G3 stage elapsed includes about75.9s post-budget exit law census; trial work stopped around388.9s with configured388.8s unchanged. See `reports_G3_pair/g3_pair_audit.md`. These stage fixtures do not claim an accepted BL mesh.

Required/checked counts reconcile: first731 required accepted +33 checked accepted=764 geometry, plus349 size=1113. Second717+33=750 geometry, plus349=1099. Child counters are rank-local: new_faces2916/2864 are measured matched children; pending8/4 are disjoint. Total registered2924/2868 equals4x731/717. Parent known/unknown partition measured children (2388+528 /2336+528). Zero above parent+tolerance applies only to measured known-parent faces; no complete or remote coverage claim. Earlier report labeling/overlap wording corrected post-only with exact old audits preserved.

## Next ordered actions

1. Commit/push the qualified N14/N15 evidence and accepted references. Preserve exact binary archives locally; no more552 numerical run is required.
2. Rebind the B3 generator to the accepted frozen engine SHA7545edb..., review the actual3-file diff, then apply Census extraction. Build553 (actual resource/source freeze), G1serial/MPI2/MPI4 by exact C2.1 against adopted552 tables (no wall exceptions, success assert totals/launcher codes report-only), strict G2 against552. One commit/push for the connected B3 substep. Actual prelaunch setup stubs before numerical launches where wrappers are retargeted.
3. Continue B4Queues, B5Reports/finish, B6Ownership, B7Control, B8Setup, each scoped G1/G2 and commit/push. Rebind drafts after prior cuts; preserve live aliases, type aliases, original lifecycle/collective/container order and all budgets. No behavior fixes.
4. End-B G3/G4/GQ. G3 uses first N14 G3 as edit reference to smaller count, >=336splits and retained physical gates; no P6c first336 equality. Then commit/push, update progress/HANDOFF, verify no owned job, record terminal handles and **STOP; no phase C**. If phase B cannot finish under R5, stop after the required revert commit/push.

## Own-helper correction ledger (no production/gate/tolerance changes)

- G1: three setup errors before any test launch (RUNTIME module, fresh receipt paths, missing inherited aliases); corrected actual setup stub passed, then each numerical cohort ran once. `validation/R3_G1_setup_attempt1/2/3`.
- G2/GQ: prelaunch inherited interface/path/argument-order corrections; actual verification passed1803 pins before any prefix solver. `validation/R3_G2_prelaunch_review`.
- Full helper: copied symlink contents changed import roots; restored symlinks to exact original targets. `R3_full_helper_setup`. No numerical repeat.
- Safeguards: first exec95768 failed BEFORE numerical test because Popen monitor wrapped ldd, then missing cleanup masked it and left an owned stale lock. Actual failed handles sealed; original helper/log/lock/pins preserved. Exclude ldd and restore saved Popen directly. `validation/R3_safeguard_setup_attempt1`.
- After merit passed, monitor review found live polls did not sample descendants. Existing merit case sampler/terminal handles covered the full run, so primary sealed it without repeat; corrected future polling. Its short nonnumerical self-check initially used a symlinked executable and was corrected to the resolved path; passed. `validation/R3_safeguard_resource_monitor`. The old custom merit3.6MB sample is not a peak; full case observed82,264,064B.
- G3 paired Markdown report had a misplaced second table row; corrected post-only and verified columns. CSV CRLF normalized only in new report output. `reports_G3_pair/report_setup_corrections.md`. Historical raw CSV/logs preserved and exact gzip archives round-trip verified.

## Remaining cut preparation — unapplied

P=`N4/section6_3_after_N11`. Whole-N4 source check `P/source_check.md` remains controlling. No B3 production edit/build/test yet.

- B3 final generator: `P/B3_preparation/proposal/generate_b3_proposal.py`, SHA f1ac3f4f75562d4494f8ea9e8b598e1ab79704638f08ad800e063008856a4a68. Manifest SHA7ad18e014cc16d037be73944a2ab4a2d60d4e1c0aee6700120fc7a09dfecfda7. Proposed census CPP SHAe208dcae1331dffd25256577dccb3eef14025319d1b99269c95ccd14bf0df599. Nine methods satisfy the <=80-line rule after the local type-alias correction. Primary reviewed all bodies/bindings/lifetimes; draft setup defects corrected (missing/duplicate surface_target, restored field_identity used by FrozenField; method-local Phase=EnginePhase aliases for three methods). Actual six lambda token proofs and main byte reversal pass. Must regenerate/rebind to post-N14 source, review final diff, then apply only at terminal boundary.
- B4: `P/B4_preparation/proposal`, final generator SHAa44eec84e1f2751bf699d3fe968224766cec14fa091738e2d7f5204b720f70a4. Four methods <=80, actual lambda token proofs/main reversal pass. Primary found template lookup order defect; corrected helper definition before caller. Needs post-B3 rebinding/review/build/gates.
- B8: read-only setup/lifetime inventory in P/B8_preparation/binding_inventory.md, SHAe5431c532020b94872aaa124c2a1bd3964a62c4aaed7071e1bdf32cb7d326368. No production edit/job.
- B5: inventory and unapplied partition plan only, **no generated code/proof yet**. Primary must implement after B4; preserve finish once, timer/election/proof order and exit observer position.
- `B3_preparation/validation_drafts/successor_553` contains concrete G1/G2/GQ successor drafts. Reference selection deliberately unbound until N14 disposition. Normalize accepted552 reference manifest/reports; no wall exceptions for later cuts; successful-assert totals and MPI launcher returncode9vs10 are observations under C2.1.
- Corrected B6/B7 inventories are in P/B6_preparation and P/B7_preparation (SHAs14e901d2030e28f4497bca9caeffce318748247d1db72a21b148779c0b41ddcb /5b138f654ca54843a296700f0017db398874a9a9c42e15aac184428d91bb7d47). Earlier N4-level drafts used a historical source check and are superseded, not new contradictions. No production edits/jobs were made.

## Earlier ledger and operating rules

B1 c8fd7ff400 and B2 76b2ad61db scoped G1/G2 passed/pushed. N13/M25 8ab6da791e passed/pushed (2408 exact prediction-prefix checks; CFD unchanged). PhaseA5ec465ca1a full native2583.997544/1727.465975s, process-peak sums14,000,402,432/17,305,853,952B; all six payloads and old integer/timer rows exact; restart246.458883s,168797points/855828cells,coordinate error0. Fresh536 reference sets were sealed/pushed72acb6c4e5 before first cut. Old pinsets are historical after intentional source changes.

Earlier N1 passed429c6c1a60 with E1P6c/E2P7 references. CacheB7 condition held but modeled13,061,193,021B>12GiB; reverted9831c787e7. N3 source check14findings6941fcc41c; N5passed3d28237f03; N9passed9867041a2e; N11purpose1046/1301=.8039969 failed/reverted0d9d627645; N10withdrawn. No N6/N7 final report yet.

Actual /proc/preflight requires escalation. One owned heavy workload, at most6 heavy processes; nice19, library threads1, build4 on4TB compile-tmp. User apps are observations, never rerun reasons. Limits:5400s/event,19073MiBjob=19,999,490,048B,modeled12GiB,geometry1e-4, all quality/height/progression/transfer/restart gates. Source/policy/active helpers frozen until actual terminal verification and pin seal. R3 <=90min per own fixture; R5 one <=60min diagnosis. Preserve unrelated dirty files; stage owned paths only; push authorized fork with gitgc.auto=0/submodule.recurse=false. No questions. No create_goal.

Historical verbose state before this rewrite: `progress_history_through_safeguards_full552_20261117.md`, SHA256 `8f82ba73c36a70ecaeb5f0a27aca9c8e88ea0546a21e7633ed6a1d489e76232d`. Older history is linked there; historical LIVE notes never override this current state.
