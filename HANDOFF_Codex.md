# Current handoff — 2026-10-29

Continue DC-17/DC-18 under [AGENTS.md](AGENTS.md). No accepted complete curved/M6
adaptation, conservative history transfer and solver restart workflow yet. BL/RANS
remains part of the goal. Private edits and primitive tests do not close either item.

Requirements and implementation decisions are separated in the
[selected foundation](Handbook/ADAPTATION_ESTABLISHED_DESIGN.md): native classified
complete cavities, transported nodal metric, coarsen/refine then quality improvement.
Boundary points move on fixed original geometry; genuine features, bodies, absolute
geometry tolerance independent of h0 and every final gate remain mandatory.
SCOREC ma/LICENSE is non-commercial; no MeshAdapt source copied.

Latest pushed WIP: `a083fdafd7`, verified on rois1995/SU2 branch `codex/native-3d-bl`.
TODO remains local. Leave unrelated files alone.

**M6 build114 RUNNING session84714** in staged57/m6_runs/
`euler_build114_classified_stages`; owned wrapper PID903240. Sources, tests and
helpers remain pinned until the complete process tree is terminal. Build114 passed
261.364s. One owned heavy job, nice19, six ranks, single-thread libraries, compile
scratch on4TB. Check load/RAM and saved handles before starting anything else.

Current connected fixes: shared cubic projection uses local coordinates and accepts
an existing classified foot whose conservative distance bound meets requested
accuracy. Ordinary associations use validated fixed entity/current parameters;
genuine feature routes and owner/archive consistency remain. Unavailable candidates
refuse locally; invalid backend/state/resource failures remain explicit. Improvement
runs after each collapse/split pair. Stage quality admission is separate from
validity-only geometric builders; final gates are unchanged.

Smaller build114 workflow FAIL30.048s by operator stagnation, not resource exhaustion:
2062/4096 proposals,35,630,824/67,207,168 atlas work. Within-quality non-regression
assertions pass, but139 private cells still have q/J1/1, size16, geometry32 failures;
curved transfer is not reached. Flat Euler/RANS current+BDF2 conservative history
transfer and SU2/CGNS reload pass. Actual solver restart is not qualified.

Fresh diagnostic115 is terminal17.178s, all pins verified. Three existing classified
collapses of worst-cell vertices refuse exact orientation at cells135178,116744,
174089 using30/42 work, without exhaustion. This proves only those proposals fail.
Investigate existing interior edge-removal on closed6-ring edge1–134792, not a new
fallback. Worst cell122900 has four points on one original wall facet and positive
roundoff-scale determinant. Preserve diagnosis before changing operators.

Known test-only correction pending after M6: Metric3D_tests.cpp new Absolute poor
shape fixture expects Progress but correctly fails earlier with Shape. MPI focused
build114 passes9/10 cases per rank; fix the assertion, not production gates.

Detailed evidence: [projection findings](Handbook/ADAPTATION_PROJECTION_FINDINGS.md),
[transfer findings](Handbook/TRANSFER_OPERATORS_FINDINGS.md),
[progress](ADAPTATION_PROGRESS.md). staged57 contains build114_mapped_summary.json,
build114_worst_cell.json, serial_replay115/status.json and raw captures.
Prior terminal M6 build106 stopped at1800s before quality/transfer after11567 edits;
its receipts remain valid historical evidence, not qualification of current staging.

Next: assess current M6, replay the existing ring operator, then make the smallest
connected correction supported by failures. Verify test filters. Qualify accepted
smaller/M6 mesh, repeated events, publication, conservative transfer/history and
actual restart before claiming completion. Do not raise caps or weaken gates.
