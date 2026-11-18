# Night work — current progress

Updated 2026-11-18. Read AGENTS.md and the copied WORK_ORDER_2.md at the start of the new session, then this file.

## Current state — phase C passed

Phase C passed on build570. C11 ended at `167e353ca7`, and the single M27 checkpoint is `4787358e7a`. G5 passed in 4058.303 s with all six payloads, old integer rows and 96 timer snapshots exact. The native event maxima were 2181.430 / 1448.062 s. The isolated restart passed in 225.848 s with zero coordinate error. All 1,783 active source pins and 41 terminal process identities were verified. The phase C report is `Handbook/ADAPTATION_CUT_PHASE_C_20261118.md`.

Phase B passed on build558 at qualification commit `97478b7abae40fcf107ac8132f0eea415a2aaaae`. Phase B production source matched B8 checkpoint `daf75b14e2a761ec1b8abaf5f5cd3e01a65ca7e3`. Its 1,781 source pins, both binary hashes and terminal handles were verified. The phase B stop audit is in `N4/section6_3_after_N11/end_B_preparation/planned_stop_terminal_audit.json`.

Work order 2 is in force. C1 checkpoint `f7017771e8` passed on build559 (70.162 s, 1,781 verified pins). G1 matched 375 / 114 / 228 case records with zero differences in 771.316 / 12.622 / 15.802 s. G2 matched work 90,809,869, rank counts 480/473/346/263/469/464, all required integer/model/timer calls, fixedstorage 4120 and census delta 0. G2 stopped at 312.308 s and sealed at 320.173 s. Six sampled ranks, summed prefix HWM 6,675,406,848 B. All C1 jobs terminal. C1 acceptance and source review are in `N4/section6_3_after_N11/C1/validation`. Pinned H, D, C and I are in `N4/section6_3_after_N11/phase_C/inputs`. Phase B remains the previous integrated checkpoint.

## Completed phase-B gates

- B1–B8 scoped identity checks passed numerically. G1 case/failure tables match375/114/228records, zero changes; accepted-base failures are explicitly preserved underC2.1.
- R1 correction: earlier B6/B7/B8 six-rank G2 runs had seven owned heavy processes because a draft renderer remained alive. They are NOT R1-compliant. The unchanged integrated558 source passed one corrected G2 run at the six-process bound: work 90,809,869/counts 480/473/346/263/469/464; stop316.078817s/sealed323.937712s; all old integer/model/timer rows exact. B6/B7/B8 R1 amendments retain the violation and corrected qualification.
- G3 PASSED617.688712s:1170splits>=336; all1113common edits exact against FIRSTN14G3; adjustedentry5113both; every physical gate passed;2036pins.
- G4 PASSED1/2/4ranks:6.492431/6.504021/6.514544s;1992pins per case.
- GQ PASSED1083.237551sstop/1091.436227ssealed: all6thirdqualitywindows/alloldinteger/model/timerrows exact/18timer snapshots/zero Census delta;1781source/221input/21extrahelperpins. ObservedprefixrankHWMsum13,999,366,144B, notfull-workflowP8peak.
- Readability:153detectedprivate definitions/23lambdasall<=80/15; known12B6/9B7/31B8helpers found, maxima60/68/73. Adapt4998formattedlines isphaseDdeadline, notendBgate. Lexical limitations and written exceptions remain explicit.

Canonical report: `Handbook/ADAPTATION_CUT_PHASE_B_20261117.md`. Acceptance: `N4/section6_3_after_N11/end_B_preparation/acceptance_summary.json`. Found-during-cut list, all own-helper causes/corrections, baseline failures, ten longest functions and exceptions are preserved there and linked evidence.

## Actual terminal handles and helper recoveries

Build558exec92164; B8G1serial56651/MPI211596/MPI429843; originalG225119; G387148; G417899. All actual identities are terminal.

CorrectedG2originalcontrollerexec2592exited143while its solver session161908/start42865736continued. Adoptedmonitor exec29712 completed the SAME numerical run and exited0. GQoriginalcontrollerexec37929exited143; its SAME solver session190560/start42931623 continued under detachedsupervisor205985/start42968501 andmonitor205993/start42968508. Supervisor's wait recorded actualmonitor exit0. All these identities are terminal. Controller SIGTERM causes are unknown; no solver restarted and unavailable reparented solver wait statuses remain null, not fabricated.

Orphan draft renderer4076260/start42326144 andsandboxparent4076259/start42326143 are terminal. The first renderer stalled in SequenceMatcher, its running session ID was discarded, and a sandbox-local process check missed it. Newpreflight includes ownedPythonhelpers. Exact host evidence and correction are under `end_B_preparation/R3_orphan_generator`; controller recovery source/pins/receipts are in the two R3_controller_recovery folders. Setup-only missing-alive-field assumptions were corrected with direct PID/start verification retained. No production/gate/tolerance change was made for own helpers.

## Historical phase B binary archive and N14 reference

Shared build: `integration_evidence/build-integrated-v3-p5-20261114`.

- CFD558 SHA256 `cb90bfd4bcec97bf0ee657789dca662ed8d416064d7455c06c38514481b87ddb`.
- Test558 SHA256 `b1462a432c443632640b92ce9c6f44e41ef390bc71641aabf8dc5935053789a4`.
- Independent read-only copies: `N4/section6_3_after_N11/end_B_preparation/qualified_build558/manifest.json`. Binary payloads are local only, not Git.

Historical N14/N15 two-event qualification build552/checkpoint0c2cb5c460: full4738.250709s, nativeevents2568.277632/1672.244847s, exact P8 process-peak sums14,054,744,064/17,397,911,552B; all six payloads exactE1P6c/E2P7, strictphysical/quality/transfer/continuedCFD/timer/resourcegates andisolated restart236.822017s passed. N14G3first/second1113/1099splits andall1099commonedits; M26 samefullrun qualified. No extra G5 is prescribed at end B; build558 is phase-B-qualified, not a new full-workflow run. No acceptedBL/RANSclaim.

## Earlier work and history

N1/P7P8 accepted429c6c1a60. ConditionalcacheB7/N2 failed modeled12GiB and was reverted9831c787e7; CUT B7 is a separate Control extraction. N3 source check6941fcc41c, N5 measurements3d28237f03, N99867041a2e, N11 failed/reverted0d9d627645, N10withdrawn, N13/M25passed8ab6da791e, N14/N15passed0c2cb5c460. PhaseA5ec465ca1a passed full/restart. Detailed source/proposals, failures, gate numbers and historical handles remain in the linked reports.

Historical progress: `progress_history_through_phase_B_qualification_20261117.md`, SHA256 `0e2c115b2607774e26d792b27b5b30b5987147c5a00d4ec2c70321966b38b982`. Its old RUNNING/PENDING notes are historical; this file controls current state.

The original C3–C11 controller was PID 586872 / start 44481329. Its launch, draft pin manifest, compiler-only check and log are under `N4/section6_3_after_N11/phase_C`. It runs one build or numerical gate at a time and pauses on the first error with evidence intact. The M27 handoff and G5 follow the passing phase-end G3, G4 and GQ receipts. The syntax check passed after restoring C8's mutable reuse-map reference.

The ordered controller reached C8 G3 and exited on a prelaunch alias collision. G1 and G2 passed on the pinned build566. All three existing aliases exactly equal the sealed build. The corrected adapter reuses only matching aliases. Recovery controller PID 823111 / start 45683107 ran C8 G3 and GQ and the remaining C9–C11 campaign. The original controller and the failed wrapper are terminal. The recovery launch and log are under `phase_C/C8_to_C11_recovery_campaign*`; the first wrapper attempt is preserved in `C8/validation/R3_G3_alias_collision`.

## Phase C checkpoint progress

- C2 PASSED at `c7b7664853`, build560, G1 [375, 114, 228] exact, G2 work 90,809,869 and rank counts exact. Receipts in `N4/section6_3_after_N11/C2/validation`.

- C3 PASSED at `1a2fab4b91`, build561, G1 [375, 114, 228] exact, G2 work 90,809,869 and rank counts exact. GQ PASSED. Receipts in `N4/section6_3_after_N11/C3/validation`.

- C4 PASSED at `7e101fcb43`, build562, G1 [375, 114, 228] exact, G2 work 90,809,869 and rank counts exact. GQ PASSED. Receipts in `N4/section6_3_after_N11/C4/validation`.

- C5 PASSED at `227187f952`, build563, G1 [375, 114, 228] exact, G2 work 90,809,869 and rank counts exact. GQ PASSED. Receipts in `N4/section6_3_after_N11/C5/validation`.

- C6 PASSED at `4a8ad57dbd`, build564, G1 [375, 114, 228] exact, G2 work 90,809,869 and rank counts exact. GQ PASSED. Receipts in `N4/section6_3_after_N11/C6/validation`.

- C7 PASSED at `bb5cac5265`, build565, G1 [375, 114, 228] exact, G2 work 90,809,869 and rank counts exact. Receipts in `N4/section6_3_after_N11/C7/validation`.

- C8 PASSED at `f3beeb21d8`, build566, G1 [375, 114, 228] exact, G2 work 90,809,869 and rank counts exact. G3 PASSED. GQ PASSED. Receipts in `N4/section6_3_after_N11/C8/validation`.

- C9 PASSED at `c0171bf783`, build567, G1 [375, 114, 228] exact, G2 work 90,809,869 and rank counts exact. G3 PASSED. Receipts in `N4/section6_3_after_N11/C9/validation`.

- C10 PASSED at `e3444aa51d`, build568, G1 [375, 114, 228] exact, G2 work 90,809,869 and rank counts exact. G3 PASSED. GQ PASSED. Receipts in `N4/section6_3_after_N11/C10/validation`.

- C11 PASSED at `167e353ca7`, build569, G1 [375, 114, 228] exact, G2 work 90,809,869 and rank counts exact. G3 PASSED. G4 PASSED. GQ PASSED. Receipts in `N4/section6_3_after_N11/C11/validation`.

- N17 M27 scoped gates passed at `4787358e7a`, build570. G1/G2/G3/G4/GQ receipts are in `N4/section6_3_after_N11/M27/validation`.

The G5 supervisor is PID 1069173 / start 46965055. The launch, resource preflight and waits are in `N4/section6_3_after_N11/phase_C/G5_campaign_launch.json` and `N4/section6_3_after_N11/phase_C/G5`. The server restart left the original supervisor and full-run controller live. The same run reached both event payloads and the isolated restart. The saved restart outputs passed the post-only validator.

The named geometry safeguards `merit481`, `saved481` and `saved_current` are bound to build570 using the accepted N14 driver. All three prelaunch checks passed without starting a test. They passed sequentially after G5 and the restart in 121.145 / 146.766 / 143.969 s. G2, G3 and G4 already cover the other shared-path safeguards.

## Work order amendment — 2026-11-18

The user withdrew 8.4a after M27. N16 now means step 8.5r (larger quality rounds). The pinned I8R copy is in `N16/inputs/IMPLEMENTATION_8_5R.md` with `SHA256SUMS`. The SHA256 matches the user order. Read I8R completely on reaching item 3.

## Phase D checkpoint progress

D1 build571 compiled in 72.732 s. G1 passed with 375 / 114 / 228 case records, zero differences, in 769.252 / 14.583 / 14.702 s. G2 passed with 90,809,869 work units, counts 480/473/346/263/469/464, and exact old integer and timer rows. G3 passed with 1,458 splits, all 1,113 shared edits exact, adjusted entry work 5,113 and all physical gates. GQ is running. The setup-only source-pin correction records the known committed AGENTS.md amendment, verifying every other original pin.

D2's reviewed source is staged outside the production tree. Its exact-flag syntax check and format inventory passed. Adapt is 58 formatted lines, and the longest helper is 58 lines. The draft helper first tried to bind a locally declared failure to slot state and included the opening of an already extracted branch. Both generator corrections are recorded; production was untouched during this check.

Controller PID 1204090 / start 47637931 waits on D1 PID 1187563 / start 47559158. It will commit and push each passing scoped cut, apply the D2 draft only after verifying D1 terminal, then start the D3 workflow on the passed D2 binary. Launch receipts and logs are in `N4/section6_3_after_N11/phase_D`.

- 2026-11-18T08:16:28+01:00 D1. Build571 passed G1 (375/114/228), G2 (90,809,869), G3 and GQ (1034.570 s). Preparing checkpoint.

- 2026-11-18T08:16:35+01:00 D1. Build571 passed G1 (375/114/228), G2 (90,809,869), G3 and GQ (1034.570 s). Pushed `829e8db24b`.

- 2026-11-18T08:16:38+01:00 D2. Applied the reviewed extraction. Adapt is 58 formatted lines. Starting build572.

D2 build572 compiled in 70.135 s with 1,784 verified source pins. Validation controller PID 1243891 / start 47850483. The moved source passed clang-format over the new file and the Adapt replacement range. G1 is running.

D2 build572 passed G1, 375 / 114 / 228 records, zero changes, 768.300 / 13.611 / 15.759 s. G2 passed with 90,809,869 work units and exact counts and rows. It stopped at 312.308 s and sealed at 320.065 s. G3 is running.

D2 build572 passed G3 with 1,514 splits, 1,113 identical common edits and adjusted entry work 5,113. G4 passed at 1 / 2 / 4 ranks in 6.472 / 6.461 / 6.474 s. GQ is running.

- 2026-11-18T09:05:35+01:00 D2. Build572 passed G1 (375/114/228), G2 (90,809,869), G3 and GQ (1034.154 s). G4 also passed. Preparing checkpoint.

- 2026-11-18T09:05:42+01:00 D2. Build572 passed G1 (375/114/228), G2 (90,809,869), G3 and GQ (1034.154 s). G4 also passed. Pushed `4c6cbddbdb`.

- 2026-11-18T09:05:42+01:00 D3. Build572, scoped D2 receipts passed. Starting full G5.

D3 full G5 is live on build572. Supervisor PID 1299722 / start 48144817; full wrapper PID 1299816 / start 48145199. All three G5 and restart no-launch checks passed. Reuse the active handles from the phase-D receipts. After full G5 and its isolated restart, the same continuation runs the standing safeguards.

D3 interim observation: event one produced all three exact P6c files. The same six-rank run continued CFD and entered event two. Final G5 qualification follows the terminal receipts.

- 2026-11-18T10:14:57+01:00 D3. Both G5 events and six payload identities passed. Starting isolated restart.

- 2026-11-18T10:19:14+01:00 D3. G5 and isolated restart passed. Standing safeguards and phase report pending.

D3 PASSED on build572. G5 finished in 4140.805 s. All six payloads, all old integer rows and 96 timer snapshots matched. Isolated restart passed in 233.893 s, zero coordinate error. All standing safeguards passed, 41 process identities terminal, 1784 source pins verified. Next, read I8R for N16. Phase report: `Handbook/ADAPTATION_CUT_PHASE_D_20261118.md`.
