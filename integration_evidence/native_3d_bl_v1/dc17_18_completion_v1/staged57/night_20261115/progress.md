# Night work — current progress

Updated 2026-11-19. Read AGENTS.md and the copied WORK_ORDER_2.md at the start of the new session, then this file.

## Current state — N21

N21 Part P passed at `08a8d7c7ff`. Part W source `7ac99f096d` passed both workflows, the isolated restart, six repeat-identical payloads and the 0.95 timing rule. The selected sum was 3,326.986 s against 3,336.027 s. Build589 uses the qualified solver hash and verified all owned identities terminal. [Step 8.5t report](N21/reports/STEP_8_5T_588.md) · [Adopted reference](N21/accepted_reference.json). Wait for the next user order.

## Current state — N20

N20 Part A PASSED. Part W FAILED its purpose rule. Build586 was restored at that checkpoint. [Step 8.5s report](N20/reports/STEP_8_5S_585.md) Wait for the next user order.

## Current state — N19

N19 passed. Part A build581 checkpoint `c6b42a748f`. Build582 printed level counts by rank [[0, 0, 0], [0, 0, 0], [0, 0, 0], [1, 103, 417], [0, 0, 0], [0, 0, 0]]. All four N18 failure comparisons matched. Diagnostic reverted at `efdedbb6d9`. Default build583 restored. 1,790 verified source pins and 200 terminal owned identities. [M29 report](N19/reports/M29_582.md). Wait for the next user order.

## Current state — work order 2 complete

Updated 2026-11-18T20:41:03+01:00. D/E passed at `3524e11a75`. F failed. Its first difference was event one NATIVE3D complete=0 against the required 1, with rank 3 q_bad=1 against 0. The F enabling change was reverted at `12690f1d99`, and build579 is active. [N18 report](N18/reports/STEP_8_5R2_580.md) All owned processes are terminal. N7 closes work order 2. Wait for the next user order.

## N6 and N18 entry reference

N6 qualification `19944869a3` accepted build578. G5 completed in 4041.079 s with all six P6c/P7 payloads and 96 timer snapshots exact. The isolated restart passed in 234.819 s, with zero coordinate error. All three standing safeguards passed. The audit verified 1,790 source pins and 271 terminal process identities.

The N18 work-order amendment `7d8f538c00` is pushed. I8R2 was verified and read completely after N6. N18 uses one D/E identity checkpoint on build579, then separate F on build580 if D qualifies. The whole-item source check is in `N18/source_check.md`. D/E passed and was pushed as `3524e11a75`. Part F build580 passed the exact wide repeat at ranks 2/4, G1/G2/G3/G4 and all three standing safeguards. Part F controller PID 2060557 / start 51553252. Full run-one wrapper PID 2179783 / start 52093696. N7 follows N18.

## Earlier integrated reference — phase C

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

D3 phase report and qualification pushed as `c9176e9439`. Current item: N16 step 8.5r.

N16: verified and read pinned I8R completely after D3. Whole-item source check recorded before production edits in `N16/source_check.md`. Next is Part A, then B identity qualification and T0.

N16 A passed on build573. No build warnings. The dedicated clearance fixture selected the distant pair, tested the exactly touching boxes and deferred the overlapping candidate with 2 peer checks. The first fixture launch had OpenMPI MPI_ERR_WIN before a result; the accepted pt2pt configuration passed on the unchanged binary in 0.318 s.

N16 A checkpoint pushed as `4694b26e82`. The raw Catch2 output is preserved and its lossless archive passed the Git staging check. Next is the disabled wider-round Part B.

N16 B build574 finished in 370.164 s without warnings. The wider capacity is disabled and G1/G2/G3/G4/GQ are running on the unchanged-default path. Controller identities: B PID 1420670 / start 48720060, T0 PID 1422081 / start 48726797, C PID 1433837 / start 48783975. T0 waits for the passed B checkpoint, and C waits for passed terminal T0. The C source draft and G5 receipt adapters are outside the pinned production tree.

N16 B build574 passed G1 375 / 114 / 228 unchanged. G2 matched 90,809,869 work and counts 480/473/346/263/469/464, all old modeled/integers/timer calls. G2 stopped at 323.171 s. G3 is running.

N16 B G3 passed with 1,525 splits, 1,113 reference edits exact, and adjusted entry work 5,113. G4 and GQ remain before T0.

N16 B. Build574 passed scoped identity. G1 375 / 114 / 228 unchanged, G2 work 90,809,869, G3 1525 splits and 1113 common edits exact, G4 at 1 / 2 / 4 ranks, GQ at 1167.459 s. Full T0 pending.

N16 B. Pushed `355a40d366`. Running the two-event T0 workflow.

N16 B. T0 passed in 4089.709 s. All six payloads and old rows matched. Next is Part C.

N16 C. Applied the reviewed factory and fixture edits. Starting build575.

N16 T0 build574 completed 4089.709 s. Native event maxima 2236.870 / 1416.499 s. Six exact payloads and 96 timer snapshots. T0 S=5546.565566 / 3420.686148 s, required C ceilings 4437.252453 / 2736.548919 s. Part C build575 started after T0 terminal.

N16 C build575 compiled without warnings. The 64-wide repeat fixture passed at ranks 2 and 4 in 0.459 / 0.453 s and matched the exact cell images. G1, G2, G3, G4 and safeguards are next.

N16 C. C3 at 2 and 4 ranks and all G1, G2, G3 and G4 checks passed. Running standing safeguards.

N16 C. The standing safeguards passed. Starting changed-mesh G5 run one.

N16 C G1–G4 and C3 passed. Geometry merit, saved481 and saved_current matched their accepted payload/row checks in 121.415 / 147.112 / 142.653 s. Run one of G5 is live on build575.

N16 C FAILED G5 run-one event one. Complete 1→0, q_bad 0→1 on rank 3. Operator stagnation with one stuck required shape and 14 blocked features, zero queued/missing candidates. Prefix identity exact. No transcription error found. Candidate `5dffa5ca5a` reverted in this checkpoint. Parts A/B and T0 retained, P6c/P7 references remain. Proceed to N6.

N16 C rollback and report pushed as `3276cbb862`. Item 4 N6 now active.

N6 whole-item source inventory written before production changes. The Remesh preview passed the exact-flags syntax check. Formatted public entry 50 lines, longest new helper 74. Next steps remain Remesh, optimizer, Selection.

N6. Applied the reviewed Remesh extraction. The public entry is 50 formatted lines. Starting build576.

N6 Remesh build576 compiled without warnings in 392.946 s. G1 serial is running on the pinned source. The optimizer and combined Selection drafts passed exact-flags syntax checks from private overlays. Continuation PID 1726962 / start 50091756 waits for the passed, terminal Remesh checkpoint, then runs the optimizer and Selection gates before the full G5 and standing safeguards.

N6 Remesh build576 passed G1 at 375 / 114 / 228 records with zero differences (777.415 / 13.842 / 15.956 s). The six-rank G2 first-collapse prefix is running.

N6 Remesh G2 passed the six first-collapse counts and old integer/model/timer identity at 90,809,869 work. Stop 317.525 s, sealed 325.345 s. G3 is running.

User amendment 2026-11-18 (R12). N18, step 8.5r2 plus M28, follows N6 and precedes N7. Its pinned I8R2 bytes are verified in N18/inputs with SHA256SUMS. The purpose rule of gate 8 is confirmed. Apply the AGENTS.md amendment after N6 terminal because the live N6 sequence holds its current hash. Read I8R2 completely at entry to N18. N7 remains deferred until N18 has an outcome.

N6 Remesh G3 passed with 1,434 splits, 1,113 exact shared edits, adjusted entry work 5,113, all physical gates, and 615.271 s elapsed. GQ is running.

N6. Remesh build576 passed G1 375/114/228, G2 90,809,869, G3 1434 splits with 1113 exact shared edits, and GQ at 1091.439 s.

N6. Remesh checkpoint pushed as `4946b2d126`.

N6. Applied the reviewed Optimizer extraction. Starting build577.

N6 Optimizer build577 compiled without warnings in 82.908 s. Its pinned controller is PID 1799092 / start 50367768. G1 serial is running.

N6 Optimizer build577 passed G1 at 375 / 114 / 228 records with zero differences (782.518 / 13.811 / 14.976 s). G2 is running.

N6 Optimizer G2 passed work 90,809,869, the same six collapse counts and all required integer, model and timer rows. Stop 333.367 s, sealed 341.282 s. G3 is running.

N6 Optimizer G3 passed 1,306 splits, 1,113 exact shared edits, adjusted entry 5,113 and all physical gates. GQ stopped at the prelaunch 2 GB free-space guard with no numerical launch. Preserving the earlier passed gates and restoring disk space.

N6. Optimizer GQ prelaunch recovery preserved all passed gates and restored 3,640,987,648 B free without changing the guard. Starting GQ only.

N6. Optimizer build577 passed G1 375/114/228, G2 90,809,869, G3 1306 splits with 1113 exact shared edits, and GQ at 1061.200 s.

N6. Optimizer checkpoint pushed as `cd2c46b73d`.

N6. Applied the reviewed Selection extraction. Starting build578.

N6 Selection build578 compiled without warnings in 82.819 s and passed G1 at 375 / 114 / 228 records with zero differences (771.375 / 14.756 / 15.836 s). G2 is running.

N6 Selection G2 passed at 317.610 s with work 90,809,869 and all exact rows. G3 passed 1,368 splits, 1,113 exact shared edits and all physical gates, adjusted entry 5,113. G4 and GQ follow.

N6. Selection build578 passed G1 375/114/228, G2 90,809,869, G3 1368 splits with 1113 exact shared edits, and GQ at 1122.623 s.

N6. Selection checkpoint pushed as `7ffb54c83e`.

- 2026-11-18T17:02:49+01:00 N6. Build578, scoped Selection receipts passed. Starting full G5.

N6 G5 interim: event one emitted the exact P6c mesh and native reference. Proof coverage 67,170 and job/process peak sum 14,014,275,584 B passed. The same six-rank run continued into event two. G5 remains running.

- 2026-11-18T18:10:37+01:00 N6. Both G5 events and six payload identities passed. Starting isolated restart.

- 2026-11-18T18:14:56+01:00 N6. G5 and isolated restart passed. Standing safeguards and phase report pending.

- 2026-11-18T18:17:21+01:00 N6. merit481 passed in 119.201 s with exact saved-state comparison.

- 2026-11-18T18:20:03+01:00 N6. saved481 passed in 146.687 s with exact saved-state comparison.

- 2026-11-18T18:22:39+01:00 N6. saved_current passed in 142.945 s with exact saved-state comparison.

- 2026-11-18T18:23:02+01:00 N6. Build578 passed G5 in 4041.079 s, six payloads and 96 timer snapshots exact. The isolated restart passed in 234.819 s. All standing safeguards passed, source pins verified, and all owned process identities are terminal.

- 2026-11-18T18:23:12+01:00 N6. Qualification pushed as `19944869a3`. N18 follows under the 2026-11-18 amendment.

N18 amendment applied after the passed terminal N6 checkpoint. The pinned copy matched the user SHA256. Next, read the complete I8R2 and complete the N18 source check.

N18 work-order amendment pushed as `7d8f538c00`.

N18. Applied the ownership cool-down and print-only M28. Compiling build579 with the default repair width.

N18 build579 compiled without warnings in 92.826 s. The RepairDemand size assertion compiled. Its D/E controller is PID 2055807 / start 51533058, with default-width G1/G2/G3/G4/GQ ordered. F controller PID 2060557 / start 51553252 waits for the passed and terminal D/E checkpoint. The three-path F patch and non-launch workflow checks are sealed.

N18 D/E passed G1 at 375 / 114 / 228 records with zero differences (766.269 / 12.623 / 13.789 s). G2 is running.

N18 D/E G2 passed work 90,809,869 and exact old rows through the stop at 312.627 s, including fixed storage 4,120 B. G3 is running.

N18 D/E G3 passed with 1,526 splits, 1,113 exact shared edits, all physical gates and adjusted entry work 5,113. G4 and GQ follow.

N18. D/E build579 passed G1 375/114/228, G2 90,809,869, G3 1526 splits with 1113 exact shared edits, G4 and GQ at 1024.978 s. No full D/E run.

N18. D/E checkpoint pushed as `3524e11a75`. Part F follows.

N18. Applied the exact three-path F patch. Compiling build580 with the production width of 64.

N18. Build580 compiled without warnings. The exact wide-repeat fixture passed at 2 and 4 ranks.

N18 F wide-repeat fixture passed at 2 and 4 ranks on build580. G1 matched 375 / 114 / 228 records with zero changes. G2 is running.

N18 F G2 passed in 317.393 s to stop with work 90,809,869 and exact old integer, modeled storage and timer rows. The G3 saved-stage fixture is running.

N18. F scoped gates passed G1 375/114/228, G2 90,809,869, G3 1,519 splits and 1,113 exact shared edits, and G4. Running the standing safeguards.

N18. All three standing safeguards passed. Starting F G5 run one.

N18. F FAILED. First difference: event one NATIVE3D complete=0 against the required 1, with rank 3 q_bad=1 against 0. Preserving the candidate evidence.

N18. N18 source disposition pushed as `12690f1d99`. The final N7 report follows.

N19 A. Applied the reviewed M29 observer. Starting default-width build581 and the G1/G2/G3/G4/GQ identity checks.

N19 A. The identity controller is PID 2333684 / start 52755311. The preflight found zero owned heavy processes. Build and each gate are sequential.

N19 A. Build581 finished without warnings in 362.857 s with 1790 verified source pins. G1 serial is running.

N19 A. G1 matched 375 / 114 / 228 case records with zero changed failures (771.239 / 14.618 / 14.734 s). G2 is running.

N19 A. G2 passed with work 90,809,869, counts 480/473/346/263/469/464, exact integer/modeled/timer rows and fixedstorage=4120. Stop 322.565 s. G3 is running.

N19 A. G3 passed all physical gates, 1,494 splits with 1,113 exact common edits and adjusted entry work {'reference': 5113, 'candidate': 5113}. G4 passed at 1 / 2 / 4 ranks. GQ is running.

N19 A. Default-width build581 passed G1 375/114/228, G2 90,809,869, G3 1,494 splits with 1,113 common edits, G4 1/2/4 and GQ 1039.935 s. Fixed storage 4,120, no M29 output on success or measured prefixes. Preparing the checkpoint.

N19 A. Pushed `c6b42a748f`. Next is the exact three-path diagnostic overlay.

N19 B. Pushed the exact three-path DIAGNOSTIC overlay `fe221d90cf`. Controller PID 2408320 / start 53093805 builds 582 and runs the frozen six-rank workflow to its first rejection.

N19 B. Build582 compiled and its pins passed. A launcher setup error stopped before MPI. Preserved the setup evidence, fixed the wrapper source binding, and started the same build's first MPI run. Recovery controller PID 2419479 / start 53143542.

N19 B. Build582 reproduced all four N18 failure values on the single MPI run. M29 captured 1 / 103 / 417 cells at levels 0 / 1 / 2. All rank wrappers are terminal. Diagnostic reverted and pushed at `efdedbb6d9`. Restoring default build583, controller PID 2458758 / start 53338575.

N19 passed. Part A build581 checkpoint `c6b42a748f`. Build582 printed level counts by rank [[0, 0, 0], [0, 0, 0], [0, 0, 0], [1, 103, 417], [0, 0, 0], [0, 0, 0]]. All four N18 failure comparisons matched. Diagnostic reverted at `efdedbb6d9`. Default build583 restored. 1,790 verified source pins and 200 terminal owned identities.

N20 A. Applied A1–A4 and the frozen fixtures. Controller PID 2586446 / start 53807406 builds 584 and runs F1–F4, then the ordered identity gates and safeguards.

N20 A. Build584 compiled without warnings in 342.782 s. Running F1–F4.

N20 A. F1–F4 passed on the pinned production sources. Starting G1.

N20 A. G1 matched all 375 / 114 / 228 records. Starting G2.

N20 A. G2 matched exact first-collapse work, rank counts and tracked rows. Starting G3.

N20 A. G3's first wrapper stopped before fixture launch on a missing host receipt field. Verified the actual terminal handles, preserved the setup evidence, and resumed G3 without rebuilding or repeating passing gates. Recovery PID 2626510 / start 53983100.

N20 A. G3 and G4 passed. Starting GQ.

N20 A. G3 passed all physical gates with 1,542 splits, all 1,113 common edits exact and observer-adjusted entry work 5,113. G4 passed at 1 / 2 / 4 ranks. GQ is running. The two F1 fine searches each used 197 evaluations, and F2 retained the 1.0073165457827788 minimum under the pinned transform. F4 reopened four demands, refused all four to Done, reopened zero on repetition and reset all four.

N20 A. GQ matched through the third quality window. Running the remaining standing safeguards.

N20 A. Build584 passed F1–F4, G1 375/114/228, G2 90,809,869, G3 1,542 splits with 1,113 matching edits, G4 1/2/4 and GQ 1025.046 s. Fixed storage 4,120. All three standing geometry safeguards passed. Next is W.

N20 A. Pushed `01d2bbd24f`. Apply the exact historical overlay for W.

- 2026-11-19T01:55:13+01:00 N20. Pushed W candidate `09d2117246`. Controller PID 2674505 / start 54201920.

- 2026-11-19T01:55:14+01:00 N20. W build585: verified exact three-path overlay. Starting build and wide-repeat MPI fixtures.

- 2026-11-19T02:01:27+01:00 N20. Build585 compiled without warnings. The exact wide-repeat fixture passed at 2 and 4 ranks.

N20 W. Build585 compiled without warnings. The exact wide fixture passed at 2 / 4 ranks, and G1 matched all 375 / 114 / 228 baseline records with zero changes. The six-rank G2 check is running.

- 2026-11-19T02:31:34+01:00 N20. W scoped gates passed G1 375/114/228, G2 90,809,869, G3 1,501 splits and 1,113 exact shared edits, and G4. Running the standing safeguards.

- 2026-11-19T02:39:02+01:00 N20. W build585: the three standing geometry safeguards passed. Starting G5 run one.

N20 W. G1 with distinct wide-repeat 2 / 4, G2 90,809,869, G3 1,501 splits and 1,113 identical common edits, G4 1 / 2 / 4 and all three safeguards passed. Candidate `09d2117246`, build585. The first six-rank G5 solver wrapper is PID 2732685 / start 54466043. Its ordered continuation is event-one prefix identity, isolated event-two restart, second G5, six payload comparisons and the confirmed purpose rule.

N20 W G5 run1 provisional: rank 3 found the same stuck cell 152845591 at debt 9.4270376918515042e-05. At round 25,850 the request reopened four vertices. At round 25,851 the 78-cell star was refused at the unchanged cap. At round 25,852 vertex 200812891 locally passed the 10-cell fine trial with one move and the full debt reduction. Final event publication, physical validation and workflow gates are pending.

N20 W G5 run1 event one reached `complete=1` on all six ranks after the fine move, with q_bad/j_bad zero at the final worker exit. Native maximum 1999.196781106 s. The two event-one gate receipts are present. Transfer, continued CFD and event two remain part of this run.

N20 W G5 run1: both native events reached `complete=1` on all six ranks and both timer/connected event receipts passed. Native maxima 1999.196781106 / 1523.675678662 s. The first-run event-two maximum exceeds its purpose comparator, which is evaluated only after run2. The workflow terminal receipt, prefix identity and isolated restart are next.

N20 W G5 run1 passed its standing workflow and exact T0 event-one construction prefix. The controller entered the isolated event-two restart. The second full run has not launched.

N20 W. Isolated event-two restart passed. Run2 is active on the same build585 binary, solver PID 2820906 / start 54894118. Next are all six payload comparisons and the two-run purpose rule.

N20 W run2: the same fine request/trials occurred at rounds 25,850–25,852. Event one closed on all ranks with native maximum 2000.498744994 s. Event-one payload comparisons [('adapted_mesh_adap_00001.su2', True), ('adapted_mesh_adap_00001.su2.native_ref', True), ('restart_flow_adap_00001.dat', True)]. Event two is active.

N20 W FAILED its purpose rule. Both workflows, isolated restart, T0 prefix and six-file repeatability passed. Event minima 3520.726914 s combined against 3288.032 s, event two 1521.530132 s against 1416.499 s. Reverted only W at `1174e4c241` and pushed. Default-width restore build586 PID 2907370 / start 55299029.

N20. Final audit verified 343 terminal owned identities and the active source and binaries.

## Current work — N21

N21 I8T SHA256 439d26f07b0b4357834b8a23dc5a723712ebd3731c8a05a8bc4257aca2eaa766 verified and read in full. Base f968325a69/build586. Scope and R4 readings recorded in N21/source_check.md. Start Part P with the default width. Part W follows only after P passes, with F=0.95.

N21 P. Applied P1–P4 and T1–T4. Controller PID 3159613 / start 56326575 builds 587, runs the new fixtures, then G1–G4, GQ and safeguards.

N21 P. Default-width G5 controller PID 3166783 / start 56355939 waits for the scoped campaign and safeguards to pass and become terminal. It then runs the two-event G5 and checks all six payloads, the permitted proof-row differences and the 53.4 s purpose rule.

N21 P. Build587 compiled without warnings in 462.808 s. Running T1–T4.

N21 P. The serial proof fixtures passed. T4 has its recorded MPI results or R3 note. Starting G1.

N21 P. G1 matched all 375 / 114 / 228 records. Starting G2.

N21 P. Fixtures on build587 passed. T1–T3 gave 925 assertions in three cases. T4 gave 103 assertions per rank at both 2 and 4 ranks. G1 matched 375 / 114 / 228 records with no changed failures. G2 is in progress.

N21 ordered continuation PID 3191786 / start 56467329 waits for terminal P G5. It seals and pushes Part P only after a passing receipt, then starts the exact W overlay and gates.

N21 P. G2 matched exact first-collapse work, rank counts and tracked rows. Starting G3.

N21 P. G3 and G4 passed. Starting GQ.

N21 P. Build587 scoped identity passed. G1 375/114/228, G2 90,809,869, G3 1,388 splits with 1,113 identical shared edits, G4 1/2/4, GQ 1060.849 s. Running the remaining standing safeguards.

N21 P. All three standing geometry safeguards passed. Next is the default-width G5 and its purpose gate.

- 2026-11-19T08:53:46+01:00 N21 P. Build587 passed scoped checks and safeguards. Starting the two-event G5.

N21 P. Full G5 wrapper PID 3258357 / start 56713543. Default-width build587. Both event payloads, rule-6 comparison and the 53.4 s gate are pending.

N21 P. Event one final certification passed its 53.4 s ceiling at 19.216551329 s maximum rank-exclusive. Six ranks reported shared proof; the timer and connected event gates passed. Mesh and native-reference P6c hashes match. Transfer, continued CFD and event two remain.

N21 P. All three event-one payloads match P6c. Maximum native time 2094.502498638 s. The same run continued CFD and entered event two.

- 2026-11-19T09:59:52+01:00 N21 P. Build587 matched all six P6c/P7 payloads and old rows. The proof purpose gate passed.

N21 P. Build587 passed scoped identity, safeguards, the two-event G5, all six payload identities and the 53.4 s certification ceiling. The P acceptance receipt records the T1 maxima. Proceed with W.

N21 W. Applied and pushed exact three-path wide candidate `7ac99f096d`. Starting build588.

N21 P. Accepted and pushed as `08a8d7c7ff`. Full build587 G5 completed in 3,960.861 s. All six payloads and 96 timer snapshots passed. The certification maxima were 19.216551329 / 21.516550950 s. T1 native maxima were 2,094.502498638 / 1,417.104791628 s. W uses a combined ceiling of 3,336.02692575270 s and both T1 event ceilings. The W controller is PID 3191786 / start 56467329.

N21 W. Build588 compiled without warnings. The exact wide-repeat fixture passed at 2 and 4 ranks.

N21 W. Build588 passed the exact wide-repeat fixtures at 2 / 4 ranks and matched G1's 375 / 114 / 228 records. G2 matched the 90,809,869-work first-collapse prefix, old integer and modeled rows and timer calls, with fixedstorage=4120. G3 is running.

N21 W. Scoped gates passed G1 375/114/228, G2 90,809,869, G3 1,550 splits and 1,113 exact shared edits, and G4. Running the standing safeguards.

N21 W. Passed all three remaining standing safeguards. Starting G5 run 1.

- 2026-11-19T10:45:59+01:00 N21 W. Build588 passed scoped checks and safeguards. Starting G5 run one.

N21 W. Run-one build588 wrapper PID 3425102 / start 57386927. All three standing safeguards are sealed. T0 construction prefix, isolated restart, run two and the 0.95 timing rule follow.

N21 W run1. Event one fine move on rank 3 at round 25,852, vertex 200812891, closed the 9.4270376918515042e-05 shape debt. Native event maximum 2048.285885933 s, final certification max 19.004350228 s. Timer and connected gates passed. Transfer, continued CFD, event two and exact T0 prefix are pending.

N21 W run1. Both native events completed. Event two native maximum 1,379.449004216 s and final certification maximum 57.537357453 s. The two maxima from run1 sum to 3,427.734890149 s. Standing transfer, exact T0 prefix, restart and run two remain before the purpose decision.

- 2026-11-19T11:51:15+01:00 N21 W. Run one passed its workflow and exact T0 prefix. Starting the isolated event-two restart.

- 2026-11-19T11:56:23+01:00 N21 W. The isolated restart passed. Starting G5 run two on the same binary.

N21 W. Run1 passed in 3,911.928 s with exact T0 construction prefix. Isolated event-two restart passed in 289.993 s. Run2 wrapper PID 3529461 / start 57809324 is active on the same pinned build588.

N21 W run2 at 12:09. Event one is still in preparation under higher host contention. The actual-host observation in N21/W_run2_contention_observation.json identified an unrelated process using roughly seven cores and verified six owned solver ranks. The same run continues under the frozen event and resource limits.

N21 W run2. The fine publication repeated at round 25,852 and event one closed on all six ranks. Native maximum 2741.581855733 s. Run one supplies the smaller event-one time. Event-two and payload repeat evidence are pending.

N21 W run2. All three event-one payloads were independently checked against run1 and match. Event two is active.

N21 W run2. Both native events completed. Event-one maximum 2,741.581855733 s and event-two maximum 1,278.700341572 s. The per-event minima from run1 event one and run2 event two sum to 3,326.986227505 s, against 3,336.026925753 s. Both event ceilings are met. W remains a candidate pending terminal transfer and six-file repeatability.

- 2026-11-19T13:10:58+01:00 N21 W. Both workflows and isolated restart passed with six exact matching files. The time rule passed.

N21 W PASSED. Build588 passed both workflows, the isolated restart, exact T0 prefix, six repeated payloads and F=0.95. Build589 kept the same solver hash, excluded the MPI-only case from default selection and repeated T1–T4. Final report follows.

N21. Final audit verified 406 terminal owned identities and 1,790 active source pins. Report and handoff updated.
