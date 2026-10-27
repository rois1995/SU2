# Codex handoff — current state, 2026-10-28

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

Build63 PASS66.1366s; all 17 build PIDs gone. Nineteen compiled filter audits
pass (245 serial/222 MPI selected). Normalization passes serial 745 assertions
in 15.9078s and MPI6 in 16.796s (727/721/721/721/715/715 assertions); all nine
MPI PIDs gone. Repeated 16-event MPI publication/source-bound/schema8 test passes
75.9713s; all nine PIDs gone. Its cost receipt records 12–20 face pieces, 13–19
traces, 56–109 bits, 30,540,601–67,213,767 work excluding decode and 10,058,008–
46,571,848 bytes aggregate admitted workspace. Sampled process-tree RSS ~214 MB
is not peak allocation. Source-envelope checks are not CFD solution transfer.

Broad serial `staged57/cases/serial_full_20261027T230545_3953311` FAIL176.0948s:
243/245 cases and 1,134,704/1,134,707 assertions pass. Two failures are the known
curved runtime; the third is a stale quality-stage query expectation (6 vs 12).
Broad MPI63 `staged57/cases/mpi6_20261027T231001_3964969` FAIL177.1677s
with unchanged pins: each rank passed 220/222 cases, with the same three failed
assertions; the empty-rank fixture passed and all nine processes exited. Source64
corrects the stale fixture expectation and strengthens row/version/content checks.
Build64 PASS96.1853s; focused quality_cache serial and MPI6 pass in 6.8561s and
6.8769s respectively, all owned processes gone. All 21 compiled filter selections pass in build64; no owned job remains live. These focused passes do not qualify the failed
broad campaigns. No complete M6 or CFD transfer/restart is accepted.

Latest pushed checkpoint `765d0d4207` on authorized origin rois1995/SU2, branch
codex/native-3d-bl; it is incomplete. Source63 broad MPI confirms the nonempty-rank
broadcast correction. Detailed earlier build/fixture failures are in [progress](ADAPTATION_PROGRESS.md).

Full-CFD M6 build61/session99828 TERMINAL REJECT after 1864.11s: solver exit 0,
native event accepted=false, pins unchanged. The working mesh grew from 303,359 to 1,376,207 private cells (4.54×), with
877,469 metric demands (no requested BL in this run); quality was not reached. The 146,862 source-envelope
cells are separate source metadata, not the initial working tetrahedron count. All nine owned processes are gone. Largest sampled process-tree aggregate
RSS was 6,387,077,120 bytes; largest sampled individual RSS 1,096,867,840 bytes;
neither is a whole-job peak. Evidence: `staged57/m6_runs/euler_build61_working_surface`.
No accepted mesh or transfer/restart workflow follows.

Source48 M6 stopped at its elapsed-time limit without an accepted mesh; its six
quality-entry archives are verified and pushed as private diagnostics, with
source_valid=false and exact_continuation=false. They do not establish transfer,
restart or a runtime checkpoint. This full-CFD run used `run_m6`, not the four-window diagnostic stop. See [progress](ADAPTATION_PROGRESS.md) and
[curved implementation history](Handbook/CURVED_ATLAS_IMPLEMENTATION.md).

The support-index and normalization candidate has serial and MPI6 fixture evidence.
The bidirectional metric-length design remains unimplemented. Preserve source, body and feature constraints, resource
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
