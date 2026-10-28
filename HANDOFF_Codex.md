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

Selected direction: fixed-model entity associations and coordinated current-patch
replacement, disposable smooth correspondence. Missing primitive: conservative
two-sided patch/model-support certificate supplying source bounds without
inherited face partitions. Replace geometry, source, owner publication and
restart dependencies together; preserve equivalent proofs until replacement is
qualified. Do not add speculative CAD formats, local fallback variants or blind
rejection caches. Detailed architecture and history:
[working-surface decision](Handbook/WORKING_SURFACE_RECONSTRUCTION_DECISION.md).

## Evidence and current job

Review checkpointff818fc703 and code checkpoint6f47de50563b are pushed to
origin rois1995/SU2, branch codex/native-3d-bl. Build79 PASS495.961s (1294 pins
unchanged). MPI6 staging PASS9 cases/rank15.850s, coupled clean/legacy32-event
movement/publication/source/schema8 PASS177.816s and all16 per-phase cost
comparisons pass. All23 compiled filter audits pass (256 serial/233 MPI/67 atlas).
Replay80 PASS16.183s: useful split succeeds; valid collapse6496→1 worsens bound
to .01; reverse collapse first fails body/feature preservation with fresh work.
No fallbacks, no exhaustion in these three proposals. Smaller curved CFD baseline
build79 FAIL14.180s: typed shared-work exhaustion, feasibility undetermined.

Trace73 curved CFD FAIL12.827s:17 productive splits, zero collapses; one
unsuccessful collapse pass costs8,160,949 operations, next split exhausts atlas
work. Final capture has no explicit covers/traces. Fresh actual SplitEdge16–6496
on final state succeeds (2→4 positive cells;1,528,800 work; bound .00375→.00125,
requested .001). Useful refinement exists, but this is not exact earlier-transaction
replay or accepted workflow. Preserve the first unsuccessful fresh-work operator
before fallbacks; distinguish scheduler waste, verification and unavailable edits.
[Geometry cost](Handbook/GEOMETRY_VERIFICATION_COST.md).

M6 build72 terminal REJECT1862.171s:6,287 splits,2,025 collapses,44,861 geometry
rejections, no quality stage, summed atlas work369,648,840,116. Source envelopes
are not working-tet counts;500k complexity is not a tet cap. No mesh/transfer
acceptance follows. Use M6 early at the next connected implementation milestone.

User cleanup restored /home17GB free; approval/MPI/push work again. Deleted
external Ninja restored as owned official1.13.0 with provenance. Mixed-version
build78 is terminal interrupted; build79 and all subsequent owned checks are
terminal. **No heavy job remains.** Production geometry replacement has not begun;
next code must implement the reviewed certificate/source dependency together,
not another unmeasured guard, fallback or blind budget increase.

## Operations

Check owned handles before new jobs; one heavy job, nice19, build-j1, libraries
single-threaded, at most6 local ranks. Check host load/RAM and save process-tree
receipts; sampled RSS is not allocation peak. Compile temp on4TB. Reuse evidence
only with source/binary/runtime/config/input/helper provenance. Push labelled
checkpoints with explicit paths; TODO stays local. Leave unrelated files alone.
[Archived inputs](Handbook/ADAPTATION_REFERENCE_INPUTS.md).
