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

FP2-B continuation 1 default proof pushed as `6a37f55fe7`. The host audit found 464 FP2-B identities terminal. Starting the single S2 on build640, using the S1 config, six-file identity gate and same-binary CORE time comparison.

FP2-B continuation 1 S2 event 1 reached native exit with q/J/geometry 0/0/0 and passing proof/resource/timer receipts. The on-disk event-1 mesh and sidecar already match S1. Event 2 continues within the same controller.

FP2-B continuation 1 S2 PASSED on build640. Main run 1,411.584 s and restart wrapper 167.837 s. The independently rehashed six files from each of S1 and S2 agree with the published ON reference. Native event maxima S1 649.954 / 446.547 s, S2 649.993 / 444.120 s, CORE 1,509.433 / 371.336 s. Both S1 and S2 passed the 1,584.905 s event-1 timing rule. FP2-B accepted; default proof was pushed at 6a37f55fe7. Close with the report, timings, handoff and terminal verification. No source or binary changes.

FX2 order archived and SHA256 verified. Whole-item source check recorded in FX2/source_check.md against c843610249. No production authority assignment or live-object replacement found. Prior FP2-B and D1 jobs verified terminal on the host (486 and 19 identities). Preparing F0 fixtures on unchanged production before the first production edit.

FX2 F0 R3. Builds 641 and 642 kept production unchanged. The F1 signed-zero test first used std::signbit under the test target's fast-math options. Comparing the raw bits then exposed that this test translation unit had folded the argument to +0.0. The fixture now uses the precise-arithmetic options already used for FP2's exact tests and checks the raw bits. All other F0 cases reached their expected outcomes in both attempts. The failed fixture logs are preserved.

FX2 F0 R3 follow-up. Build643 still folded the constant negative-zero argument in an interprocedural FX2Policy clone. Disassembly showed the same argument-less specialized wrapper used at both call sites. The fixture reads the same pinned bits through volatile integer storage at runtime and verifies that the policy retained them. Build644 F1–F4 passed 121 assertions across 4 cases. F5 is running on the unchanged production source.

FX2 F0 passed on build644 with c843610249 production source and the unchanged build640 solver SHA. F1–F4 passed 121 assertions. The B1 baseline constructor took 25.968 s, with work 19,049,655, modeled peak 3,992,061,856 bytes and snapshot equality. Both fixture jobs ended. The ordered production edit is applied for build645; F1–F5, the inherited fixture cohort and G1 are next.

FX2 build645 compiled both targets without new warnings. F1–F5 passed exact budget, exception, digest and timer-call comparison against F0. B1 constructor 25.968 → 10.285 s (15.683 s saved), unchanged work and modeled peak. The M6 memo is estimated at 1,583,080 bytes for one live entry. The inherited fixture cohort and G1 are now running on the same candidate.

FX2 §4.1 passed on build645. All 18 FP1 cohorts, the build640 fixture cohort and the separate sheet fixture passed. G1 matched 375 / 114 / 228 case records in 744.981 / 10.089 / 9.820 s. Commit and push the fixture checkpoint. Next G2/GQ, G3/G4 and executed safeguards.

FX2 §4.1 checkpoint pushed as `f5660817e6`. Host audit found 52 FX2 process identities terminal. Build645 is pinned for §4.2.

FX2 G2 passed on build645 in 242.847 s with the exact rows and scheduling check. GQ is running on the pinned binary.

FX2 GQ passed on build645 in 666.349 s with the exact rows and scheduling check. G3 is running on the same binary.

FX2 G3 passed on build645 in 601.474 s. The fixture accepted 1,543 splits with 1,113 common edits exact and observer-adjusted entry work 5,113 on both runs. G4 and the saved safeguards follow.

FX2 G4 passed its executed native 2D BL/no-BL checks at 1/2/4 ranks in 36.303 s. merit481 passed in 120.642 s. The saved481 and current saved-state checks follow.

FX2 §4.2 passed on build645. G2/GQ exact rows and scheduling, G3 physical/common prefix, G4 native 2D at 1/2/4 ranks, and merit481/saved481/saved_current all passed. The three saved safeguards took 120.642 / 114.147 / 113.228 s. Checkpoint before running N1 and N6P.

FX2 §4.2 checkpoint pushed as `7b2eca2059`. The host audit found 110 FX2 process identities terminal before the labs. N1 then passed on build645 with no exact field differences. Surface-state build 29.858 → 14.439 s. N6P is running on the same binary.

FX2 §4.3 passed on build645. N1 and N6P matched all required fields, with build times reported separately. N1 took 302.635 s under supervision and N6P took 279.543 s. Push the lab checkpoint, then run G5 OFF against N21 W and build640 OFF.

FX2 lab checkpoint pushed as `d5fba11a48`. OFF preflight stopped before launching a job because the saved Python comparator receipt encodes rank keys as JSON strings, while the in-memory self-check uses integers. R3 corrected the representation check to compare the same JSON form. The comparator and all gates are unchanged.

FX2 §4.4 started G5 OFF on the pinned build645 solver after a terminal audit of 127 FX2 identities. Its run.cfg matches build640 OFF (both options NO), with the N21 W six-file gate and the self-checked full-stream comparator.

FX2 G5 OFF event 1 reached remesh final and passed its proof/resource and timer receipts. Owned proof faces summed to 61,906, and the job peak equals the process-peak sum at 14,043,447,296 bytes. Transfer mobility replay was below 8 µs on all six ranks. The full controller is still active.

FX2 G5 OFF event 1's three published files were independently hashed and matched N21 W. Event 2 is active in native construction.

FX2 G5 OFF event 2 reached remesh final and passed its proof/resource and timer receipts. Owned proof faces summed to 58,260, and job peak equals the process-peak sum at 16,787,030,016 bytes. Transfer mobility replay took below 3 µs on all ranks. Output, final workflow validation and the isolated restart follow in the same supervised sequence.

FX2 §4.4 G5 OFF passed on build645. Both events, continued CFD, and the isolated restart passed. All six SHA256 values match N21 W. The full comparison has zero exact differences in 19 snapshots and 297 storage rows per rank. Every transfer mobility replay was below 8 µs. Main controller 3,149.267 s, isolated restart 246.790 s. Next push OFF proof and run S.

FX2 OFF checkpoint pushed as `8388ba721b`. The host audit found 146 FX2 identities terminal before S. FX2 §4.5 is running on build645 with both options YES and the pinned build640 S1 config, SHA256 07b91a94996bf3cb48352041d8e1643ffe17a61bed07432085cfcf87b1725382.

FX2 G5 S event 1 reached remesh final and passed its proof/resource and timer receipts. Owned proof faces summed to 80,650. The job peak equals the process-peak sum at 12,603,252,736 bytes. Construction and transfer mobility replay each took under 3 µs on every rank. The rank-0 surface-state clock was 15.599 s versus 32.702 s on build640. The controller continues to event 2.

FX2 G5 S event 1's three published files matched the accepted FP2-B S reference. Event 2 passed its proof/resource and timer receipts with 79,274 owned proof faces and a job/process-peak sum of 13,036,724,224 bytes. Output and the isolated restart are still in the supervised sequence.

FX2 §4.5 passed on build645. S matched its six accepted FP2-B hashes and had zero exact differences in all 16 snapshots, 69 storage rows and two surface-state rows per rank. Both events, continued CFD and isolated restart passed. Construction / transfer replay maxima were 8.633 / 2.728 µs. Main controller 1,341.171 s and isolated restart 170.981 s.

FX2 §4.6–8 accepted on build645. Native times OFF 1,735.398 / 1,016.311 s and S 614.463 / 410.522 s. The minimum named coverage was 98.158%. Reports, handoff and timing handbook updated. Commit and push FX2: accepted, verify all owned jobs terminal, then wait for the user.

FP2-C1 S0 is `7e6953e721` (`FX2: accepted`), build645. The new order and SHA256 are verified and archived. The actual-host audit found all 163 FX2 identities terminal. Preparing the test-only rows for the S0 N1/N6/N6P reference before any production edit. FP2-C2 step 0 is queued after C1 closure.

FP2-C1 R4 source check recorded before the production edit. Baseline build646 compiled only the test driver in 25.833 s, with the accepted solver SHA unchanged and no new warning. S0 N1 passed in 298.522 s with 565,931 cells, 87,814 faces and zero donor/proof unexplained issues. N6 and N6P follow on the same baseline binary.

FP2-C1 S0 N6 passed in 319.515 s with 560,663 owned cells, 87,802 final surface faces, zero donor rejections and zero unexplained classifications. N6P is active on build646.

FP2-C1 §5.1 passed. S0 N6P was valid and passed in 276.280 s with 556,553 cells, 86,252 final surface faces, no donor rejection or unexplained classification, and exact production partition pins. All three references are sealed on build646. Checkpoint the test-only baseline, then apply the reviewed production and fixture edits.

FP2-C1 S0 reference pushed as `671858086e`. The audit found all 30 C1 identities terminal before the edit. The ordered length-collapse option, print counters and five hidden fixtures are applied. Build647 and the §5.2 fixture gates are next.

FP2-C1 build647 compiled both targets in 46.662 s with no new warnings. All five wall fixtures passed (1,021 assertions). The narrow first-layer x pair was accepted. The S0 regression fixtures and G1 follow.

FP2-C1 §5.2 passed on build647. The wall, FX2, inherited and all 18 FP1 fixture cohorts passed. G1 matched 375 / 114 / 228 records in 744.323 / 9.914 / 11.621 s. Checkpoint before the two trials each of N1, N6 and N6P.

FP2-C1 fixture checkpoint pushed as `8e3f1ce081`. A host audit found 65 C1 identities terminal before the lab campaign. The pinned build647 solver and test driver are being used for two separate trials of each hand-over scenario, one supervised job at a time.

FP2-C1 N1 trial 1 passed in 332.144 s. It handed over 475,815 cells versus 565,931 on S0, with the same 87,814 surface faces, zero donor rejections and minimum edge 0.02488. The bad-cell count after V was 4,481 versus 3,846. Trial 2 is active.

FP2-C1 N1 trial 2 passed in 331.178 s. The N1 comparator matched both trials and the S0 pass-1 surface rows. Both trial digests are 15904394763081022199. N1 made 16,846 wall-near length collapses. N6 trial 1 is running.

FP2-C1 N6 trial 1 passed in 326.657 s with 470,940 owned cells, 87,796 final surface faces, zero donor rejections, and minimum edge 0.02668. Bad cells after V were 4,155 versus 3,693 on S0. Trial 2 is active.

FP2-C1 N6 trial 2 passed in 326.576 s. The two trials and S0 pass-1 surface rows matched. N6 made 17,413 wall-near length collapses across both passes. N6P trial 1 is active.

FP2-C1 N6P trial 1 passed in 286.470 s with valid production partition pins, 468,571 owned cells, 86,210 final surface faces, zero donor rejections, and minimum edge 0.02040. Bad cells after V were 4,072 versus 3,604 on S0. Trial 2 is active.

FP2-C1 §5.3 passed on build647. N6P trial 2 passed in 285.423 s; the comparator matched the required integer rows and digests for N1/N6/N6P and the pass-1 surface rows against S0. Counts of wall-near collapses per trial were 16,846 / 17,413 / 16,974. Next push the lab checkpoint, then start the single G5 S.

FP2-C1 lab checkpoint pushed as `a3457575aa`. The actual-host audit found all 121 C1 identities terminal. The single G5 S is running on build647 with the pinned surface-on configuration and changed-mesh standing gates.

FP2-C1 G5 S event 1 reached native exit with q/J/size/geometry bad counts zero. The proof and timer receipts passed. Owned proof faces summed to 79,996 and the job peak was 11,388,039,168 bytes, equal to the process-peak sum. Its three payloads were published. Event 2 follows under the same supervisor.

FP2-C1 G5 S event 2 reached native exit and passed its proof and timer receipts. Owned proof faces summed to 77,370. Job peak 11,620,970,496 bytes equals the process-peak sum. The supervisor continues through workflow validation and the isolated restart.

FP2-C1 §5.4–5 passed on build647. The single G5 S ran both events and the isolated restart under unchanged limits. Native times 606.213 / 387.779 s, final cells 467,139 / 464,565, proof faces 79,996 / 77,370, minimum coverage 98.092%. All six candidate file hashes independently verified. Close S with default path pending, verify terminal jobs, then proceed to the queued C2 step 0.

FP2-C1 closed and pushed as `4730bd59ec`. The post-push host audit found 139 C1 identities terminal. FP2-C2 step 0 order is verified and archived. Its passing C1 lab branch uses wall_collapses ON, with build647 N1 and N6P trial 1 as the sanity references. The measurement is test code only and defers T12 until after the existing proof and cell digest.

FP2-C2 step 0 build648 compiled only the test-driver target in 27.691 s. The complete Common/SU2_CFD diff against C1 is empty, and the solver SHA is still b824f37ee37f2458d334b3534569078437f0a321a4b4166e52288ee111e46b4c. N1 is running under the 3,600 s wrapper.

FP2-C2 step 0 N1 completed the dry in-pass measurements and retained the C1 hand-over count of 475,815 with zero nonpositive cells. The normal proof precedes the deferred T12 queries and final sanity comparison.

FP2-C2 step 0 N1 passed in 2,134.237 s with zero sanity differences against C1 build647, including proof work/peak and the owned-cell digest. It measured 118,885 short physical edges and 237,770 directions. T12 reached the ordered cap after 79,808 of 90,196 scheduled directions. Starting N6P on the same test binary.

FP2-C2 step 0 N6P printed the original proof and all six owned-cell digests. The proof work (44,591,524), modeled peak (1,119,171,112 bytes), unexplained count and all six digests match C1. Deferred strict queries are active.

FP2-C2 step 0 complete on test-driver build648. N6P ended in 991.403 s with zero sanity differences and all 167,814 scheduled T12 directions evaluated across both passes. N1 / N6P pass 2 whole-chain counts were 66,380 / 71,085 and one-sweep estimates were 9,578 / 9,766 points. Omitting T8 gave 66,407 / 71,113, and substituting T8m gave 66,397 / 71,104. N1 stopped T12 at the 25-minute cap after 88.5% of scheduled directions. The required report and handoff updates are ready. Production sources and the solver match C1; only the two dry labs ran. Commit, push, audit and wait.

RCFD1 §2.1–2 prepared. Test-only restoration committed as `f2781622a5`. Build649 compiled only test_driver in 25.604 s. Both active binary hashes match build647. All 1,829 live source pins matched; the initial byte-hash difference came from set order in the manifest writer. Both serializations and the original build receipt are preserved; the verified build647 ordering restores the required manifest hash. Both base configs matched their pins and received the four exact replacements and the history line. Next verify the runner and start R-S.

RCFD1 preparation pushed as `051c62f4f0`. R-S launched on build647 under its actual-host supervisor. Preflight found 49,703,236 kB available and no competing owned heavy process. Seed CFD wrote cycle 0 restart and ParaView output, and event 1 entered native adaptation. R-OFF follows after R-S and its isolated restart end.

RCFD1 R-S event 1 passed F3b and P7/P8. Maximum remesh timer root 732.174 s, minimum coverage 97.943%, job peak and process-peak sum both 11,406,061,568 bytes, and 79,996 owned proof faces. Cycle 1 is executing on the published event 1 mesh.

RCFD1 R-S event 1 mesh and native reference sidecar are byte-identical to build647 C1 S. Cycle 1 completed all 500 history rows, ending at rms[Rho] −3.95795. Event 2 metric and native adaptation follow.

RCFD1 R-S event 2 passed F3b and P7/P8 on the evolved cycle 1 flow. Maximum remesh timer root 488.832 s, minimum coverage 98.488%, job peak and process-peak sum both 11,711,254,528 bytes, and 78,222 owned proof faces. Cycle 2 CFD is executing.

RCFD1 R-S full workflow exited successfully in 1,884.516 s. Cycle counts were 1/500/500. Cycle 2 ended at rms[Rho] −5.590929. The solver wrote all three cycle ParaView files. The isolated disk restart is active.

RCFD1 R-S passed both remesh events, both 500-iteration continued CFD cycles and isolated disk restart on build647. The full and isolated supervisors are terminal. The first event mesh and sidecar matched C1 S. Checkpoint the R-S evidence and proceed to R-OFF.

RCFD1 R-S passed and pushed as `75f93240e0`. Native maxima were 732.093 / 488.784 s. The isolated restart passed in 136.003 s and wrote flow.vtu. After all 18 R-S identities were terminal, R-OFF launched with 47,428,812 kB available and no competing owned heavy process.

RCFD1 R-OFF event 1 first-collapse gate passed. All six rows, the P6c counts, zero upper refinements and the exact work sum of 90,809,869 matched. The limit remains 108,459,426. The full event is still running.

RCFD1 R-OFF event 1 passed F3b and P7/P8. Minimum coverage 98.970%, job peak and process-peak sum both 14,009,892,864 bytes, and 61,906 owned proof faces. The workflow is publishing the event 1 mesh for cycle 1.

RCFD1 R-OFF event 1 mesh and native reference sidecar are byte-identical to the N21 W reference. Cycle 1 is executing on the published mesh.

RCFD1 R-OFF cycle 1 completed all 500 history rows, ending at rms[Rho] −3.98157. Event 2 metric and native adaptation follow. The event 1 identity checks passed for both modes.

RCFD1 R-OFF event 2 passed F3b and P7/P8. Minimum coverage 99.331%, job peak and process-peak sum both 17,392,070,656 bytes, and 56,060 owned proof faces. Cycle 2 CFD is executing.

RCFD1 R-OFF cycle counts were 1/500/500. Cycle 2 ended at rms[Rho] −4.85661. Final output, workflow sealing and isolated restart follow.

RCFD1 R-OFF full workflow passed and was terminal in 4,323.849 s. The isolated restart preflight returned 1 before writing a launch receipt because PID 51464 (SU2_CFD, start ticks 92229855) was active. It was outside the sealed full run and the identity had ended when checked. Preserve the preflight failure and launch only the pending isolated restart under the same preflight and validators.

RCFD1 passed both build647 full workflows, all four 500-iteration continued CFD cycles and both isolated restarts. R-OFF native maxima 1,961.595 / 1,372.908 s. The isolated restart ran once in 215.844 s after the earlier host preflight returned before launch. All eight ParaView files are present. The terminal audit found 37 RCFD1 identities terminal. Commit and push the evidence, then read FP2-C1 continuation 1. RCFD1 accepted no mesh.

RCFD1 closed and pushed as `635c7ee619`. Post-push audits found 37 RCFD1, 141 C1 and 19 C2 step 0 recorded identities terminal. FP2-C1 continuation order is verified and archived. Its item 1 was fulfilled by the RCFD1 preparation; verify the three hashes, reuse build647 fixtures/G1/lab, and execute §1.2(a) default regressions.

FP2-C1 continuation verified build647/649 fingerprints and live pins. G2 on build647 passed its exact rows, modeled storage, 4,120 fixed storage, decisions and scheduling in 275.060 s. GQ is active.

FP2-C1 continuation GQ passed exact decision, modeled storage, timer and scheduling comparisons in 769.670 s on build647. The already-launched §1.2(a) supervisor advanced to G3. The RCFD2 order is verified and archived. Its runs follow the terminal scoped supervisor and its pushed evidence checkpoint, before C1 §1.2(b).

FP2-C1 exact G2 and GQ prefix evidence pushed as `7544a2659f`. G3 passed in 639.650 s with 1,148 accepted edits and the first 1,113 matching N14. G4 completed with exit 0 in 39.167 s. The scoped supervisor is advancing through saved-state safeguards before the RCFD2 boundary.

FP2-C1 continuation §1.2(a) passed on build647. G2 and GQ exact, G3 1,148 splits with 1,113 exact common edits, G4 passed, and merit481 / saved481 / saved_current passed in 130.296 / 123.339 / 122.834 s. The scoped supervisor is terminal. Checkpoint this evidence, then execute RCFD2. Resume C1 at §1.2(b) OFF after RCFD2.

FP2-C1 §1.2(a) evidence pushed as `142f3e8751` after 199 process identities were confirmed terminal. RCFD2 configs and copied runner passed source-only checks on the existing build647 binary. R-SM and R-SG differ by exactly the ordered gradation line. Execute R-SM, then R-SG. C1 resumes at full OFF.

RCFD2 R-SM launched after the scoped proof checkpoint using build647. The run preflight found 44,913,376 kB available and no competing owned heavy process. R-SG follows after R-SM ends.

RCFD2 inputs and runner pushed as `7776367be0`. R-SM event 1 F3b and P7/P8 passed. Minimum timer coverage 98.176%, job peak and process-peak sum both 7,950,757,888 bytes, and 77,130 owned proof faces. The effective minimum size and norm printed as 0.003 and 2. Cycle 1 follows.

RCFD2 R-SM event 1 published 91,345 points and 429,478 cells. Both event-1 mesh hashes differ from R-S. Cycle 1 completed 500 rows and the event-2 metric still reports the ordered minimum size and norm.

RCFD2 R-SM event 2 F3b and P7/P8 passed. Minimum timer coverage 98.767%, identical reported job peak and process-peak sum of 9,586,515,968 bytes, and 72,946 owned proof faces. Cycle 2 is executing.

RCFD2 R-SM cycle counts 1/500/500. Cycle 2 ended at rms[Rho] −7.06138. All remesh gates passed; final output sealing and isolated restart follow.

RCFD2 R-SM passed both remeshes, 500/500 post-remesh iterations and isolated restart. Native maxima 648.383 / 363.454 s. Last cycle ends on 88,424 points and 423,547 cells. After 54 recorded RCFD identities were terminal, R-SG launched with 46,070,344 kB available and no competing owned heavy process.

RCFD2 R-SM passed and pushed as `15ea00574c`. R-SG event 1 F3b and P7/P8 passed with 98.553% minimum coverage, job peak and process-peak sum of 8,429,637,632 bytes, and 78,274 owned proof faces. Effective gradation 1.6 was printed by the native requirements row. Cycle 1 follows.

RCFD2 R-SG event 1 published 97,480 points and 465,829 cells. Its mesh and native reference sidecar both differ from R-S and R-SM. Cycle 1 completed 500 rows, ending at rms[Rho] −5.08759. The second metric reported density 3.7037e+07 and a fixed point after 42 gradation sweeps.

RCFD2 completed. R-SM passed 500/500 continued CFD and isolated restart. R-SG FAILED event 2 at complete=0 vs 1 following 500 iterations in cycle 1; rank 1 exit q_bad=2 and j_bad=1. Cycle 2 and isolated restart did not run. Event-1 mesh and sidecar hashes differ in all three comparisons. The read-only diagnosis is complete. Push both results; resume C1 §1.2(b) OFF.

RCFD2 closed and pushed as `bcaf43dddc`. Post-push host audit found all 65 RCFD identities terminal. C1 continuation resumed at §1.2(b), verified 199 prior identities terminal, and launched the default OFF proof under build647.

FP2-C1 continuation OFF preflight found 45,801,776 kB available and no competing owned heavy process. Event 1 first collapse passed with the six ranks' exact P6c work of 90,809,869, matching counts and zero upper refinements.

FP2-C1 continuation OFF event 1 F3b and P7/P8 passed. Job peak and process-peak sum both 14,043,414,528 bytes, owned proof faces 61,906, minimum timer coverage 99.004%. The workflow is publishing the first mesh and proceeding to event 2.

FP2-C1 continuation OFF event 1 wrote all three payloads with the expected N21 W hashes. Event 2 is in quality repair under the same build647 run.

FP2-C1 continuation OFF event 2 F3b and P7/P8 passed with 58,260 certified faces, 99.483% minimum timer coverage, and both reported memory values 16,794,025,984 bytes. Continued CFD, isolated restart and OFF identity checks follow.

FP2-C1 continuation OFF passed all standing gates, two accepted remeshes and isolated restart. Both events match all six N21 W payload hashes. FX2 build645 OFF comparison found zero exact differences. Native maxima 1,801.488 / 1,070.726 s. CORE follows the OFF checkpoint.

FP2-C1 OFF evidence pushed as `78e6aca10b`. The actual-host boundary audit found 217 C1 identities terminal. CORE launched on build647 after verifying the OFF receipt and the FP1 reference.

FP2-C1 continuation CORE preflight saw 45,471,372 kB available. Its event 1 first-collapse report repeated build640 CORE's 92,045,474 work and zero upper refinements. Both CORE logs report the expected difference from the OFF count prediction, which is a report row in CORE. The full run remains active.

FP2-C1 continuation CORE event 1 F3b and P7/P8 passed with 68,006 certified faces and 99.076% minimum coverage. Job peak and process-peak sum were both 11,586,916,352 bytes. The workflow is publishing the first CORE mesh before event 2.

FP2-C1 CORE event 1 mesh, sidecar and restart SHA256 match the FP1 reference. Event 2 is active.

FP2-C1 continuation CORE event 2 F3b and P7/P8 passed. Both memory values equal 11,663,958,016 bytes, proof faces 64,758, minimum coverage 98.127%. Continued CFD, isolated restart and six-file identity follow.

FP2-C1 continuation CORE failed the isolated restart. The first failing value was exit 124 vs 0 from the inherited 600 s watchdog; outer wait 617.692 s. The restart loaded and recorded iteration 0 before timeout during final metric computation. Both full CORE events passed and all six on-disk payloads match FP1. Read-only diagnosis complete. Commit the failure evidence, restore part 1 production and test code to FX2 in one commit, rebuild both targets, and close. S2 and C2 are blocked.

FP2-C1 default-proof failure evidence pushed as `935fa3ff9a`. The source rollback is committed as `6825979092`. The Common, SU2_CFD and UnitTests diff against FX2 `7e6953e721` is empty. All 1,829 FX2 source pins matched before the two-target rebuild. Build preflight saw 41,449,500 kB available and no competing owned heavy process.

FP2-D is queued behind the specific FP2-C2 S-passed label. The C1 continuation failure leaves the C2 and D prerequisites unmet; their production orders have not been opened.

FP2-C1 reverted; production sources = build645. The two-target rebuild finished in 433.180 s. Every one of the 1,829 FX2 source pins holds on disk, and both rebuilt binaries match their exact build645 hashes. All C1, RCFD and C2 step 0 owned identities are terminal. The C1 continuation closed at the failed default proof. S2, FP2-C2 production and FP2-D did not start.

FP2-C1 continuation 2 order verified and archived. The previous rollback rebuild had already finished with both build645 hashes matching and all owned handles terminal. The six CORE files and passed pre-watchdog checks remain evidence. The new order supersedes the watchdog failure label. Exact C1 production and tests restored in `c62fe7f22e`. Both targets are rebuilding as build650 under `FP2_C1/restored647`. The new helper-watchdog rule applies to the remainder of the queue. Build preflight recorded load 15.36 and 38,346,628 kB available.

FP2-C1 continuation 2 §1.1 passed. Build650 finished in 71.828 s with both exact build647 binary hashes and all 1,829 build647 source pins. The post-build convenience assertion initially iterated the verification dictionary's keys. The saved per-binary equality values both verify independently. Next launch only the CORE isolated restart with the 1,800 s helper watchdog.

FP2-C1 continuation 2 restoration and order pushed as `cab8d520cb`. The terminal 600 s CORE watchdog attempt and its receipts moved to `CORE/restart_attempt1_watchdog` under a 50-file SHA256 manifest. CORE full-run and payload pins verified. The authorized isolated restart retry launched with a 1,800 s watchdog, start load 9.46 and no competing owned workload at preflight. The full CORE run was reused.

FP2-C1 continuation 2 §1.2 passed. CORE isolated restart completed in 162.735 s at start load 9.46 with the 1,800 s watchdog. The build640 reference was 148.863 s. The unchanged validator and run_default_g5.verify_run passed, and all six CORE hashes match FP1. Commit the default proof, then run S2 on build647.

FP2-C1 continuation 2 default proof pushed as `e1c1085b68`. The audit found 247 recorded C1 identities terminal before the S2 launch. S2 uses the same config and build647 solver as S, the 600 s initial isolated restart helper, and the continuation 2 watchdog rule if needed.

FP2-C1 S2 event 1 F3b and P7/P8 passed. The job peak and sum of process peaks were both 11,377,864,704 bytes, with 79,996 proof faces and 96.866% minimum timer coverage. The maximum remesh timer root was 1,029.737 s. The separate timing rule is exceeded for this event. Event 2, restart, and identical payloads remain the result checks.

FP2-C1 S2 event 1 mesh, native reference and restart all match S by on-disk SHA256. The second remesh is running.

FP2-C1 S2 event 2 F3b and P7/P8 passed. Both memory values were 11,612,327,936 bytes, with 77,370 proof faces and 98.487% minimum coverage. The maximum remesh timer root was 499.068 s. Continued CFD, restart and final identity follow.

FP2-C1 continuation 2 §1.3 completed. S2 passed standing gates and initial isolated restart in 156.522 s. All six payloads matched S and the full comparison had zero exact differences. Native maxima 1,029.687 / 498.925 s exceed 645.186 / 431.048 s, with one-minute host load 6.43 at launch and 6.32–19.67 during the run. Label FP2-C1: accepted, time rule OPEN for the user. Commit and push the reference and report, audit, then open FP2-C2.

FP2-C1 accepted with time rule OPEN for the user at `de08269a8a`; all prior audits terminal. FP2-C2 order verified, archived and read. S0 = `de08269a8a`, build647 with both targets reproduced by build650. Whole-item R4 check recorded in `FP2_C2/source_check.md`. Implement §3 and the hidden §4 fixtures under the continuation 2 helper-watchdog rule.

FP2-C2 §6.1 implementation and fixture source complete. Terminal audit clear. Build651 starts both targets with the new fixture include in the source pins. Run the acceptance fixtures, off cohort and G1 after the build.

FP2-C2 build651 is terminal and passed both targets in 68.778 s. No new build warnings. The solver and tester hashes are 36265f76…a996 and 6544c7d5…040b. Preflight load 3.96, 39,182,736 kB available, zero owned competing processes. Start the C2 serial, border and exchange fixtures, then the option-off cohort and G1.

FP2-C2 build651 passed serial (2,716 assertions in 6 cases) and the two-rank border fixture. The first serial execution was exit 0. Its wrapper compared tuples to JSON arrays while sealing. The repaired wrapper verified the original driver hash and resumed the terminal result without a rerun. Fixture (i) ended at its setup assertion on rank 0, line 595. The shifted midpoint was still marked border (true, wanted false), before it was eligible for collapse. R3 test setup diagnosis: the old border list has no midpoint, and a child cell from which the split removed the only old border vertex stayed on rank 1. Preserve build651 evidence; correct only the fixture migration setup.

FP2-C2 R3 changed only the fixture (i) first section's initial cell ownership. Both endpoints are in the original rank meeting, each split child follows one of them to rank 0, and the far cube remains on rank 1 for state assembly. Geometry, test edge and decisions are unchanged. Build652 terminal in 25.129 s, no new warnings, its solver hash equals build651 and only the fixture include differs among source pins. Re-run hidden fixtures and continue with the part 1 option-off cohort.

FP2-C2 build652 passed all three dedicated fixture groups. Serial 2,716 assertions; two-rank border 271 / 255; two-rank exchange 1,433 / 808. The removals-only run passed with its Shape guard enabled. The original build651 fixture source was reconstructed byte-for-byte against its archived SHA256. The FP2-C1 option-off cohort, the 18 FP1 cohorts and G1 are executing in the same pinned validation campaign.

FP2-C2 §6.1 build652 passed. All new fixtures and the surface option-off cohort, 18 FP1 cohorts, and G1 375 / 114 / 228 match. IN1 state modeled peak was 7,531,924,972 versus 7,531,924,892, the ordered 80-byte overlay constant. The initial test comparator required equality on that row. The corrected comparator verified that exact delta and all other rows and preserved the first receipt and driver. No tests repeated. Next commit and push fixtures, then launch both repetitions of N1, N6 and N6P.

FP2-C2 fixtures pushed as 553ea0f147. Pre-lab actual-host audit found all owned handles terminal. Begin §6.2 in N1, N6, N6P order with two trials each.

FP2-C2 build652 N1 trial 1 is terminal at runner exit 1. Evidence: cases/build652/N1. Inspect the first observed difference before any further run.

FP2-C2: lab FAILED; default path not yet proven. Build652 N1 trial 1 exited 1 after 441.308 s. First gate difference was classify_unexplained=4, expected 0. All 8,800 surviving C-face probes passed. Step C accepted 5,937 collapses, including 1,977 in sweep 1. Read-only R5 checked five equal input hashes and 34 equal L/D rows. Sheet 10154 failed the absolute certificate after the specified trial screens passed. No wiring error found. Keep the code at fixture checkpoint 553ea0f147 and archive the failure. No further lab trials or S were started. FP2-D prerequisite unmet.

FP2-C2 continuation 1 start. Verified and archived FP2_C2_CONTINUE_1.md with its SHA256 and read the whole order. S0=71d4ba519b, production=553ea0f147, build652. User accepted the C1 timing flag. Whole-item R4 recorded in FP2_C2/continuation1_source_check.md before edits. Step 1 will reproduce the build652 N1 counts and localize the mesh-side and model-side failures with test code.

FP2-C2 continuation 1 build653 completed in 197.034 s without a new warning. Its solver and all Common/SU2_CFD pins match build652. Starting the one pinned N1 diagnostic on the test driver.

FP2-C2 continuation 1 Step 1 passed its diagnostic reproduction. Build653 N1 reproduced 5,937 collapses, four above-tolerance and unexplained classifications, bins 0,2,1,0,1, and expected test exit 1 at 567.114 s. All four are step-C faces on sheet 10154. Model-side witness facet 11045, with two flagged faces among the nearby boxes. R4 and diagnostics are documented in FP2_C2/reports/FP2_C2_continue1_diag_653.md. Push the diagnosis, then implement the ordered Bound test.

FP2-C2 continuation 1 diagnostic pushed as 2df5aeb78c. Step 2 adds canonical test 11 (Bound), keeps both original screens ahead of it, and adds S33/rank checks, strict rank-face comparison and modeled memory reporting. Both targets now build as 654. Next fixtures and G1 on the pinned candidate.

FP2-C2 continuation 1 build654 passed both builds (116.149 s, no new warnings), all 8 serial tests (3,991 assertions) and the border fixture (271 / 255). The MPI exchange removals-only section failed: first result 16 Bound instead of 0 None on rank 0. The pinned uncurved S33 backend returns only the inherited facet to non-direct classification; the example child keepers in both directions are separated by at least one grid unit from that facet along x. Read-only diagnosis found no transcription or wiring error. Label FP2-C2 continuation 1: fixtures FAILED; default path not yet proven. Keep the candidate, preserve receipts, and stop. Build654 option-off, FP1, G1, lab and S are unevaluated. FP2-D prerequisite unmet.

FP2-C2 continuation 2. The signed order and SHA256 were verified, archived and read. The build654 fixture failure label is superseded. Fixture (i) pins the real Bound refusal and three unchanged snapshots, then tests removals-only exchange on a fresh mesh with only the bound answer supplied by a test wrapper. Added report-only C2 entity face totals and false-bound query counts by rank, pass and sweep. Starting test-only build655 against the build654 production pins.

FP2-C2 continuation 2 build655 passed in 32.709 s without a new warning. Solver hash and production pins equal build654. All hidden fixture groups passed with assertion summaries {'serial': [[['3991', '8']]], 'border': [[['271', '1']], [['255', '1']]], 'exchange': [[['1721', '1']], [['1073', '1']]]}. The option-off, FP1 and G1 cohorts are executing under the same pins.

FP2-C2 continuation 2 build655 fixtures passed. The pinned real Bound refusal and unchanged snapshots passed, and wrapped removals-only exchange assembled the identical state on two ranks. The option-off comparison is exact against build652. All 18 FP1 cohorts and G1 375 / 114 / 228 matched. Production source pins and solver hash equal build654. Push the required fixtures-passed label, then begin the six ordered lab trials.

FP2-C2 continuation 2 fixtures pushed as 4494472267. Start the build655 lab with the build654 solver and all prior handles terminal.

FP2-C2 build655 N1 trial 1 passed in 525.297 s. Across passes and ranks C accepted 5,930 collapses and returned 3 Bound refusals.

FP2-C2 build655 N1 first proof passed. All 75,954 classified faces are below tolerance, strict answers match for all 75,954, and model_gap_entities=0. Bound refused three directions on sheet 10154, at C sweeps 3 and 5. The second N1 trial follows under the existing campaign controller.

FP2-C2 build655 N1 trial 2 passed in 441.984 s. Across passes and ranks C accepted 5,930 collapses and returned 3 Bound refusals.

FP2-C2 build655 N6 trial 1 passed in 569.022 s. Across passes and ranks C accepted 7,089 collapses and returned 5 Bound refusals.

FP2-C2 build655 N6 trial 2 passed in 547.965 s. Across passes and ranks C accepted 7,089 collapses and returned 5 Bound refusals.

FP2-C2 build655 N6P trial 1 passed in 383.921 s. Across passes and ranks C accepted 7,027 collapses and returned 2 Bound refusals.

FP2-C2 build655 N6P trial 2 passed in 385.153 s. Across passes and ranks C accepted 7,027 collapses and returned 2 Bound refusals.

FP2-C2 build655 lab PASSED all six trials. Repeat comparisons exact for N1/N6/N6P; final strict face counts 75,954 / 73,618 / 72,208 with zero mismatches, zero sink mismatches, zero unexplained and zero model gap entities. The comparator writer initially compared a tuple to its JSON list and failed post-write sealing. Normalized the list and verified the same completed evidence; no trial repeat. Next commit and push lab, then start S on the build654 solver.

FP2-C2 build655 lab passed was pushed as a38be6c805. Start the single ordered S workflow and its isolated restart on the pinned build654 solver.

FP2-C2: S passed; default path not yet proven. Both build655 S events passed with native maxima 598.347 / 416.655 s and timer coverage at least 98.037% / 98.943%. Owned proof faces 67,512 / 62,802; job peaks equal summed process peaks. Isolated restart passed in 112.807 s and six payload hashes sealed. Production pins still match build654. Commit and push the S-passed label, audit, then start FP2-D.

FP2-D session start. S0 = 240330d3aa (FP2-C2: S passed; default path not yet proven), verified at origin/codex/native-3d-bl. Scoped status shows no tracked changes under Common, SU2_CFD or UnitTests. Two untracked CNativeDistributed3D .orig backups predate S0. The process list shows no owned build, test or solver. S0 uses build655 with the build654 solver. The passed lab receipt is FP2_C2/cases/build655/lab_acceptance.json. The passed S receipt is FP2_C2/G5/build655/S/run_receipt.json (native maxima 598.347 / 416.655 s). Verified and archived FAST_PATH_STAGE2_D1_REV2.md and its SHA256 69178513638b8741ae412cd689c50bc0b2396676c3395d867bb3ba868a27d8f1. FAST_PATH_STAGE2_D1.md is superseded and retained as evidence. Next is the whole-item R4 source check.

FP2-D R4 whole-item source check recorded in FP2_D/source_check.md against S0 before production edits. The host process list also shows no owned job. Implement the step-D choice, locked-path floor, modeled storage and hidden fixture/lab instrumentation.

FP2-D build656 passed both targets in 86.29 s with no warnings and unchanged build source pins. The 12 directional fixtures passed 1,358 assertions on the first attempt. The pre-existing/shared fixture edits that appeared after the start audit are included in the pinned build. Next run the option-off coverage, corrected C2 cohorts, FP1 and G1.

FP2-D build656 corrected C2 serial, border and exchange cohorts matched the S0 Catch totals (3991/8, 271/255, 1721/1073 assertions). All 18 FP1 cohorts executed and passed. G1 serial began under the same build pins. The copied lab harness loaded all three qualified S0 repeats using the S0 comparator, and parser checks covered visible altitude failure and the new invariant row. The copied S launcher verified the S0 receipt and the S configuration hash. Next seal G1 and the complete fixture acceptance, push, then start N1 trial 1.

FP2-D build656 fixture gates PASSED. Exact option-off comparison, 12 directional cases with 1,358 assertions, corrected C2 serial/border/exchange, 18 FP1 cohorts and G1 375 / 114 / 228. G1 finished in 791.667 / 9.347 / 10.536 s. FP2_D/fixtures/build656/d_acceptance.json is sealed. Commit and push the fixture checkpoint, then run lab N1 trial 1.

FP2-D fixtures pushed as 673c328dd9. Start ordered N1 lab trial 1 on build656 after the fixture controller was verified terminal.

FP2-D build656 N1 trial 1 has a visible result failure at rank 0, pass 1: sweeps_distance=40 versus the gate maximum 39. Sweep 40 accepted all 22 marks. The minimum final physical altitude cleared Limit(), and the donor scan rejected zero cells. The owned trial is completing the final proof. Preserve the terminal receipt and perform the single read-only R5 diagnosis. No later trial or S is authorized if the review finds no wiring error.

FP2-D: lab FAILED; default path not yet proven. Build656 N1 trial 1 completed normally in 322.010 s (pass 155.507 s). Sole lab gate failure: rank 0 pass 1 sweeps_distance 40, maximum 39. Sweep 40 accepted 22 splits. Sweeps 18–40 continued to accept 22/32 terminals per sweep with no deferrals or Near refusals, path_max=184 and left_limit=1,176. The final mesh had 50,222 physical faces, min altitude 7.0005039215203653e-05 versus Limit 6.9999999999999994e-05, strict_checked=50,222, qualified=1 and zero strict/sink mismatches, unexplained faces or donor rejections. Single read-only diagnosis found the ordered pooled paths converge on sparse terminals and did not exhaust the waiting faces at the sweep cap. No wiring error found and no repeat. Preserve the candidate. N1 repeat 2, both N6/N6P repeats, N1Invariant, alarm, S and default proof are unevaluated. Report FP2_D/reports/FP2_D_656.md. Verify terminal state, commit and push the failed-gate checkpoint, then stop for the user.

FP2-D build656 terminal audit sealed at FP2_D/terminal_audit_656.json. The host process list has no owned solver, MPI launcher, test, or build. The failed-gate checkpoint preserves the source at the fixture checkpoint and adds the N1 receipt, diagnosis and handoff. Next is the user's decision under REV2.

FP2-D continuation 1 started at d5e8ec972a with tracked source clean and no owned job. Verified, archived and read FP2_D_CONTINUE_1.md with SHA256 8c3b55c6c4b7903776ed64aab8c90f3f88c6e703bfea285da28aa29b80ad8977. Whole-item R4 is in FP2_D/continuation1_source_check.md. Next implement the nonfeature error stop, revise fixtures and probe, and build657. FP2-C3 is conditional on the exact FP2-D S-passed commit.

FP2-D continuation 1 build657 completed both targets in 55.230 s with no warnings and verified source pins. Starting the 15 directional cases before the option-off, FP1 and G1 cohorts.

FP2-D continuation 1 build657 directional fixtures PASSED 15 cases and 1,641 assertions. The revised skewed path, border, fan and floor pins passed, along with the four/eight-layer strip, centroid walk and feature exception. Next run inherited option-off, all 18 FP1 cohorts and G1.

FP2-D continuation 1 build657 cylinder (a) used 15 directional splits in one marked sweep versus 27 with the option off, with one Refused residual. The corrected C2 serial/border/exchange cohorts matched S0, and all 18 FP1 cohorts passed. G1 serial started. Next seal option-off and G1, push the fixtures checkpoint, then start the ordered lab.

FP2-D continuation 1 build657 fixture gates PASSED. All 15 FP2-D cases, exact option-off coverage including corrected C2 cohorts, 18 FP1 cohorts and G1 375 / 114 / 228 passed. Serial G1 took 751.223 s. FP2_D/fixtures/build657/d_acceptance.json is sealed. Commit and push the fixtures checkpoint, then start lab N1 trial 1.

FP2-D continuation 1 fixture checkpoint pushed as 4fcbf45d0b. All fixture controllers are terminal. Start the build657 lab at N1 trial 1.

FP2-D continuation 1 build657 N1 trial 1 passed in 365.029 s, strict_checked=59476, unexplained=0, model_gap_entities=0. Receipt FP2_D/cases/build657/N1/measurement.json. Next run N1 trial 2.

FP2-D continuation 1 build657 N1 trial 2 passed in 361.054 s, strict_checked=59476, unexplained=0, model_gap_entities=0. Receipt FP2_D/repeat2/cases/build657/N1/measurement.json. Next run N6 trial 1.

FP2-D continuation 1 build657 N6 trial 1 failed in 509.264 s, first recorded difference={'rank': 4, 'pass': 2, 'field': 'lt_1e_2', 'observed': 5, 'maximum': 0}. Receipt FP2_D/cases/build657/N6/measurement.json. Next stop for gate disposition.

FP2-D continuation 1 build657 lab FAILED. Both N1 trials (365.029 / 361.054 s) passed and matched exactly. N1 had 7 marked sweeps, 8,636 D splits, 92 waits, path_max=1, no residual and 59,476 strict-checked faces with zero above tolerance. N6 trial 1 (509.264 s) sole failure was rank 4 pass 2 lt_1e_2=5 versus 0. Two short physical edges survived from pass 1 and three more were below the threshold in the pass-2 census. The shortest metric length was 0.0025017691612903318. The single read-only diagnosis confirmed the physical seed and altitude gates do not supply a metric-edge lower bound, and found no wiring error in the ordered rule. Keep the candidate with no retry. N6 repeat 2, N6P, N1Invariant, the alarm, S, the default proof and FP2-C3 are unevaluated. Next terminal audit, commit and push the failed lab checkpoint, then stop per the user order.

FP2-D continuation 1 terminal audit sealed at FP2_D/continuation1_terminal_657.json. No owned job is running. The failed lab checkpoint includes completed N1 and N6 trial receipts and the R5 diagnosis. FP2-C3 remains gated on the exact FP2-D S-passed label.

FP2-D continuation 2 started at 3c494fac91 with tracked sources clean and the host idle. Verified, archived and read FP2_D_CONTINUE_2.md, SHA256 47f5d8edb7ca66ab2d067669179aa3bd30c73b92de3ef6855c310156dfa39a00. R4 check recorded in FP2_D/continuation2_source_check.md before edits. Next implement the metric floor, diagnostic capture and fixtures. FP2-C3 remains gated on the exact FP2-D S-passed commit.

FP2-D continuation 2 build658 completed both targets in 55.467 s without warnings, with unchanged source pins. Start the 18 directional fixtures.

FP2-D continuation 2 build658 passed 17/18 directional fixtures. All ordered metric-floor assertions passed. The new test's option-off control incorrectly predicted line-only bisection (63 splits); the unchanged longest-edge walk instead first cuts 2-3 and produced 227 splits. Preserved the build658 receipt. R3 correction changes only that control's derivation and asserts the initial terminal and the existence of short edges. Rebuild test driver as build659, with the build658 solver unchanged.

FP2-D continuation 2 build659 stopped at a test-only compile error: PersistentLineGeometry needed the existing attempt recorder. Switched the fake's base to DistanceSampleGeometry, which records Place and keeps the same flat placement. Build660 will recompile the fixture with the same production pins and rerun all 18 directional cases.

FP2-D continuation 2 build660 passed in 30.449 s with no warnings and solver SHA unchanged from build658. All 18 directional fixtures passed (2,263 assertions). Next run inherited option-off, FP1 and G1 before the fixture checkpoint.

FP2-D continuation 2 build660 option-off fixtures and corrected C2 cohorts passed, along with all 18 FP1 cohorts. G1 serial started. The option-off chain repeated build658's 227 splits, 22 sweeps and 726 short edges. The corrected control confirmed the initial longest-edge terminal. Lab helpers verified both historical orders, the old field sets, the Short-face parser and N6P alarm reference 324 / 601. Next seal G1 and the fixture receipt, push, then start the lab.

FP2-D continuation 2 build660 fixture gates PASSED. All 18 directional cases (2,263 assertions), exact option-off including corrected C2 cohorts, 18 FP1 cohorts and G1 375 / 114 / 228. G1 serial took 756.466 s. FP2_D/fixtures/build660/d_acceptance.json is sealed. Commit and push, then start the lab.

FP2-D continuation 2 fixture checkpoint pushed as 89a8eaccde. Fixture controllers are terminal. Start the build660 lab at N1 trial 1.

FP2-D continuation 1 build660 N1 trial 1 passed in 364.995 s, strict_checked=59476, unexplained=0, model_gap_entities=0. Receipt FP2_D/cases/build660/N1/measurement.json. Next run N1 trial 2.

FP2-D continuation 2 build660 N1 trial 1 PASSED. Both new metric-floor counters were zero, with 7 marked sweeps and 59,476 strict-checked faces. The reused controller's prose prefix says continuation 1 for build660; build number and pinned orders identify all these trials as continuation 2. Next N1 trial 2.

FP2-D continuation 1 build660 N1 trial 2 passed in 363.842 s, strict_checked=59476, unexplained=0, model_gap_entities=0. Receipt FP2_D/repeat2/cases/build660/N1/measurement.json. Next run N6 trial 1.

FP2-D continuation 1 build660 N6 trial 1 passed in 541.888 s, strict_checked=57118, unexplained=0, model_gap_entities=0. Receipt FP2_D/cases/build660/N6/measurement.json. Next run N6 trial 2.

FP2-D continuation 1 build660 N6 trial 2 passed in 537.239 s, strict_checked=57118, unexplained=0, model_gap_entities=0. Receipt FP2_D/repeat2/cases/build660/N6/measurement.json. Next run N6P trial 1.

FP2-D continuation 1 build660 N6P trial 1 passed in 361.938 s, strict_checked=57664, unexplained=0, model_gap_entities=0. Receipt FP2_D/cases/build660/N6P/measurement.json. Next run N6P trial 2.

FP2-D continuation 1 build660 N6P trial 2 passed in 359.908 s, strict_checked=57664, unexplained=0, model_gap_entities=0. Receipt FP2_D/repeat2/cases/build660/N6P/measurement.json. Next run N1Invariant trial 1.

FP2-D continuation 1 build660 N1Invariant trial 1 passed in 358.607 s, strict_checked=59476, unexplained=0, model_gap_entities=0. Receipt FP2_D/cases/build660/N1Invariant/measurement.json. Next compare repetitions and evaluate the alarm.

FP2-D continuation 2 build660 lab PASSED. Exact pairs N1/N6/N6P, 59,476 / 57,118 / 57,664 strict faces, zero mismatches, no final metric edges below 0.01. N1Invariant 8,636 checked / zero violations. Metric split refusals 0 / 12 / 77 per trial. N6P alarm both trials 105 / 136, first failing thresholds 487 / 902. Lab receipt FP2_D/cases/build660/lab_acceptance.json. Next commit and push lab, then run S.

FP2-D build660 lab pushed as c1e5a59639. Start S using the build658 solver under build660 provenance. The lab controller is terminal. FP2-C3 remains gated on the exact S-passed checkpoint.

FP2-D build660 S event 1 receipts p7p8_event1_gate.json and f3b_event1_timer_gate.json passed. The production hand-over census showed no edge below 0.01 on all six ranks. The connected workflow is still running. Next await event 2, continued CFD and isolated restart.

FP2-D build660 S event 2 remesh and timing receipts passed. Its production hand-over census reported one edge below 0.01 on rank 3 (minimum 0.00896807). This is an observation outside the lab hand-over gate, and the present rows do not attribute its creation to a specific operation. Next await continued CFD and the isolated restart receipt.

FP2-D build660 S PASSED. Native maxima 550.220 / 330.735 s; timer coverage minima 97.892% / 98.800%; proof faces 53,188 / 47,718; job peaks 10,250,842,112 / 10,463,076,352 bytes. Both events and continued CFD passed, six payload digests verified, isolated restart 83.336 s, no owned job. Commit and push the exact FP2-D S-passed checkpoint, then verify, archive and read FP2-C3 under the user order.

FP2-C3 started after exact FP2-D S-passed checkpoint 0c6860fd3a (build660, solver identical to build658). The signed order is archived under FP2_C3/inputs; SHA256 87b804b6f54de048cb1c05f26dc87ab189096b6e6761526d86a91687849d511f. S0 receipts: FP2_D/cases/build660/lab_acceptance.json and FP2_D/G5/build660/S/run_receipt.json. Whole-item R4 recorded in FP2_C3/source_check.md before production edits. Next implement the opt-in local index, remote screen and collective pass-end proof, then build and qualify the fixtures.

FP2-C3 build661 stopped during Meson regeneration. Adding the requested source triggered the regeneration, and Meson could not find the existing repo Ninja on PATH. Both targets are unevaluated. Preserve build661 logs and use its absolute Ninja path for regeneration in build662.

FP2-C3 build662 regenerated the graph with Ninja found. C++ compilation stopped because the new ADT caller did not include the declaration of LINE. Included the same option header as the existing distributed ADT caller. Meson also emitted its existing CMake probe and SU2_PY install deprecation warnings during regeneration. Build663 will recompile without a graph change.

FP2-C3 build663 compiled both targets in 48.121 s with zero warnings and verified source pins. Start the focused serial and two-rank contact fixtures, then the option-off S0 and FP1 cohorts and G1.

FP2-C3 build663 focused serial/MPI receipts sealed. R6-open groups: []. Starting option-off, FP1 and G1.

FP2-C3 build663 passed 7 serial contact cases (9,648 assertions) and 3 MPI contact cases on each rank (898 / 727 assertions). The FP2-D control reproduced its S0 cylinder and chain counts. The inherited corrected C2 and FP2-A/B cases and all 18 FP1 cohorts completed. G1 serial started. Next seal G1 and the option-off comparison, then push the fixtures checkpoint and start the lab.

FP2-C3 build663 fixture gates PASSED with exact S0 option-off comparisons, 18 FP1 cohorts and G1 375 / 114 / 228 (748.269 / 10.154 / 9.966 s). The scoped option-off logs emitted no contact rows and no surface-counter rows. FP2_C3/fixtures/build663/c3_acceptance.json is sealed. Commit and push, then start N1 trial 1.

FP2-C3 fixture checkpoint 7ecd2a62da pushed and verified at the remote branch head. All fixture controllers are terminal. Start the build663 lab at N1 trial 1.

FP2-C3 build663 N1 trial 1 step C completed with 21,960 collapses (17,869 outward) in 15 sweeps and 26,920 physical faces. Of 877.460 s in step C, 41.132 s was the contact tests and 784.810 s was the existing Bound queries. Local contact refused zero directions. This is an in-flight measurement. Next await embedding, pass-end verification and rebuilt-state qualification.

FP2-C3 build663 N1 trial 1 passed in 1093.303 s, strict_checked=26920, unexplained=0, model_gap_entities=0, zero embedding violations on 1 rank-pass checks. Receipt FP2_C3/cases/build663/N1/measurement.json. Next run N1 trial 2.

FP2-C3 build663 N1 trial 2 passed in 1213.391 s, strict_checked=26920, unexplained=0, model_gap_entities=0, zero embedding violations on 1 rank-pass checks. Receipt FP2_C3/repeat2/cases/build663/N1/measurement.json. Next run N6 trial 1.

FP2-C3 build663 N1 exact repeat comparison passed with zero differences. Trial elapsed 1093.303 / 1213.391 s; pass 950.332 / 1049.104 s at starting one-minute load 0.77 / 1.31. Both exceed the 600 s reporting threshold. The serial checks tested 373,889 cells against 26,676 owner rows with 1,716,221 exact calls, and embedding checked 17,796,216 pairs with zero violations. Next await N6 trial 1.

FP2-C3 build663 N6 trial 1 passed in 1530.060 s, strict_checked=24530, unexplained=0, model_gap_entities=0, zero embedding violations on 12 rank-pass checks. Receipt FP2_C3/cases/build663/N6/measurement.json. Next run N6 trial 2.

FP2-C3 build663 N6 trial 2 passed in 1431.399 s, strict_checked=24530, unexplained=0, model_gap_entities=0, zero embedding violations on 12 rank-pass checks. Receipt FP2_C3/repeat2/cases/build663/N6/measurement.json. Next run N6P trial 1.

FP2-C3 build663 N6 exact repeat comparison passed with zero differences. Trial elapsed 1530.060 / 1431.399 s; slowest passes 465.489 / 106.736 s and 525.709 / 99.820 s. Both trials qualified 24,530 faces with eight accounted above tolerance and zero strict sink mismatches. The final classification took 630.745 / 506.289 s. Next await N6P trial 1 on the event-1 production partition.

FP2-C3 build663 N6P trial 1 reported the first observed result difference at rank 0, pass 2: lt_1e_2 = 1 against a maximum of 0. Both pass-end contact checks and embedding checks completed on all six ranks. Let this active trial finish and seal its evidence, then make the single read-only R5 diagnosis. N6P repeat and S are gated on the result.

FP2-C3 build663 N6P trial 1 failed in 858.792 s, first recorded difference={'rank': 0, 'pass': 2, 'field': 'lt_1e_2', 'observed': 1, 'maximum': 0}. Receipt FP2_C3/cases/build663/N6P/measurement.json. Next stop for gate disposition.

FP2-C3 build663 lab FAILED. N1 and N6 passed both trials and matched exactly. N6P trial 1 ended in 858.792 s with one gate miss: rank 0, pass 2, physical edge (69215,74584) metric length 0.0090487725598145056, giving lt_1e_2=1 against 0. All 38 rank-pass checks across the five trials passed embedding and global contact verification. N6P's six snapshots matched, with 26,480 strict-checked faces, zero unexplained faces, zero model gaps and zero strict sink mismatches. The single read-only R5 diagnosis traced the edge to step C and found no wiring error. Keep the candidate with no retry. N6P trial 2, S and the default proof are unevaluated. The terminal audit confirms no owned build, test or solver remains. The closing checkpoint carries the exact FP2-C3 lab FAILED label. Stop under the signed order.

FP2-C3 continuation 1 started at ae9ef407fa, verified at origin/codex/native-3d-bl. Scoped tracked sources are clean and the host has no owned workload. Verified and archived FP2_C3_CONTINUE_1.md, SHA256 f5d5c6ac5bc2e37a06bc53d0318ac4e86c734826ef52bd5c89061dbfb6184bb8. Whole-item source check is FP2_C3/continuation1_source_check.md. S0 stays 0c6860fd3a/build660, with build663 as the second report baseline. Next implement the step C floor, measurements and fixtures.

FP2-C3 continuation 1 implemented the step C metric refusal, three print-only censuses, fixtures (k) and (l), and lab diagnostics. The five C3 harness drivers retain S0 and build663 evidence, and the report/parser self-checks passed. Build664 will compile both targets with the continuation and source check pinned. Next run all ordered fixture and identity cohorts.

FP2-C3 continuation 1 build664 compiled both targets in 58.601 s with no new warnings and all source pins verified. Start the nine serial fixtures, three two-rank fixtures, option-off comparisons, FP1 and G1.

FP2-C3 build664 focused serial/MPI receipts sealed. R6-open groups: []. Starting option-off, FP1 and G1.

FP2-C3 continuation 1 build664 focused tests passed 9 serial cases (10,314 assertions) and 3 MPI cases on both ranks (898 / 727 assertions). The new fixture refused one direction with two short fresh edges, accepted the 0.011 arm, and measured six edges at all three gated-on checkpoints. Its option-off output printed all four zeros. The old fixture pins did not change. Next await option-off, 18 FP1 cohorts and G1.

FP2-C3 continuation 1 build664 option-off cohorts and all 18 FP1 cases ran successfully. The FP2-D cylinder and chain matched S0. G1 serial is running. Next seal the complete option-off comparison and G1 identity results, then push and start N6P trial 1.

FP2-C3 continuation 1 build664 fixtures PASSED. Exact option-off, four zeros independently verified on one control row, all 18 FP1 cohorts passed and G1 matched 375 / 114 / 228 (749.443 / 10.022 / 11.149 s). Receipt FP2_C3/fixtures/build664/c3_acceptance.json is sealed; fixture controller is terminal. Push the continuation fixture checkpoint, then run N6P trial 1.

FP2-C3 continuation 1 fixture checkpoint 2b008206ae pushed and verified. Start the build664 lab with N6P trial 1. The campaign stops at the first failed trial; exact comparisons follow six passing trials.

FP2-C3 continuation 1 build664 N6P trial 1 passed the observed final-edge census on every rank. The three intermediate counts were zero in both passes. The floor refused 6 directions in pass 2 (rank 0: 4, rank 1: 1, rank 2: 1) and none in pass 1. Twelve embedding and contact checks passed. This is an in-flight measurement; await rebuilt-state proof and qualification before trial 2.

FP2-C3 build664 N6P trial 1 passed in 858.019 s, strict_checked=26482, unexplained=0, model_gap_entities=0, zero embedding violations on 12 rank-pass checks. Receipt FP2_C3/cases/build664/N6P/measurement.json. Next run N6P trial 2.

FP2-C3 build664 N6P trial 2 passed in 853.561 s, strict_checked=26482, unexplained=0, model_gap_entities=0, zero embedding violations on 12 rank-pass checks. Receipt FP2_C3/repeat2/cases/build664/N6P/measurement.json. Next run N1 trial 1.

FP2-C3 continuation 1 build664 N6P passed twice (858.019 / 853.561 s). Both trials qualified 26,482 faces, with zero short final edges and zero embedding/contact failures. The old failing rank-0 pass-2 minimum rose from 0.0090487725598145056 to 0.010048002855871579. The campaign now runs N1 trial 1 and leaves the exact pair comparisons until all six trials pass.

FP2-C3 build664 N1 trial 1 passed in 1086.701 s, strict_checked=26920, unexplained=0, model_gap_entities=0, zero embedding violations on 1 rank-pass checks. Receipt FP2_C3/cases/build664/N1/measurement.json. Next run N1 trial 2.

FP2-C3 build664 N1 trial 2 passed in 1116.495 s, strict_checked=26920, unexplained=0, model_gap_entities=0, zero embedding violations on 1 rank-pass checks. Receipt FP2_C3/repeat2/cases/build664/N1/measurement.json. Next run N6 trial 1.

FP2-C3 build664 N6 trial 1 passed in 1507.171 s, strict_checked=24530, unexplained=0, model_gap_entities=0, zero embedding violations on 12 rank-pass checks. Receipt FP2_C3/cases/build664/N6/measurement.json. Next run N6 trial 2.

FP2-C3 continuation 1 build664 N6 repeat 2 did not launch on the first attempt. Actual host preflight saw sleeping mpirun PID 3642077 and stopped the controller. The process is now gone and the host is clear. Sealed the preflight observation and resume the same campaign; the five completed trials are pinned and will not run again.

FP2-C3 build664 N6 trial 2 passed in 1259.155 s, strict_checked=24530, unexplained=0, model_gap_entities=0, zero embedding violations on 12 rank-pass checks. Receipt FP2_C3/repeat2/cases/build664/N6/measurement.json. Next compare repetitions and seal the lab qualification.

FP2-C3 continuation 1 build664 lab PASSED. Six individual trials and all three exact repeat comparisons passed. Strict faces N6P/N1/N6: 26,482 / 26,920 / 24,530. The 50 embedding and contact checks passed; no intermediate or final count was above zero. N6P had six new floor refusals and one fewer collapse than build663. R3 fixed the JSON report writer's event-key type and resealed the unchanged report digest from the six archived trials. The lab receipt is FP2_C3/cases/build664/lab_acceptance.json. Push the lab checkpoint, then start the single S observation.

FP2-C3 continuation 1 lab checkpoint a94e5cc4c3 pushed and verified. All six controllers are terminal. Start the one build664 S observation with the sealed build660 S reference.

FP2-C3 continuation 1 build664 S event 1 passed its remesh and timer receipts. All 25,202 proof faces were covered on the six ranks; job peak 9,520,005,120 bytes equals the process peak sum. The core hand-over was 26,480 faces and its short-edge counters were zero. Event 2 is active.

FP2-C3 continuation 1 build664 S event 2 passed the remesh and timer gates; proof face sum 20,244, job peak 9,662,668,800 bytes matched the process peak sum. Continued CFD ended with SU2_CFD Exit Success. The controller started the isolated restart; await its receipt before closing S.

FP2-C3 continuation 1 build664 S isolated restart observation failed before its post-only validator. The CFD child exited 0, summed rank peak was 884,408,320 bytes, and all handles were terminal, but runtime_receipt_written=false (expected true). R5 found that the wrapper permanently suppressed further live sampling after a startup EIO, although its normal sample loop and the child continued. The specific read site was not logged. Preserve the first attempt, correct only the EIO latch in the restart sampler, and repeat the isolated restart once from the sealed S outputs. The full S solver is not restarted.

FP2-C3 continuation 1 build664 S PASSED. Native maxima 951.321 / 689.037 s; minimum timer coverage 98.891% / 99.599%; proof faces 25,202 / 20,244; peak bytes 9,520,005,120 / 9,662,668,800 equal to their rank peak sums. The one corrected isolated restart passed in 46.276 solver seconds (50.890 s wrapper) with all six live runtime pins verified. The original S adaptations and continued CFD were reused. The six payload hashes are sealed in FP2_C3/G5/build664/S/run_receipt.json. The terminal audit confirms one full S and no remaining owned workload. Close with the exact FP2-C3 S-passed label and stop. The same-binary repeat and default-path proof are unevaluated.

FP2-M1 start at af5844543d. Verified f9388a056ba264c7612e1c8a415783dc7b7ce75dd28dbbf263141dccdcef8ef5 and archived the signed order. Scoped status shows no tracked production or test edits, with only the two existing Common .orig copies. Host preflight found no owned computation, load 0.90 / 1.05 / 0.78 and 34,829 MiB available. The R4 source check is in FP2_M1/source_check.md; the observer can stay in the lab TU and its helpers. Next wire the isolated authority and hand-over counters, then build only the test driver.

FP2-M1 source and integration sub-step at af5844543d. The remote branch resolves to the full base commit af5844543d31c4071a028680c2a218152c15762a. The lab wiring loads an independent budgeted authority before the first pass, brackets measured forwards after shadow queries, and records strict hand-over statistics. The original production pins are checked by a test-driver-only build helper. Next compile the observer and fixtures, then run the option-off and trace checks.

FP2-M1 build665 launched from af5844543d. The integrated lab translation unit passed syntax checking, and the trace comparator, historical trial pins, watchdog wiring and replay arithmetic passed their helper checks. Actual host build preflight found no owned heavy process and 34,913,668 kB available. Target is UnitTests/test_driver only; await the build receipt before running the three fixture checks.

FP2-M1 build665 passed in 38.111 s with zero warnings and all pins verified. SU2_CFD still matches build664 e1b25b45969b9cd778082a164218a1bfaaca5ab5780343b802d11a62a656be13. The build controller is terminal. Run the option-off serial cohort next.

FP2-M1 R3 runner correction before any fixture launch. The build665 option-off command loaded a historical `run_fixtures` module without the C3 `hidden` adapter. The invocation exited before creating a fixture directory or starting a child. The runner now loads that adapter by its exact path, and its self-check verifies both fixture selectors. Seal the corrected helper under build666 with the same test binary and rerun the serial entry point.

FP2-M1 build666 sealed the helper correction in 0.022 s with Ninja reporting no work. Compiler output and both executable hashes are the same as at build665. The controller is terminal; start the three fixture cohorts in sequence.

FP2-M1 build666 option-off serial passed all 9 selected cases with exact build664 Catch counts (2.534 s). The controller is terminal; run the two-rank option-off checks next.

FP2-M1 build666 option-off MPI passed 3 cases on each of 2 ranks with exact build664 Catch counts (0.796 s). The controller is terminal. Run the curved fixture with geometry work trace enabled.

FP2-M1 build666 observer fixtures PASSED. The curved fixture passed 727 assertions in 13.379 s and matched 20/20 forwarded query entries bit for bit. The indexed strip has 48 curved top facets and the known wrong anchor selects a different support. Stage 1 proved all 20 using mean support 8.0 versus 15.6 full facets, and all 20 full-support replicas agreed. Stage 2 was not needed. Option-off counts matched build664 (10,314 assertions serial and 898/727 at two ranks). Push the fixture checkpoint, then run N6P.

FP2-M1 fixture checkpoint 9622b66f80 pushed and verified. Start the measured N6P trial, six ranks, with a 2,575 s wrapper watchdog and campaign trace off. The lab production-state deadline and modeled/admitted memory remain the build664 limits.

FP2-M1 N6P shadow admission passed on all six ranks, with 536,218,624 resident modeled bytes and 1,818,343,680 modeled load-peak bytes per rank against the separate 3,333,248,341-byte load ceiling. The observed process RSS after shadow load was 399,773,696 to 446,668,800 bytes. The first production pass remains active and its identity comparison is pending.

FP2-M1 N6P live pass 1 emitted six ok observers and six zero-violation embedding rows. Stage 1 proved 63,899 of 64,004 non-direct calls (66,116 inclusive calls); stage 2 added no proofs. Stage-1 small selection and bound totaled 767.862 rank-s, and all forwarded calls totaled 817.166 rank-s. These pass-1 measurements are provisional until the whole trial passes and identity matches. Pass 2 is next.

FP2-M1 N6P PASSED. The single build666 trial ended in 1,160.010 s. All 12 observer rows passed, with 26,482 calls across hand-over, rank strict observations and the rebuilt proof. The complete repeat projection matched sealed build664 trial 1 without differences, including the unchanged per-rank modeled peaks. Next run the independent N1 trial with the 3,261 s wrapper watchdog and trace off.

FP2-M1 N1 shadow admission passed. Modeled shadow resident was 3,343,436,800 bytes and load peak 4,625,561,376 bytes under the separate 12,884,901,888-byte ceiling. Process RSS before and after load was 586,588,160 bytes, with historical high water 1,190,256,640 bytes at that point. The production pass is active.

FP2-M1 N1 retrospective qualification at checkpoint 9622b66f80. The child finished at 18:18:22 with exit 0, 37 Catch assertions, and 2,238.47 elapsed seconds under the original 3,261 s watchdog. The actual host had no owned workload and all six recorded PIDs were absent. All 40 trial pins and the sealed build664 reference verified. The inherited gates, 26,920 strict hand-over calls, and the full exact projection including modeled peaks passed. The supervisor stopped writing at 18:08:58. Its wait and owned-process receipts remain absent. The recovery receipt preserves the 4,224,188,416-byte sampled lower bound and GNU time's separate 4,446,085,120-byte serial maximum RSS. No repeat was launched. The first post-run evaluator check used a string rank key; the archived result uses the inherited evaluator's integer key convention and checks the optional round difference as the existing runner does.

FP2-M1 build666 concluded with both trials exact against build664. The stage-1 replay was 771.714 / 201.818 rank-s for N6P passes 1 / 2, compared with 817.166 / 206.529 rank-s forwarded. N1 was 882.769 versus 924.039 s. Stage 2 added zero proofs in all three passes. The section 4 report and JSON are in FP2_M1/reports/FP2_M1_666. Close with the exact label FP2-M1: measurement done; default path not yet proven.

FP2-M2 started at 0957977801. Verified and archived the signed order (6ea8a6633ec03d02acb9737b4de2893a1ad4bb0af8bc22727e8c3398584b2075). Actual host preflight showed no owned build or solver, 35,102 MiB available and load 1.49 / 1.14 / 1.37. The whole-item source check is FP2_M2/source_check.md. All changes will be in lab test code and evidence. Next implement the stage observer, tracing and hand-over tables, then run the focused and model fixtures.

FP2-M2 source and helper sub-step at 0957977801. Integrated the serial option T, isolated stage-forward statistics and phase marks, and the post-digest hand-over and model exporters. The report histogram, trace parser including archived production-format replay, tolerance-config derivation and baseline pin checks passed their self-checks. A detached user-unit smoke test wrote its durable log. Launch build667 for UnitTests/test_driver only.

FP2-M2 build667 passed in 47.836 s, zero new warnings and all source pins verified. The build664 solver remains e1b25b45969b9cd778082a164218a1bfaaca5ab5780343b802d11a62a656be13. The build supervisor is terminal. Run the three option-off fixtures and the independent model pin fixture in sequence.

FP2-M2 build667 fixture off-serial controller returned 0. See FP2_M2/fixtures/build667/off-serial/case_summary.json.

FP2-M2 build667 fixture off-mpi controller returned 0. See FP2_M2/fixtures/build667/off-mpi/case_summary.json.

FP2-M2 build667 fixture curved controller returned 0. See FP2_M2/fixtures/build667/curved/case_summary.json.

FP2-M2 build667 fixture model controller returned 0. See FP2_M2/fixtures/build667/model/case_summary.json.

FP2-M2 build667 inherited fixtures and model pin PASSED. Option-off serial and MPI matched the build666 counts (9 cases and 10,314 assertions, 3 cases per rank and 898 / 727 assertions). The curved fixture matched 20 query traces and 727 assertions. Model elapsed 46.717 s, 48,954 facets, 72,757 same-entity edges, 674 between different entities. All 363,785 seam checks and 291,028 endpoint checks passed (worst 2.8421709430404007e-14 against 3e-10, endpoints exact). The model files are sealed. Push the fixture and model checkpoint before launching BASELINE with trace off.

FP2-M2 fixtures/model checkpoint 466ffc2a2e pushed and verified. Launch BASELINE at 1E-4, with trace off and hand-over CSV capture on. The detached controller will preserve the live host and terminal receipts.

FP2-M2 BASELINE live pass completed with 373,889 cells, 26,920 surface faces and no nonpositive cells. The stage observer reported complete with 0.333 s observer work. These are provisional until the inherited proof, post-digest exports and exact comparison finish.

FP2-M2 BASELINE done in 1,180.061 s. The inherited evaluation qualified, the proof was accepted with zero classified faces above tolerance, and the exact projection including modeled storage matched sealed build664. Both CSVs completed: 26,920 faces, 40,380 edges, 80,760 evaluated corners and zero missing metric edges. The durable supervisor and recorded children are terminal. Launch ANATOMY at 1E-4 with trace on and no hand-over CSV exporter.

FP2-M2 ANATOMY done in 1,253.027 s. The inherited gates and full build664 repeat comparison passed. Trace tables are complete with 17,087,833 rows, 72,564 begun and closed forwards, 300 retained forwards in each length bin, zero UNRESOLVED rows and zero parser errors. No CSV exporter ran. Push the baseline/anatomy checkpoint, then launch 2E-4 under the unchanged 3,261 s watchdog.

FP2-M2 baseline/anatomy checkpoint 74a05f17dd pushed and verified. Both cases are terminal. Start the first independent tolerance measurement at 2E-4 with trace off, M1 off and CSV capture on.

FP2-M2 2E-4 finished in 809.411 s as a finding. Rank 0 pass 1 first failed the inherited altitude check: FP2_D altitude=7.7325745101917715e-05, limit=0.00013999999999999999. The pass retained 364,922 cells and 22,208 faces. The rebuilt proof separately qualified and accepted (zero above tolerance), and the CSVs completed with 22,208 faces and 33,312 edges. The bounded check confirmed that the inherited evaluator compares the observed altitude directly against the reported 0.7-tolerance limit. No correction or repeat; next run 5E-4.

FP2-M2 5E-4 finished in 485.966 s as a finding. First failed inherited gate was again rank 0 pass 1 FP2_D altitude: 7.7325745101917715e-05 below the reported 0.00035 limit. The pass retained 370,213 cells and 25,146 faces. The proof separately qualified and accepted with zero above tolerance. Both CSVs completed (25,146 faces, 37,719 edges). Confirmed the same inherited altitude field and pass in the bounded check. Next run the final 1E-3 measurement.

FP2-M2 1E-3 finished in 369.420 s as a finding. Rank 0 pass 1 failed FP2_D altitude at 7.7325745101917715e-05 below the reported 0.00069999999999999999 limit. The bounded look confirmed the same inherited 0.7-tolerance comparison. The pass retained 375,293 cells and 28,892 faces. The rebuilt proof separately qualified and accepted with zero above tolerance. The CSVs completed (28,892 faces, 43,338 edges). All five authorized first attempts finished, with no watchdog or R5 repeat. Generate and verify the report and update the handoffs.

FP2-M2 build667 report verified. All fixture and trial artifact hashes checked, all report copies match their sealed sources, and hand-over CSV row counts reconcile for the four trace-off cases. Actual-host check found all five detached supervisors and recorded children terminal, with no owned build, test or solver. Production sources and build664 solver stayed pinned. The report and handoffs are ready for the exact final label FP2-M2: measurement done; default path not yet proven. Stop after it is pushed.

FP2-M3 start at d9357fcb73. The FP2-M2 final label is present locally and on origin, and no owned build, solver or test process is live. Host load was 5.35 / 3.33 / 2.67 with 34,458 MiB available. Verified the signed SHA256 50603b9671295ffc71ba92a3fa067c32be0d2eb3e678e35edd0bdcebec069ac0 and archived the order. R4 check is FP2_M3/source_check.md. Next add the three guarded trace sites and lab helpers.

FP2-M3 source sub-step at d9357fcb73. The sole production diff is the trace-only additions to prepare_facet, the joined-witness construction and BuildPlanes in CNativeAffineReference.cpp. The exact diff receipt is 5777717f655412bf45c1624a6cc9aebb34ab615ba4915e281fc7f2b088b3c6e7. The lab facet export is after the inherited digest. Build668 will pin the C++ and order inputs; runner and analysis helper pins are sealed with the checks that use them.

FP2-M3 build668 passed in 142.307 s with no warnings. The candidate solver and test driver hashes are pinned in build_status.json with the authorized AR patch hash. The build is terminal. Start G1 at 1/2/4 ranks with trace explicitly off.

FP2-M3 analyzer/helper self-check passed while G1 serial runs. The synthetic packet covers several leaf ordinals, an unpublished interrupted facet build, cropped and entity-border edges, and both intra- and inter-facet T-junctions with one inside pair of each. The helpers are sealed in FP2_M3/helper_pins.sha256. G1 serial startup shows trace=0 in the command receipt.

FP2-M3 G1 passed exact case, outcome and failed-assertion comparison at all ranks, 375 / 114 / 228 records in 761.061 / 10.472 / 10.244 s. Trace was explicitly off. The detached G1 controller is terminal. Run the inherited focused fixtures and curved packet pin sequentially.

FP2-M3 build668 fixture off-serial returned 0. See FP2_M3/fixtures/build668/off-serial/case_summary.json.

FP2-M3 build668 fixture off-mpi returned 0. See FP2_M3/fixtures/build668/off-mpi/case_summary.json.

FP2-M3 build668 fixture curved returned 0. See FP2_M3/fixtures/build668/curved/case_summary.json.

FP2-M3 build668 focused fixtures passed. Serial and two-rank Catch counts equal build666. The unchanged curved checker matched 20 production forwards. The first usable production witness contained 8 depth-zero leaves (all ordinal 0 in their facets); all 24 exact-rational cubic corner comparisons had zero error at the 6.25e-6 limit. G1 and the analysis self-check are complete. Commit the trace and fixture checkpoint, then run N1 TRACE-OFF.

FP2-M3 trace and fixtures checkpoint 7f10fd1452 pushed and verified. All required parts of the checkpoint passed. Launch TRACE-OFF at 1E-4 with the post-digest facet export on and M1 and M2 hand-over exports off.

FP2-M3 TRACE-OFF finished in 1,126.088 s with the full build664 exact projection, inherited qualification and accepted proof of 26,920 faces. The facet CSV completed and its 48,954 rows and SHA256 verified against the trial. All observed processes are terminal. Launch TRACE-ON at the same tolerance with M2 streaming and the compressed provenance collector.

FP2-M3 TRACE-ON finished in 1,206.416 s. It qualified and matched the full inherited build664 projection. The compressed provenance and M2-format anatomy receipts are complete; both trials are terminal. Verified the facet CSV, provenance artifact and sealed M2 model edge inputs. Start the one low-priority offline analysis in analysis/build668; the compressed stream remains local and will not be committed.

FP2-M3 offline analysis completed in 113.726 s with exit 0. It reconciled 68,853 usable batches and 1,629 uncached forwards with no batch, with no malformed or incomplete measured batch. There are 46,617 complete leaf tables and 20 preserved witness examples. All four M2-format comparison fields matched M2 in every bin. Report input and output hashes, the facet CSV, G1, both identity results and all terminal processes were verified. Close with the exact label FP2-M3: measurement done; default path not yet proven. Preserve the 136,934,750-byte compressed provenance locally at its receipt path. Stop after push.

FP2-P1 start, 2026-11-25 06:52 CET. Verified pushed base 3228ca0e1ecff17eb003faedafd061206cc09167 and no owned solver, build or test process. Load 1.91/1.23/1.00, 33,232 MiB available. Archived and verified FP2_P1_CONFORMING_WITNESS.md and its SHA256 at FP2_P1/inputs/. Read the entire order.

FP2-P1 step 0 complete at base 3228ca0e1e. R4 source check is FP2_P1/reports/source_check.md. No contradictions. Next implement the default-off closure and hidden fixtures, then freeze the first build pair.

FP2-P1 build669 compiled and linked both targets with exit 0. A 513-action regeneration also compiled unchanged vendored HDF5 and produced eight qualifier warnings. The strict build helper withheld qualification. The build669 receipt preserves the warnings. Recompile the two changed translation units as build670 before result gates. No source bytes change in this build correction.

FP2-P1 build670 frozen. Both binaries built with exit 0 and no warnings. Their SHA256s equal the build669 outputs. Direct hidden certificate fixtures passed with closure 0 and trace 0. Next run the three G1 cohorts, inherited fixtures and the paired public-query fixtures in sequence. Each child uses its own host preflight and receipt.

FP2-P1 launcher setup checks. Verified inherited input and runtime setup through the child launch boundary for first collapse, C492, merit481, saved_current and MPI 2D. Corrected duplicate 2D MPI export arguments before execution. Verified S and isolated restart switch insertion after cleanup, and reproduced the two-event timer and hand-over comparisons on the sealed build664 run. No shared or S result gate has run yet.

FP2-P1 build670 steps 1.1 and 1.2 passed. G1 matched 375 / 114 / 228 records. The candidate adapter was looking for inherited_summary.json while the G1 helper had saved all three successful comparisons as case_summary.json. The original receipts were retained and verified against their launch pins and terminal handles, then sealed by seal_g1.py without reruns. The inherited 9 serial and 3 two-rank fixtures matched build668 Catch counts. The curved fixture passed both FP2-M3 validators. Next run the closure-off N1 identity trial.

FP2-P1 build670 step 1.5 passed. The direct fixtures passed 2,016 Catch assertions. The public witness joined two footprint groups into one and proved the same within classification in 1 prepared node instead of 253. The complete and cropped sample-distance bounds each matched across switches, and each cache run preserved bound bits through eviction and rebuild. The signed default-path N1 trial and then the five shared checks are running in su2-fp2-p1-build670-step1.service.

FP2-P1 implementation and focused fixtures checkpoint 679c781a75 pushed. The closure-off N1 trial is still active under the build670 step 1 supervisor. Record its exact projection before lab or workflow launch.

FP2-P1 build670 step 1, default-N1: passed. Exit 0. Evidence default/build670/N1/trial1/case_summary.json. Gate result equal.

FP2-P1 build670 default N1 qualified in 1121.645 s and exactly matched the build664 projection, including the modeled peak fields. Final cells 373889, hand-over faces 26920, pass/end/state modeled peak 12884891280, proof modeled peak 3760680068 bytes. Continue the five closure-off shared checks.

FP2-P1 build670 step 1, first-collapse: passed. Exit 0. Evidence shared/build670/closure0/first-collapse/case_summary.json. Gate result {'first-collapse': 'passed'}.

FP2-P1 build670 closure-off first collapse passed after 250.896 s. All six accepted counts match, work 90809869 / 108459426, zero upper refinements and terminal owned handles. The host sampler records VmHWM and VmRSS in procfs KiB. The first receipt had labeled this sum as bytes. Preserved it and resealed the unit conversion from the unchanged six-rank raw samples. Correct peak sum 7967969280 bytes, within the job limit. No runner or result rerun was needed.

FP2-P1 build670 step 1, exact-c492: passed. Exit 0. Evidence shared/build670/closure0/exact-c492/case_summary.json. Gate result {'bl': 'passed'}.

FP2-P1 build670 closure-off exact C492 passed with 1394 splits, positive sensed private cells, zero upper refinements, unchanged 388.8 s stage limit and frozen input. The p6 validator recorded its first historical edit difference at index 32 (zero-based) as the required decision evidence. The child exited 0 and the parent and sampled child identities are terminal.

FP2-P1 build670 step 1, merit481: passed. Exit 0. Evidence shared/build670/closure0/merit481/case_summary.json. Gate result {'merit481': 'passed'}.

FP2-P1 build670 step 1, saved-current: passed. Exit 0. Evidence shared/build670/closure0/saved-current/case_summary.json. Gate result {'saved_current': 'passed'}.

FP2-P1 build670 step 1, native2d: passed. Exit 0. Evidence shared/build670/closure0/native2d/case_summary.json. Gate result {'2d1': 'passed', '2d2': 'passed', '2d4': 'passed'}.

FP2-P1 build670 step 1 passed and its checkpoint is sealed in FP2_P1/build670/p1_checkpoint.json. All five shared checks passed with closure 0. Merit481 passed 5947 assertions and retained exact payload identity. The current-target fixture passed 611527 assertions and retained exact payload and planning identity. Native 2D BL and no-BL passed 39 tests per rank at 1, 2 and 4 ranks. Next launch N6P twice, N1 twice, N6 twice, the exact comparisons, N1 ANATOMY, and the five closure-on shared checks.

FP2-P1 exact fixtures-passed checkpoint 050c019527 pushed and remote verified. The step 2 service su2-fp2-p1-build670-step2 is submitted. It verifies the frozen build/checkpoint and starts N6P trial 1, closure 1 and trace 0, with the 2575 s wrapper limit. One owned heavy job at a time. Next inspect the trial result and preserve the first finding if any.

FP2-P1 build670 step 2, campaign-N6P-1: passed. Exit 0. Evidence campaign/build670/N6P/trial1/case_summary.json. Gate result not evaluated.

FP2-P1 build670 closure-on N6P trial 1 qualified in 1283.069 s, compared with sealed build664 at 858.019 s. Every rank had exactly one enabled marker. First historical difference is rank 0 FP2_B proof classify_above_tol 109 to 110. Proof work 187822605 to 293166336 and rank-0 classify seconds 422.905 to 660.642. Final 26482 hand-over faces and 371837 cells. These historical comparisons are observations. Next N6P trial 2 and the exact same-binary projection comparison.

FP2-P1 build670 step 2, campaign-N6P-2: passed. Exit 0. Evidence campaign/build670/N6P/trial2/case_summary.json. Gate result not evaluated.

FP2-P1 build670 N6P trial 2 qualified in 1283.655 s. The same-binary N6P exact projection is equal across both trials, with one switch marker per rank in each. The proof configuration, terminal evidence and first historical difference remain in the sealed trials. Next N1 trial 1.

FP2-P1 build670 step 2, campaign-N1-1: FAILED. Exit 1. Evidence campaign/build670/N1/trial1/case_summary.json. Gate result {'rank': 0, 'field': 'switch marker count', 'observed': 0, 'expected': 1}.

FP2-P1 build670 N1 closure-on trial 1 exited 0 in 1529.068 s and all 36 assertions passed. The trial receipt initially flagged a missing marker. The raw combined log has one enabled marker inserted byte-for-byte in the middle of a sweep row by stderr/stdout interleaving. The new seal_mixed_marker.py verifies the original pins and terminal identities, removes only that marker line's bytes in a derived log, rechecks the unchanged inherited evaluation, and preserves both raw and corrected receipts. The corrected trial qualified with one marker, 26920 hand-over faces and 373889 cells. Bound clock 1210.548 s. Next inspect N1 trial 2 and normalize only if the same receipt defect recurs.

FP2-P1 build670 step 2, campaign-N1-2: FAILED. Exit 1. Evidence campaign/build670/N1/trial2/case_summary.json. Gate result {'rank': 0, 'field': 'switch marker count', 'observed': 0, 'expected': 1}.

FP2-P1 build670 N1 trial 2 qualified in 1527.192 s after the same terminal-log marker repair. The N1 pair is exact across all compared fields. Both tests passed 36 assertions with one marker each. Continue N6 trial 1.

FP2-P1 build670 N6 trial 1 pass 1 elapsed 712.081 to 713.523 s across ranks. The inherited FP2-C3 continuation evaluator and FAST_PATH_STAGE2_C3.md section 5 make the 600 s threshold an observation. It is not a failed result gate. Rank 5 Bound rose from 386.241 to 680.694 s beside build664, and rank 5 step C from 405.830 to 700.905 s. Pass-1 faces and C-removal counts matched build664 per rank. Leave the trial running for the actual qualification and historical projection.

FP2-P1 build670 step 2, campaign-N6-1: passed. Exit 0. Evidence campaign/build670/N6/trial1/case_summary.json. Gate result not evaluated.

FP2-P1 build670 N6 trial 1 qualified in 2746.051 s, compared with sealed build664 at 1507.171 s. Hand-over 24530 faces, 368098 final cells. First historical difference {'row': 'FP2_B proof', 'rank': 0, 'index': 1, 'field': 'modeled_peak', 'expected': 991683316, 'observed': 991700828, 'pass': 'not available'}. Continue N6 trial 2.

FP2-P1 build670 step 2, campaign-N6-2: missing receipt. Exit 1. Evidence campaign/build670/N6/trial2/case_summary.json.

FP2-P1 build670 step 2, compare: OPEN. Exit 75. Evidence campaign/build670/comparison.json.

FP2-P1 build670 step 2, anatomy-N1: missing receipt. Exit 1. Evidence anatomy/build670/N1/trial1/case_summary.json.

FP2-P1 build670 N6 trial 2 preflight declined launch when three unrelated vtk-rebase-build compiler processes were active. No N6-repeat test_launch.json or test child exists. No wrapper attempt was consumed. The partial campaign comparison is OPEN with N6 repeat not evaluated. Continue independent ANATOMY and shared checks. After the host clears and the owned service is terminal, run the original N6 repeat with the same pinned build and 4522 s watchdog, preserve the partial comparator output, and recompute the comparisons.

FP2-P1 independent ANATOMY and first-collapse preparations also declined launch while the unrelated VTK compilers were active. Verified that the original stage 2 service and all owned native processes are terminal. The resume service applies the unchanged host preflight and waits for it to clear before each missing native launch. It reuses the same prepared inputs for N6 trial 2 and ANATOMY, leaves the completed five trials untouched, and preserves the provisional comparison as comparison_before_n6_preflight_resume.json. The N6-repeat and ANATOMY watchdog attempts are still unused.

FP2-P1 stage 2 recovery launcher corrected a Python string escaping error before any native work. Active standby service su2-fp2-p1-build670-step2-recovery.service. The prior resume service stopped at Python parse time.

FP2-P1 build670 step 2, campaign-N6P-1: passed. Exit None. Evidence campaign/build670/N6P/trial1/case_summary.json. Gate result not evaluated.

FP2-P1 build670 step 2, campaign-N6P-2: passed. Exit None. Evidence campaign/build670/N6P/trial2/case_summary.json. Gate result not evaluated.

FP2-P1 build670 step 2, campaign-N1-1: passed. Exit None. Evidence campaign/build670/N1/trial1/case_summary.json. Gate result not evaluated.

FP2-P1 build670 step 2, campaign-N1-2: passed. Exit None. Evidence campaign/build670/N1/trial2/case_summary.json. Gate result not evaluated.

FP2-P1 build670 step 2, campaign-N6-1: passed. Exit None. Evidence campaign/build670/N6/trial1/case_summary.json. Gate result not evaluated.

FP2-P1 five qualified trial receipts and both completed exact pairs are pushed in evidence checkpoint a920cd3c1f. The recovery service is waiting at the existing host preflight. The next native launch is N6 trial 2 with unchanged pins and watchdog.

FP2-P1 build670 step 2, campaign-N6-2: passed. Exit 0. Evidence campaign/build670/N6/trial2/case_summary.json. Gate result not evaluated.

FP2-P1 build670 N6 trial 2 qualified in 2250.350 s. The same-binary N6 projection is exactly equal. Both trials recorded the same proof work 385372991, modeled peak 991700828, classified 24530 and strict mismatches zero, with different clocks. All six lab trials are now qualified, and all three pair projections are equal. Next seal the comparator and run N1 ANATOMY and the closure-on shared checks.

FP2-P1 build670 step 2, compare: passed. Exit 0. Evidence campaign/build670/comparison.json.

FP2-P1 build670 step 2, anatomy-N1: FAILED. Exit 1. Evidence anatomy/build670/N1/trial1/case_summary.json. Gate result {'row': 'FP2_B sweep', 'rank': 0, 'index': 14, 'field': 'enabled', 'expected': None, 'observed': 1, 'pass': 1}.

FP2-P1 build670 ANATOMY qualified in 1996.941 s. The original merged log inserted the enabled marker in a stdout sweep row, the same receipt defect as both N1 trace-off trials. Preserved the raw and corrected receipts. The trace observer completed and the reconstructed inherited projection is exactly equal to qualified N1 trial 1. Each trace saw 72564 completed forwards. Across 70362 grouped miss/bypass forwards, M2 versus P1 result work is 480614358 / 703296684, prepared nodes 7552490 / 50438362, and root-enough counts 51772 / 52249. P1 recorded 135091 fans and 68690 successful scans. The 200406006-byte raw sample stays local with its SHA256. The closure-on first-collapse safeguard is active.

FP2-P1 build670 step 2, first-collapse: FAILED. Exit 1. Evidence shared/build670/closure1/first-collapse/case_summary.json. Gate result {'first-collapse': 'upper_work_pass'}.

FP2-P1 build670 closure-on first collapse FAILED at work 111921030 / 108459426. Accepted counts 480 / 473 / 346 / 263 / 469 / 464 match, zero upper refinements, corrected sampled peak sum 7934201856 bytes, all six markers present. One read-only R5 pass traced the entire 21111161 work increase beside closure off to scope building (+10787910) and traversal (+10323251), with 12494 builds and 22889 upper calls in both runs. Rechecked the signed Work call sites and scope-miss placement. No transcription or wiring error was found. Preserve the closure-on failure and default-off code. No production correction or repeat. Independent exact C492, merit481, saved-current and native2d continue. S cannot launch.

FP2-P1 build670 step 2, exact-c492: FAILED. Exit 1. Evidence shared/build670/closure1/exact-c492/case_summary.json. Gate result {'bl': 'test exit'}.

FP2-P1 build670 closure-on exact C492 first attempt ended at the unchanged 600 s wrapper with exit 124. All sampled processes are terminal and the input pins verified. The completed stage-exit row has 1046 splits and zero upper refinements. No failed result gate is present in the terminal log. Preserve this attempt and retry C492 one time with the same watchdog after the remaining independent shared checks.

FP2-P1 build670 step 2, merit481: passed. Exit 0. Evidence shared/build670/closure1/merit481/case_summary.json. Gate result {'merit481': 'passed'}.

FP2-P1 build670 step 2, saved-current: OPEN. Exit 0. Evidence shared/build670/closure1/saved-current/case_summary.json. Gate result {'saved_current': {'field': 'trajectory_working_owned.bin', 'reference': '31139b0f1766dddb7122b0d3aa6d7378c37b59f9ece2fe472ef1d82327c478aa', 'candidate': '207e9e66096ec10264a6bb78ca646cb8ffa47197596019e7b7f543055087b605'}}.

FP2-P1 build670 step 2, native2d: passed. Exit 0. Evidence shared/build670/closure1/native2d/case_summary.json. Gate result {'2d1': 'passed', '2d2': 'passed', '2d4': 'passed'}.

FP2-P1 closure-on merit481 and native2d at 1 / 2 / 4 ranks passed. Saved-current ran successfully and is OPEN for the historical trajectory_working_owned.bin comparison, whose SHA differs. The independent step 2 service is terminal. Submitted the one same-limit C492 wrapper retry under su2-fp2-p1-build670-c492-retry. First collapse remains FAILED and S is gated out.

FP2-P1 build670 C492 retry ended at the unchanged 600 s wrapper with exit 124. Catch's sole failure line was its report of wrapper SIGTERM. Verification found no earlier result failure. The stage recorded 822 splits, zero upper refinements and the same bounded time-budget stop used by the qualified closure-off fixture. Preserve both attempts and classify C492 OPEN. Lab checkpoint p1_lab_checkpoint.json is sealed FAILED for first-collapse work. All six trials qualified, all three pairs are equal, and ANATOMY passed. Merit481 and native2d passed. Saved-current is OPEN for the historical comparison. No owned test, solver or MPI launcher is live. Commit the lab FAILED label, then record S and its parts as not evaluated.

FP2-P1 lab FAILED checkpoint a91e7f437e pushed. S is not evaluated because the closure-on first-collapse work gate failed. The S record marks both events, isolated restart, per-rank timers, hand-over and mesh counts, job peak and six-payload comparison not evaluated. The default-off implementation is retained. The report and handbook entries are complete. Close with the exact S OPEN label and stop after verifying the push.

## FP2-P2 start and R4 source check

FP2-P2 order archived and SHA256 verified (8e81a43642858f677b906dc5086d3bf593863e97be14e558ffa0a34d19522f76). Base and tracking tip dec45f8a71. No owned build, test or solver process or service. Tracked Common, SU2_CFD and UnitTests clean. Two pre-existing untracked CNativeDistributed3D .orig files left alone. Host precheck observed 35,158 MiB available. Whole-item pre-edit source check recorded in FP2_P2/reports/source_check.md. Next implement vertex selection, check the new hidden cases and run the closure-off qualification.

FP2-P2 production edit is confined to the two witness files. Build671 failed on hidden fixture aggregate initialization and was invalidated by a final hidden-fixture edit that overlapped pinning. Preserved its logs and source manifest. The fixture owner corrected the point initializers and confirmed the final test source frozen before build672. Build672 launched at nice 19, with 35,312 MiB available and no owned job present at launch. Next verify binaries and execute the hidden and inherited tests.

FP2-P2 build672 step 1, direct: passed. Exit None. Evidence fixtures/build672/direct/case_summary.json.

FP2-P2 build672 step 1, public-off: passed. Exit 0. Evidence fixtures/build672/public-off/case_summary.json.

FP2-P2 build672 passed, no warnings. Solver SHA256 05686918907f15e159c786716055015485377a7951e68882c8af56a70075b647 and test-driver SHA256 3859f2c750bee004632c516274fd5ef1af51d2a107e8ad1b68f597a5784db24e. Hidden direct check passed 3,464 assertions in two cases, including both analytic certificates, net live counts, inclusive endpoint work, and the cap. Build and test terminal receipts verified. The step 1 supervisor now runs public, G1, inherited fixtures, N1 control and shared closure-off checks serially under their preflights. D1 remains gated on the step 1 checkpoint.

FP2-P2 build672 step 1, public-on: passed. Exit 0. Evidence fixtures/build672/public-on/case_summary.json.

FP2-P2 checkpoint 687cde6b39 pushed. Both public-query processes and their cross comparison passed. The improvement uses 253 prepared nodes with closure off and 1 with closure on, with 2 and 1 footprint groups. The cropped witness split once (4 to 5 chords) and retained the qualified lower enclosure. Complete/cropped classifications and rebuild bits passed. Evictions were 60 off and 68 on. G1 serial is active.

FP2-P2 build672 step 1, G1-serial: passed. Exit 0. Evidence fixtures/build672/G1-serial/case_summary.json.

FP2-P2 build672 step 1, G1-mpi2: passed. Exit 0. Evidence fixtures/build672/G1-mpi2/case_summary.json.

FP2-P2 build672 step 1, G1-mpi4: passed. Exit 0. Evidence fixtures/build672/G1-mpi4/case_summary.json.

FP2-P2 build672 step 1, off-serial: passed. Exit 0. Evidence fixtures/build672/off-serial/case_summary.json.

FP2-P2 build672 step 1, off-mpi: passed. Exit 0. Evidence fixtures/build672/off-mpi/case_summary.json.

FP2-P2 build672 step 1, curved: passed. Exit 0. Evidence fixtures/build672/curved/case_summary.json.

FP2-P2 build672 G1 identity passed 375, 114, 228 with no unexpected changes. Inherited focused Catch counts were serial 10,314 assertions in nine cases, MPI two ranks 898 / 727 assertions in three cases each, and curved 727 assertions in one case with both checkers passed. Start the closure-off N1 control with the qualified G1 and inherited fixture pins.

FP2-P2 build672 step 1, default-N1: passed. Exit 0. Evidence default/build672/N1/trial1/case_summary.json. Gate result equal.

FP2-P2 build672 N1 control is qualified and exactly equal to sealed build664, including modeled peak. Control elapsed 1151.251 s, step C 913.506 s, Bound 811.483 s. Ten distinct stage keys and complete observer receipt. Work sums classification 463538756, traversal 416021808, scope build 6651129. All 36 Catch assertions passed and the owned processes are terminal. The step 1 sequence proceeds through the default-off shared safeguards.

FP2-P2 build672 step 1, first-collapse: passed. Exit 0. Evidence shared/build672/closure0/first-collapse/case_summary.json. Gate result {'first-collapse': 'passed'}.

FP2-P2 build672 closure-off first collapse passed all four hard checks and the historical decision comparison. Work 90809869 / 108459426, zero upper refinements, six captured ranks. Sampled sum of solver process peaks 7972147200 bytes / 19999490048 bytes, converted from procfs KiB. Controlled stop at 263.777 s after all six first-collapse exit rows. Next C492 and the rest of the default-off shared checks.

FP2-P2 build672 step 1, exact-c492: passed. Exit 0. Evidence shared/build672/closure0/exact-c492/case_summary.json. Gate result {'bl': 'passed'}.

FP2-P2 build672 step 1, merit481: passed. Exit 0. Evidence shared/build672/closure0/merit481/case_summary.json. Gate result {'merit481': 'passed'}.

FP2-P2 build672 step 1, saved-current: FAILED. Exit 1. Evidence shared/build672/closure0/saved-current/case_summary.json. Gate result {'saved_current': "missing evaluation: [Errno 2] No such file or directory: '/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/phase_N6/safeguards/cases/curved505_current_target_surface_corrections_serial_20261118T172009_2039448/trajectory_working_owned.bin'"}.

FP2-P2 build672 step 1, native2d: passed. Exit 0. Evidence shared/build672/closure0/native2d/case_summary.json. Gate result {'2d1': 'passed', '2d2': 'passed', '2d4': 'passed'}.

FP2-P2 build672 saved-current initial evaluator failure was a test-harness defect. It could not open two removed historical payloads after the child passed. The same terminal run passed on the authoritative N6 SHA256 values, corroborated by the committed exact build670 reference comparison, plus the original N6 planning rows. All three candidate hashes and the planning rows matched. Original summary and raw evidence retained. Native2d at 1/2/4 ranks passed. Step 1 controller is terminal. Seal the fixture checkpoint, commit, then run D1.

FP2-P2 fixtures-passed checkpoint 745b8c82bf pushed. Step 1 sealed on build672. D1 submitted under su2-fp2-p2-build672-d1 with its inherited N1 watchdog and frozen same-build control. No D2 or step 2.3 work queued. Seal D1 before deciding D2.

FP2-P2 build672 step D1, campaign-N1-1: passed. Exit 0. Evidence campaign/build672/N1/trial1/case_summary.json. Gate result not evaluated.

FP2-P2 build672 D1 sealed passed, promote=True. Evidence build672/d1_promotion.json.

FP2-P2 D1 qualified on build672 in 640.378 s. Both selected N1 receipts have the ten stage keys and a complete observer at identical settings except closure. Gate D passed, classification 156105696 versus control 463538756 (−307433060 units); step C 419.957 versus 913.506 s, Bound 319.598 versus 811.483 s, traversal 77469988 versus 416021808, scope build 37769889 versus 6651129. The merged startup marker was proven and removed from the derived projection only; raw stdout and pin receipt preserved. D1 promotion is sealed. Next D2 first collapse.

FP2-P2 D1 promotion checkpoint 959caa227e pushed. D1 supervisor terminal. D2 submitted under su2-fp2-p2-build672-d2, guarded by the D1 seal. No step 2.3 trials or safeguards are queued until D2 is sealed.

FP2-P2 build672 step D2, first-collapse: passed. Exit 0. Evidence shared/build672/closure1/first-collapse/case_summary.json. Gate result {'first-collapse': 'passed'}.

FP2-P2 build672 D2 sealed passed, promote=True. Evidence build672/d2_promotion.json.

FP2-P2 D2 passed and promotion is sealed. observed_work: 76652506, work_limit: 108459426, observed_solver_peak_sum_bytes: 6125809664, resource_limit_bytes: 19999490048, seconds_until_stop: 207.34979057405144, captured_ranks: [0, 1, 2, 3, 4, 5]. Zero upper refinements, historical adopted counts 480 / 473 / 346 / 263 / 469 / 464 exact, all six switch markers, all owned processes terminal. Next step 2.3 under both promotion checks.

FP2-P2 D2 checkpoint fdf6ae613f pushed and supervisor terminal. Submitted step 2.3 under su2-fp2-p2-build672-step23. Both promotions are required. Sequence: N1 repeat, N6P pair, N6 pair, pair comparison, traced N1, and the four remaining shared safeguards with closure on. S remains gated.

FP2-P2 build672 step 2, campaign-N1-2: passed. Exit 0. Evidence campaign/build672/N1/trial2/case_summary.json. Gate result not evaluated.

FP2-P2 N1 trial 2 qualified in 645.077 s with the same 156105696 classification, 77469988 traversal and 37769889 scope-build work. The raw marker interleaving was verified and the derived log pinned. The same-binary N1 pair has an empty comparison. Next N6P trial 1.

FP2-P2 build672 step 2, campaign-N6P-1: passed. Exit 0. Evidence campaign/build672/N6P/trial1/case_summary.json. Gate result not evaluated.

FP2-P2 N6P trial 1 qualified in 494.160 s, six markers, peak lower bound 4355772416 bytes, hand-over 26482 faces and 371837 final cells. Next N6P trial 2 on identical inputs.

FP2-P2 build672 step 2, campaign-N6P-2: passed. Exit 0. Evidence campaign/build672/N6P/trial2/case_summary.json. Gate result not evaluated.

FP2-P2 N6P trial 2 qualified in 536.812 s, and the same-binary N6P pair compared exactly. Both trials record 26482 hand-over faces and 371837 cells. Next N6 trial 1.

FP2-P2 build672 step 2, campaign-N6-1: passed. Exit 0. Evidence campaign/build672/N6/trial1/case_summary.json. Gate result not evaluated.

FP2-P2 N6 trial 1 qualified in 742.555 s, six ranks, 4266397696-byte sampled lower bound, hand-over 24530 faces and 368098 final cells. First build664 comparison difference is rank 0 FP2_B proof modeled_peak, 991683460 versus 991683316. Historical comparison is report-only. Next N6 trial 2.

FP2-P2 build672 step 2, campaign-N6-2: passed. Exit 0. Evidence campaign/build672/N6/trial2/case_summary.json. Gate result not evaluated.

FP2-P2 build672 step 2, compare: passed. Exit 0. Evidence campaign/build672/comparison.json.

FP2-P2 N6 trial 2 qualified in 792.215 s. N6 pair exact. The sealed comparator passed all six trials and all three pairs (N1, N6P, N6). Build664 comparison is exact for N1 and has report-only differences for N6P and N6. Continue traced N1 ANATOMY, then the four closure-on shared checks.

FP2-P2 build672 step 2, anatomy-N1: passed. Exit 0. Evidence anatomy/build672/N1/trial1/case_summary.json. Gate result equal.

FP2-P2 ANATOMY qualified in 680.214 s with a complete observer and an exact trace-off N1 projection. Across 70482 completed miss/bypass forwards: 61785 root enough, 412482 prepared nodes, 174429681 result work. The 68858 PLANES rows report 72944 groups, 72765 accepted, 8 invalid and 931 boundary trimmed. The closure committed 197387 fans and left 649 contacts. Successful scans 68858, failed scans zero, incomplete closure rows zero; cap zero. The raw 99700922-byte sample stays local with SHA256 22f915db427141c6b315a0db49b147e38fdfa8b99ae09826910a025d12992db1. The remaining closure-on shared checks are in sequence.

FP2-P2 build672 step 2, exact-c492: passed. Exit 0. Evidence shared/build672/closure1/exact-c492/case_summary.json. Gate result {'bl': 'passed'}.

FP2-P2 build672 step 2, merit481: passed. Exit 0. Evidence shared/build672/closure1/merit481/case_summary.json. Gate result {'merit481': 'passed'}.

FP2-P2 closure-on C492 passed the bounded-state checks with 1794 positive splits, zero upper refinements and the original input, target and budgets. As the C492 contract requires, recorded the first differing edit from RunA, index 32 (zero-based), edge [24242, 24427] versus [24240, 24242]. The merit481 check passed. Saved-current and native2d follow.

FP2-P2 build672 step 2, saved-current: OPEN. Exit 75. Evidence shared/build672/closure1/saved-current/case_summary.json. Gate result {'saved_current': {'field': 'trajectory_working_owned.bin', 'reference': '31139b0f1766dddb7122b0d3aa6d7378c37b59f9ece2fe472ef1d82327c478aa', 'candidate': '207e9e66096ec10264a6bb78ca646cb8ffa47197596019e7b7f543055087b605'}}.

FP2-P2 build672 step 2, native2d: passed. Exit 0. Evidence shared/build672/closure1/native2d/case_summary.json. Gate result {'2d1': 'passed', '2d2': 'passed', '2d4': 'passed'}.

FP2-P2 all closure-on shared runs and supervisor terminal. Native2d passed at 1/2/4 ranks; saved-current qualified in execution and is OPEN on its historical trajectory_working_owned.bin SHA, matching FP2-P1 closure-on. Lab status OPEN under the signed rule, with no failed part. Both promotions, six qualification receipts and all three exact pairs are available to the S launcher.

FP2-P2 lab OPEN checkpoint f832a4af2b pushed. Saved-current's historical difference is the only OPEN part. The lab checkpoint and both promotions were sealed on build672. Submitted a single closure-on, trace-off S workflow and its dependent isolated restart under su2-fp2-p2-build672-s. The launcher verifies both promotions, all three equal pairs and the existing run-S preflights before launching native work.

FP2-P2 S setup stopped before any launch receipt. The P2 adapter replaced the inherited helper basename in a historical path and looked for a nonexistent FP2_D/run_s.py. Preserved the setup-only receipt. Corrected the test driver binding to retain the C3 historical name and use the P2 entry point only for the owned child. Self-checked both paths and inherited S reference/watchdogs. No S attempt or retry was consumed; resume S with unchanged pins and limits.

FP2-P2 corrected S supervisor su2-fp2-p2-build672-s-recovered launched after the original setup service was terminal. The archive has no native or restart launch receipt. Next monitor its full receipt and isolated restart.

FP2-P2 next S attempt was rejected as a launcher-wiring defect: all six actual solver environments omitted both switches. Verified the launcher ancestry against the pinned controller, terminated only the owned MPI launcher and confirmed all handles terminal with no completed event. Preserved S_invalid_switch_launch. Bound both generated scripts inside the inherited campaign namespace. Dry-checked the real generated full and effective restart scripts and added a prelaunch assertion. No eligible closure-on S child has run yet.

FP2-P2 corrected generated-script supervisor su2-fp2-p2-build672-s-bound submitted. Verify recorded process switches and actual six-rank markers as soon as the solver starts.

FP2-P2 correct S native job launched at 19:24:01 CET. Provenance and all six live owned solver environments attest closure=1 and trace=0 with one MPI export for each; one enabled marker on each rank. Both prior R3 setup defects are preserved, and the first qualifying S attempt is running.

FP2-P2 S event 1 p7p8 proof/resource and f3b timer checks passed. Owned proof faces sum and each rank's surface count are 25188. Job peak and summed process peaks both 9586192384 bytes within 19999490048 bytes. First adapted mesh write observed. Continue uninterrupted to event 2 and the dependent isolated restart.

FP2-P2 S event 2 p7p8 proof/resource and f3b timer checks passed. Owned proof faces sum 20238 matches the surface size on all six ranks. Job and summed process peaks 9686908928 bytes. Continued CFD is active; await terminal workflow validators and isolated restart.

FP2-P2 build672 S completed and the supervisor is terminal. The workflow accepted both events with continued CFD and isolated restart. Native maxima 655.988 / 350.407 s, minimum per-rank timer coverage 98.015% / 99.000%. Proof faces 25188 / 20238, hand-over faces 26480 / 21268, cells 372715 / 352661, peak job RSS 9586192384 / 9686908928 bytes. Isolated restart verified in 56.881 s with zero coordinate error and unchanged pins. All six payload hashes differ from the sealed build664 S. No owned native process remains. The exact final label is FP2-P2: S passed; default path not yet proven. After the report and handbooks are committed and pushed, stop.

## FP2-P3 start and R4 source check

Archived signed FP2_P3_CLOSURE_DEFAULT.md and verified SHA256 16fec22406c931490718e23e58da2840cd4bb05b1748189d6a12e14bff5c05e5. HEAD and tracking tip both 8f1325ef35. No owned native process or service. Common, SU2_CFD and UnitTests tracked sources clean, with the two pre-existing untracked .orig files left in place. R4 whole-item pre-edit source check in FP2_P3/reports/source_check.md. Next preserve build672 for any required control diagnosis, change the process predicate and marker only, and build the frozen pair.

FP2-P3 Part A build673 passed with no warnings. Solver 9f766f8b188c21d44faba37b7e6a7b0b924562fc045fb3baff638d0125311e08, test driver b6f2def1c88f60007ebe397d255ec7711dd37871d3d71142b54a007595f47da3, source manifest 79362d52fe64867ce40f0959f9488248e399acd8af531416a9f795c3bbda4944. Build672 control pair preserved locally and its two hashes verified. Next B0 focused/curved and public-off, then B1 N1 only after B0 passes.

FP2-P3 build673 B0 passed. Evidence build673/B0.json.

FP2-P3 B0 sealed passed. The serial, two-rank and curved fixtures match build668 Catch counts; the two curved trace checks and hidden public-off proof passed. Focused custom-geometry ranks have no witness-query entry, and the curved and public-off serial ranks show one enabled=0 marker each. Next B1 on the pinned closure-off configuration.

FP2-P3 B0 checkpoint b36b116e75 pushed. B1 began under su2-fp2-p3-build673-b01 at 21:19:31, with the specified closure-off setting, M2 observer only, frozen 1E-4 config and 3261-second first wrapper. B2a is queued behind a terminal B1 supervisor and a passed B1 seal.

FP2-P3 build673 B1 passed. Evidence build673/B1.json.

FP2-P3 B1 passed in 1189.192 s, complete observer and ten stage rows. Classification 463538756, traversal 416021808, scope build 6651129. The build664 projection, including modeled peaks, matched exactly. One enabled=0 marker with raw and byte-rejoined logs retained. B2a G2 launched after the terminal B1 seal.

FP2-P3 B1 checkpoint 1b9968e2b7 pushed. The B2a supervisor is active with one G2 job. The B2b then B2c service will proceed only after B2a is terminal and its passed seal verifies. It stops on the first unresolved control checkpoint.

FP2-P3 build673 B2a/G2 passed. Receipt /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FP2_P3/scoped/build673/B2a/G2.json.

FP2-P3 build673 B2a/GQ passed. Receipt /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FP2_P3/scoped/build673/B2a/GQ.json.

FP2-P3 build673 B2a/G3 passed. Receipt /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FP2_P3/scoped/build673/B2a/G3.json.

FP2-P3 B2a exact G2 and GQ passed, including scheduling calls and fixedstorage=4120. G2 stopped on all six first-collapse rows at 267.686 s. GQ reached the third quality-window row on all six ranks. G3 passed physically with 1473 positive splits above the 336 minimum and zero upper refinements. All 1113 common N14 edits matched, and observer-adjusted entry work was 5113 on both. G4 and the three saved-state controls follow.

FP2-P3 build673 B2a/G4 passed. Receipt /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FP2_P3/scoped/build673/B2a/G4.json.

FP2-P3 build673 B2a/merit481 passed. Receipt /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FP2_P3/scoped/build673/B2a/merit481.json.

FP2-P3 build673 B2a/saved481 FAILED. Receipt /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FP2_P3/scoped/build673/B2a/saved481.json.

FP2-P3 B2a initial saved481 red receipt diagnosed as an R3 launch-input mismatch. The actual child failed one Catch assertion before any independent witness-query entry. The N6 saved481 command pins curved492 producer artifacts and scalars 0.0047263575870155755 / 0.28717458874925855; the legacy P3 adapter omitted N6's saved481 binding and launched curved441, which the checkpoint importer rejected. Frozen config bytes match, and merit481 and saved-current command contracts match their own historical inputs. Preserve this attempt, finish saved-current, then re-evaluate only saved481 under the exact N6 binding.

FP2-P3 build673 B2a/saved-current passed. Receipt /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FP2_P3/scoped/build673/B2a/saved-current.json.

FP2-P3 build673 B2a FAILED. Evidence build673/B2a.json.

FP2-P3 B2a reselected from the corrected saved481 attempt. passed. Receipt /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FP2_P3/build673/B2a.json.

FP2-P3 B2a reselected passed. Corrected saved481 used the exact N6 curved492 inputs, recorded one off marker, and passed all historical payload and planning comparisons. Original rejected-input launch and both B2a selections are sealed. Saved-current also passed its historical comparison. The held full OFF then CORE supervisor may now proceed.

FP2-P3 B2a checkpoint c1e795ba45 pushed. B2b OFF launched at 22:35:19 CET on build673 with exact FX2 config bytes and explicit closure=0 / trace=0 set and exported. B2c and aggregate B are conditional on the passed B2b seal.

FP2-P3 B2b OFF first-collapse anchor passed with configured historical equality. All six captured, zero upper refinements, work 90809869 against the 108459426 limit. Every rank recorded one enabled=0 marker. Continue the full two-event run.

FP2-P3 B2b OFF event 1 proof/resource and timer gates passed. Owned proof faces sum 61906, job and summed process peak 14021840896 bytes. Continued CFD proceeds to event 2.

FP2-P3 B2b OFF completed both events and continued CFD at 23:36 CET. Event 2 proof count 58260 on every rank and by owned sum, and job peak and summed process peaks both 16852041728 bytes. Both timer gates passed. All six disk payloads match N21 W. Isolated restart launched at 23:37:48; the FX2 row gate and B2b seal still follow.

FP2-P3 build673 B2b FAILED. Evidence build673/B2b.json.

FP2-P3 B2b OFF isolated restart passed in 276.890 s with unchanged pins and zero coordinate error. The exact FX2 row comparison was zero. The first terminal seal instead stopped at the switch receipt's rank parser, which selected the scheduler's nice -n 19 rather than mpirun -n 6. Both executed commands and per-rank logs retain the specified settings and markers. R3 recovery revalidates the same completed events and restart without a new native launch; B2c waits for the recovered B2b seal.

FP2-P3 build673 B2b passed. Evidence build673/B2b.json.

FP2-P3 B2b OFF was re-evaluated from the original completed build673 full and restart children, passed the switch receipting correction, and sealed at 23:46:06 CET. Exact FX2 row differences 0, both events and six N21 W payload identities passed. Native maxima 1984.549 / 1187.970 s, minimum per-rank coverage 98.963% / 99.419%. B2c CORE started at 23:46:23 after the B2b seal; aggregate B follows only if B2c passes.

FP2-P3 B2b passed checkpoint 1cb8d428d2 pushed. B2c has closure=0 and trace=0 in the actual launch, with one OFF marker on each of six ranks and the exact FP1 configuration SHA256 1ec6c26d49d4ec7c62103d5bd096471b5422290cd06c44b2f1073b7489f2b4c1. Continue the two-event CORE control and its isolated restart.

FP2-P3 B2c CORE captured all six first-collapse rows. The inherited CORE observation reports work 92045474, zero upper refinements and accepted counts 465 / 459 / 428 / 573 / 451 / 445. Comparison to historical OFF work and counts is report-only in this mode. Event 1 continues.

FP2-P3 B2c CORE event 1 proof, memory and timer gates passed. Owned proof sum 68006, job and summed process peaks 11572654080 bytes. The event-1 mesh and sidecar match the two FP1 hashes. Continued CFD is active; the transferred restart payload and event 2 follow.

FP2-P3 B2c CORE continued CFD entered event 2. The event-1 transferred restart also matches FP1, giving all three event-1 disk identities.

FP2-P3 B2c CORE event 2 proof, memory and timer gates passed. Owned proof sum 64758 and all six surface sizes agree. Job and summed process peaks both 11649990656 bytes. Continued CFD and the final disk payloads are in progress, followed by isolated restart.

FP2-P3 B2c CORE solver exited normally after continued CFD. The inherited full workflow passed and all six disk payloads exactly match FP1. The isolated restart started at 00:31:01 CET. Its terminal validation and switch receipts are still required for B2c and aggregate B.

FP2-P3 build673 B2c passed. Evidence build673/B2c.json.

FP2-P3 build673 B passed. Evidence build673/B.json.

FP2-P3 B2c CORE passed both events, continued CFD, all six exact FP1 payload hashes and the isolated restart. Native maxima 1808.734 / 496.878 s, minimum per-rank timer coverage 98.956% / 98.552%, isolated restart 162.568 s and coordinate error 0.0. All owned handles terminal. Part B has a passed aggregate seal.

FP2-P3 B2c checkpoint d5abf580b4. All five closure-off stages passed. Commit the Part B label and start Part C with C1 on frozen build673.

FP2-P3 Part B label 4d77e303c0 pushed as "FP2-P3: default proven with the closure off"; B2c passed checkpoint d5abf580b4 pushed. Started C1 under su2-fp2-p3-build673-c1 after the pushed label. C1 checks G1, the focused and curved fixtures and both public-query settings plus their comparison. Next C2, then C3 and C4 in order.

FP2-P3 C1 default G1 matched the exact adopted case, result and failed-assertion tables for 375 serial, 114 two-rank and 228 four-rank records. Elapsed 782.380 / 10.440 / 13.882 s. The focused serial and MPI checks passed; curved and hidden queries proceed.

FP2-P3 build673 C1 passed. Evidence build673/C1.json.

FP2-P3 C1 passed all ten sealed checks. Focused and curved fixtures completed, the curved trace and leaf pins passed, and the direct and public fixtures passed. In the public improvement case, the prepared physical nodes were 253 off and 1 on, with footprint groups 2 and 1. The complete and cropped lower-bound and cache checks also passed. C1 controller terminal. Next C2, beginning with the hard first-collapse checks.

FP2-P3 C1 passed checkpoint eaa458e420 pushed. C2 service su2-fp2-p3-build673-c2 launched first collapse, exact C492, merit481, native 2D at 1/2/4 ranks and the two saved_current executions on build673 in sequence. Production diff remains confined to ConformingWitness().

FP2-P3 C2 first-collapse setup stopped before native launch. The inherited validator requires disjoint primary and supplemental pin maps. The P3 adapter added the signed order and checksum again even though the primary build manifest already pins both files byte for byte; it stopped at the duplicate-key assertion before writing a case status. This is an R3 launcher defect. Preserve the original setup, let C492 and the other independent C2 checks finish, then run first collapse with the deduplicated manifest on the same pair and unchanged gates.

FP2-P3 C2 exact C492 passed its executed-state checks with the unchanged 388.8 s configured stage and 1646 positive splits, zero upper refinements, and byte-identical frozen input and target. First RunA edit difference is zero-based index 32, edge [24240,24242] versus [24242,24427]. That comparison is report-only. Merit481 follows, then native 2D and the saved_current pair.

FP2-P3 C2 C492 clocks: native bounded work stopped with time_budget at the configured 388.8 s selection deadline. The saved-state 461.175 s includes the subsequent terminal proof and result accounting in EngineControl::Finish. The inherited test exit was zero and all executed-state checks passed. The clocks are reported separately.

FP2-P3 C2 merit481 and native 2D at 1, 2 and 4 ranks passed. The saved_current pair is in progress. First-collapse repair is queued behind the terminal C2 supervisor and a check that the original C2 seal's only red receipt is the duplicate-pin launcher error.

FP2-P3 build673 C2 FAILED. Evidence build673/C2.json.

FP2-P3 C2 saved_current passed both executions and their direct disk SHA256 and planning comparison. All three payloads and all 1297 ordered planning rows are identical across the pair. The original C2 controller is terminal and marked FAILED only for the prelaunch first-collapse duplicate-pin defect. No C3 launch until the corrected first-collapse result is selected.

FP2-P3 C2 reselected passed. Corrected first collapse captured all six exits and stopped in 216.782 s against 1500 s. Work 76652506 against 108459426, sampled solver peak sum 8009142272 bytes against 19999490048, no upper refinements, and accepted-by-rank vector 480 / 473 / 346 / 263 / 469 / 464. No off markers at default on. C492, merit481, native 2D and the saved_current pair remain sealed and passed. The original setup error is archived. Both C2 controllers are terminal. Next C3 candidate prefixes.

FP2-P3 C2 passed checkpoint bfa3e03e88 pushed. C3 started under su2-fp2-p3-build673-c3. The controller sequence is two G2 captures, then two GQ captures on unchanged inputs and stopping anchors with the closure switch absent. Candidate-pair equality is the gate. Historical differences will be reported. C4 follows after terminal C3.

FP2-P3 build673 C3/G2 passed. Receipt /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FP2_P3/scoped/build673/C3/G2.json.

FP2-P3 C3 G2 passed both admissible captures and the exact candidate-pair comparator, including scheduling and the census rule. Both first-collapse work sums 76652506, the six accepted counts exact, zero upper refinements. The first historical change is rank 0 atlas_work 20350736 versus 15662380 at the first-collapse row. C3 GQ capture 1 started at 01:27:24 CET.

FP2-P3 C3 GQ capture 1 passed the unchanged third quality-window anchor on all six ranks, stopping after 666.881 s. Its embedded first-collapse gate passed at 76652506 work units. The controller was terminal before capture 2 started at 01:38:58 CET. Pair identity remains pending.

FP2-P3 build673 C3/GQ passed. Receipt /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FP2_P3/scoped/build673/C3/GQ.json.

FP2-P3 build673 C3 passed. Evidence build673/C3.json.

FP2-P3 C3 passed both GQ captures and their exact pair comparison, including scheduling calls and census. Each completed all six third quality windows and the embedded first-collapse gates. First historical GQ difference is the same rank-0 atlas_work as G2. GQ historical coverage by projected type includes 510 changed stage rows, 510 storage rows, 62 timer rows, one missing timer row and one extra timer row. The group comparison and ownership evidence are sealed. C3 supervisor terminal. Next C4 G3 physical check and rebound G4.

FP2-P3 C3 passed checkpoint 755e3015af pushed. C4 started under su2-fp2-p3-build673-c4. G3 runs with the closure variable absent and keeps its physical gates. The historical common edits and adjusted entry work are reported. G4 will re-evaluate the C2 native 2D executions against the historical summaries.

FP2-P3 build673 C4/G3 passed. Receipt /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FP2_P3/scoped/build673/C4/G3.json.

FP2-P3 build673 C4/G4 passed. Receipt /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FP2_P3/scoped/build673/C4/G4.json.

FP2-P3 build673 C4 passed. Evidence build673/C4.json.

FP2-P3 C4 passed G3 physically with 1592 positive splits above the 336 minimum, zero upper refinements and valid storage and resource checks. Across 1113 common edits, the first historical mismatch is zero-based 47, edge [24302,24303] versus [15048,24462]. Observer-adjusted work is 5113 versus 5113. G4 passed the historical Catch comparison for the same C2 native 2D executions at 1, 2 and 4 ranks. Supervisor terminal. C1 through C4 are passed. Next C7, two S workflows and isolated restarts.
