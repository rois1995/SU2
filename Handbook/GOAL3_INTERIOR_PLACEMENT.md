# Goal3 — coordinated interior transition construction

Working branch `codex/native-3d-bl`, repository
`/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`.
Full Goal3 remains active and incomplete; this page does not replace its gates.

The published420647bf59 checkpoint qualifies a local32→38-cell cap replacement,
but its strict saved438-cell whole-grid run still rejects at473 cells/287 oversized
edges. The last reported cone failure is only a diagnostic endpoint, not proof of
physical infeasibility. The associated interior edge has a four-free-apex cover
whose complete stars fit40 cells; the existing64-cell bound is retained.

The connected implementation under qualification now supports an interior inserted
point away from the old diagonal while retaining the complete cavity skin. Positive
tetrahedra, complete-edge admission, preserved physical reference/markers and full
geometry/metric/height validation remain mandatory. Surface behavior is retained.
The shared placement search adds two transverse point freedoms for interior edges,
actual-metric steps and coordinated free-apex movement. A bounded best private
proposal is saved even when shape/progress fails; it is never an accepted mesh.

Production now admits the actual proposed edge/apex stars independently of unused
endpoint support from abandoned proposals. Original edge dependency versions and
owners remain checked; moved stars are complete, reserved and subjected to the
unchanged final streamed metric and external clearance gates. Explicit placement
trial/validated counters expose actual execution; validation precedes publication.

The first interior replay rejected before optimization because the old duplicated
cover selection chose an edge endpoint as a free apex when no physical facet was
present in the local edge patch. Its closed negative evidence is preserved in
`wall_reconstruction_v1/curved_interior_endpoint_selection_rejected_v1`.
Selection is now shared by engine and replay through `SplitPatchApices`, which
excludes endpoints and locally known physical vertices. Imported complete stars
must still establish that provisional candidates are globally free.

## Current evidence

The first positive coupled replay passed702 assertions in4 trials on40→48 cells.
Its exact independent geometry audit passed. Inspection showed all four apices
retained their original coordinates; only the inserted point moved. That proof
is retained under `curved_interior_coupled_pass_v1` and
`interior_coupled_geometry_v1.json`.

The engine now tries spatial point placement on the complete original edge cavity
before importing optional endpoint/free-apex support. Coupled expansion remains a
fallback; stationary points retain their original identities to avoid manufacturing
changed-edge debt. `PlaceSplitPatch` permits a point-only proposal. The actual
473-cell replay passes158 assertions in4 trials on8→16 cells with no apex moves.
Exact independent positive volumes, incidence, non-overlap, fixed interface and
volume/donor/reference checks pass. `interior_geometry` preserves the84-assertion
primitive/cover control at its recorded build; the latest private replay independently
checks the changed path on its current binary. No full regression was repeated.

Bounded actual-engine MPI1/2/4 cases all pass their protocol checks:

| Ranks | Rejected cells | Placement trials | Validated placements | Work |
| --- | ---: | ---: | ---: | ---: |
|1|533|325|6|512|
|2|596|482|15|512|
|4|549|408|8|512|

Every case executes the new path, preserves the accepted input signatures and
stops at its work ceiling. Independent audit verifies every artifact, current
relevant C++ source, a common recorded test binary, exact before/after original
sensor donors/reference and identical physical donors across rank counts. This
is protocol/state/source preservation, not final mesh or M<N/CFD transfer proof.
Curved selection is still owner-dependent, so trajectories differ. No scaling claim.

The strict original-field continuation of the saved473-cell grid still rejects,
but advances165 edits/164 insertions to1110 cells. Native qmin .200176394 and
Jmin .050160731 pass;174 edges remain oversized. Squared length excess drops from
142.380264709 to3.482565802. It consumes the unchanged4096-work ceiling
(2920 placement trials/41 validated placements), rather than an elapsed-time stop.
No CFD replacement occurred. The remaining target is mandatory; partial progress
is not robust complete generation or evidence of speedup.

Evidence: `wall_reconstruction_v1/interior_placement_summary.json`,
`joint_protocol_audit.json`, `curved_interior_search`, `curved_joint_protocol`,
`curved_joint_protocol_mpi2`, `curved_joint_protocol_mpi4`, `curved_interior_engine`.
The last folder retains `curved_grid_partial_rejected.su2`, JSON/reference and the
full native residual summary. Complete receipts/source snapshots preserve both
failed and passing probes, including the first endpoint-selection failure.

## Next substantial step and remaining gates

Audit the1110-cell residual and perform a bounded original-field continuation to
distinguish remaining productive repair from a local stall. Preserve cumulative
work/time and the exact original target. The4096 ceiling was reached during useful
progress; that does not justify silently raising a production/configured limit.
Use actual failed private reconstructions to guide any further coordinated
reconnection, rather than another ladder of fixed placements or cold reruns.

Full curved construction/removal, interacting thin integration, independent
combined-field gradation/metric/geometry qualification, M<N/current+BDF/SA/CGNS/
reference/restart and physical RANS/M6/accuracy/performance gates remain open.
SST/transition/non-ideal gases remain the separately saved deferred questions.
All owned jobs terminal. Full Goal3 ACTIVE and incomplete.
