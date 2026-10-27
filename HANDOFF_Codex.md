# Codex handoff — current state, 2026-10-27

Continue DC-17/DC-18 and ensuing defects. Keep [progress](ADAPTATION_PROGRESS.md),
local [TODO](ADAPTATION_TODO.md) and the [transfer ledger](Handbook/TRANSFER_OPERATORS_FINDINGS.md)
current. Follow [AGENTS.md](AGENTS.md) for architecture, validation and resource rules.

## Goal and unchanged gates

Complete accepted full ONERA M6 Euler adaptation, then the requested BL/RANS
workflow, including conservative solution transfer, BDF/history, publication and
restart. Final gates: q>=.20, J>=.05, edge<=1.8, requested h0 within 10%, 500k
metric complexity (not tet count), prescribed geometry, body separation,
features/contact and source/transfer checks. ADAP_HAUSD is an absolute mesh-unit
length independent of h0. No full M6 adaptation has passed acceptance.

Construction is validity-first: exact positive nondegenerate cells, topology,
authority and contact are mandatory; no q/J measurement, ranking or quality veto.
Metric/BL-aware improvement follows and enforces all final gates. Boundary nodes
may move on prescribed geometry. Scenario labels must not dispatch reconstruction.

## Current work and evidence

Latest pushed checkpoint `2e9d787265` on authorized origin rois1995/SU2, branch
codex/native-3d-bl; it is incomplete. Source61 build PASS36.124s. Source60
reference I/O PASS 5 cases/96 assertions; previous failed receipt remains preserved.
Source61 broad MPI `staged57/cases/mpi6_20261027T214746_3747115` aborted exit134
in 19.795s after 24/25 cases per rank, before the complete campaign or new overlap
case. Address resolution identifies CNativeImport3D_tests.cpp:1743: fixture reads
point 0 on an empty rank. Reviewed `/tmp/native_bl_empty_rank_draft.patch` elects
an inhabited rank and broadcasts initial values; it is unapplied. Combine it with
the next implementation build instead of rebuilding the large test unit alone.

Source61 overlap_publication6_20261027T215151_3758648 PASS74.923s, all 16
events: rank 0 passed 3494 assertions, each other rank 3445; all nine recorded
processes are gone. publication_cost_summary.json verifies aggregate admitted
workspace and phase descriptor/work bounds, retaining per-rank statistics.
Archive arithmetic is excluded; source-envelope checks are not CFD transfer.

Full-CFD M6 is live: run_m6.py build61 euler_build61_working_surface,
session99828 PID3775811. Native source setup passed at 174193 work, source
reconstruction 12.235s. Six ranks, unchanged native 1800s/outer 2400s limits
and configured working storage 12288 MiB/rank. Static command/pins/config are
saved; C++ and run_cases/run_m6 helpers stay frozen until the owned handle is
terminal. This is the single owned heavy job.

Source60 MPI had failed strict per-rank peak comparisons. Directory hashes IDs
before modulo, so stride alone did not fix home balance; source61 aggregate
admitted-workspace comparisons now pass and do not imply physical peak memory.

Source58 serial connected 16-event test PASS, 3488 assertions. It exercises actual
neighboring moves, directory publication, schema8 reload and source-envelope
bounds, not CFD transfer or solution restart. Archive scratch, configured caller
ceiling and unlimited-capacity fixes and their limitations are in the
[geometry-cost ledger](Handbook/GEOMETRY_VERIFICATION_COST.md). Full curved mapped
runtime and broad integrated qualification remain open.

Source48 M6 stopped at its elapsed-time limit without an accepted mesh; its six
quality-entry archives are verified and pushed as private diagnostics, with
source_valid=false and exact_continuation=false. They do not establish transfer,
restart or a runtime checkpoint. The current full-CFD run uses `run_m6`, not the four-window diagnostic stop. See [progress](ADAPTATION_PROGRESS.md) and
[curved implementation history](Handbook/CURVED_ATLAS_IMPLEMENTATION.md).

The support-index and normalization drafts remain unmerged; local retained-path
closure is still required. Preserve source, body and feature constraints, resource
limits and all final gates. Related decisions: [working-surface reconstruction](Handbook/WORKING_SURFACE_RECONSTRUCTION_DECISION.md),
[curved mobility](Handbook/CURVED_BOUNDARY_MOBILITY.md), [staged adaptation](Handbook/STAGED_NATIVE_ADAPTATION.md)
and [transfer findings](Handbook/TRANSFER_OPERATORS_FINDINGS.md).

Other open scope includes changing-frame gradation, M6 complexity normalization,
curved BL support/chord/quadrature, FIRST_ROW geometry demand, fitted seams,
automatic curved initialization, and full repeated adaptation/transfer/restart.
Primitive passes and private snapshots do not close those obligations.

## Operations

Push authorization persists for the origin and branch above; no new permission
question. Git metadata requires escalated execution and explicit paths. TODO is
local/untracked; leave unrelated dc11/dc15 logs and planning files alone. One owned
heavy job at a time, nice19, one build worker, single-thread libraries, at most six
local MPI ranks. Check load, available RAM and owned handles first; use
`integration_evidence/build-integrated-v2/compile-tmp` on 4TB for builds. Preserve
unique source, binary, configuration, input, helper and resource receipts, and
resume only after verifying provenance. [Archived inputs](Handbook/ADAPTATION_REFERENCE_INPUTS.md).
