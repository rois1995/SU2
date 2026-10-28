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

Build70 is running with sources frozen; the connected scheduler change is unvalidated.
ServiceClaimedVertex retires measured local frontier directions using existing
ChangedVertices notifications, without new cache, state or communication; Directory
still guards unmeasured, remote and expanded support. Deferred-frontier progress
prevents stagnation fallthrough; both legacy generic/stagnation quality handoffs
are disabled for bidirectional mode, leaving pair quiescence as the construction-to-
quality handoff. A tight-window (1) plain/composed fixture requires both pass counts
before quality. Review found no blocker. `length_pass_deferred` now counts selected
deferrals plus bulk-retired potential directions, including balanced edges; raw
counts cannot be compared as equivalent work to earlier runs.

Build69 PASS1198.313s, 1293 pins unchanged; all owned build processes exited.
Audit passes 23 selections (atlas 63, serial 252, MPI 229). Atlas checks pass
63 cases/12,847 assertions in 132.018s, including actual smooth/curve/unpublished
endpoint collapses. Mapped runtime FAIL10.806s: curved flows reached 14 splits,
zero collapses, 136 rejections/274 geometry trials and the unchanged 67,207,168
arithmetic cap (global work 744); 153 early constraint filters and 19 orientation
vetoes in 420 checks. Flat workflow passes with 31 edits (27 splits, 4 collapses;
work 282). The connected construction blocker and accepted M6 remain open.
M6 source69 `euler_build69_incident_collapse` terminated rejected after 1876.115s (solver exit 0, pins unchanged; all nine owned processes gone). Each rank reports zero completed length split/collapse passes yet one quality stage: a stagnation-window branch entered quality while pass-deferral scans changed neither attempts nor mesh progress. There were 289,853 directory elections per rank and 66,365 deferred candidates summed across ranks. Sampled process-tree maximum RSS was 3,757,772,800 bytes, not an allocation peak. The initial working mesh had 303,359 cells; 146,862 is source-envelope metadata. The next connected MIS bulk-retirement and lifecycle-gate design addresses this without weakening quality gates or caps. No M6 acceptance or transfer follows.


Build67 focused bidirectional serial/MPI checks passed; mapped curved runtime failed at the arithmetic guard. See [progress](ADAPTATION_PROGRESS.md).

Build63 normalization and repeated MPI publication checks passed; see [progress](ADAPTATION_PROGRESS.md) for their scoped evidence.

Build63 broad serial/MPI failed curved runtime and a stale fixture expectation;
source64 focused correction passed. Details are in [progress](ADAPTATION_PROGRESS.md).

Geometry-validation checkpoint `9ae6b94886` on authorized origin rois1995/SU2, branch
codex/native-3d-bl; it is incomplete. Source63 broad MPI confirms the nonempty-rank
broadcast correction. Detailed earlier build/fixture failures are in [progress](ADAPTATION_PROGRESS.md).

Full-CFD M6 build61/session99828 TERMINAL REJECT after 1864.11s: solver exit 0,
native event accepted=false, pins unchanged. The working mesh grew from 303,359 to 1,376,207 private cells (4.54×), with
877,469 metric demands (no requested BL in this run); quality was not reached. The 146,862 source-envelope
cells are separate source metadata, not the initial working tetrahedron count. All nine owned processes are gone. Largest sampled process-tree aggregate
RSS was 6,387,077,120 bytes; largest sampled individual RSS 1,096,867,840 bytes;
neither is a whole-job peak. Evidence: `staged57/m6_runs/euler_build61_working_surface`.
No accepted mesh or transfer/restart workflow follows.

Source48 quality-entry archives are private diagnostics, not accepted restart evidence; see [progress](ADAPTATION_PROGRESS.md).

The support-index and normalization candidate has serial and MPI6 fixture evidence.
The bidirectional metric-length candidate is unvalidated. Preserve source, body and feature constraints, resource
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
