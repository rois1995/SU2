# Codex handoff — current state, 2026-10-28

Continue DC-17/DC-18 under [AGENTS.md](AGENTS.md). Keep tracked
[progress](ADAPTATION_PROGRESS.md), local [TODO](ADAPTATION_TODO.md) and
[transfer findings](Handbook/TRANSFER_OPERATORS_FINDINGS.md) current.

## Goal and design

Accepted native distributed curved adaptation, M6 Euler then requested BL/RANS,
with conservative transfer, BDF/history, repeated events and restart. **No complete
curved CFD or M6 workflow is accepted.** Validity-first construction precedes
metric/BL quality improvement. Boundaries may redistribute on immutable geometry;
features, bodies, absolute ADAP_HAUSD independent of h0 and final gates remain.

The [requirements/design review](Handbook/ADAPTATION_REQUIREMENTS_REVIEW.md)
separates explicit user requirements from inherited numeric settings and custom
mechanisms. q=.20, J=.05, edge=1.8, h0±10% and M6 complexity500k are the current
comparison baseline; direct user provenance of those numbers is unconfirmed.
Rational chart ownership, smooth-edge contracts and budget formulas are agent
choices. Caps can change with measured justification, not to hide repeated work.

No complete curved CFD/M6 adaptation, transfer and restart workflow is accepted.
DC-17/DC-18 remain open. The selected foundation is SCOREC/MeshAdapt classified
cavities and transported nodal size fields, implemented inside SU2's existing
geometry, Directory/publication and separate conservative CFD transfer facilities.
See [connected design](Handbook/ADAPTATION_ESTABLISHED_DESIGN.md). This replaces
agent-selected metric-support prisms; it does not replace the native backend with
an external executable. Features, bodies and all final acceptance gates remain.

Build92 compiled successfully (303.443s). Curved workflow92 reached quality after
295 splits/241 collapses with construction q/J measurements zero, but rejected;
no curved transfer/restart. M6 build92 rejected before operators on a degenerate
metric-support prism at face734 (386.112s total, pins unchanged). These prisms
carry sensor metrics, not conserved flow/history. Atlas92 completed all four
16-event publication/source/reload variants; broad result72/73, with one redundant
feature-contract constructor assertion failing. These are supporting diagnostics,
not an accepted workflow. All those owned jobs are terminal; receipts preserved.

Build94 is compiling the connected current-nodal field change: import attributes,
versioned owner payload, cavity transport and measurement, publication, complexity
reassessment, bounded uniform normalization of the current field and reader packing.
No new proof hierarchy, source hull, CAD format adapter or raised arithmetic cap.
The old original-family trial graph is an agent choice; current-field normalization
is deliberately replacing it rather than retaining interpolation ancestry.
Build/runtime qualification and M6 remain pending. Known source-review gaps before
qualification: regional projection outside the stage snapshot and FIRST_ROW stage-two sensor
construction. Remote first-row context, core-eigenvalue scaling and collective
preparation fixes are now implemented, awaiting build94. Do not declare BL complete from Euler results.

Check owned handles before editing pinned sources. Current build94 session36793. Build93 was deliberately stopped after12/32 targets;
its zero raw exit did not establish completion. The receipt explicitly marks it
interrupted, and the helper now checks both targets are up to date.
One heavy job, nice19, build-j1, library threads1; at most6 ranks. Preflight load2.10,
RAM26GiB available, workspace70GB free. Checkpoints b35d3f626e/7e24ddebb0/2cffdba6c8
are pushed; new changes are not yet checkpointed. TODO remains local.


## Operations

Check owned handles before new jobs; one heavy job, nice19, build-j1, libraries
single-threaded, at most6 local ranks. Check host load/RAM and save process-tree
receipts; sampled RSS is not allocation peak. Compile temp on4TB. Reuse evidence
only with source/binary/runtime/config/input/helper provenance. Push labelled
checkpoints with explicit paths; TODO stays local. Leave unrelated files alone.
[Archived inputs](Handbook/ADAPTATION_REFERENCE_INPUTS.md).
