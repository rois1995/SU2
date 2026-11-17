# Native adaptation — current handoff

Updated 2026-11-17. **work order 2 in force; next: cut phase C, sub-step C1; a new session continues.** Phase B passed on build558; qualification commit `97478b7abae40fcf107ac8132f0eea415a2aaaae`. All owned jobs were terminal at the completed phase-B stop. Current source is code-equivalent to B8 checkpointdaf75b14e2; all 1,781pins and binary hashes are verified. Latest actual terminal audit: `N4/section6_3_after_N11/end_B_preparation/planned_stop_terminal_audit.json`.

Work order 2 is now the order in force and the planned stop is over. **A new session continues with cut phase C, sub-step C1.** This session is limited to the instruction handover, commit/push and stop; no phase C work starts here. N10 remains withdrawn and 8.5ab is not implemented.

Instruction evidence: `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/work_order_2_handover_20261117`. Previous AGENTS.md and its SHA256 are archived there. Current AGENTS.md is byte-identical to the preserved AGENTS_CLEAN.pinned.md snapshot (`a256611d0e3a02675e7ba6fea54e5d116a65e7818eeab798549f4f0927df9403`); copied WORK_ORDER_2.md SHA256 is `07512c7b4ced334464eb8b9dcb3ca28cbe3f789bc932d29eacf5e17235629761`.

The two source paths were revised after the verified copy. This handover preserves the exact SHA256 versions supplied by the user; the evidence manifest records the later observed source hashes. Use the pinned evidence copy of WORK_ORDER_2.md for this handover.

[Phase-B report](Handbook/ADAPTATION_CUT_PHASE_B_20261117.md) contains commits, gate numbers, baseline G1 failures, found-during-cut list, exceptions, ten longest functions and every own-helper correction. [Current progress](integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/progress.md) holds handles and evidence links. At the start of the new session, read AGENTS.md, the copied WORK_ORDER_2.md and progress. The earlier NIGHT_WORK_ORDER.md remains history.

End gates: G3 PASSED: 1,170 splits; all 1,113 common edits exact; adjusted entry work 5,113; G4 PASSED at 1/2/4 ranks; GQ PASSED: all six third windows, 18 timer snapshots, all old integer/modeled/call rows exact, zero Census delta. Readability's153 private definitions / 23 lambdas meet80/15; Adapt's80-line deadline remains phase D.

Resource correction: original B6/B7/B8 G2 numerical rows passed with an overlooked seventh owned process, so those runs are NOT R1-compliant. The verified orphan renderer was terminated and the same integrated558 source passed a corrected G2 run at the six-process bound. G2/GQ controllers later received SIGTERM for unknown reasons; their SAME solver runs were adopted without restart. Original signals and unavailable solver wait statuses are explicit; all actual handles are terminal. See report and R3 evidence.

Shared build: `integration_evidence/build-integrated-v3-p5-20261114`.

- CFD558 SHA256 `cb90bfd4bcec97bf0ee657789dca662ed8d416064d7455c06c38514481b87ddb`.
- Test558 SHA256 `b1462a432c443632640b92ce9c6f44e41ef390bc71641aabf8dc5935053789a4`.
- Source pins: `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/build558/source_pins.json`.
- Independent read-only558 binary copies and manifest: `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/end_B_preparation/qualified_build558` (payloads local, not Git).

Last full two-event qualification remains N14/N15 build552/checkpoint0c2cb5c460:4738.250709s, all six payloads exact, full physical / quality / transfer / continued CFD / resource / timer gates andisolated restart passed. Native event maxima2568.277632/1672.244847s; exact P8 process-peak sums14,054,744,064/17,397,911,552B. Its preserved references/binaries andM26report remain qualified. No G5 is scheduled at end B and no accepted BL/RANS claim is made.

N=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115`; P=`N/N4/section6_3_after_N11`. Phase-B acceptance/terminal receipts are inP/end_B_preparation andP/B8/validation. Whole-cut sourcecheckP/source_check.md remains controlling with latest6.12reference correction. Resource rules: one numerical workload, at most six owned heavy processes including Python helpers; nice 19 / single-threaded libraries, actual host preflight and PID/start checks; all 5,400 s / 19,073 MiB / 12 GiB model / geometry / quality / height / progression gates unchanged.
