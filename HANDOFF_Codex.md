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

Implementation now follows fixed-model entity associations and current-patch
replacement. Ordinary smooth paths and face covers are absent in the new curved
production contract; only canonical genuine-curve routes persist. Two-sided
geometry certification, source tubes, local topology/owner publication and
explicit v9 persistence are connected in source, **not integrated-validated**.
Legacy1–8 decodes remain separately validated before migration. The current cubic
backend remains authoritative; no CAD format infrastructure was added.
[Working-surface decision](Handbook/WORKING_SURFACE_RECONSTRUCTION_DECISION.md).

## Evidence and current job

Checkpoints b35d3f626e/7e24ddebb0 are pushed to rois1995/SU2,
`codex/native-3d-bl`. Build90 PASS189.258s. Curved workflow90 FAIL72.964s:
295 splits/157 collapses, zero cached geometry/size debt, but arithmetic exhaustion
before quality. Flat contexts pass; no curved transfer/restart acceptance.

M6 build90 was deliberately stopped after481.694s before operators; unchanged
pins, owned host processes verified gone. Its120-sample profile places all stacks
in full surface-union setup. Source enclosure now requests model→frozen mesh only;
the final engine gate explicitly requests both directions. Source-only queries
never claim final tolerance acceptance. The empty-interior feature-reconstruction
veto is also fixed, with focused regression coverage.

**Build91 running**,44 targets, one low-priority worker, session96860/PID1900268.
Do not edit pinned C++/helpers until terminal. Changes also add four16-event
neighboring-move publication/source/schema8–9 reload variants and independently
admitted first-failure replay. These changes are unqualified and not yet pushed.
Next: actual curved workflow, compiled-filter audit, entity repeated MPI/source/
reload checks, M6 early; preserve all final gates. Generic cubic/Euler curvature
sizing remains open. See [decision/evidence](Handbook/ADAPTATION_REQUIREMENTS_REVIEW.md),
[geometry cost](Handbook/GEOMETRY_VERIFICATION_COST.md) and
[progress](ADAPTATION_PROGRESS.md) for preserved failures and historical receipts.

RAM27GiB available, load1.22 at build91 preflight. /home16GB and workspace71GB
free after user cleanup; matching owned Ninja1.13.0 restored. Sampled process RSS
is not allocation peak. Remaining risks: source-tube shell feasibility, entity
publication/reload and final real-mesh quality/geometry/transfer acceptance.

## Operations

Check owned handles before new jobs; one heavy job, nice19, build-j1, libraries
single-threaded, at most6 local ranks. Check host load/RAM and save process-tree
receipts; sampled RSS is not allocation peak. Compile temp on4TB. Reuse evidence
only with source/binary/runtime/config/input/helper provenance. Push labelled
checkpoints with explicit paths; TODO stays local. Leave unrelated files alone.
[Archived inputs](Handbook/ADAPTATION_REFERENCE_INPUTS.md).
