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

Review checkpointff818fc703 and validation checkpoint2a98e46d8f are pushed to
origin rois1995/SU2, branch codex/native-3d-bl. New connected source is awaiting
its labelled checkpoint/build qualification. Private kernel union84 PASS9 cases/
215 assertions141.684s overall, pins unchanged. Union81 timed out on a planar
seam; FinitePlaneReference now handles coplanar unions. Union83 then exhausted
work estimating a precise maximum distance. Tolerance certification with explicit
outside witnesses removes that unnecessary objective; it does not raise caps.
Syntax86 PASS11/11 TUs42.218s, pins unchanged. **Build86 running**,44 targets,
one low-priority worker, handle75478/PID1728810. Do not edit pinned C++/helpers
until terminal. Smaller curved CFD is the next runtime; then connected MPI6
publication/restart and M6 early. No accepted full workflow is claimed.

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

Disk at build86 preflight: /home16GB, workspace71GB free; RAM26GiB available,
load2.92. Matching owned Ninja1.13.0 is restored. Build86 is the sole owned heavy
job. Remaining integration risks include new feature-route publication, restart
roundtrips, source-tube shell feasibility and real M6 cost; fixtures do not close
those. Generic curvature sizing/BL and other earlier open scope remains in TODO.

## Operations

Check owned handles before new jobs; one heavy job, nice19, build-j1, libraries
single-threaded, at most6 local ranks. Check host load/RAM and save process-tree
receipts; sampled RSS is not allocation peak. Compile temp on4TB. Reuse evidence
only with source/binary/runtime/config/input/helper provenance. Push labelled
checkpoints with explicit paths; TODO stays local. Leave unrelated files alone.
[Archived inputs](Handbook/ADAPTATION_REFERENCE_INPUTS.md).
