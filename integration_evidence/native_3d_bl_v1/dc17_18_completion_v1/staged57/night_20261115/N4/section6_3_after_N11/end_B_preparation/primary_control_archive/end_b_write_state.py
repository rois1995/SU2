from pathlib import Path
import json,hashlib,sys
N=Path('integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115');P=N/'N4/section6_3_after_N11';E=P/'end_B_preparation';mode=sys.argv[1];assert mode in ['qualified','stop'];qcommit=sys.argv[2] if mode=='stop' else 'pending qualification checkpoint; code-equivalent to daf75b14e2';summary=json.loads((E/'acceptance_summary.json').read_text());assert summary['phase_B_complete'];build=json.loads((P/'build558/build_status.json').read_text());manifest=json.loads((E/'qualified_build558/manifest.json').read_text());assert len(manifest['files'])==2
history=N/'progress_history_through_phase_B_qualification_20261117.md'
if not history.exists():history.write_text('# Historical progress — superseded by current progress.md\n\n'+(N/'progress.md').read_text())
hash_history=hashlib.sha256(history.read_bytes()).hexdigest();state='STOPPED — waiting for the user; no phase C' if mode=='stop' else 'PHASE B PASSED — qualification checkpoint next';audit='planned_stop_terminal_audit.json' if mode=='stop' else 'phase_end_after_archive_terminal_audit.json'
progress=f'''# Night work — current progress

Updated2026-11-17. Read the complete latest NIGHT_WORK_ORDER.md after compaction, then this file.

## Current state — {state}

Phase B passed on build558. Qualification commit: `{qcommit}`. Production source is identical to B8 checkpointdaf75b14e2a761ec1b8abaf5f5cd3e01a65ca7e3. No C++ change followed B8 application. All1781source pins and both current binary hashes were verified; owned C++ matches committed source. No owned job is running; actual PID/start identities are in `N4/section6_3_after_N11/end_B_preparation/{audit}`.

The latest user order requires STOP after phase B's end gates and commit/push, current state, no-owned-job verification and terminal handles. Do not start phase C. N6/N7/C/D are deferred. N10 is withdrawn; do not implement8.5ab. No question or automatic continuation is pending.

## Completed phase-B gates

- B1–B8 scoped identity checks passed numerically. G1 case/failure tables match375/114/228records, zero changes; accepted-base failures are explicitly preserved underC2.1.
- R1 correction: earlier B6/B7/B8 six-rank G2 runs had seven owned heavy processes because a draft renderer remained alive. They are NOT R1-compliant. The unchanged integrated558 source passed one corrected G2 run at the six-process bound: work90,809,869/counts480/473/346/263/469/464; stop316.078817s/sealed323.937712s; all old integer/model/timer rows exact. B6/B7/B8 R1 amendments retain the violation and corrected qualification.
- G3 PASSED617.688712s:1170splits>=336; all1113common edits exact against FIRSTN14G3; adjustedentry5113both; every physical gate passed;2036pins.
- G4 PASSED1/2/4ranks:6.492431/6.504021/6.514544s;1992pins per case.
- GQ PASSED1083.237551sstop/1091.436227ssealed: all6thirdqualitywindows/alloldinteger/model/timerrows exact/18timer snapshots/zeroCensusdelta;1781source/221input/21extrahelperpins. ObservedprefixrankHWMsum13,999,366,144B, notfull-workflowP8peak.
- Readability:153detectedprivate definitions/23lambdasall<=80/15; known12B6/9B7/31B8helpers found, maxima60/68/73. Adapt4998formattedlines isphaseDdeadline, notendBgate. Lexical limitations and written exceptions remain explicit.

Canonical report: `Handbook/ADAPTATION_CUT_PHASE_B_20261117.md`. Acceptance: `N4/section6_3_after_N11/end_B_preparation/acceptance_summary.json`. Found-during-cut list, all own-helper causes/corrections, baseline failures, ten longest functions and exceptions are preserved there and linked evidence.

## Actual terminal handles and helper recoveries

Build558exec92164; B8G1serial56651/MPI211596/MPI429843; originalG225119; G387148; G417899. All actual identities are terminal.

CorrectedG2originalcontrollerexec2592exited143while its solver session161908/start42865736continued. Adoptedmonitor exec29712 completed the SAME numerical run and exited0. GQoriginalcontrollerexec37929exited143; its SAME solver session190560/start42931623 continued under detachedsupervisor205985/start42968501 andmonitor205993/start42968508. Supervisor's wait recorded actualmonitor exit0. All these identities are terminal. Controller SIGTERM causes are unknown; no solver restarted and unavailable reparented solver wait statuses remain null, not fabricated.

Orphan draft renderer4076260/start42326144 andsandboxparent4076259/start42326143 are terminal. The first renderer stalled in SequenceMatcher, its running session ID was discarded, and a sandbox-local process check missed it. Newpreflight includes ownedPythonhelpers. Exact host evidence and correction are under `end_B_preparation/R3_orphan_generator`; controller recovery source/pins/receipts are in the two R3_controller_recovery folders. Setup-only missing-alive-field assumptions were corrected with direct PID/start verification retained. No production/gate/tolerance change was made for own helpers.

## Current binaries and last full workflow

Shared build: `integration_evidence/build-integrated-v3-p5-20261114`.

- CFD558 SHA256 `{build['binary_sha256']}`.
- Test558 SHA256 `{build['test_binary_sha256']}`.
- Independent read-only copies: `N4/section6_3_after_N11/end_B_preparation/qualified_build558/manifest.json`. Binary payloads are local only, not Git.

Last full two-event qualification remains N14/N15 build552/checkpoint0c2cb5c460: full4738.250709s, nativeevents2568.277632/1672.244847s, exactP8process-peak sums14,054,744,064/17,397,911,552B; all6payloads exactE1P6c/E2P7, strictphysical/quality/transfer/continuedCFD/timer/resourcegates andisolatedrestart236.822017s passed. N14G3first/second1113/1099splits andall1099commonedits; M26 samefullrun qualified. No extra G5 is prescribed at end B; build558 is phase-B-qualified, not a new full-workflow run. No acceptedBL/RANSclaim.

## Earlier work and history

N1/P7P8 accepted429c6c1a60. ConditionalcacheB7/N2 failed modeled12GiB and was reverted9831c787e7; CUT B7 is a separate Control extraction. N3 source check6941fcc41c, N5 measurements3d28237f03, N99867041a2e, N11 failed/reverted0d9d627645, N10withdrawn, N13/M25passed8ab6da791e, N14/N15passed0c2cb5c460. PhaseA5ec465ca1a passed full/restart. Detailed source/proposals, failures, gate numbers and historical handles remain in the linked reports.

Historical progress: `{history.name}`, SHA256 `{hash_history}`. Its old RUNNING/PENDING notes are historical; this file controls current state.

Next action: **STOP AND WAIT FOR THE USER. DO NOT START PHASE C.**
''';(N/'progress.md').write_text(progress)
handoff=f'''# Native adaptation — current handoff

Updated2026-11-17. **{state}.** Phase B passed on build558; qualification commit `{qcommit}`. No owned job runs. Current source is code-equivalent to B8 checkpointdaf75b14e2; all1781pins and binary hashes are verified. Latest actual terminal audit: `N4/section6_3_after_N11/end_B_preparation/{audit}`.

The user ordered this stop after phase B. **Wait for the user; do not start phase C.** C/D, N6 and final N7 are deferred. N10 is withdrawn and8.5ab is not implemented.

[Phase-B report](Handbook/ADAPTATION_CUT_PHASE_B_20261117.md) contains commits, gate numbers, baseline G1 failures, found-during-cut list, exceptions, ten longest functions and every own-helper correction. [Current progress]({N}/progress.md) holds handles and evidence links. Read the complete latest scratchpad NIGHT_WORK_ORDER.md and progress after compaction.

End gates: G3 PASSED1170splits/all1113commonedits exact/adjustedentry5113; G4 PASSEDat1/2/4ranks; GQ PASSEDall6thirdwindows/18timersnapshots/alloldinteger/model/callrows exact/zeroCensusdelta. Readability's153private definitions/23lambdas meet80/15; Adapt's80-line deadline remains phase D.

Resource correction: original B6/B7/B8 G2 numerical rows passed with an overlooked seventh owned process, so those runs are NOT R1-compliant. The verified orphan renderer was terminated and the same integrated558 source passed a corrected G2 run at the six-process bound. G2/GQ controllers later received SIGTERM for unknown reasons; their SAME solver runs were adopted without restart. Original signals and unavailable solver wait statuses are explicit; all actual handles are terminal. See report and R3 evidence.

Shared build: `integration_evidence/build-integrated-v3-p5-20261114`.

- CFD558 SHA256 `{build['binary_sha256']}`.
- Test558 SHA256 `{build['test_binary_sha256']}`.
- Source pins: `{P}/build558/source_pins.json`.
- Independent read-only558 binary copies and manifest: `{E}/qualified_build558` (payloads local, not Git).

Last full two-event qualification remains N14/N15 build552/checkpoint0c2cb5c460:4738.250709s, all6payloads exact, fullphysical/quality/transfer/continuedCFD/resource/timer gates andisolatedrestart passed. Nativeeventmaxima2568.277632/1672.244847s; exactP8process-peak sums14,054,744,064/17,397,911,552B. Its preserved references/binaries andM26report remain qualified. No G5 is scheduled at end B and no acceptedBL/RANSclaim is made.

N=`{N}`; P=`N/N4/section6_3_after_N11`. Phase-B acceptance/terminal receipts are inP/end_B_preparation andP/B8/validation. Whole-cut sourcecheckP/source_check.md remains controlling with latest6.12reference correction. Resource rules: one numerical workload, at most6ownedheavyprocesses including Pythonhelpers; nice19/libs1, actualhost preflight and PID/start checks; all5400s/19073MiB/12GiBmodel/geometry/quality/height/progression gates unchanged.
''';Path('HANDOFF_Codex.md').write_text(handoff)
Path('HowToProceed.md').write_text(f'''# Planned stop after phase B

**{state}.** Qualification commit: `{qcommit}`. Do not start phase C, N6 or final N7 until the user continues.

Phase B's scoped/end gates and readability checks passed on build558. G3/G4/GQ are complete; corrected integratedG2 qualifies the current source after the documented R1 ownership error. No owned job runs; actual terminal handles and pins are sealed.

Read [HANDOFF_Codex.md](HANDOFF_Codex.md), [progress]({N}/progress.md), [phase-B report](Handbook/ADAPTATION_CUT_PHASE_B_20261117.md), and the complete latest NIGHT_WORK_ORDER.md before any future work. Preserve build558 source/binary pins and independent local copies. Last full two-event qualification is N14/N15 build552; no extra G5 was required at end B.

N10 is withdrawn; do not implement8.5ab. Keep all gates and resource limits. User applications are observations; our Python helpers count toward owned work. **STOP AND WAIT FOR THE USER.**
''')
if mode=='qualified':
 f=Path('Handbook/ADAPTATION_TIMINGS.md');s=f.read_text();i=s.index('\n\n');s=s[:i]+'''\n\nPhase B completed on558: G3 617.688712s/1170splits/1113commonedits exact/adjustedentry5113; G4 6.492431/6.504021/6.514544s; GQ1083.237551stop/1091.436227sealed/all6thirdwindows/18strictsnapshotrows. Corrected integratedG2 316.078817stop/323.937712sealed/work90,809,869/counts480/473/346/263/469/464. Prefix HWM sums6,440,038,400/13,999,366,144B are notfullP8peaks. EarlierB6/B7/B8G2 timings below are numerical observations from runs with an overlooked seventh owned draft-generator process; they do NOT qualify R1. The connected558 state was requalified after terminating the orphan and fixing ownership accounting. G2/GQ controllers were recovered on the SAME numerical runs, no solver restart; see phase-B report. No controlled efficiency gain is claimed. Last full qualification remains552; no new G5 at endB. Planned user stop; no phaseC.\n''' +s[i:];f.write_text(s)
replacements={"Updated2026":"Updated 2026", "all1781":"all 1,781", "All1781":"All 1,781", "N10 is withdrawn and8.5ab":"N10 is withdrawn and 8.5ab", "PASSED1170splits/all1113commonedits exact/adjustedentry5113":"PASSED: 1,170 splits; all 1,113 common edits exact; adjusted entry work 5,113", "PASSEDat1/2/4ranks":"PASSED at 1/2/4 ranks", "PASSEDall6thirdwindows/18timersnapshots/alloldinteger/model/callrows exact/zeroCensusdelta":"PASSED: all six third windows, 18 timer snapshots, all old integer/modeled/call rows exact, zero Census delta", "153private definitions/23lambdas":"153 private definitions / 23 lambdas", "at most6ownedheavyprocesses including Pythonhelpers; nice19/libs1":"at most six owned heavy processes including Python helpers; nice 19 / single-threaded libraries", "actualhost preflight":"actual host preflight", "all5400s/19073MiB/12GiBmodel/geometry/quality/height/progression":"all 5,400 s / 19,073 MiB / 12 GiB model / geometry / quality / height / progression", "all6payloads":"all six payloads", "fullphysical/quality/transfer/continuedCFD/resource/timer":"full physical / quality / transfer / continued CFD / resource / timer", "Nativeeventmaxima":"Native event maxima", "exactP8process-peak sums":"exact P8 process-peak sums", "No G5 is scheduled at end B and no acceptedBL/RANSclaim":"No G5 is scheduled at end B and no accepted BL/RANS claim", "integratedG2":"integrated G2", "isolatedrestart":"isolated restart", "work90,809,869":"work 90,809,869", "counts480/473/346/263/469/464":"counts 480/473/346/263/469/464", "zeroCensusdelta":"zero Census delta"}
for target in [N/'progress.md',Path('HANDOFF_Codex.md'),Path('HowToProceed.md')]:
 content=target.read_text()
 for old,new in replacements.items():content=content.replace(old,new)
 target.write_text(content)
print(state)
