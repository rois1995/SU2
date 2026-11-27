# CL1 whole-item source check (R4)

FP3-E's final acceptance commit c31f95b071 matched upstream at intake. Its build, scoped, S1, S2 and probe jobs were terminal and the host had no owned solver, MPI or build process. AGENTS.md section 2a selects FP3-E ordinary S. Its sealed build680 and S1 receipts name the local solver and digest. The production solver was built from 62a270c4d6 during build679. Build680 relinked the test driver only and preserved the solver digest. The checkout, solver source and build680 fixture correction commits are recorded separately.

## Source checks

- Common/src/CConfig.cpp registers all 15 prescribed keys: ADAP_SIZES, ADAP_SUBITER, ITER, ADAP_FLOW_ITER, ADAP_FLOW_CFL, CFL_ADAPT, LINEAR_SOLVER_ERROR, LINEAR_SOLVER_ITER, CONV_FIELD, CONV_CAUCHY_ELEMS, CONV_CAUCHY_EPS, CONV_STARTITER, HISTORY_OUTPUT, OUTPUT_FILES and OUTPUT_WRT_FREQ. The output enum maps PARAVIEW and SURFACE_PARAVIEW to XML VTU output.
- CFlowOutput registers LIFT and DRAG as coefficient history fields, and COutput's monitored-field loop uses the registered names. The launcher preserves the inherited SCREEN_OUTPUT and the history ITER group supplies Adap_Cycle and Adap_Iter. Gate evidence confirms both fields in cycle 0's convergence summary.
- CSinglezoneDriver's adaptation loop prints the exact cycle marker before each solve, propagates each used iteration count to the next Adap_Iter offset, and separates solve and adapt times in its final cycle summary. The solver exit prioritizes interruption, then convergence, then the cap. An interruption alone establishes neither convergence nor the cap.
- Use the terminal remesh timers to partition each rank's native log. Several worker stage exits can precede a terminal timer after the normalization-correction path in NativeRemeshRun.inc, so select the final exit and associate the event-local summary and refine detail. Reuse standing physical and timer helpers with four event packets. Transfer marker and table searches stay within adjacent driver cycle markers. None of the signed identity-chain, retry or isolated-restart exclusions enters the capability gate.
- The inherited launch used six unbound ranks, native trace 0, probe 0 and single-thread libraries. Use that environment with case paths adjusted and other native variables removed. A rank-local wait4 receipt provides the solver's terminal RSS high-water mark and raw wait status. Per-process snapshots support process ownership, watchdog cleanup and host-load reporting. They do not stand in for terminal high-water marks.
- Preserve the starting geometry, native sidecar and restart seed from D S1 by content hash. The archived signed shock script runs unchanged on the retained mesh and restart pairs after the single solver launch.

## Contradictions and readings used

1. The inherited controller and connected-event formatter assume a two-event identity workflow. CL1 uses their per-event validators and scopes observations to four events, with no reference or retry.
2. The template already specifies ITER=1 and CFL_ADAPT=NO. Keep their existing textual values. Emit a diff of the exact requested changes and additions. An ADAP_FLOW_CFL of 5 applies to adapted cycles; cycle 0 uses the fixed inherited CFL 1.
3. The source accepts both ParaView keys, but output after repeated topology replacement remains an observation. Record every expected filename, hash and size, with any missing or empty VTU and the corresponding output trace.

No unsupported config key, missing convergence-field registration, or conflicting required result was found.
