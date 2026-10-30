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
  User clarified500k complexity is RANS-only; current Euler target50k is endorsed.
  Build15150k was stopped on the user's geometry-aware MPI balancing request;
  all nine recorded handles are terminal and pins verified. Its baseline shows
  coarsening exit2272.87s and three productive split generations by2945.83s;
  global size hinge debt fell while bad-cell counts rose. No quality/acceptance.
  The5s profile has79%aggregate collective cycles and rank5's3.208GB prepared
  pool full. Source review/implementation now adds independent geometry weights
  to initial partitioning, cache/query telemetry and failure-election timing.
  Production build153 and test-only builds154/155 pass; prepared-piece observations
  are globally agreed before weighting. Corrected estimator155 passes42assertions;
  connected MPI6 passes13cases/rank. The clustered fixture's geometry max/mean
  improves2.0->1.028 with balanced metric/count and preserved records. Earlier
  fixture setup failures remain recorded. Build155 M6 was stopped at the user's
  request after3798.03s; pins verified and all nine handles gone. Last complete
  private split generation22 had484886 cells,46763 accepted splits, size debt
  16466.18, geometry debt0.00038483 and maximum metric edge3.8544; quality stages0.
  Initial balancing improved comparable stage times but sampled tree RSS rose to
  about20.8GiB. The user superseded waiting for the run result and requested
  two-lane split scheduling and bounded-progress handoff now. That connected
  implementation is built as157. Focused serial11cases/5306assertions and MPI6
  18cases/rank pass. The complete two-event M6 Euler50k attempt
  `euler_build157_split_lanes50k` is running; consult its status/resources and verify
  owned handles before acting. No accepted mesh, transfer or restart yet. The
  neighboring-edge lane fixture still needs the concurrent candidate-batch variant,
  and the stall-handoff counter needs printed/tested coverage at the next safe edit
  point. Existing bidirectional MPI checks already cover concurrent commits. Details: [geometry balancing](Handbook/ADAPTATION_GEOMETRY_BALANCING_20261030.md)
  and `staged57/build155_m6_summary.json`.
  The prior149 500k Euler attempt
  was stopped on that steering, pins verified/all recorded handles terminal.
  Build151 passes108.262s and retains build149's validated repeat-split staging.
  Upper-only geometry uses two conservative search shortcuts; captured queries
  retain identical bounds and reduce total work803053→630312 versus146. This is
  diagnostic query evidence, not an accepted mesh. Geometry upper-search passes
  8cases/185assertions; the sheared-graph section still exhausts its work budget,
  although its other14 assertions pass. It remains unqualified, not a whole pass.
  Evidence: staged57/euler50k151_checkpoint.json and dyadic144/comparison151.json.
  Require strict accepted Euler CFD → mesh → CFD → mesh, then BL/RANS/history and
  restart qualification; final gates remain unchanged. Earlier failure evidence
  and construction decisions are preserved in the linked handbook and receipts.
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
