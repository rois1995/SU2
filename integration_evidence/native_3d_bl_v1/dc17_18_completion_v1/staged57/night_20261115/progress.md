# Night work — current progress

Updated 2026-11-19. Read Work order 3 in AGENTS.md and this file at the start of each session.

## Current state — Work order 3 in force

Installed the verified Work order 3 (SHA256 `93f16431048dfd6013d79ddb7dba7d7ff114c40fa104f843973a76c7092092e9`). The user accepted and closed both N22 timing flags. No item is open. Wait for the user’s fast path stage 1 order.

## Current state — FP0 complete

FP0 build596 completed the test-only measurements on the accepted N21 source. G1 matched 375 / 114 / 228 records and all recorded failed assertions. The six-rank frozen replay produced 695,524 cells and 138,878 points. Standalone Refine took 343.064097 s maximum, was valid on all ranks, and changed no local meshes. The terminal audit verified 44 process identities and 1,387 source pins. The build591 production solver hash remains verified. [FP0 report](FP0/reports/FP0_596.md). Stop and wait for the user.

## N22 — complete

N22 standing result gates passed. The user accepted and closed the Part M timing flag at `484f29315f` and the Part C purpose timing flag at `e40d7e3795` on 2026-11-19. Active build591. All six N21 W hashes matched in both parts. The audit verified 315 terminal identities and 1,790 source pins. [Step 8.5u report](N22/reports/STEP_8_5U_591.md).

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

## Current work — N22, then FP0

Verified and read the pinned I8U, FP0 order and FP0 design context. The design is context. N21 wide reference 4fe083d460/build589, six payloads and native maxima 2048.286 / 1278.700 s. The accepted-build GQ reference is running at controller PID 3673595 / start 58403966, solver group PID 3673960 / start 58405070. The full N22 source check is in N22/source_check.md. Keep production sources frozen until GQ is terminal and sealed. A C purpose-only miss is OPEN and retained. FP0 follows the N22 report independently.

N22. Ordered M/C controller PID 3686062 / start 58451924 waits until the accepted GQ is terminal and sealed. It applies the reviewed drafts in order and stops for disposition on a result failure.

N22 M. Applied the reviewed draft after the predecessor was terminal. Starting build590.

N22 M. Build590 compiled without warnings. Running the pinned fixtures and scoped gates.

N22 M. M1 PASSED, M2 PASSED. Starting G1.

N22 M. The accepted N21 GQ stop was 767.437 s with three windows per rank, two identical migrate=1 evaluations per rank, 18 fixedstorage=4120 snapshots and sampled rank peak sum 14,005,395,456 B. Build590 compiled in 372.832 s without warnings. M1 passed 10 assertions and M2 passed 7 per rank at two ranks, including exact local ownership with timings on/off. G1 is running.

N22 M. G1 matched all 375 / 114 / 228 records. Starting G2.

N22 M. G2 matched 90,809,869 first-collapse work units and all six rank counts. Starting G3 and G4.

N22 M. G3 passed with 1,532 splits and 1,113 exact common edits. G4 passed. Starting GQ.

N22 M. GQ reached three windows per rank in 762.305 s, and ownership outcomes matched the reference. Starting standing safeguards.

N22 M. Scoped gates and all standing safeguards passed. Next is the full workflow.

N22 M. Build590 G5 is live. Controller PID 3780441 / start 58833622, solver wrapper PID 3780598 / start 58834221. GQ matched both decisions on all six ranks, with 762.305 s to the prefix stop. The same full run must pass both events and the payload hashes before M is committed and C starts.

N22 M. G5 event one passed the timer and connected event receipts. All 14 ownership decisions on each rank and the three event-one payloads match N21 W. Job peak and process-peak sum both equal 14,001,750,016 B. The same run continued CFD and entered event two.

N22 M. Both native events completed and their timer and connected receipts passed. All 19 ownership decisions match on every rank. Native maxima 1,981.678851184 / 1,348.621272310 s. Event two is 5.986 s above the 5% report-only comparison of 1,342.635 s. Final transfer, continued CFD and the six-file comparison remain before the checkpoint.

N22 M. Build590 reached OPEN for the user with passing standing workflow gates. Preparing checkpoint.

N22 M. The two-event workflow and scopes are terminal. Repacked the final scoped log after the open-log archive mismatch. Reusing the passing numerical results for the checkpoint.

N22 M. Build590 reached OPEN for the user with passing standing workflow gates. Preparing checkpoint.

N22 M. Pushed `484f29315f`. Run Part C next.

N22 C. Applied the reviewed draft after the predecessor was terminal. Starting build591.

N22 C. Build591 compiled without warnings. Running the pinned fixtures and scoped gates.

N22 C. C2 PASSED. Starting G1.

N22 C. Build591 compiled without warnings in 403.046 s. C2 ended in 0.375 s, with 10 / 9 assertions across its two ranks. Both ranks returned the same memory refusal and stop code after the census reached the constrained rank. The ordered recovery controller is PID 3870974 / start 59229379. G1 is running.

N22 C. G1 matched all 375 / 114 / 228 records. Starting G2.

N22 C. G2 matched 90,809,869 first-collapse work units and all six rank counts. Starting G3 and G4.

N22 C. G3 passed with 1,531 splits and 1,113 exact common edits. G4 passed. Starting GQ.

N22 C. GQ reached three windows per rank in 671.241 s, and ownership outcomes matched the reference. Starting standing safeguards.

N22 C. Scoped gates and all standing safeguards passed. Next is the full workflow.

N22 C. Build591 G5 is live. Controller PID 3942632 / start 59561940, solver wrapper PID 3942757 / start 59562547. GQ passed both exact ownership decisions and required integer and modeled-memory rows in 671.241 s to stop. C's timing limits are the qualified M maxima, with combined ceiling 3,163.785117319 s. An isolated purpose miss retains C per the user order.

N22 C. At event one's first-collapse wall, mean preparation communication was 250 calls and 6.932 exclusive seconds per rank, versus M's 33,986 calls and 77.965 seconds. G5 remains active.

N22 C. Event one passed timer and connected gates at 95.2903% minimum coverage and 13,996,220,416 B job/process-peak sum. All 14 ownership decisions per rank and three output files match M. Its maximum native time is 2,018.805346030 s versus M's 1,981.678851184 s. The same run entered event two.

N22 C. Event two passed its timer and connected receipts at 95.2306% minimum coverage and 16,842,575,872 B job/process-peak sum. Maximum native time 1,083.551455574 s. The two native maxima sum to 3,102.356801604 s, below the 3,163.785117319 s ceiling. Event one exceeds its individual purpose limit. The final workflow receipt and six-file comparison remain.

N22 C. Build591 reached OPEN for the user with passing standing workflow gates. Preparing checkpoint.

N22 C. Pushed `e40d7e3795`. N22 reporting follows.

## Current work — FP0

N22 report pushed at `03323fa621` before FP0 began. Both FP0 pinned texts were verified and read. The isolated N21 source view verifies 1,383 accepted source and external pins and restores only the 11 later N22 paths in that view, including the pre-N22 test file. The active N22 solver and the reviewer handoff folder are untouched. Exact input extraction verified A at 146,676 points, 726,442 cells, 67,170 surface faces and B at 61,330 points, 303,359 cells, 48,954 faces. Build592 is the isolated test driver, PID 4061893 / start 59989667. Next are the ordered test-only measurements and G1.

FP0. Build592 linked in 829.834 s. The seal verified 13 dependency/configuration warnings and zero new warnings. A's first fixture stopped before measurement because the no-BL remesher factory did not load the sidecar. Preserved the terminal case under FP0/recoveries/A_kernels_592. Build593 (PID 4099073 / start 60112667) rebuilds the test-only sidecar load. Ordered controller PID 4099074 / start 60112667 follows it.

FP0. Part A kernels terminal, 10 measurement rows.

FP0. Part A optimize_10000 terminal, 3 measurement rows.

FP0. Part A optimize_100000 terminal, 3 measurement rows.

FP0. Part A census terminal, 2 measurement rows.

FP0. A10's first call invoked absolute shape admission. The first stop named cell 360 at q=0.18024384633056384 and J=0.028573106646635871. Source confirms the default measurement call still counts both shape failures. Preserve this receipt, let the current pinned campaign finish, and retry only the affected measurement after a test-only rebuild. A1–A8 and both A9 regions have independent receipts.

FP0. Part A proof terminal, 1 measurement rows.

FP0. Part A census terminal, 1 measurement rows.

FP0. Part A float_surface terminal, 1 measurement rows.

FP0. Build595 compiled without warnings after the build594 fixture namespace correction. A10 measured all 726,442 cells in 37.942836 s, matching A5 q/J/long-cell counts 17,427 / 16,623 / 90,050. A12 measured 67,170 faces with zero control-order mismatches in 1,000 comparisons and eight nonfinite projection retries using the same finite face. A1–A9 and A11 are reused from build593. Part B replay is live at six ranks. Controller PID 4123473 / start 60200883, replay wrapper PID 4127371 / start 60216874.

FP0. Part B frozen replay and six valid exit shards verified. Starting standalone Refine.

FP0. Part B replay ended in 2,002.454 s wrapper wall and 1,991.772 s native wall. Valid exit had 695,524 cells / 138,878 points, against the Part P comparator 726,442 / 146,676. Six valid exit manifests and payloads sealed. Native job-peak sum 13,996,642,304 B. The standalone Refine wrapper is PID 4181559 / start 60417519.

FP0. Standalone build595 was valid on all ranks in 366.005233 s maximum, with zero rounds, zero accepted changes and identical local meshes. The test-only aggregate wrapped a 7,292,792,832 B sum through World::sum(int). Preserved all measured rows and logs under FP0/recoveries/B_refine_595. Build596 PID 6867 / start 60474118 changes the aggregate to the existing 64-bit collective. Controller PID 6868 / start 60474118 reuses the six exit shards and runs only standalone Refine and G1.

FP0. Part B frozen replay and six valid exit shards verified. Starting standalone Refine.

FP0. Part B standalone Refine terminal and measured.

FP0. Build596 standalone Refine completed in 343.064097 s maximum rank wall. Six valid identical local meshes, zero rounds and zero accepted changes. The 64-bit job sum matches six process peaks at 7,293,308,928 B. Seed census max 252.659968 s, final proof max 41.747119 rank-exclusive seconds. G1 serial wrapper PID 24960 / start 60545498 is running.

FP0. G1 serial matched 375 case records.

FP0. G1 mpi2 matched 114 case records.

FP0. G1 mpi4 matched 228 case records.

FP0. 44 owned process identities verified terminal. The active N22 solver hash is unchanged.

## Current work — FP1

FP1 input copied with matching SHA256 and read. Whole-item source check recorded before any production edit. Baseline `a59380df18`; no tracked production or test changes in the scoped paths at start. Next is Part A's local operators, fixtures and G1, followed by its checkpoint before Part B.

FP1 A. All five operators, the pass and fixtures 1–8 are wired for build597. The first preflight stopped before compilation on the Work order 3 pin. The pin updater now includes the tracked order change. Next is the build and the single-rank fixtures.

FP1 A. Build597 stopped at the test-only ambiguous `Edge` name after compiling all new core sources. Corrected the two fixture declarations after the job ended. Sweep decisions now use edit results directly, with report counters gated on timings. Next is build598, fixtures 1–8 and G1.

FP1 A. Build598 compiled and linked with zero warnings. Fixtures 1–8 passed 57,484 assertions in seven cases. Box measure maxima were 8.75819e-16 (length), 5.55112e-16 (q) and 2.22045e-16 (J). Length exit left zero closed long edges and zero admissible short edges, with no sweep limit or memory refusal. The box went from 1,296 to 1,788 cells. G1 is next in the same controller.

FP1 A. G1 matched exactly 375 / 114 / 228 records at 1 / 2 / 4 ranks. All Part A gates passed on build598. Archive the evidence and commit and push A before changing Part B.

FP1 A. Pushed `68d28afad0` with all Part A gates passed. Part B is next.

FP1 B. Implemented the worker-only border rendezvous, frozen surface flag propagation, memory-admitted row exchange and directional shift. Part A remains gated at `68d28afad0`. Next are the 2- and 4-rank fixtures and the new binary's G1.

FP1 B. Build599 passed with zero new warnings. Fixtures 9–12 passed at 2 and 4 ranks. Each pass collapsed its designated short star and the final 768 rows repeated exactly. Lower/higher sent 99/96 and 291/288 rows, freeing 10/28 points, with 0 frozen in both for these cuts. Nine remote-only surface marks propagated at each rank count; strict Refine returned valid, with 0 rounds and 0 changes. G1 serial is running.

FP1 C preparation evidence. G2 reference provenance exposes construction communication calls of 340,843 per rank in N14 versus 333,799 in the accepted N22 C build. Section 2a explicitly substitutes N22 C for preparation only. Logged in the source-check report and R13 ledger. The written OFF gate remains the next check when part C starts.

FP1 B. G1 serial matched 375, and the two-rank table matched all 114 records. The G1 helper stopped on MPI's aggregate exit 10 versus 9. Both runs contain the same rank-local failures (10/9), and MPI selected a different first rank. Removed the helper's extra aggregate-exit equality requirement under R3. Reuse the first two terminal logs and run G1 at four ranks.

FP1 B. All Part B gates passed. G1 matched 375 / 114 / 228 records in 774.618 / 10.428 / 10.972 s. Archive and checkpoint Part B before changing Part C.

FP1 B. Pushed `6dc801d609` with all Part B gates passed. Part C is next.

FP1 C. Wired the native-only option (default NO), the guarded worker call before SnapshotSource, rank-striped IDs, existing construction/topology timers and the two admitted passes with boundary shifts. Fixture 13/14 source is in a new hidden tag. Archived the accepted solver and Part B test executable by hash before linking any C binary. Next is build600, fixture ranks 1/2/4, G1 and the exact OFF G2 comparison.

FP1 C. Build600 is frozen and active under controller PID 301708/start 61412555 (supervisor 301571/start 61412129). The remesher and option fixture translation units compiled. Build resource samples are in C/build600/build_samples.jsonl. The ordered shell runs the 1/2/4 fixtures and G1 after the link. The part C prefix adapter `FP1/run_prefix_fp1.py` has passed its no-launch checks and is next after G1. Independently authenticated the C workflow adapter's no-launch self-check; no G5 workflow has started.

FP1 C. Build600 passed in 843.508 s with zero new warnings. The option fixtures passed 42 assertions per rank at 1, 2 and 4 ranks. The two ON outputs at each rank count matched exactly and each remesh split 8 edges. The wall-height control reported one skipped core row per rank, and the default parsed as NO. G1 serial is running. The next live check is OFF G2 with the code and binary still pinned.

FP1 C. G1 matched all 375 / 114 / 228 adopted records in 768.429 / 9.214 / 10.552 s. The fixture and G1 controllers are terminal. Starting the pinned G2 OFF replay on build600.

FP1 C. OFF G2 is live with the build600 source and solver pins. The runtime controller and process-group identity are in C/prefix/G2_launch.json and C/prefix/cases/G2/status.json; the first-collapse exact comparison will run after the process group is terminal.

FP1 C. OFF G2 FAILED. At rank 0 row 19 the collective call count was 114,332 against 120,046. Construction timer calls were 333,799 against 340,843 on all ranks, and preparation matched N22 C at 250. Physical capture passed 90,809,869 work with collapse counts 480/473/346/263/469/464 in 251.972 s. The one read-only diagnosis matched all compared rows to the accepted N22 C capture and showed zero core/shift rows. No R5 correction or repeat applies. Preserve and revert C only. GQ, G3, safeguards and all G5 runs remain unevaluated.

FP1 close. Parts A/B passed and are retained at `68d28afad0` / `6dc801d609`. Failed C candidate `5bf9b8b4fd` was reverted by `d6e2ee8c4c`. Restored the accepted build591 solver and build599 test driver and verified their hashes. All 41 recorded owned process identities are terminal. Handoff, next-work status and timings updated. GQ, G3, safeguards and full workflows remain unevaluated. Next action under Work order 3 is the next user order.

## FP1 resumed — 2026-11-20

Archived and verified the resume order. Reviewed the original candidate and the F call paths before production restoration. Section 1 replaces the erroneous preparation-only G2 reference sentence. Next is the exact restoration of candidate `5bf9b8b4fd` by reverting `d6e2ee8c4c`, then Part F.

FP1 resume. Pushed Work order 3 correction 1 at `6733db27dd` and restored Part C at `61ad198cda`. The code tree at restoration matches candidate `5bf9b8b4fd`. The original stop at `d6e2ee8c4c` was caused by the erroneous reference sentence. F1–F3 are implemented. Next is the production precompile, then the complete F fixture build and G1.

FP1 F. Production precompile passed with F source pins verified. The four new fixture files are frozen and added to Meson. Starting build601 and the original and new fixture cohorts.

FP1 F. Build601 passed. Its fixture wrapper used a relative output path after changing directory, so GNU time exited 125 before any test. Resolved the output root and retained the original F3 report argument through the border reply call. Starting incremental build602 and all fixture cohorts.

FP1 F. Build602 stopped compiling because the retained report argument was still unnamed at the meeting definition. Named it so the private reply call can receive it. The argument stays unused in the callee and admission is the same for both modes. Retrying as build603.

FP1 F. Build603 passed with zero new warnings. Fixtures A, B, C and F4.1–3 and F4.5 passed. F4.1 changed 2,058 to 2,600 cells and made all five kinds of edit. F4.3 skipped 1,134 evaluations with identical output. F4.5 completed at 1/2/4 ranks with 16 total collapses per case and deterministic transfer. F4.4 is active at two ranks. Direct CPU sampling showed progress in the point-move quality operator on one rank while the other waited at the paired reduction. G1 follows the fixture.

FP1 F4.4 R3. Build603's two-rank exploratory fixture reached the quality sweep. Rank 1 reported `Degenerate metric donor`, then entered MPI finalization while rank 0 waited in the test reduction. Verified the owned timeout group's identity and terminated it. All seven recorded group and supervisor handles are terminal. Preserve both logs and retry the same box with a less extreme exploratory metric and collective exception reporting. No production correction was made.

FP1 F4.4 R3. Build604 passed and fixtures A/B/C, F4.1–3 and F4.5 passed again. The F4.4 two-rank retry ended in 2.487 s. Validity, repeat bytes, border writeback and strict Refine passed. All three quality edit counts were zero in both passes. Next is a fixture-only adjustment to create quality opportunities under the same box, cuts and assertions.

FP1 F4.4 R3. Build605 passed with the same solver hash. Tested both layouts. Two ranks produced 29/87/4/0/0 edits in pass 1 and 3/3/0/0/0 in pass 2 (collapse/split/edge removal/face swap/move). Four ranks produced 29/75/4/0/0 and 3/14/0/0/0, with 9 three-rank points, 4 four-rank points and 9 frozen at both meetings. The conforming and repeat checks and strict refinement passed. Face swaps and moves remain absent, and pass 2 makes no quality edit. Next is another fixture-only field adjustment.

FP1 F4.4 R3. Build606 passed with the same solver hash. Sharpened the x metric band and changed shear signs to vary the weak direction. At 2 / 4 ranks, pass 1 made 30/91/14/0/2 and 30/77/13/0/1 edits. Pass 2 made 2/3/7/0/0 and 2/16/7/0/0. Both cases passed all assertions except face swaps in pass 1. Test a small number of nearby exploratory field variants next.

FP1 F4.4 R3. Build607 field variant 1 passed at 2 and 4 ranks. Pass 1 made 32/96/19/3/4 and 32/81/17/3/3 edits. Pass 2 made 2/5/5/1/7 and 2/19/8/1/2. Both runs repeated exactly and strict refinement accepted. Pin this field and run all fixtures and G1 on build608.

FP1 F. Build608 passed in 14.307 s with zero new warnings. All original and resume fixtures passed on the pinned solver and test driver. F4.4 counts matched build607. Serial G1 started under controller PID 1050241/start 64901709 with MPI G1 queued. One short no-launch Python review observed during preflight completed in under 5 s. Next is the Part F gate-scope audit and checkpoint after G1.

FP1 F. G1 matched 375 / 114 / 228 records at 1 / 2 / 4 ranks in 765.088 / 8.946 / 11.937 s. Every Part F gate passed. The source scope is confined to the reviewed files and the PrintLocalPass field. Checkpoint Part F before any live Part C gate.

FP1 F. Pushed `f36b66eadd` after all gates passed. Section 4's code-tree comparison to Part B `6dc801d609` passed. The audit script initially omitted Part C's permitted PassCount declaration from its expected paths. Included that core header in the audit without a source change. Run fresh G2/GQ on build608, then G3 and the shared safeguards.

FP1 resume C. G2 passed on build608 at 246.927 s.

FP1 resume C. GQ passed on build608 at 696.140 s.

FP1 resume C. G3 passed 1535 splits with 1113 identical shared edits.

FP1 resume C. G4 passed at 1 / 2 / 4 ranks, matching the adopted 39-case assertion signatures on every rank. The three saved-state safeguards are next in the same controller.

FP1 resume C. The 481 merit safeguard passed byte-exactly in 120.191 s, with no private work. The saved 481 trajectory replay started next.

FP1 resume C. Saved 481 replay passed in 190.235 s with exact payload and planning rows and no private work. Current target replay is in progress.

FP1 resume C. All scoped checks and standing safeguards passed. Next is G5 OFF followed by the two ON runs.

FP1 resume C. All scoped controllers and saved safeguards are terminal and accepted. Current target replay took 144.886 s and matched its payload and planning records exactly. G5 OFF is running on the sealed build608 solver. The G5 controller launches ON1 then ON2 only after OFF and its isolated restart have passed.

FP1 resume C. Pushed scoped OFF gate checkpoint `20f425e3e6`. The G5 OFF first-collapse check passed with 90,809,869 work. Event 1 is in quality repair under the same pinned build608 source and solver. Find the live controller and resource records in FP1/resume/G5/OFF.

FP1 resume C. G5 OFF event 1 connected and timing gates passed with job peak 14,031,564,800 bytes, proof face total 61,906 and minimum coverage 95.2570%. The first continued CFD row was written. The same solver run proceeds to event 2.

FP1 resume C. G5 OFF event 1 mesh, native reference and restart flow match all three adopted SHA256 values. Event 2 started in the same workflow.

FP1 resume C. G5 OFF event 2 connected and timing gates passed with job peak 16,756,809,728 bytes, proof face total 58,260 and minimum coverage 95.3334%. Full two-event payload and isolated restart gates follow.

FP1 resume C. G5 OFF PASSED on build608. All six payloads match the adopted hashes. The event 2 isolated restart passed in 284.115 s with zero coordinate error. OFF native maxima are 1,818.319 / 1,039.725 s. ON1 started directly afterward with the same solver and config SHA256 `1ec6c26d49d4ec7c62103d5bd096471b5422290cd06c44b2f1073b7489f2b4c1`.

FP1 ON FAILED. ON1 event 1 first reported `complete=0` against 1, with `Degenerate metric donor`. The one read-only R5 diagnosis traced rejection to the prescribed point-move affine-weight calculation in pass 1, before either shift. No wiring correction or shift fallback applies. All solver handles are terminal. OFF passed both events, six hashes and isolated restart. Retain Parts A/B/C/F. ON2, ON event 2 and the ON reference are unevaluated. Seal the failure and close the item.

FP1 closed on build608. Last production checkpoint `f36b66eadd`. Verified 179 recorded identities terminal and zero active owned heavy processes. OFF remains the validated default. ON is failed under §12.5. The progress report, handoff and timings are complete. Next action is the next user order.

## FP1 resume 2

Archived and SHA256-verified `FP1_RESUME_2.md` and confirmed all 179 previous owned handles terminal. The whole-order source check identified a print-only scope discrepancy and recorded the R4 reading. Next is the exact filtered donor rule, pass diagnostics and focused flat-donor fixture. The standing OFF G5 proof is retained.

FP1 G. Implemented the filtered donor rule, Numeric step refusal and report-only timing/count fields. The flat-donor fixture is registered under a new hidden tag. Starting build609 on the verified host, then the new and inherited fixtures, G1 and fresh corrected OFF prefixes.

FP1 G. Build609 linked in 134.787 s with zero new warnings. The flat-donor fixture passed 12 assertions. Every inherited fixture passed with identical assertion totals and wired edit rows. The new timing, evaluation and sweep fields reconciled with the printed edit totals. G1 is running before corrected G2 and GQ.

FP1 G. G1 passed on build609, matching 375 / 114 / 228 exact records in 784.477 / 10.448 / 10.411 s. Code scope against f36b66eadd passed under the recorded print-only R4 reading. Corrected OFF G2 and GQ are next in the same controller.

FP1 G. Fresh OFF G2 passed in 257.098 s to cutoff. Exact first-collapse work 90,809,869 and accepted counts 480 / 473 / 346 / 263 / 469 / 464. The corrected comparator matched the exact rows, modeled storage, decisions and fixedstorage=4120. GQ is live.

FP1 G. Fresh OFF GQ passed in 906.877 s to cutoff with exact rows, modeled storage and decisions and fixedstorage=4120. An unrelated compiler workload was observed during the run and was terminal before replay preflight. Both fresh OFF gates and G1 are now accepted. Starting the frozen event 1 ON replay with the verified build609 driver, no CFD and no state capture.

FP1 G replay preparation. The adopted preflight stopped event 1 before spawning ranks because a separate Ninja/cc1plus build resumed. Confirmed no launch, wait record, run log or MPI output. Preserved the attempted command and the two process identities. Wait for the unrelated build's active compiler handles to clear, then retry only the preflight and launch. The case and build pins are intact.

FP1 G. The second preflight also stopped before rank spawn. The third cleared and launched exactly one event 1 replay at 10:48:58 Europe/Rome under controller PID 1331506/start 66044450. The replay uses six nice-19 ranks and the event 1 config SHA256 `1ec6c26d49d4ec7c62103d5bd096471b5422290cd06c44b2f1073b7489f2b4c1`.

FP1 G replay remains live near one hour of the 5,400 s cap. No local-pass, shift or rejection rows yet. Two bounded CPU samples show rank 3 in quality work while peers are in MPI in the later sample. Periodic actual-host RSS samples remain near 3,221 MiB sum of rank high-water marks.

FP1 G. Event 1 completed pass 1 and the lower shift on all six ranks. Rank 3 grew from 51,202 to 1,112,091 cells, spent 3,677.323 s in quality work and finished the first pass at 4,074.199 s inclusive. Rank 2 spent 1,060.993 s in quality. The lower shift preserved 1,642,036 cells globally. All six pass rows reported zero Numeric step refusals. Pass 2 is live.

FP1 resume 2 closed by Resume 3 §0. The verified replay group was ended with SIGTERM after 5,301 s and the controller returned terminal at 5,303.001 s. Six pass-1 and six lower-shift rows were preserved. No pass-2 or higher-shift row and no native event result. Resume 2 items 5.1 to 5.3 passed. Checkpoint Part G before editing H.

## FP1 resume 3

Archived and verified Resume 3. Part G and its stopped-replay evidence are pushed at 9940a85314. The whole-order H source check and readings are recorded. Capture the exact G Owned bytes before modifying the core, then implement H and run the required short gates.

FP1 H. Captured the independent Part G F4.1 canonical Owned payload using unchanged production sources and the temporary serial observer. Implemented the CoreOptions switches, shape/float/surface rules, work-list path and sweep sink. Starting build611 and separate new and inherited fixture cohorts.

FP1 H. Build611 linked but stopped before fixtures on one GCC 13 range-insertion warning in the new hand-mesh fixture. Three ordered appends kept the fixture vertices and assertions identical. Build612 then passed with zero new warnings. HO, HB, B, C, FA, FR, FC, FB and G passed. H matched the work-list and scan bytes on both test boxes and reproduced the 1,102,400-byte G control. The new second box reached only 9 collapse and 1 split sweeps in one call. Its only hot point was eligible for collapse before splitting. The inherited A probe also recorded 72 unclassified residual closed long edges near the unresolved surface. Correct the fixture's hot-node tensor and classify Blocked directly in the inherited probe. Rebuild with production H frozen.

FP1 H. Build613 passed with zero new warnings and all eleven fixture cohorts at the required rank counts. The second work-list box reached 38 collapse and 23 split sweeps in one call, with matched Owned bytes and edits. The legacy probe accounted for all 72 residual closed edges as blocked under H4. The code-scope proof passed. Fresh G1 and corrected OFF G2/GQ are running in order on the same pinned build. Commit H only after they pass.

FP1 H G1 passed on build613. Adopted case tables matched exactly for serial 375, two ranks 114, and four ranks 228. Clocks were 766.897 / 9.922 / 9.928 s. The OFF G2 prefix started after G1 terminated.

FP1 H G2 passed. OFF cut at 246.756 s. Work 90,809,869, all six ranks captured, exact corrected comparator passed with fixedstorage 4120. GQ started after G2 terminated.

FP1 H GQ passed. OFF cut at 685.827 s, exact corrected comparator passed with fixedstorage 4120. All build613 short gates and input provenance checks passed. Commit and push FP1 H, then run LAB2 V2, LAB1 V2, LAB2 V3, LAB1 V1, LAB2 V1 and LAB2 V0 in order.

FP1 H checkpoint pushed and verified at 1391148436. Started the ordered one-rank lab campaign on build613. Each pair has its own 1,200 s wrapper. Apply Resume 3 §9.1 only after all six pair receipts.

FP1 H lab pair 1 of 6 completed. LAB2 V2 pass wall 88.835 s, self-check passed and all written-back cells exactly positive. The separate strict quality counts and every sweep row are preserved. LAB1 V2 is running.

FP1 H lab pair 2 of 6 crossed the §9.1 timing threshold. LAB1 V2 had 301 complete sweep rows totaling 670.801 s before write-back, already beyond the 600 s inclusive pass threshold. Its 1,200 s wrapper remains in force. Preserve the terminal result and finish all ordered pairs before closing at §9.1.

FP1 H lab pair 2 of 6 completed. LAB1 V2 pass wall 1,072.121 s, self-check passed and all output cells exactly positive. The timing stop is confirmed. LAB2 V3 started after LAB1 V2 terminated.

FP1 H lab pair 3 of 6 completed. LAB2 V3 pass wall 60.969 s, self-check passed and all output cells exactly positive. LAB1 V1 started after the terminal receipt.

FP1 H lab pair 4 of 6 reached its 1,200 s limit during quality. GNU timeout sent SIGTERM and SU2's handler acknowledged it but sweep execution continued. Ended the verified owned group with SIGKILL at 1,292.141 s. Treat the rows before the first SIGTERM marker as the 20-minute result and preserve all raw rows separately. The receipt records exit 137 and the stop evidence. LAB2 V1 started after its terminal wait. A separate nice-19 guard now enforces the existing limit for the two remaining pairs.

FP1 H lab pair 5 of 6 completed the pass in 66.450 s and wrote 580,374 exactly positive cells with a passing self-check. The strict MeasureMetricCells audit then failed with Degenerate metric donor at cell 732833, so strict totals for LAB2 V1 are unavailable. LAB2 V0 started after that terminal result.

FP1 H lab pair 6 of 6 completed the pass in 162.988 s and wrote 566,975 exactly positive cells with a passing self-check. The strict MeasureMetricCells audit raised Degenerate metric donor at cell 729907. All six pair receipts and the wall guard are terminal.

FP1 Resume 3 STOP at §9.1. LAB1 V2 1,072.121 s exceeds 600 s by 472.121 s. LAB2 V2 88.835 s. Both production-form outputs passed exact validity and self-check. H checkpoint 1391148436 stays recorded. Seal the six ordered results and hold the replay, G3, safeguards, and G5 ON until the next order. The accepted G5 OFF proof is build608.

## FP1 resume 4

Archived Resume 4 and verified its SHA256. All six Resume 3 lab receipts are terminal. The already-pushed lab archive at 0b4d3b2baf includes the two V2 receipts, so §0 requires no job interruption or duplicate archive. Recorded the whole-item R4 source check. Next is H6, its hand and parity fixtures, and fresh OFF gates.

FP1 H6. Added the midpoint frontier check with the default split_progress switch and existing Size counter. The new hand ring uses all production defaults and isolates a spoke of length 2.1 against the old edge of length about 1.871. The inherited probes cover the new Size refusal and the G byte control disables H6. Starting build614 and the focused and inherited fixtures.

FP1 H6. Build614 linked cleanly. The new hand fixture passed 50 assertions and the H5 parity suite passed on the two pinned boxes. Before / after H6, the second box's maximum collapse and split sweeps in one call were 38/23 and 7/16. Both exceed the five-sweep minimum. The H6-off G byte control still matches 1,102,400 bytes. Inherited cohorts continue.

FP1 H6. All new and inherited serial/MPI fixture cohorts passed on build614. The unchanged Part A box had 72 residual closed long edges, all blocked, and zero admissible short collapses. Source scope against 1391148436 passed. Starting fresh G1 and then the two corrected OFF prefixes under one controller.

FP1 H6 G1 matched exactly 375 / 114 / 228 adopted records in 767.842 / 10.282 / 11.300 s. G2 OFF started after G1.

FP1 H6 G2 passed in 241.778 s to cutoff with 90,809,869 work units. Exact corrected comparison passed and fixedstorage 4120. GQ OFF started after G2.

FP1 H6 GQ passed in 716.584 s to cutoff. All corrected exact rows and fixedstorage 4120 matched. Commit FP1 H6, then run LAB1 V2, LAB2 V2, LAB1 V5 and LAB2 V3 in that order.

FP1 H6 checkpoint pushed and verified at c9ba8b3cb5. Started the ordered Resume 4 lab campaign on build614 with the hard 1,200 s wall wrappers. The source and config pins match.

FP1 H6 lab pair 1 of 4 passed both audits. LAB1 V2 pass wall 43.571 s, down from 1,072.121 s on H. All 422,857 written-back cells exactly positive. LAB2 V2 started after the terminal receipt.

FP1 H6 lab pair 2 of 4 passed both audits. LAB2 V2 pass wall 51.102 s, down from 88.835 s on H. All 558,310 written-back cells exactly positive. LAB1 V5 is next.

FP1 H6 lab pair 3 of 4 passed both audits. LAB1 V5 with surface blocking off completed in 47.670 s, with a passing self-check and exact-positive output. LAB2 V3 is next.

FP1 H6 lab pair 4 of 4 passed both audits. LAB2 V3 pass wall 26.658 s. All four owned lab receipts and observed processes are terminal. V2 on LAB1 and LAB2 passed its timing and exact-validity stop conditions. Seal the lab evidence and run the six-rank frozen event 1 replay under Resume 3 §9.2.

FP1 H6 lab evidence pushed at 8f484c76d4. Starting one six-rank frozen event 1 replay with the same build614 test driver.

FP1 H6 replay stage 0 completed both passes and both shifts on all six ranks. The sum of completed core and shift seconds per rank is 16.793–16.806 s. All higher shifts were visible under two minutes from process launch. The engine has 420,249 cells after the higher shift. Event acceptance is still live.

FP1 H6 replay accepted event 1 in 1,568.109 s with 554,738 cells, job peak equal to rank sum, and 95.055% minimum timer coverage. The stage-0 inclusive core/shift max was 16.806 s, and all higher shifts arrived within 600 s. Replay and all owned descendants are terminal. Seal and run §9.3 scoped OFF safeguards on build614.

FP1 H6 replay evidence pushed at a25cb93f9c. Run G3, G4, and the remaining saved-state safeguards sequentially.

FP1 H6 post-replay. G3 passed with 1148 splits and 1113 identical shared edits.

FP1 H6 post-replay. G4 2D cohorts passed.

FP1 H6 post-replay. Scoped gates and saved-state safeguards passed. The OFF workflow proof is build608.

FP1 H6 post-replay OFF scope passed. G3 1,148 splits and 1,113 exact shared edits. G4 at 1/2/4 ranks and saved safeguard modes merit481, saved481 and saved_current passed. Reuse build614 short prefixes and build608 G5 OFF. Seal then run two G5 ON workflows sequentially.

FP1 H6 scoped safeguards pushed at 92d1247868. Start the paired G5 ON campaign with the accepted build608 OFF proof.

FP1 H6 G5 ON1 event 1 native remesh returned complete on all six ranks, max 1572.119 s. Mesh and authority sidecar published. The full workflow continues toward event 2 and the standing gate collector.

FP1 H6 G5 ON1 ended at its event 2 F3b timer gate. Both native events returned complete=1 on all six ranks. Event maxima 1,572.119 / 383.244 s. Event 2 rank 0 named coverage 0.936883297417 versus 0.95, the first failed number. All six coverage values fail while join and the remaining timer ledger checks pass. The read-only diagnosis found that lower named time left a larger unattributed fraction, led by construction. There is no supported wiring or border-shift retry. FP1 ON FAILED under §12.5. ON2 and event 2 publication, workflow validation and restart are unevaluated. Retain H6 and build608 OFF. Seal the terminal evidence, update the handbooks and stop.

## FP1 Resume 5

Archived the order and verified its SHA256. Resume 4 ON1 and its owned descendants are terminal. The whole-item R4 source check identifies each exact lexical boundary for the four Part N timers. Next are the production edit, build615 and fresh fixtures and scope gates.

FP1 N. Installed the four scoped timers. Scope diff against eb9500b5d6 has exactly four hunks in three native-engine files. Starting build615, the complete FP1 fixture cohort and G1.

FP1 N. Build615 completed in 190.520 s with no new warnings. FP1 fixtures are running. The G5 adapter passed the no-launch checks against build608.

FP1 N. All 12 FP1 fixture selectors passed at their prescribed rank counts and match the build614 assertion totals. G1 is running on the same pinned build.

FP1 N. G1 matched all 375 / 114 / 228 adopted case records, at 761.093 / 9.872 / 9.572 s. The same build615 now runs the G2 and GQ prefixes, G3, G4 and saved-state safeguards.

FP1 N. G2 passed the corrected comparator at 236.503 s.

FP1 N. GQ passed the corrected comparator at 659.869 s.

FP1 N. G3 passed with 1563 splits and 1113 identical shared edits.

FP1 N. G4 passed at 1, 2 and 4 ranks.

FP1 N. Saved-state safeguards passed. The Part N checkpoint and fresh OFF G5 are next.

FP1 N pushed at d76fa0d5bc after Resume 5 §2 items 1 to 3. Starting the fresh build615 G5 OFF workflow.

FP1 N G5 OFF event 1 native remesh completed on all six ranks in at most 1,778.627 s. The F3b timer and P7/P8 connected proof gates passed. Minimum coverage rose from build608's 95.257% to 97.467%. Continued CFD and event 2 are next in the same workflow.

FP1 N G5 OFF event 1 published mesh, authority sidecar and restart all match the adopted reference hashes. Event 2 is executing in the same owned workflow.

FP1 N G5 OFF PASSED. Both events and isolated restart passed all standing gates. All six files match the adopted reference. Native maxima 1,778.627 / 1,029.081 s, per-event minimum coverage 97.467% / 98.019%. Commit OFF, then start the two same-binary ON runs.

FP1 N fresh OFF checkpoint pushed at d242c9421a. The same pinned build615 is running the ordered ON pair.

FP1 N ON1 event 1 passed the final timer and connected gates. Native maximum 1521.154 s and minimum named coverage 96.952%. Publication and continued CFD proceed toward event 2.

FP1 N ON1 event 2 passed final timer and connected proof gates. Native maximum 383.071 s and minimum coverage 95.110% on rank 0. Event 2 publication and full validation continue before the isolated restart and ON2.

FP1 N G5 ON1 passed both events, continued CFD and isolated restart. Native maxima 1,521.154 / 383.071 s, coverage minima 96.952% / 95.110%. Both timing comparisons passed. The same binary is running ON2. Pair identity still pending.

FP1 N ON2 event 1 passed final timer and connected proof gates. Native max 1519.720 s and minimum coverage 96.978%. Continue publication and event 2.

FP1 N ON: FAILED. Build615 ON1 passed both events and isolated restart. ON2 native maxima were 1,519.720 / 393.250 s. Event 2 rank 0 failed the first gate at 94.8416718673% named coverage against 95%, confirmed from the final exclusive rows. Event 1 files matched ON1. Event 2 publication, isolated restart and six-file repeat identity are unevaluated. The failed-run checkpoint retains the code and proven OFF default. Resume 5 ends here with no repeat.

FP1 S. Resume 6 SHA256 verified. R4 found no print checkpoint inside the three approved scopes. Prior workflows verified terminal. Starting the pinned build616.

FP1 S fixture A. Passed at the prescribed ranks; assertion counts match build615.

FP1 S fixture B. Passed at the prescribed ranks; assertion counts match build615.

FP1 S fixture C. Passed at the prescribed ranks; assertion counts match build615.

FP1 S fixture FA. Passed at the prescribed ranks; assertion counts match build615.

FP1 S fixture FR. Passed at the prescribed ranks; assertion counts match build615.

FP1 S fixture FB. Passed at the prescribed ranks; assertion counts match build615.

FP1 S fixture FC. Passed at the prescribed ranks; assertion counts match build615.

FP1 S. Build616 passed in 102.720 s with no new warnings. The scheduling comparator self-check and the build615 OFF proof binding passed without launching any solver.

FP1 S fixture G. Passed at the prescribed ranks; assertion counts match build615.

FP1 S fixture HO. Passed at the prescribed ranks; assertion counts match build615.

FP1 S fixture H. The first invocation omitted SU2_FP1_G_OWNED_BASELINE and stopped on its input check. The failed receipt is preserved. Retrying the fixture on build616 with the verified build609 F4.1 artifact.

FP1 S fixture H. Passed at the prescribed ranks; assertion counts match build615.

FP1 S fixture HB. Passed at the prescribed ranks; assertion counts match build615.

FP1 S fixture H6. Passed at the prescribed ranks; assertion counts match build615.

FP1 S. All 12 fixture selectors passed. Starting build616 G1.

FP1 S. G1 matched adopted records 375 / 114 / 228. Starting the G2, GQ, G3, G4 and saved-state checks.

FP1 S. G2 passed the corrected comparator at 236.468 s.

FP1 S. GQ passed the corrected comparator at 660.034 s.

FP1 S. G3 passed with 1552 splits and 1113 identical shared edits.

FP1 S. G4 passed at 1, 2 and 4 ranks.

FP1 S. Saved-state safeguards passed. The Part S checkpoint is next, with G5 OFF pending (FX1).

FP1 S gates 1 to 4 passed. G2 / GQ took 236.468 / 660.034 s. The per-rank Scheduling calls are sealed. G5 OFF pending (FX1). Next is the build616 ON pair with the sealed build615 OFF proof.

FP1 S gates 1 to 4 checkpoint pushed at 23ef6e9d3f. G5 OFF pending (FX1). Starting the two build616 ON runs.

FP1 S ON1 event 1 passed final timer and connected proof at 1499.541 s and 99.041% minimum named coverage. The workflow continues through event 1 publication and continued CFD to event 2.

FP1 S ON1 event 1 mesh, geometry sidecar and restart match build615 ON1 byte for byte. Event 2 is running.

FP1 S ON1 event 2 passed final timer and connected proof at 386.884 s. Minimum named coverage was 98.658%. Publication and restart validation continue.

FP1 S G5 ON1 passed both events, continued CFD and isolated disk restart. Native maxima 1,499.541 / 386.884 s and minimum named coverage 99.041% / 98.658%. The second run uses the same pinned build616 solver. Repeat identity is pending.

FP1 S ON2 event 1 passed final timer and connected proof at 1500.864 s and 99.043% minimum named coverage. Continuing publication and CFD toward event 2.

FP1 S ON2 event 1 published all three files. They match build616 ON1 and build615 ON2 byte for byte.

FP1 S ON2 event 2 passed final timer and connected proof at 373.634 s. Minimum named coverage was 98.650%. Publication, continued CFD, isolated reload and full repeat identity are next.

FP1 ON accepted. Build616 ON1 1,499.541 / 386.884 s at 99.041% / 98.658%, ON2 1,500.864 / 373.634 s at 99.043% / 98.650%. Both continued CFD and disk restart gates passed. Six output hashes match. All owned identities terminal. Part S G5 OFF pending (FX1). No further FP1 work in this order.

FX1. Verified order and completed R4. Start projection passed at 98.5820%. Skipped P1 and P2 under their explicit R4 conditions because the skipped build feeds later admission or exception classification. Applying independent Part M. Full OFF and ON workflow checks remain in scope.

FX1 M. Build617 passed without a new warning in 49.752 s. Starting G1, G2, GQ, G3, G4 and saved-state safeguards.

FX1 M. G1 matched adopted records 375 / 114 / 228. Starting prefix and safeguard validation.

FX1 M. G2 passed the corrected comparator at 236.506 s.

FX1 M. G2 initial metric SHA256 and gradation rows match the reference.

FX1 M. GQ passed the corrected comparator at 660.503 s.

FX1 M. GQ initial metric SHA256 and gradation rows match the reference.

FX1 M. G3 passed with 1576 splits and 1113 identical shared edits.

FX1 M. G4 passed at 1, 2 and 4 ranks.

FX1 M. Saved-state safeguards passed. The Part M checkpoint is next, with G5 pending.

FX1 M short gates passed on build617. Both initial metric hashes and all gradation rows match, and all eighteen adopted scheduling rows are exact. G5 pending. Next is the fresh OFF workflow.

FX1 M short-gate checkpoint pushed at 15bafef314. Starting build617 G5 OFF.

FX1 G5 OFF first metric published the required initial restart SHA256. The three count rows agree, with 33.368 s maximum nested gradation. Native event 1 continues.

FX1 G5 OFF event 1 passed final timer and connected proof at 1783.323 s and 99.008% minimum coverage. Publication and continued CFD advance toward event 2.

FX1 G5 OFF event 1 mesh, geometry sidecar and restart match the adopted payloads byte for byte.

FX1 G5 OFF event 2 passed final timer and connected proof at 1051.580 s and 99.529% minimum coverage. Publication, continued CFD and isolated restart are next.

FX1 G5 OFF passed at 1,783.323 / 1,051.580 s and 99.008% / 99.529% minimum named coverage. All six payloads, the initial restart and all gradation counts match. This completes the FP1 Part S OFF proof. Starting the same-binary ON check.

FX1 G5 ON event 1 passed final timer and connected proof at 1552.606 s and 99.065% minimum coverage. Publication and continued CFD advance toward event 2.

FX1 G5 ON event 1 mesh, geometry sidecar and restart match the accepted build616 ON payloads byte for byte.

FX1 G5 ON event 2 passed final timer and connected proof at 378.535 s and 98.738% minimum coverage. Publication, continued CFD and isolated restart are next.

FX1 closed. Part M passed both G5 modes on build617. ON native maxima 1,552.606 / 378.535 s, coverage minima 99.065% / 98.738%. Both modes match their exact reference and initial metric files. Part P omitted under R4. FP1 S OFF proof is complete. All owned jobs terminal.

FP2-0. Order copied and SHA256 verified. The source check and test-only measurement fixtures are prepared. The new fixtures use the pinned sidecars and frozen sensor data.

FP2-0. Verified FX1 terminal handles and host resources. Starting test-only build618.

FP2-0. Build618 passed without a new warning. Starting the three serial measurement fixtures at nice 19.

FP2-0. bounds fixture is terminal and its measurement receipt is sealed.

FP2-0. projection fixture is terminal and its measurement receipt is sealed.

FP2-0 R3. Build618 split stopped at the copied volume sensor count. Verified the unchanged B sensor contains 61,330 records and exactly matches all 24,479 boundary coordinates. Corrected the fixture reader. Building test-only build619.

FP2-0. Test-only build619 passed without a new warning. Starting the final pinned measurement series.

FP2-0 build619. bounds fixture is terminal and its measurement receipt is sealed.

FP2-0 build619. projection fixture is terminal and its measurement receipt is sealed.

FP2-0 build619. IN1 matched all 24,479 boundary points. L accepted 2,719 splits and D accepted 12,873, with each reaching 20 passes. The prototype has 80,138 faces and 40,071 points. The strict comparisons are running.

FP2-0 build619. Strict Classify covered all 48,954 IN1 faces and 36,189 new PD faces at stride 1. PD has 18 local entity-check failures and 46 faces above the census cutoff. Tight lists and the bulk constructor are next.

FP2-0 build619. split fixture is terminal and its measurement receipt is sealed.

FP2-0 build619. P3 rejected PD after 27.087 s on an entity/orientation check. P4 is not applicable. T completed 145 IN1 and 200 PD queries. All measurement receipts and per-face ledgers verified. G1 validation is running.

FP2-0 done on build619. G1 matched 375 / 114 / 228 adopted records. Length and distance steps produced 15,592 splits. The bulk constructor rejected PD on entity/orientation, so P4 was not applicable. The IN1 tight list reached its five-minute limit with 145 of 200 queries complete, and the PD list completed 200 of 200. This was a test-only measurement. All owned jobs are terminal.

FP2-0b. Archived and verified the order. R4 source check complete, including the public feature proposal, two-face reconstruction, strict constructor and P4 paths. Starting test-only fixture changes.

FP2-0b. Prior FP2-0 controllers verified terminal. Host load 0.85 / 0.72 / 0.48 with 45,418 MiB available. Starting test-only build620.

FP2-0b build620 passed in 22.717 s without a new warning. Solver digest matches FP2-0. Starting fresh IN1 Run A, one rank at nice 19, with strict child checks off.

FP2-0b build620 Run A finished L and D after 7 / 8 nonempty passes. L accepted 2,380 walk and 35 feature splits, D accepted 15,325 walk and 22 feature splits. PA has 84,478 faces, 42,241 points and 731 traces. Strict comparisons are running.

FP2-0b build620 Run A strict census measured 48,954 IN1 and 40,734 new PA faces at stride 1. All 84,478 PA faces passed entity checks and zero were above tolerance by Classify. P3 accepted PA in 28.722 s. Running P4; the Run B condition was not met.

FP2-0b build620 Run A is terminal and sealed. P4 accepted all four entities, with mesh-to-model 13.662 s and model-to-mesh 42.486 s. Peak RSS 2,034,744 KiB. Run B was not triggered. Starting G1 on the same pinned test binary.

FP2-0b build620 G1 serial matched 375 adopted case records in 753.158 s. Running the two MPI cohorts sequentially.

FP2-0b closed on test-only build620. G1 matched 375 / 114 / 228 adopted case records in 753.158 / 9.954 / 10.820 s. P3 accepted PA and P4 accepted all four entities. The report and ledgers are complete. All five owned controllers and three supervisors are terminal.

FP2-A. Verified input SHA256 and read complete order. Confirmed prior owned controllers terminal. Completed one whole-item R4 source check on abc9450b86. Part A implementation and test fixtures next.

FP2-A part A. Implemented the dormant surface split core and one-rank state builder. Starting build621 with unit-test target only.

FP2-A part A build621 passed in 41.892 s without new warning. Starting serial box, IN1, LAB-S, then qualified FP1 fixture and G1 selectors.

FP2-A part A build621 box and cylinder passed all 615 assertions across 3 cases. Checking pinned IN1 with production geometry.

FP2-A part A build621 IN1 passed in 76.611 s. The validating constructor accepted 84,086 faces, 42,045 points and 731 traces after 17,566 splits. Running LAB-S on the saved volume.

FP2-A part A build621 LAB-S accepted 16,201 surface splits (56 feature), validated 469,411 exact-positive cells and rebuilt the 81,356-face state. Zero outward or validity refusals. Proof and full Classify census running.

FP2-A part A build621 LAB-S passed in 274.368 s. All 469,411 cells exact-positive, 81,356 final faces classified with zero above tol, both proof directions accepted for all four entities. Inherited FP1 and G1 checks running.

FP2-A part A build621 all 18 pinned FP1 fixture cohorts passed. G1 serial has started.

FP2-A part A build621 G1 serial matched 375 adopted records in 754.401 s. The two MPI cohorts follow.

FP2-A part A PASSED on build621. All 18 FP1 cohorts and G1 375 / 114 / 228 matched. Ready to checkpoint Part A, then begin the worker exchange and state build.

FP2-A part A checkpoint pushed as f7ec841a9a. Part B worker-only overlay and state builder implemented without an NRR change. Starting test-only build622.

FP2-A part B build622 passed in 24.396 s without new warning. Starting the 2/4-rank overlay and LAB1 rebuild cases, the 18 FP1 cohorts, and G1.

FP2-A part B build622 rank-2 overlay fixture passed (including migrated feature child split and duplicate refusals). The LAB1 fixture is active. A noninterrupting 116-sample callchain attributes 88.79% to Mesh::Star from MovePoint in QualitySweep and 6.90% to EdgeRing through RemoveEdgePlan in the same volume-quality phase. No files or running job were changed.

FP2-A part B rank-2 LAB1 is still in pass 1 as of 12:42 CET. Its controller is live and the 5400 s fixture bound is unchanged. R4 permits a small independent §10.7 case if the LAB fixture reaches its bound.

FP2-A part B build622 rank-2 LAB1 reached its 5400 s wrapper limit (124, 5402.006 s including exit) with no completed pass row. Verified both ranks and controllers terminal. Retained the attempt and exact fixture patch. Proceeding with the explicitly permitted independent two-cube MPI rebuild fixture.

FP2-A part B build623 passed with the small reconstruction fixture, no new warning. Starting the 2/4-rank exchange and state proof, inherited FP1 and G1.

FP2-A part B R3. Build623 rank-2 small fixture passed 150 assertions per rank, with 24 surface faces and equal snapshots. The report parser found 0 rows because an ANSI reset adjoined the marker. Preserved the receipt and parser, stripped ANSI before row detection, and extended the parser self-check to cover that prefix. Retesting with the same pinned binary.

FP2-A part B build623 passed the small 2- and 4-rank exchange and rebuild cases. Both reconstructed 24 faces, 14 points, 18 traces, revision 2, with 9664 identical Snapshot bytes on every rank and owned-face sum 24. FP1 and G1 are sequentially running.

FP2-A part B build623 all 18 inherited FP1 cohorts passed. G1 is in its serial cohort.

FP2-A correction 1 received and SHA256 verified. Build623 Part B G1 remains pinned and is finishing. At the next boundary, remove the surface length step’s use of the split progress test, rerun §2 checks (including the LAB-S alarms), and repeat Part B on the corrected binary.

FP2-A build623 uncorrected G1 finished 375 / 114 / 228 and all owned jobs are terminal. These Part B receipts are historical under Correction 1. Applied the correction in one surface L call; both surface calls now pass false and the volume progress rule is intact. Corrected build624 next.

FP2-A correction 1 build624 passed without a new warning. Running repeated Part A fixtures with explicit LAB-S runaway alarms before the inherited gates.

FP2-A correction 1 build624 repeated box/cylinder and IN1 successfully. Box/cylinder counts and IN1 split/face/trace counts match build621. Corrected LAB-S is active.

FP2-A correction 1 build624 LAB-S finished the volume pass at 554,554 cells with 7 surface L sweeps. L accepted 2,416 splits and left 0 long edges, with 0 size, outward and validity refusals. Neither alarm triggered. Constructor and owned-face check passed; proof/census active.

FP2-A correction 1 build624 LAB-S passed in 284.768 s. All 554,554 cells exact-positive, all four entities proved, full Classify 0 of 84,088 above tolerance. No alarm. Continuing the inherited FP1 and G1 gates.

FP2-A correction 1 build624 all 18 inherited FP1 cohorts passed. G1 is running.

FP2-A Part A correction 1 PASSED on build624. G1 matched 375 / 114 / 228 and all 18 FP1 cohorts passed. Committing the correction and evidence before the corrected Part B MPI fixtures.

FP2-A Part A correction 1 pushed as 299e2928ec. Repeating the Part B MPI cases on build624 and verifying the freshly repeated same-binary FP1 and G1 receipts.

FP2-A part B PASSED on corrected build624. Exact MPI snapshots matched at 2 and 4 ranks with 24 faces, 14 points and 18 traces, and the re-executed same-binary 18 FP1 cohorts plus G1 375 / 114 / 228 are verified. Checkpoint before Part C.

FP2-A part B after correction 1 pushed as b569e34e7e. Part C option and owned-worker call implemented. Starting full build625, including the solver, at nice 19.

FP2-A correction 2 archived and SHA256 verified while full build625 is pinned. The new bounded LAB1 MPI rank check precedes every §11.2 gate.

FP2-A full build625 passed in 790.285 s with no new warning. Its controller is terminal. Correction 2 test source can now be added and compiled before its rank gate.

FP2-A correction 2 build626 passed without new warning. Its sole source-pin difference from build625 is the rank-check test TU, and the solver hash matches. Starting K1 then K2 with 1200 s each; K3 is conditional on K1.

FP2-A rank check: stopped before 11.3. K1 reached the 1200 s wrapper limit with no completed pass 1 on either rank; first-sweep max stars 65,654 and 98,388 with zero unresolved/blocked at that observation. K2 ended in 70.818 s with both passes, exact positive cells and geometry/Snapshot agreement. K3 skipped by the K1 rule. All owned rank-check handles terminal. Committing evidence before starting 11.2.

FP2-A rank-check evidence pushed as e056028b0a. Starting §11.2.1 and §11.2.2 on build626: fixtures and G1, then exact G2/GQ, G3/G4 and saved safeguards.

FP2-A Part C §11.2.1 passed on build626. Corrected A/B and all 18 FP1 fixture cohorts passed, and G1 matched 375 / 114 / 228 in 759.081 / 9.050 / 9.749 s. Moving to §11.2.2.

FP2-A Part C G2 passed on build626 (242.814 s wrapper), with exact governed rows, adopted scheduling calls and fixedstorage=4120. GQ is running.

FP2-A Part C GQ passed on build626 (661.142 s wrapper) with exact governed rows and adopted scheduling calls. G3 is running.

FP2-A Part C G3 passed on build626 (610.141 s wrapper). 1,538 splits, 1,113 matching common edits, and observer-adjusted entry work matched. G4 and safeguards follow.

FP2-A G4 passed, 3 cohorts, 36.104 s wrapper. The merit481 saved safeguard passed in 122.416 s; the remaining saved-state safeguards are running.

FP2-A Part C §11.2.2 complete on build626. G2/GQ, G3/G4 and merit481, saved481 and saved_current safeguards passed. Starting §11.2.3 G5 OFF with both local-core options disabled.

FP2-A Part C R3. First G5 OFF adapter preflight stopped before launch because a string binary path was passed to the checksum helper. Converted the binary path to Path at both caller sites. Solver and receipts unchanged; no owned job started.

FP2-A §11.2.3 build626 G5 OFF launched at 15:58 CET with both options disabled. Host check confirmed six owned solver ranks at nice 19. Event 1 reached the complete shape/geometry census and is in quality improvement. No gate result yet.

FP2-A §11.2.3 build626 OFF event 1 passed the terminal proof and timer checks. Native max 1,760.966 s, minimum coverage 99.012%, peak 14,077,104,128 B equal to the sum of rank peaks, owned-face sum 61,906. Transfer, continued CFD and event 2 follow.

FP2-A Correction 3 arrived during build626 G5 OFF. Copied the complete order, checked SHA256 01d871b7e2999e820d18529271b7a7050f3471f50983c552acd88e775406d758 and read it in full. The three event 1 payloads match N21 W. Event 2 is running. Drafting Correction 3 without changing pinned code, then repeating its serial and rank checks at the next boundary.

FP2-A build626 OFF event 2 passed terminal proof and timers. Native max 1,040.040 s, coverage minimum 99.515%, job/process-sum peak 16,755,060,736 B, owned-face sum 58,260. Continued CFD and isolated restart remain in the owned workflow. Correction 3 will apply after its terminal audit.

FP2-A build626 OFF full workflow passed both event validations and continued CFD. All six payloads are byte-identical to N21 W. The isolated restart has started. Neither local-core option is enabled and neither surface marker appears in the full run. Superseded build626 evidence will be kept separate from the corrected qualification.

FP2-A build626 G5 OFF sealed PASSED. Both full events, their six exact payloads, continued CFD and isolated restart passed. Full wrapper 3,201.354 s, restart wrapper 234.563 s. Re-verified the sealed receipt and owned handles on the real host, then applied Correction 3. Its surface sweep printing adds a surface-only call to the existing solver sink printer in NativeRemeshRun.inc, so the pre-correction OFF proof falls outside the file-list exception. Build627 solver and test driver are compiling. Next: serial fixtures and LAB-S, then K1 and conditional K3.

FP2-A build626 historical default checkpoint pushed as 4d1d2ea579. Build627 source manifest differs in three production files (core surface declarations, surface sweeps, and the existing solver print sink) and two test translation units. Correction 3 serial checks will start after the build controller is terminal.

FP2-A Correction 3 build627 passed in 98.945 s without a new warning. Running the §3.1 serial fixtures and LAB-S before the rank checks.

FP2-A Correction 3 build627 box and cylinder gate passed, including refusal of the longest edge without fallback and both ordinary and feature length floors. The pinned IN1 geometry fixture is running.

FP2-A Correction 3 build627 IN1 passed in 77.004 s, with 84,086 faces, 42,045 points, 731 traces and 17,566 prototype splits, matching the pinned fixture. LAB-S is running on the saved volume.

FP2-A Correction 3 build627 LAB-S completed the core pass with 554,558 exactly positive cells. Surface L/D accepted 2,416 / 15,158 splits in 7 / 8 nonempty sweeps and finished with 84,102 faces. Zero long edges or above-limit sampled faces remain. The post-surface maximum star is 168 with zero above 1,000. Both correction-1 alarm values are inside their bounds. State build and strict proof are running.

FP2-A Correction 3 serial gates passed on build627. LAB-S took 285.266 s, all four strict geometry entities accepted, Classify flagged 0 of 84,102 faces, and both correction-1 alarms were clear. All serial controllers terminal. Starting K1 on 2 ranks, retaining the verified build626 K2 receipt and making K3 conditional on K1.

FP2-A Correction 3 K1 ended in 126.387 s with both ranks completing both passes, exact positivity and the state/Snapshot checks. First post-surface stars are 168 and 124 (build626 first-volume observations 65,654 and 98,388). Correction 3 raised no alarm on either pass. K3 has launched at six ranks.

FP2-A rank check stopped before 11.3 on build627. K3 completed both passes and every mesh/Snapshot check in 114.542 s, with max rank pass 26.570 s and max post-surface star 168. Its first qualification alarm is rank 4 pass 1 faces_out=19,559 > 18,101.061 (faces_in=5,269). No star or sweep-limit alarm. Build626 K2 retained. All rank-check controllers terminal. Preserving the rank alarm checkpoint before the required default tests.

FP2-A build627 rank-alarm evidence pushed as 5814de1d8b. Starting the same-binary Part B MPI fixtures, all 18 FP1 fixture cohorts and G1. Their receipts also satisfy §11.2.1.

FP2-A Correction 3 build627 passed both Part B MPI state comparisons and all 18 inherited FP1 fixture cohorts. G1 serial is running. The correction checkpoint follows G1 identity, then §11.2.2.

FP2-A Correction 3 §3.3 passed on build627. G1 matched 375 / 114 / 228 adopted records. Part B and all inherited FP1 fixtures passed on the same binary. The correction checkpoint excludes the still-unqualified Part C integration. Next: §11.2.2 default prefix and safeguard checks.

FP2-A correction 3 pushed as 44b5b199a4. Only the original Part C production integration remains uncommitted. Starting build627 default G2/GQ, G3/G4 and shared-engine saved safeguards.

FP2-A build627 G2 passed in 238.103 s with all governed rows, six adopted scheduling counts and fixedstorage=4120 exact. GQ is running.

FP2-A build627 GQ passed in 656.240 s with all governed rows, twelve adopted scheduling counts and fixedstorage=4120 exact. G3 is running.

FP2-A build627 G3 passed in 615.279 s, with 1,559 accepted splits, exact common prefix of 1,113 edits, observer-adjusted entry work equal and all physical gates passed. G4 and saved safeguards follow.

FP2-A build627 G4 passed the 1-, 2- and 4-rank cohorts in 36.556 s. Running the saved-state safeguards.

FP2-A build627 merit481 saved-state safeguard passed in 121.675 s. The saved481 and saved_current repeats remain.

FP2-A build627 §11.2.2 complete. Saved481 and saved_current passed in 143.906 / 144.107 s. G2/GQ, G3/G4 and all three saved-state safeguards passed and the scoped controller is terminal. Starting the fresh same-binary §11.2.3 OFF workflow.

FP2-A build627 scoped default evidence pushed as f88fbc758d. Part C production sources remain pinned and uncommitted during G5. The build627 OFF runner is active.

FP2-A Correction 4 received during build627 G5 OFF. Copied, fully read and verified SHA256 43b828333e7bcd4db85445de7ee94b660785a8aab485ac509140536637a2b3dd. Re-evaluation of the sealed receipts found K1 pass sums 84,118 / 84,174 and K3 sums 84,124 / 84,184 against 126,153. Max star 168 and max distance sweeps 14 in both, K3 max rank pass 26.570 s. K1 and K3 count as ended, K2 retained. The prior Correction 3 stop remains historical. No rank rerun or production change. Current OFF continues under its pinned script, then CORE, S1 and S2 on build627.

FP2-A Correction 4 evaluation pushed as c74b7223ea. The original OFF driver and rank summary remain byte-identical. The test-only successor uses the new sealed evaluation to admit the remaining same-binary workflows. The §11.4 report helper now checks the S1 event 1 per-stage/pass face sums.

FP2-A build627 G5 OFF event 1 passed the terminal proof and timers. Native max 1,779.827 s, minimum coverage 99.004%, job peak and rank-peak sum 14,007,259,136 B, owned-face sum 61,906. Transfer and continued CFD are under way.

FP2-A build627 G5 OFF event 1 published the mesh, geometry sidecar and restart. All three SHA256 values match N21 W. Event 2 is running.

FP2-A build627 G5 OFF event 2 passed terminal proof and timers. Native max 1,023.870 s, minimum coverage 99.530%, job peak and rank-peak sum 16,775,716,864 B, owned-face sum 58,260. Publication, continued CFD and isolated restart follow.

FP2-A build627 OFF full workflow passed both event validations and continued CFD. All six payloads match N21 W exactly. Both local-core options are absent and the six rank logs contain no surface pass or state rows. The isolated restart is running.

FP2-A build627 G5 OFF passed both events, all standing workflow gates and isolated restart. All six outputs match N21 W. Native maxima 1,779.827 / 1,023.870 s with minimum coverage 99.004% / 99.530%. OFF controller and supervisor terminal. Starting §11.2.4 CORE on the same binary with the unchanged FP1 ON config and surface option at its NO default.

FP2-A build627 CORE event 1 passed terminal proof and timers. Native max 1,512.056 s, minimum coverage 99.045%, job peak and rank-peak sum 11,576,320,000 B, owned-face sum 68,006. The §11.3 event 1 comparison bound will be 1.05 × the exact CORE maximum, approximately 1,587.659 s. Transfer and continued CFD are running.

FP2-A build627 CORE event 1 published all three payloads, and all match the accepted FP1 ON hashes. Event 2 is running.

FP2-A build627 CORE event 2 passed terminal proof and timers. Native max 369.654 s, minimum coverage 98.654%, job peak and process peak sum 11,655,073,792 B, owned-face sum 64,758. Publication, continued CFD and isolated restart follow.

FP2-A build627 CORE full workflow passed two events, transfer and continued CFD. All six payloads match accepted FP1 ON exactly. The six rank logs contain zero surface pass and state rows. The isolated restart is running.

FP2-A Part C default proven on build627. G5 OFF and CORE passed both events and isolated restart and each matched its six adopted payloads. Correction 4 opens §11.3 after the default checkpoint. Next: S1 and S2 on this same binary.

FP2-A C default-proven checkpoint pushed as 36d41fc3c3. Both default jobs and their isolated restarts ended. Starting §11.3 S1 on the same pinned build627 solver. S2 follows only if S1 passes its standing gates.

FP2-A S1 launched with the surface-enabled configuration SHA256 07b91a94996bf3cb48352041d8e1643ffe17a61bed07432085cfcf87b1725382. The active case config matches the pinned file and enables both options.

FP2-A build627 S1 event 1 stage 0 completed both surface passes and installed the verified state on all six ranks. It accepted 18,120 surface splits and rebuilt 85,194 faces, 42,599 points and 748 traces (revision 2). Native engine construction is next.

FP2-A S1 event 1 remains in native repair. At 20:30 CET (2,850 s workflow elapsed) the latest full per-rank windows still sum to five q failures, one J failure and nineteen geometry failures. The native event has exceeded the 1,587.659 s purpose threshold and remains under its separate 5,400 s standing limit. All six owned ranks remain active.

FP2-A S1 FAILED event 1. First standing failure complete=0 on all ranks against 1. Maximum 5,430.853544 s against 5,400 s. Existing controller stopped the rejected workflow and all recorded handles are terminal. The one read-only R5 diagnosis found unchanged final counts of 5 q, 1 J and 19 geometry failures for at least 3,759 s, then the native geometry/private-work deadline. No wiring error established. S2, second-event qualification, isolated restart and repeat identity are unevaluated. Default proof stays accepted at 36d41fc3c3. Closing reports, archive and terminal audit follow.

FP2-A §11.3 closed. FP2-A ON FAILED in S1. The build627 default qualification stays passed. Terminal audit verified 363 owned identities. Reports and handoff updated. Stop and wait for the user after the closing checkpoint.

FP2-B0 order archived and SHA256 verified. Completed the whole-item source check at 852e566d17. Test-only measurement will reproduce the frozen one- and six-rank passes, observe c1–c4, and execute bounded fixture V operations. No FP2-A qualification state changes.

FP2-B0 build628 stopped on one test-only name ambiguity (Incidence). Its controller is terminal. Renamed the helper to LiveIncidence and starting build629, with the solver binary still pinned.

FP2-B0 build629 passed without a new warning in 21.795 s. The production audit found zero SHA256 differences outside UnitTests and the solver binary is unchanged. Running the serial surface fixtures, then H1, H0, H6, H6off, X1, X2, X3, and the selected X6 in sequence.

FP2-B0 build629 serial surface fixtures passed 675 assertions in four cases. The new frozen H1 measurement is running.

FP2-B0 R3. The serial gate passed, but JSON receipt equality rejected tuple-vs-list representation before H1 launched. Preserved the original runner and passing receipt, fixed normalization and verified the completed cohort on resume. Build629 remains pinned.

FP2-B0 H1 completed on build629 in 108.761 s. It reproduced the LAB-S cells and faces exactly (554,558 / 84,102) and measured 11,296 c3 bad cells with no free-count discrepancy. H0 is running.

FP2-B0 H0 completed in 80.033 s, with 422,857 cells, 48,954 faces and 2,836 c3 bad cells. H6 is running with unit weights and the frozen K3 inputs.

FP2-B0 H6 completed in 76.362 s. It reproduced K3 exactly at 549,519 cells after the higher shift and 84,124 / 84,184 summed faces by pass. The last c3 census found 11,099 bad cells, with no class (d) mismatch. H6off is running.

FP2-B0 H6off completed in 67.064 s, with 419,732 cells, 48,954 owned physical faces and 2,538 c3 bad cells. H6 and H6off had no c3 class (d) discrepancy. X1 is running the serial V-split trial.

FP2-B0 X1 completed in 116.968 s. Its five rounds accepted 1,180 V-splits with a zero-operation terminating round. It reached 4,393 c4 bad cells and 562,386 exactly positive cells with the surface fixed. X2 is running.

FP2-B0 X2 completed in 122.049 s with 145 V-collapses in four rounds. It reached 10,645 c4 bad cells and 553,451 exactly positive cells with the surface fixed. X3 follows.

FP2-B0 X3 completed in 124.055 s. Seven rounds of collapse then split reached 3,756 c4 bad cells and 561,067 exactly positive cells with the surface fixed. The pinned X6 choice is X3 (both), the smallest c4 count among X1/X2/X3 (4,393 / 10,645 / 3,756). X6 is running.

FP2-B0 completed on build629. Hand-over counts: H6off 2,538 / H6 11,099 / X6 3,741 / H0 2,836 / H1 11,296 / X1 4,393 / X2 10,645 / X3 3,756. X6 selected X3. All exact positivity checks passed. The production SHA256 audit found zero differences across 1,624 paths, and 63 witnessed owned process identities were terminal. Report and handoff updated. Closing checkpoint and post-push verification follow.

FP2-B order archived and verified. Whole-item R4 source check sealed at bb352600c5. Prior host audit matched all 1,624 production pins and found all 63 FP2-B0 identities terminal. Next A1 implementation, small fixtures and Gate E on an A1-only build.

FP2-B A1 build630 passed in 45.867 s with no new warnings. Solver SHA256 unchanged at build627. Test driver SHA256 26402da296b71347f49a8eb331727acba8e4dc8f1da13c1920d91310a1fe716c. Starting Gate E on the A1-only binary.

FP2-B build630 E1 execution completed in 124.093 s. All raw Gate E rows exactly match X3 (3,756 bad, 561,067 cells, 84,102 faces, 11,296 before V, 172 V-collapses, 1,175 V-splits, seven exact rounds). R3: the result file was written, but tool readback compared JSON rank keys against Python integers. Preserving the initial helper and binding the existing receipt without rerunning E1. Running the focused A1 serial fixture next.

FP2-B A1 serial fixtures passed on build630 (105 assertions in three cases). V-split acceptance and rollback, unchanged boundary nodal values after V-collapse, and the ON/OFF empty-round counters are verified. Next E6 after E1 receipt verification.

FP2-B A1 Gate E passed on build630. The E1 read-only verification authenticated the archived original helper and all artifacts. E6 matched 49 rank/pass/round rows in 77.353 s, 3,741 final bad cells, 556,192 cells after the higher shift, 84,124 / 84,184 pass face sums, 209 V-collapses and 1,237 V-splits. Before-V sums 11,644 / 3,843. All A1 jobs are terminal. Checkpoint A1 as default path not yet proven, then apply A2.

FP2-B A1 pushed as 51647d8ecf. Host audit found 21 A1 identities terminal. Added A2 Step D guards and the A3 hand-over fixtures. Starting build631 for both the solver and test driver, then the A2 serial fixtures.

FP2-B build631 stopped at compile time. The A2 feature guard used EdgeUse.seed instead of its existing EdgeUse.cell field. The build handle is terminal. Corrected the seed wiring, preserving the same ring and guards. Starting build632.

FP2-B build632 compiled both binaries in 39.337 s with no new warnings. Solver SHA256 8098ac047727496492dc8888f12fe8f42315bff1ec76da19eb7643527bb2a942, test driver SHA256 54a7646e4d661a6024b34c38283912f22174019e496f143c037597b28eccb8e0. A2 serial fixtures passed (134 assertions in three cases). Checkpoint A2, then N1 and N6 on build632 before any full run.

FP2-B A2 pushed as 535d311882 with the default path pending. Host audit verified 27 process identities terminal before launching the pinned build632 N1 lab.

FP2-B build632 N1 passed in 294.076 s. Handover 3,474 bad cells (1,905 / 1,450 / 0 / 119 by class), zero metric edges below 0.01, minimum 0.023404202387000857, all 559,193 cells exactly positive, 82,968 owned faces and four strict entities accepted. Classify above tolerance and unexplained both zero. N1 terminal. Starting N6.

FP2-B build632 N6 terminal in 305.821 s. Lab FAILED the last-pass edge gate: first rank 2 had four rows below metric 0.01 vs zero, with eight rows total and seven distinct edges. Smallest 0.00075945199938384203. The 3,420 bad cells, exact positivity, geometry, 27,167,880-byte snapshot agreement, qualified proof (20 refused, one neighbor-locked, zero unexplained), and rank alarms passed. The one read-only R5 diagnosis traced all short edges to Step D. Six distinct edges survived pass 1 and one new edge came from pass 2. A1 51647d8ecf and A2 535d311882 retained, default path not yet proven. Remaining fixtures, G1, S1, the default proof and S2 unevaluated. Close with the lab stop evidence checkpoint and host audit.

FP2-B correction 1 order archived with SHA256 b735b39ea86da2b780d8108e79782466f9d8eb5f492b570b87c9b3b160455b45. R4 sealed at 457822008c before production edits and 44 prior process identities terminal. Next implement the Step D path and fixtures. S1 is conditional on the corrected hand-over gate.

FP2-B correction 1 Step D and test observers are ready. Changes.cpp and Pass.cpp match build632 pins. The production diff since 535d311882 is confined to Step D and its print fields in Core.hpp and Surface. Gate E remains at build630. Starting build633 for SU2_CFD and test_driver.

FP2-B correction 1 build633 compiled the solver and test driver in 46.863 s without new warnings. Running correction fixtures a–d (cylinder), A1, inherited FP1/FP2-A fixtures and G1 before the corrected lab.

FP2-B correction 1 R3. The first fixture command stopped before launching a workload because imported FP1 tools moved a same-named runner ahead of FP2-B on the Python path. Restored local runner precedence. Resuming fixtures on the same pinned build633.

FP2-B correction 1 scoped fixture cohorts passed on build633. The revised box fixtures matched the expected split order and the cylinder checked 27 accepted D splits with zero violations. A1 and the inherited FP1 and FP2-A cohorts passed. G1 serial is active, then MPI2 and MPI4.

FP2-B correction 1 fixtures and G1 passed on build633. A1 and all 18 FP1 cohorts, the requested FP2-A cohorts, and the new three box cases passed. Cylinder probe checked 27 accepted D splits with zero violations. G1 identity 375 / 114 / 228 in 761.056 / 10.138 / 9.797 s. All fixture jobs ended. Checkpoint and push, then run N1 on the same binary.

FP2-B correction 1 fixture checkpoint pushed as beb1374ff8. Host audit verified 75 prior identities terminal. Starting unwrapped N1 on build633, then the separate N1 invariant and unwrapped N6.

FP2-B correction 1 N1 passed in 292.431 s on build633. 548,127 exactly positive cells, 2,705 bad, 76,736 faces against 95,413, zero metric edges below 0.01, minimum 0.027364967690064603. Proof qualified by accounting for all 1,204 above-tolerance faces (100 refused, 1,104 neighbor-locked). Starting the independent wrapped N1 check on the same build.

FP2-B correction 1 N1 invariant passed in 288.250 s. Zero longest-edge violations and identical NATIVE3D_LOCAL_SURFACE integer fields and bad_out to unwrapped N1 on build633. Starting unwrapped N6.

FP2-B correction 1 hand-over PASSED on build633. N6 320.498 s, 2,512 bad cells, zero metric edges below 0.01, minimum 0.027364967690064603, face sums 76,612 / 76,772 against 95,305 / 95,422, exact cells and snapshots, no unexplained proof faces. N1 invariant passed separately. Save and push the lab checkpoint, then run S1 first on this same build633.

FP2-B correction 1 lab passed checkpoint pushed as ccd9d60593. Host audit found 98 prior identities terminal. S1 launched first on pinned build633 using the exact surface-enabled recipe SHA256 07b91a94996bf3cb48352041d8e1643ffe17a61bed07432085cfcf87b1725382. Native standing gates and isolated restart are pending.

FP2-B correction 1 S1 event 1 stage 0 completed both local passes on all ranks and installed the shared state with 76,940 faces, revision 2. Native engine repair is now running.

FP2-B correction 1 S1 event 1 entered native quality improvement at 05:44 CET. Core state installed on all six ranks with 76,940 faces and 13,993 surface splits. At the quality entry the recorded job peak was 12,300,984,320 B. S1 remains live.

FP2-B correction 1 S1 event 1 finished the native remesh on all six ranks with complete=1 and zero q/J/geometry debt. Native root maximum 1667.955 s, minimum named coverage 99.2429%. Transfer, physical validation and continued CFD still pending. Same-binary CORE timing comparison follows only after S1 passes.

FP2-B correction 1 S1 event 1 physical/transfer/continuation gate and timer gate passed. Event 1 published its mesh, geometry sidecar, and restart. Event 2 is in native construction.

FP2-B correction 1 S1 FAILED at event 2. First standing failure complete=0 on all six ranks versus 1 after an incomplete quality-refresh measurement. Rank 0 exited with shape_complete=0 and measured 107,960 of 108,211 cells. The timer supervisor stopped because event 2 had no quality-window or quality-exit rows. Named coverage still cleared 95%. All 109 observed process identities were terminal. The one read-only R5 diagnosis and event 1 comparison have been captured. Restoring FP2-B code and test files to the pre-item tree, rebuilding, and closing in one revert commit.

FP2-B correction 1 closure. Build634 rebuilt both binaries without a new warning. 1,624 build627 production SHA256 pins match, production and pre-FP2-B test diffs are empty, and both binary hashes match their baselines (solver 627, test driver 629). Terminal audit verified 111 identities. FP2-B ON FAILED. S2 and corrected default proof unevaluated. Commit and push the single revert, then verify terminal handles again.

FP2-B D1 archived the order and verified SHA256 78b34a13b5f7050ea8935b93ac023ba7fc6549c481088e38f64dfcf6b001d320. R4 sealed before any source edit. Prior host audit found 111 FP2-B identities terminal. Restore exactly the build633 code paths for the print-only diagnosis.

D1 restored the nine build633 paths exactly and added P1–P4 in the five scoped production files. Their output uses only local data under the existing report switches. Starting build635 for both binaries.

D1 build635 compiled both binaries in 69.213 s with no new warnings. The three corrected path cases, cylinder invariant (27 checked, zero violations), and three A1 cases passed once. Checkpoint the diagnosis-only code and start the single S1 on the unchanged controller.

D1 temporary checkpoint 873eaf5a9e pushed. The single S1 was launched from build635 with config SHA256 07b91a94996bf3cb48352041d8e1643ffe17a61bed07432085cfcf87b1725382 and controller source byte-identical to build633. Actual-host preflight found no active owned workload. Wait for the inherited controller.

D1 S1 event 1 captured all local checkpoints. The 12 LOCAL_SURFACE rank/pass integer rows already match build633. Rank 2 pass 1 first gains its near-zero edges by c1; the minimum physical edge 64572–64650 (3.9070055295542924e-10) also appears at engine seeding, with the same coordinates and metric to precision. Both point IDs are within the first-pass Step L allocation range. The first engine LENGTH minimum 5.4527443514081955e-10 matches historical event 1 on rank 2. Native improvement is active.

D1 event 1 completed and all three payloads match S1 of build633 byte-for-byte. Event 2 seeded the core-generated rank-0 edges 115118–115130 (1.6169413268309967e-09) and 115112–115124 (3.4973302189904171e-09). Await the first LENGTH row and captured P3 reason.

D1 build635 S1 ended under the inherited controller after 2096.810 s. Event 1 integer rows and three payloads matched build633. Event 2 rejected rank 0 cell 21740526 with original reason "Degenerate metric donor. at native 3D metric cell 21740526." Its cached metric edge minimum was 0.88835203236367702. The near-zero event-2 LENGTH minimum was already in seed from a Step L sheet split. All handles terminal. Restoring 240ca96db8 source and test trees and rebuilding both binaries.

D1 closed. Build636 rebuilt both binaries without new warnings and matched the build634 binary hashes. All 1,624 build627 production pins matched. Scoped diffs against 240ca96db8 and 852e566d17 were empty. Report completed, 19 owned D1 identities terminal. Push the single restoration commit and verify terminal handles again.

FP2-B correction 2 order archived and verified. Whole-item R4 check recorded against D1 `873eaf5a9e` before restoring production. Actual-host audit confirmed 111 FP2-B and 19 D1 identities terminal. Next restore the exact D1 tree, then the test-only production-partition lab.

FP2-B correction 2 item 1 pushed as `6415b6dab9`. Tracked Common, SU2_CFD and UnitTests matched D1 exactly; the two untracked backups were left alone. Item 2: test-only production partition support is in place. Test-driver-only build637 started with Common and SU2_CFD still exact D1.

FP2-B correction 2 build637 compiled only test_driver on D1 source in 68.326 s with zero new warnings. The N7P input pins cover 112,400 points, 552,946 cells, 76,126 faces, and the prepared event-2 sensor. N6P completed in 309.304 s with all 12 partition/face counts equal. It reproduced the required shifted minimum and IDs (rank 2, 64572–64650 at 3.9070055295542924e-10), 362 below 0.01 and 138 below 1e-6, and rank 3 zero below 0.01. The extra 0.1 bins and rank-3 above-floor minimum differed. N7P started on the same build637.

FP2-B correction 2 item 2 complete. N7P on build637 completed in 456.941 s. All before-partition counts and CFD cuts matched D1, but rank 0 first differed at native partition cells (95,994 vs 95,408), then pass-1 faces (14,216 vs 8,490). N7P is report-only. The v9 restart lacks the prepared geometry cache retained by D1; those piece counts feed the geometry partition weights. N6P reproduced all named fault values. Preserve both receipts and checkpoint the test-only code, then apply the production rules and corrected fixtures.

FP2-B correction 2 item 2 pushed as `7f6e262cfc`; 135 owned FP2-B process identities terminal. The N6P/N7P receipts, independent event-2 input pins and first partition mismatch are archived. Item 3 now has the Step L order, shared child-altitude rule, retained D1 diagnostics and donor text, fixtures e/f/g/g2, and the donor-scan lab code. Changes.cpp and Pass.cpp still match D1 and build633. Next build638 for both binaries.

FP2-B correction 2 build638 compiled both binaries in 329.406 s without a new warning. Solver SHA256 `165ff93c42ca453e6483bc0481af29db005f91b1789189b0ea35a2f5fbb5d58b`, test_driver `88e284396115fa7fd73b0a244f4bde2191750f2e776b67e02fee9802974e79d1`. Gate E builds on the existing build630 replay. Both volume source hashes are exact to build633. Starting prescribed fixtures and G1 on build638.

FP2-B correction 2 build638 focused fixtures passed. Existing A1 had 105 assertions in 3 cases. The box/cylinder cohort passed 797 assertions and the cylinder counted 27 accepted Step D splits with 0 violations. The corrected path cohort passed 351 assertions in 7 cases, including e, the optional f wait branch, g and g2. The remaining inherited cohorts and G1 are in sequence.

FP2-B correction 2 item 3 passed on build638. All 18 inherited FP1 cohorts and requested FP2-A cases passed. G1 identity was 375 / 114 / 228 in 752.544 / 9.769 / 10.812 s. Checkpoint the fixtures, then N1, the existing longest-edge observer on N1, N6 and H6off donor scan.

FP2-B correction 2 fixtures pushed as `4a0b746d57`. The actual-host audit found all 166 recorded FP2-B process identities terminal. Starting unwrapped N1 on the same build638.

FP2-B correction 2 N1 passed on build638 in 280.055 s. It had 546,565 exactly positive hand-over cells, 2,706 bad, 76,518 faces, zero edges below 0.01 and zero donor rejects. The strict proof had 1,204 above-tolerance faces fully assigned to causes. L accepted 2,307 splits in 9 sweeps with 653 longest-edge waits and no Near refusals. Starting the independent N1 invariant.

FP2-B correction 2 build638 N1Invariant passed with zero longest-edge violations and exact surface integer and bad-out agreement. Starting unwrapped six-rank N6 on the same binary.

FP2-B correction 2 N6 passed in 311.162 s on build638. It had 541,410 exactly positive cells, 2,511 bad, zero short edges below 0.01 and zero donor rejects. Face sums were 76,394 and 76,554. All snapshots agreed at 26,680,752 bytes and 1,203 above-tolerance faces had reasons with zero unexplained. Starting H6off for the report-only donor comparison.

FP2-B correction 2 item 4 passed. H6off also produced a complete zero-rejection donor scan, as a report. Item 5 starts with corrected N6P compared against the archived D1 baseline.

FP2-B correction 2 N6P valid and passed on build638 in 303.939 s. All six partition/face pairs were exact. The shifted rank-2 minimum moved from 3.9070055295542924e-10 to 0.038049534891454399, with zero metric edges below 0.01 on all ranks before and after the final shift. The donor scan rejected zero of 538,657 cells. Bad count 2,501, face sums 72,920 / 75,964 and qualified proof 1,365 above tolerance with zero unexplained. N7P corrected rerun started.

FP2-B correction 2 item 5 complete. N7P build638 ended in 452.839 s and remained report-only. Rank 0 metric weight 5,716,288 equals D1; geometry weight 380,291 vs 395,176 with an empty prepared-facet cache. N7P measured 0 donor rejects, 1,739 bad and 84,790 / 85,310 faces on that different partition. The earlier valid N6P gate and frozen hand-over gate passed. Commit and push lab evidence before S1 on the exact same build.

FP2-B correction 2 lab checkpoint pushed as `6b0fc2812f`. Actual-host audit found all 222 FP2-B identities terminal. Starting S1 first on that same build638, with the pinned production controller and both options YES.

FP2-B correction 2 S1 event 1 entered engine quality improvement on build638. All 12 core surface rows are present; the six shifted and six seed short-edge censuses have zero edges below 0.01. Rank 2 shifted minimum 0.038049534891454399 matches N6P. No P2 or P3 rows observed through entry. The original standing gates are pending.

FP2-B correction 2 S1 event 1 remained live through 2,697.433 s of engine work with q/J counts 0 / 0 and geometry count 405 (from 741 at 1,280.566 s). Recorded peak 13,995,974,656 B. Event 1 had yet to publish its payloads. The pinned controller continues under the standing limits.

FP2-B correction 2 S1 event 1 finished with q/J/geometry 0 / 0 / 0. Native root maximum 4,109.014 s, minimum coverage 99.7120%. P7/P8 and F3b event-1 receipts passed. The reported job peak equals the sum of process peaks at 13,995,974,656 B. The event-1 mesh and reference sidecar are published, and the run continues toward event 2.

FP2-B correction 2 S1 event 2 is active. All 12 core surface rows and six shifted/seed rows are present. Every rank reports zero edges below 0.01 after the shift and at seeding. Event-2 rank 0 shifted minimum is 0.020951597058329163 versus D1's 1.616941326830997e-09; lowest shifted minimum across ranks is 0.010819175121351721 on rank 5. No P2 or P3 row observed in these checkpoints. Event-1 restart published.

FP2-B correction 2 S1 PASSED on build638. Both full adaptation events and the isolated restart passed. Max native seconds 4,108.976 / 423.776, minimum named coverage 99.7120% / 98.8869%, final cells 537,985 / 566,001, proof faces 73,784 / 77,958, exact summed peak 13,995,974,656 B. No P2 near, P3 reject or enriched donor exception. Both events had no sub-0.01 edges at shifted hand-over and seed. Main run 4,845.646 s and isolated restart 158.658 s. Push S1 evidence, then run the default proof before S2.

FP2-B correction 2 S1 checkpoint pushed as `da8088eae2`. All 244 witnessed FP2-B identities terminal. Starting the default proof on the same build638, first the scoped G2/GQ/G3/G4 and shared-engine safeguards.

FP2-B correction 2 default proof G2 passed on build638 in 242.894 s with its inherited exact-row comparison. GQ is active on the same binary.

FP2-B correction 2 default GQ exact-row comparison passed on build638 in 661.731 s. G3 is running on the same binary.

FP2-B correction 2 default G3 and G4 passed on build638. Controllers were terminal in 616.080 and 36.416 s respectively. The shared safeguards are in sequence, beginning with merit481 (terminal in 120.591 s) and saved481.

FP2-B correction 3 arrived during build638 saved_current, the last check of the scoped default campaign. The child finished naturally in 144.086 s and the scoped receipt passed (G2, GQ, G3, G4, merit481, saved481 and saved_current). OFF, CORE and S2 were never launched. Actual-host audit found 286 FP2-B identities terminal and Common/SU2_CFD/UnitTests equal to da8088eae2. The correction 3 order is archived with verified SHA256 2b305d1c1fcd00aa5f4c59c1ca437068aee33a21095843fc344ac73d09e9adc4. Next whole-item source check and test-only pins before production edits.

FP2-B correction 3 boundary archive pushed as `4ba64e9f8b`. The scoped build638 G3 run passed with 1,567 accepted splits and an exact common prefix of 1,113. G4 executed 2D safeguards at 1/2/4 ranks. Whole-item C3 R4 reading is recorded before production edit. The build639 test-driver-only build uses the da8088eae2 production tree with the independent placement-pin fixture.

FP2-B correction 3 build639 test-driver-only baseline passed the hidden IN1 placement test. Test driver built in 59.578 s without new warnings and ran in 37.288 s with 11 passing assertions. It captured 3,321 Walk refusals out of 72,757 edges and three exact converged placements. R3: wrapper status printing raised a missing output-root key after sealing the passing receipt; verified the receipt without repeating the fixture. Next checkpoint the pins transcript and then implement the scoped production correction and fixtures.

FP2-B correction 3 pre-edit baseline pushed as `ef9b508f6a`; actual-host audit found 290 prior identities terminal. Production now applies the projector's outward-rounded tolerance bound for unconverged sheet walks only. Captured pin set SHA256 cb872f38f3efdb8cdf4de216b24a0b042cea7616956c135f8111119b9c595348. Candidate sources frozen for build640, both binaries.

FP2-B correction 3 build640 compiled both binaries in 47.174 s with zero new warnings. Solver ce93af9ef95c3b5aa5295a850bb4f74f92facdb00e93e5ef8fa27cee57b48b07, test driver 9319873b17e14ec9956d666798ee76e110e24ae7909b67ea424f886dff7879a4. Gate E sources match build633. The corrected IN1 fixture passed 50 assertions and counted 69,436 ordinary and 3,321 rescued placements, with no Far, NoStart or Exception outcomes. The first three rescued placements passed all checks and all three ordinary bindings/coordinates matched their pins bit for bit. Requested inherited fixtures passed and G1 serial is running.

FP2-B correction 3 item 2 passed on build640. Required C1/C2, A1, FP1 and FP2-A fixture cohorts passed. G1 matched 375 / 114 / 228 records in 754.356 / 9.849 / 10.555 s. Push the fixtures checkpoint, then N1, N1Invariant with j, N6, and N6P.

FP2-B correction 3 fixtures checkpoint pushed as `cc21031eff`. Host audit found all 322 recorded owned identities terminal before starting N1 on the same build640.

FP2-B correction 3 N1 passed on build640 in 312.863 s. Exactly positive hand-over 565,931 cells, 87,814 faces, 3,846 bad (report), zero edges below 0.01 and donor rejects. Proof accepted four entities, with zero above tolerance versus build638's 1,204. Walk counters 0 refused, 253 tolerance, 0 Far/NoStart/Exception. Starting N1Invariant with the tolerance outcome reconciliation.

FP2-B correction 3 N1Invariant passed at 311.701 s with zero longest-edge violations and exact integer/bad-out agreement. The 253 tolerance placements reconciled as a=0 direct refusals, b=253 accepted, c=0 later split refusals. N6 is active on build640.

FP2-B correction 3 N6 passed on build640 in 337.949 s. Final cells 560,663, bad 3,693 (report), face sums 87,570 / 87,802, snapshots agreed at 27,535,600 bytes. Walk counts 255 tolerance and zero refused; donor rejects 0. Four faces above tolerance versus 1,203, two refused and two border, zero unexplained. Starting N6P and its shifted spatial-longest-edge border report.

FP2-B correction 3 N6P valid and passed on build640 in 293.813 s. Every partition cell/face pair matched. Final positive cells 556,553, bad 3,604 (report), zero short edges below 0.01 and donor rejects. Face sums 80,972 / 86,252. Walk counts 262 tolerance and 0 refused. Proof above tolerance 319 vs 1,365, all explained; shifted longest-edge rings 2 border / 317 nonborder. All item 3 gates passed. Push the lab checkpoint and start one S1 on this binary.

FP2-B correction 3 lab checkpoint pushed as `da2758a1e8`. All 356 witnessed owned identities were terminal. Launching the single authorized S1 on the exact build640 that passed the corrected lab.

FP2-B correction 3 S1 event 1 reached quality improvement on build640. The 12 surface rows recorded 262 tolerance placements and no Walk/Far/NoStart/Exception outcomes; every shifted and seed rank row had zero edges below 0.01. No P2 or P3 rows through entry. At 293.750 s the completed quality window had q/J 1,241 / 1,261 and 154 geometry failures.

FP2-B correction 3 S1 event 1 finished with q/J/geometry 0 / 0 / 0; event-1 resource/proof and timer receipts passed. Maximum remesh timer about 650 s versus build638's 4,109 s, proof faces 80,650, summed peak 12,618,448,896 B. Rank 0 first geometry repair ran from engine elapsed 414.454 to 438.765 s; final engine exit 503.283 s. Event 2 completed both local surface passes and remains active.

FP2-B correction 3 S1 PASSED on build640. Both complete no-BL adaptation events and the isolated restart passed. Native maxima 649.954 / 446.547 s, minimum named coverage 98.2433% / 98.9311%, final cells 550,027 / 575,733, proof faces 80,650 / 79,274. Peaks 12,618,448,896 / 13,027,512,320 B equaled per-rank sums. No shifted or seed edges below 0.01, no near-placement or metric-rejection rows, 262 / 24 tolerance placements and no Walk refusals. First geometry repair dropped from 2,538.204 s to 24.311 s. Main run 1,411.953 s, restart wrapper 160.970 s. S1 evidence and handoff close this order with build640 retained; default proof and S2 are pending.

FP2-B continuation 1 archived with verified SHA256 50c3c7a7fc8a62959a424473deadd36b13ada8ca92f27b4a7398c25d87bc8bb9. Common/SU2_CFD/UnitTests match ecaed957db; both build640 binary SHA256 values, the CORE config, the fixtures/G1 and the four hand-over receipts were verified. Actual-host audit found 378 FP2-B and 19 D1 identities terminal. Reusing S1 and the passing fixtures; starting the required build640 scoped default proof before OFF and CORE.

FP2-B continuation 1 R3. The generic scoped entry stopped at the sealed fixture-provenance comparison before launching any check. Build640 includes the correction 3 sheet and order pins, so the scoped entry now supplies those exact existing pins through the same controller. No source, fixture, or gate values changed.

FP2-B continuation 1 R3. A temporary launcher located a similarly named draft in /tmp. The read-only provenance comparison matched after resolving the committed wrappers from the evidence directory. Retrying the launcher with Python's safe path option, preserving the sealed fixture provenance. Both earlier attempts ended in preflight, with no gate run.

FP2-B continuation 1 G2 passed on build640 in 237.768 s. The exact-row comparator accepted the inherited reference. GQ is active on the same binary.

FP2-B continuation 1 GQ passed on build640 in 656.293 s with the inherited scheduling and exact-row checks. G3 is active.

FP2-B continuation 1 G3 passed its physical and identity checks on build640. The controller ended in 613.799 s. G4 is active, followed by the saved shared-engine checks.

FP2-B continuation 1 scoped default proof passed on build640. G4 executed the 1/2/4-rank 2D cases in a 36.251 s controller, and merit481, saved481 and saved_current passed in 121.473, 144.338 and 143.402 s. Starting G5 OFF on the same build, with both options NO and the N21 W six-file gate.

FP2-B continuation 1 OFF event 1 reached native exit with q/J/geometry 0/0/0. Its proof/resource and F3b timer receipts are present. Event 2 continues under the same supervised controller.

FP2-B continuation 1 OFF passed both adaptation events, the isolated restart, and all six N21 W on-disk SHA256 comparisons. Main controller 3,169.556 s and isolated restart wrapper 238.061 s. Starting CORE on the same build640 with the pinned FP1 run.cfg and six-file reference.

FP2-B continuation 1 CORE event 1 reached native exit with q/J/geometry 0/0/0. Its proof/resource and timer receipts passed. Event 2 is proceeding under the same controller.

FP2-B continuation 1 CORE passed both adaptation events, the isolated restart, and all six FP1 on-disk SHA256 comparisons. Main controller 2,171.547 s and isolated restart wrapper 148.863 s. Native maxima 1,509.433 / 371.336 s. The same-build event-1 time limit for S1 and S2 is 1,584.90509065035 s. The full default proof passed. Commit and push the required default-proof checkpoint before S2.
