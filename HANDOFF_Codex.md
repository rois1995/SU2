# Native adaptation — current handoff

Updated 2026-11-17. **N14/N15 PASSED every ordered check**, including the ONE full workflow (4738.250709s, all6payloads EXACT E1P6c/E2P7), isolated restart (236.822017s,168797points/855828cells, coordinate error0, finite positive changed fields), and M26 reconciliation. Acceptance summary: `N/N14/section6_12/acceptance_summary.json`. All N14 actual handles and pins are sealed.

CUT B8/build558 PASSED scoped gates: build 80.122834s/1781 pins; G1 serial/MPI2/MPI4 782.588145/13.597257/15.629537s, zero changed cases; strict G2 stop/seal 364.021415/372.503794s, all old integer/model/timer rows exact against adopted552. All actual handles and pins sealed. Prefix HWM sum 6,337,683,456B is not a full-workflow P8 peak. No numerical repeat or efficiency gain claim. Commit/push B8, then end-B G3/G4/GQ. After end-B gates and commit/push, update current state, verify no owned job, write terminal handles, STOP and wait for the user. Do not start phase C. N6/N7/C/D are deferred at this planned stop.

Current shared558 CFD SHA256 `cb90bfd4bcec97bf0ee657789dca662ed8d416064d7455c06c38514481b87ddb`; test SHA256 `b1462a432c443632640b92ce9c6f44e41ef390bc71641aabf8dc5935053789a4`. N14 full552 qualification, adopted references and local archived binaries remain preserved.

Evidence root: `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115` (N). Qualified full case: `N/N14/section6_12/validation/full552`; restart `validation/disk_restart_full552_event2`. Primary terminal receipts are `validation/full_primary_terminal_receipt.json` (10actual IDs,1775source/212input/25helper pins) and `restart_primary_terminal_receipt.json` (11actual IDs,2022case/10helper pins). No full or restart numerical repeat occurred. Own helper setup/report corrections are recorded in progress and their R3 history folders.

After compaction, read the complete latest scratchpad `NIGHT_WORK_ORDER.md`, then [current progress](integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/progress.md). Progress holds the complete check/handle/correction ledger and next commands; historical LIVE notes do not override it.

## Latest user order

N14 is released under Night6.12: strict split-point lift replaces the law condition; N15/M26 is in the same build. The latest AGENTS amendment was applied at the committed/pushed B2 terminal boundary before N14 edits.

If N14 passes, adopt its sealed G1/G2/GQ/G3 as cut references without another reference run. The corrected10:10 bullet uses first N14 G3 edits for later cut identity, while retaining at least336 splits and all other physical gates. If reverted, keep old references.

Finish N14/N15, then B3–B8 and end-B G3/G4/GQ. Commit/push, update this handoff and progress, verify no owned job, record terminal handles, **STOP AND WAIT FOR USER. Do not start phase C.** If phase B cannot finish under R5, stop after the required revert is committed/pushed. N10 is withdrawn; 8.5ab is not authorized. N6/N7 are deferred at the planned stop.

## Preserved build552 completed qualification

Shared build: `integration_evidence/build-integrated-v3-p5-20261114`; sealed build552 receipts: `N/N14/section6_12/build552`.

- CFD SHA256: `9ef34ecc72357e5deb31256e4c5739fedca2170f80f304996a5284e3a280ca35`.
- Test SHA256: `efbfd37bb53d947f1b194860b76c76c9e29bf1b27feae19ecea8808017feac8a`.
- Build, focused113assertions/12cases, all G1 cohorts, strict G2/GQ, both G3 runs and their common edit comparison passed. G3 accepted1113/1099splits, both>=689; all1099common edits identical.
- Six-rank wall report completed normally:459splits,577.391s, conservative process-VmHWM sum5,483,991,040B. It is a report, not accepted BL capability.
- All six safeguards passed: merit481127.197s; saved481157.009s; saved-current154.507s; 2D1/2/4ranks7.213/7.014/7.095s. All passed-case pins and actual terminal identities are sealed.
- Implementation checkpoint93ea0f93e8 and evidence checkpointsb09fa8046d/cb5f79ee86/213fb7af05 are pushed, originally labelled **qualification pending**; build552 is now qualified. No production result gate failed.

Full workflow, isolated restart and M26 tables are now complete and qualified. On a result failure use R5; own helper/setup errors use R3. Never change a gate or production behavior to make a helper pass. The helper correction ledger and exact raw archives are linked from progress.

N14/N15 [report](integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N14/section6_12/REPORT.md) and [G3 paired observations](integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N14/section6_12/reports_G3_pair/g3_pair_audit.md). Parent bounds have explicit unknown/unmeasured coverage; no complete coverage or final BL mesh claim is made. Stage exit timing includes the post-budget exit census.

## Remaining phase-B cut

Commit/push B8, then end-B G3/G4/GQ. After end-B gates and commit/push, update current state, verify no owned job, write terminal handles, STOP and wait for the user. Do not start phase C. N6/N7/C/D are deferred at this planned stop.

Detailed timings: [ADAPTATION_TIMINGS](Handbook/ADAPTATION_TIMINGS.md). The fully qualified phase-A workflow had native event times2583.997544/1727.465975s and process-peak sums14,000,402,432/17,305,853,952B, all six payloads and old integer/timer rows identical; isolated restart passed. Older pinsets are historical after intentional source changes.

Resource rules: one owned heavy workload, at most6 heavy processes, nice19, single-threaded libraries; actual host preflight and PID/start verification. User applications are observations only. Preserve all standing geometry/quality/height/transfer/restart and memory/time gates. No questions before the planned stop.
