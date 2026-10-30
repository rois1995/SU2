# Native adaptation — current work path

Updated 2026-10-30. This is the active work-path entry point. Follow current user
instructions and [AGENTS.md](AGENTS.md). Earlier designs and archived handoffs are
historical evidence, not additional requirements. Keep numerical defaults,
implementation choices and uncertain provenance separate from user requirements.

## Complete objective

Deliver robust native distributed SU2 adaptation in **2D and 3D**, for **Euler and
RANS**, in **steady and unsteady** simulations, including requested boundary-layer
constraints, conservative solution/history transfer and actual solver restart.
Construction follows geometry, target metric and current wall constraints. Solver
labels, case names and inferred input-mesh classes must not select reconstruction
modes. Euler-to-BL and BL-to-Euler are validation scenarios only.

## Connected adaptation workflow

1. **Reuse an established foundation.** The current implementation choice is
   classified complete cavities, current nodal metric transport and coordinated
   coarsening/refinement followed by improvement, informed by mature implementations.
   Reuse existing SU2 facilities and suitable licensed implementations before adding
   machinery. Native integration does not require inventing every algorithm.
   SCOREC MeshAdapt's module license is non-commercial; no source has been copied.
   Algorithm choices remain replaceable when evidence supports a simpler solution.

2. **Keep fixed geometry authoritative.** Working boundary nodes may move on the
   prescribed geometry. Preserve genuine corners/curves, bodies, boundary identities
   and separation. Geometric tolerance is configured independently of h0. Rebuild
   affected working patches and all incident shared-edge/face owners together;
   discard obsolete subdivisions. Persist current semantic associations and genuine
   features, with validated legacy/restart migration. Derived data must not accumulate
   adaptation history. Use curves in 2D and surfaces/feature curves in 3D; share useful
   semantics/utilities without importing unnecessary 3D machinery into 2D.

3. **Construct valid cells first.** Use general split, collapse and cavity operators
   against the requested target. Intermediate cells remain positive/nondegenerate,
   with valid topology, geometry associations and body/feature preservation. Omit
   q/J measurements, shape ranking and quality-progress vetoes unnecessary for
   validity. Freeze the donor state and retain the accepted mesh/state on failure.

4. **Improve metric/BL quality explicitly afterward.** Use smoothing, constrained
   boundary redistribution and topology changes to resolve quality and target debt.
   Stage admission is independent of the validity-only geometric builders: that
   builder mode must not disable geometry constraints during quality improvement.
   Iterate construction and improvement as needed. Enforce all final geometry,
   quality, metric-size and requested-height gates before acceptance.

5. **Drive BL adaptation through actual constraints.** Support requested first
   height, wall-normal resolution, growth and transition to the outer metric in both
   dimensions. General operators construct, redistribute or remove resolution from
   those requests. Preserve requested wall constraints and genuine trailing-edge/
   junction features. Retain the full BL/RANS goal while first closing the curved
   Euler foundation. Existing 2D staging also needs alignment and qualification.

6. **Implement MPI-aware methods from the outset.** Obtain complete distributed
   cavities and incident-owner closure; enforce ownership/version checks and atomic
   publication of mesh, geometry associations and metric attributes. Validate serial
   and MPI correctness throughout development. Avoid serial-only methods requiring
   a later redesign. Keep solver compatibility separate from remeshing decisions.

7. **Complete transfer, unsteady history and restart.** Transfer flow/turbulence
   variables and all required time-history levels and metadata consistently. Check
   conservation, admissibility and boundary-domain accounting. Validate physical-time
   adaptation, repeated events, mesh reload and actual solver continuation/restart.
   Requested h0 progression must support unsteady adaptation without relying only
   on steady adaptation-loop counters. History correctness is part of development
   throughout, not an optional final extension.

8. **Qualify complete workflows.** First close the smaller curved workflow and use
   M6 early under comparable final acceptance conditions; then qualify BL and steady/
   unsteady workflows in 2D and 3D. Exercise different fields/geometries, neighboring
   edits, repeated events, MPI publication, conservative history transfer and restart.
   Reuse valid evidence with complete provenance and verify test selection. Primitive
   fixtures and private edits support diagnosis; they do not establish completion.

## Performance comes after mesh validation

First obtain validated, accepted meshes and complete adaptation/transfer/restart
workflows. Then assess and improve performance in **both serial and MPI paths**,
including scaling, repeated-event cost, memory and communication. During correctness
work, retain failure diagnostics and machine resource safeguards; defer performance
campaigns and tuning. Resource limits are implementation controls, revisable with
measured justification; exhaustion does not prove geometric impossibility. Preserve
first failures with fresh work and resolve demonstrated design defects rather than
masking repeated unsuccessful work with larger caps or additional local fallbacks.

After correctness, consider a **temporary adaptation-specific partition** distinct
from the CFD solver partition. Weight nodes by expected adaptation workload,
redistribute for adaptation, then return/repartition the accepted mesh and solution/
history for the solver. Preserve global identities, associations and state through
both redistributions. This is an option to evaluate, not a prerequisite delaying
accepted meshes. Derive workload-formula examples from Alauzet–Loseille papers,
citing the exact equation and assumptions; distinguish predicted adapted-mesh
population from operator cost. Validate any SU2-specific modifications separately.

## References, status and working discipline

- Active target: accepted ONERA M6 Euler adaptation and actual CFD → mesh → CFD →
  mesh, with conservative transfer and usable published meshes. The user explicitly
  authorized pushing every checkpoint. Code checkpoints are pushed regularly;
  no complete curved/M6 workflow is accepted yet. Connected implementation and
  target-driven evidence:
  [current diagnosis/design](Handbook/ADAPTATION_ESTABLISHED_DESIGN.md#complete-surface-bounds-and-two-event-m6--2026-10-29).
  Current coherent build143: exact-scope/tolerance joint footprints over retained
  certified facet pieces, plus after-first/first-violation coarsening admission.
  Gates unchanged; no q/J construction measurements introduced. Build105.259s
  passes; serial19cases5199assertions16.916s and MPI2 publication/reload/construction
  pass162.152s/pins unchanged. Gate diagnostic419904 comparisons passes. Captured
  M6 query cold2.314s→.07355s/warm2.253s→.003861s at unchanged1e-4 tolerance;
  this is query evidence, not an accepted mesh or whole-run speedup claim.
  Full M6/build143 terminal after3625.1s with pins verified and all owned handles
  terminal. Both events reject on unresolved surface-union exact dyadic resolution;
  they reach6368/6024 global work and2505/2515 private collapses, respectively,
  without an accepted mesh or transfer. Both retain the original CFD mesh/solution.
  Sampled process-tree RSS peaked12.78GB (sampled, not whole-job peak).
  Evidence: staged57/build143_m6_summary.json and combined143_checkpoint.json;
  build142's prior time-budget failure remains in staged57/build142_m6_summary.json.
  Next capture the exact failing geometry query, diagnose the resolution failure,
  then address it under unchanged gates and retry the complete two-event workflow.
  Keep pinned C++/tests/helpers unchanged while active; verify handles before restart.
  User's permanent overnight workflow is saved in AGENTS.md: stop demonstrated
  computational bottlenecks or errors, preserve evidence, fix/validate and retry M6
  autonomously until the Euler loop succeeds. Then verify BL generation before
  the RANS workflow. User unavailable for routine questions. Push every reviewable
  checkpoint; one heavy workload,2–4 build workers. BL/RANS/history/restart open.
- Available papers: [local collection](/media/rausa/4TB/SU2_Versions/Prove/Papers),
  including Loseille–Alauzet–Menier, *Unique cavity-based operator and hierarchical
  domain partitioning for fast parallel generation of anisotropic meshes* (2017,
  DOI: 10.1016/j.cad.2016.09.008). Inspect the full paper before adopting formulas.
  Use Consensus or accessible author copies for additional literature; ask the user
  in the conversation for specific papers that remain unavailable.
- Keep the geometry backend small: current native geometry first; optional Open
  CASCADE support later, STEP then faceted STL and IGES. STL does not define an exact
  smooth CAD surface. Avoid speculative format infrastructure before qualification.
- Current status/evidence: [progress](ADAPTATION_PROGRESS.md), local
  [TODO](ADAPTATION_TODO.md), [design](Handbook/ADAPTATION_ESTABLISHED_DESIGN.md),
  [projection findings](Handbook/ADAPTATION_PROJECTION_FINDINGS.md),
  [transfer findings](Handbook/TRANSFER_OPERATORS_FINDINGS.md) and
  [runtime history](Handbook/ADAPTATION_RUNTIME_HISTORY_20261028.md).
  These records carry detailed results, blockers and provenance. Verify live handles
  and receipts before resuming; do not treat stale status text as a running job.
- Before substantial coding, state the missing capability, evidence, affected callers
  and acceptance. Group connected changes; run focused checks for concrete risks
  and broader regressions at integration milestones. Keep validation resumable.
- Respect repository resources: check host load/RAM and owned handles; one heavy job,
  nice19,2–4 build workers (user update2026-10-29), single-threaded libraries and at most six available local
  MPI ranks. Use the 4TB compile-tmp. Do not edit pinned sources/helpers during jobs;
  leave unrelated processes/files alone. Use 6.1 Terra for bounded routine tasks;
  retain integrated design and acceptance assessment with the primary agent.
- Keep this handoff focused on the current path; place changing job/build details
  and historical results in the linked records. Keep TODO/findings current. Save
  clearly labelled checkpoints, distinguishing WIP from validated capability.
  Existing approved push destination: rois1995/SU2, branch codex/native-3d-bl.
