# Bounded bad-cell and cavity export

The missing diagnostic is the actual rejected working mesh around the surviving
bad cells. Build199 naturally time-budget rejected with a complete census of
802117 cells: 4368 q-bad, 2537 J-bad, 3098 geometry-bad and zero size-bad. Its
aggregate counters cannot recover point placement or an individual cavity.
Production CFD installed no working-state observer, and the private mesh was
discarded. Original-grid restart files do not contain that rejected mesh.

The user selected this diagnostic as the next work within the existing cycle.
The complete goal remains two strictly accepted M6 Euler events at50k, conservative
admissible transfer, finite continued CFD and isolated adapted disk restart; then
BL/MPI qualification and RANS500k. All gates and budgets remain unchanged.

## Connected implementation

Reuse the existing stage/rejected-state/trial observer APIs and the bounded
rank-local `NativeStageCapture` streaming precedent. Do not gather the complete
volume mesh or add a scheduler, a configuration option or a second baseline run.

1. Capture once at the final rejected exit, before the working mesh and matching
   private geometry are released. Retain global node/cell IDs, coordinates,
   versions, physical faces, current nodal tensors and association information.
   Wire production CFD through the existing observer facility and existing output
   naming. Keep diagnostics separate from accepted mesh publication.
2. Expose already measured cell-cache values to the observer. Export actual q/J,
   measurement/completeness flags, geometry excess with its precise definition,
   all six cached metric edge lengths, boundary/feature classification and blocked
   repair-feature state. Preserve actual q/J while they are measured; do not add
   q/J work to validity-only construction or recertify the failed mesh after its
   geometry budget is exhausted. Missing measurements must be explicit.
3. Retain bounded recent rejection metadata using the existing rejection hook:
   feature, operation/ladder, attempted cavity identifiers, diagnosis, reason and
   state/round context. Keep this small; do not copy complete patches per trial.
   Distinguish a historical rejection from evidence applying to the final cavity.
   Do not invent reasons for cells or features without matching evidence.
4. Stream bounded rank-local raw shards using the existing snapshot writer's
   byte-cap and overwrite-refusal practices. Reserve bounded scratch through the
   existing observer admission. Report diagnostic I/O, resource refusals and any
   truncation independently; diagnostics never authorize a mesh edit or weaken a
   gate. Empty ranks must produce valid empty records with aligned control flow.
5. Reuse exact reference-sidecar/encoder facilities and the active point bindings.
   Carry immutable authority provenance, genuine features, body/sheet/marker IDs,
   curved controls and requested-height policies. Keep CAD optional. A rendered
   reference approximation is a visualization, never a new geometry certificate.
6. Offline, combine the bounded shards by global IDs and extract the remaining
   bad cells and their complete neighboring vertex-star cavities, including rank
   cuts. Produce inspectable volume/surface artifacts and a compact manifest with
   q/J, geometry excess, metric edge lengths and rejection evidence. Bound the
   selected/output population and memory. State total/retained/omitted counts and
   cavity/reference completeness; never silently clip a cavity and then infer
   that a topology change or a constrained move is impossible.

The raw snapshot may stream a bounded whole owned shard so that offline extraction
can reconstruct cross-rank stars without collectives in an observer. It is not a
gathered mesh or a restart. Reuse the existing128MiB-per-shard precedent initially;
make saturation explicit and validate it. Final filtered artifacts should focus
on the bad cells and neighboring cavities. The exact limits and emitted schema
must be recorded with the implementation and actual artifacts.

## Acceptance and return to M6

Focused serial checks must cover cached values versus direct measurement, no
construction q/J measurements, correct local cavities and reference associations,
unchanged accepted inputs/decisions, output caps, empty data, overwrite refusal,
and incomplete or stale rejection evidence. Check the offline parser/exporter on
actual produced data, not a hand-written imitation.

One MPI6 milestone must check cavities crossing ownership cuts, global ID/version
matching, empty ranks, preserved census flags and curved/feature/body/height
metadata. Diagnostic storage and I/O failures must not turn rejection into
acceptance. Reuse unaffected qualification evidence with verified provenance.

Then immediately run the full two-event M6 using the same saved real CFD seed,
50k target,5400s native budget and all unchanged gates. If it rejects, validate the
actual bounded export and inspect where surviving bad cells lie, whether movement
is constrained, and which topology trials failed. Use that evidence to select
placement, repair-power or construction work. If accepted, pursue the second
accepted mesh, transfer/CFD and isolated adapted restart immediately. Passing
export fixtures or lower memory/work counters are not workflow completion.
