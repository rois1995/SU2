# Codex handoff — current state, 2026-10-28

Continue DC-17/DC-18 under [AGENTS.md](AGENTS.md). Keep tracked progress, local
TODO and transfer findings current. Full goal includes requested BL/RANS, movable
boundaries on fixed geometry, genuine features/bodies, absolute tolerance independent
of h0 and all final gates. Numeric baseline settings and agent algorithms are
separate from user requirements; see the linked decision.

No complete curved CFD/M6 adaptation, transfer and restart workflow is accepted.
DC-17/DC-18 remain open. The selected foundation is SCOREC/MeshAdapt classified
cavities and transported nodal size fields inside SU2. Requirements, source-code
comparison, decisions and acceptance: [connected design](Handbook/ADAPTATION_ESTABLISHED_DESIGN.md).

Current pushed checkpoint22aa374238 is WIP. Build96 passes with unchanged1295
pins; focused nodal tests execute122 cases:119 pass/3 fail. Fresh stage target,
missing-ID bootstrap and normalization tests now pass. Remaining issues: a too
specific rejection assertion, initialized output metadata comparison, and the
round cap preventing a measurement-only final census after valid operations.

M6 build96 fails340.302s after10 private edits/rank. The old internal memory
refusal is gone. New precise blocker: “Surface-union parameters exhausted exact
dyadic resolution; bounds unresolved.” This is verification failure, not proof of
an unavailable valid operation. Native277.872s/import174.85s/refine88.74s; sampled
tree about3.46GB, largest rank610MB (not whole-job peak). M6 build96 owned jobs terminal. Build99 PASS1276.434s, all1295 pins
unchanged. Focused nodal122/122 and geometry backend11/11 (414 assertions)
pass. All25 compiled filter selections match intended cases. Atlas batch was
stopped at442.217s with legacy prism-expectation failures; not a passing suite.
Saved repeated edit/publication event receipts remain partial evidence.
M6 build99 deliberately stopped1369.419s, all ranks terminal and pins unchanged.
Four split-only passes:315809→354684 cells,12673 splits/0 collapses; size-bad
112523→116291, geometry-bad4115→5396; construction q/J remain0. Native schedule
repeated productive splits, preventing a collapse pass. This is scheduling/cost
evidence, not unavailable operations or geometric impossibility. Short profile
(3s/792 samples) finds certified closest-point work and MPI waiting; full-run
fractions cannot be inferred, DWARF warnings qualify stacks.
Build100 PASS291.483s, unchanged1295 pins. Opposing-demand3 cases PASS5.987s:
coarsen-first and alternating complete passes, zero construction q/J and final
gates retained. Actual mapped curved workflow RUNNING session26378 with fresh
failure capture. Source/tests/helpers pinned. M6 rerun follows this workflow;
no accepted hard-target mesh or full solver restart yet.

Connected change under implementation: existing fixed-geometry backend projection
restricted before search to body/sheet; actual attempted work charged on failures;
local immutable projection/deviation and curvature guidance for candidate faces,
replacing per-operation whole-surface subdivision. Guidance is not a Hausdorff
certificate. Final two-sided geometry acceptance remains unchanged, as do final
quality/height/transfer/restart requirements. Final census allowed at round cap
without admitting another operation. No accepted curved/M6 workflow yet.

Next: focused checks and accepted-face checks, actual curved workflow and M6 early;
then repeated neighboring edits/MPI, transfer/history and restart. BL regional
projection and FIRST_ROW source handling remain unqualified. The 3D independent
P1 history audit reuses existing readers; self-check only passes, no M6 data or
restart claim yet. TODO stays local. Historical details/receipts:
[runtime archive](Handbook/ADAPTATION_RUNTIME_HISTORY_20261028.md),
[transfer findings](Handbook/TRANSFER_OPERATORS_FINDINGS.md).

## Operations

Check owned handles before new jobs; one heavy job, nice19, build-j1, libraries
single-threaded, at most6 local ranks. Check host load/RAM and save process-tree
receipts; sampled RSS is not allocation peak. Compile temp on4TB. Reuse evidence
only with source/binary/runtime/config/input/helper provenance. Push labelled
checkpoints with explicit paths; TODO stays local. Leave unrelated files alone.
[Archived inputs](Handbook/ADAPTATION_REFERENCE_INPUTS.md).
