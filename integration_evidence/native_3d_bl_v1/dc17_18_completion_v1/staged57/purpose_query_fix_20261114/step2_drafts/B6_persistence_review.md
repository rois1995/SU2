# B6 persistence review: target-bound selection census

Read-only source review, 2026-11-14. No runtime files changed.

## Existing pieces

- `SU2NativeTargetCheckpoint3D::Encode/Decode` already persists the reusable target, not just a sensor: source node IDs/coordinates, selected tensors, full composed witnesses, owned frames/eigenvalues, layer controls, marker roles, full/base profile histories and stamps, Option-A radius, and encoded geometry/association authority (`CNativeTargetCheckpoint3D.hpp:115-163,168-241`). `CNativeMetricFamily3D::ValidateCheckpoint` checks source coordinates, profile/witness stamps, configuration and tensor validity (`CSolver.cpp:2653-2702`); `RestoreNativeMetricFamily3D` validates before publishing selected metric storage (`CSolver.cpp:2704-2727`). The fresh-process reload fixture recomputes and compares composed witnesses (`native_saved_cfd_metric_stage.inc:117-155`).
- Engine `Selection::CellState` and `CachedRow` already have `Fields` serialization, including geometry/quality/height/size debt, per-cell version and topology, six geometry scores and edge lengths, plus repair masks/epochs (`CNativeEngine3D.cpp:289-314`). `CachedScalar` persists long-double values losslessly as exponent plus existing `OriginalWeights3D` words (`:191-196`).
- Ownership migration is the producer/consumer precedent: stage every required row and edge length (`CNativeEngine3D.cpp:2917-2934`), exchange under memory admission (`:2937-2942`), reconstruct via `Selection::Restore` (`:975-994`), verify census coverage (`:2951-2955`), rebuild phase queues (`:2956-2957`), then atomically swap state (`:2960-2966`). Restore validates cell ID/version/vertex IDs and edge endpoints/coordinates/lengths, and rebuilds edge-user maps and aggregate debt.

## Minimal persistence contract

Persist a separate versioned selection-census sidecar next to the exact saved working-owned mesh and its parameter payload. Reuse the existing `CachedRow::Fields`/`CachedScalar` representation in `CNativeEngine3D.cpp`; do not move `Selection` into a new framework. Write only at a stable stage boundary after a complete census. This is a census checkpoint, not a mid-transaction scheduler checkpoint: omit transient queues/history and reconstruct them through existing `Restore` plus phase setup.

Header fields should be explicit:

- magic, schema version, payload byte count and checksum;
- census mode/scope (`SurfacePlanning` versus `FullTarget`), `census_started`, `progress_consistent`, and the phase/repair flags that determine how a restored row is interpreted;
- surface tolerance, wall-height policy (marker, first height, growth, thickness and tolerance), and relevant quality/size/geometry repair controls;
- stable target provenance: digest of the complete target checkpoint bytes (or an equivalent exact profile/authority/config stamp), plus the immutable geometry/parameter authority payload digest. Do not serialize `FrozenMetricIdentity*`;
- working-mesh provenance: sorted owned cell IDs and versions are carried by row records and checked against the independently restored owned mesh. The mesh/parameter checkpoint must retain coordinates, physical facet bindings and ownership;
- row count and serialized `CachedRow` records, including the six edge lengths and repair state fields already present.

On restore, load and validate the target and working mesh/parameter payload first. Compare target and control provenance, create a fresh event-local `FrozenMetricIdentity` with the recorded scope, then feed each row through `Selection::Restore`. Require every cell demanded by this census to have exactly one row and verify measured/required counts; reject duplicates, missing rows, mismatched version/topology/coordinates, target digest, policy, authority, or checksum. Recompute aggregate counters with `Account` during restore. Rebuild phase-specific queues via the same `ChangePhase`/`RestoreRepairState` path used after ownership migration. If the stored scope/config cannot be proven identical, discard the census and measure again; never treat an old row as valid by pointer identity alone.

## Producer / consumer integration points

- Producer: after the stable measurement/census boundary in `CNativeEngine3D.cpp` (the `measure` lambda sets `selection.census_started` at `:2635-2655`; only publish after its requested IDs all complete). The writer must receive the exact working owned mesh and parameter/target provenance used for those measurements. If a commit makes census state inconsistent, do not publish until reseeded/repaired to a complete boundary.
- Consumer: immediately after constructing the fresh `Selection` and before normal seeding/measurement in `CNativeEngine3D.cpp` initialization (`field_identity` is created at `:2142-2145`). Validate header against current mode/options/target and the already loaded working mesh; use `Restore` for row replay, then rebuild queues. The existing ownership-migration restore at `:2947-2958` is the behavioral template.
- Storage: include checkpoint input/output and row buffers in existing `admit(...)`/`bytes()` accounting; the migration path demonstrates this at `:2912-2942`. Do not count a sidecar as free just because it is written to disk.

## Why weaker state is insufficient

- A sensor tensor alone cannot reconstruct the same composed BL target. The composition also depends on full/base profiles and histories, mapped geometry/associations, marker roles, Option-A strips/radius, ramp and wall constraints. Existing target checkpoint stores these and composed witnesses specifically so fresh-process replay can verify the same target.
- `FrozenMetricIdentity` is an event-local shared-pointer token whose only field is a scope enum (`CNativeField3D.hpp:26-32`); it intentionally distinguishes separately imported fields in memory, but has no persistent identity. Serializing pointer value or scope alone would incorrectly authorize stale `EdgeMeasure` values. Bind a restored census to the complete target/geometry/config provenance, validate mesh endpoints, then mint a new identity only for that validated restore.
- Saving only aggregate census totals cannot restore per-cell invalid/debt/geometry records, validate ownership transfer, or rebuild edge users. Conversely, saving queue internals adds unneeded mid-event semantics; the current migration deliberately restores row certificates and reconstructs phase scheduling.

## Minimal tests

1. Round-trip a complete surface-planning census with a matching target and working mesh; restored `Progress`/`SurfaceCensus` counts and debt totals match, `Restore` does no metric/geometry recomputation, and queues rebuild.
2. Reject checksum/schema errors and mismatched target digest, controls/authority, cell version, vertex/edge coordinates, missing/duplicate demanded rows; verify failed restore leaves the live mesh/target untouched.
3. Verify sensor-only or a fresh `FrozenMetricIdentity` with a different target digest cannot admit cached rows; after an unrelated parameter-state revision with the same exact point bindings/mesh and identical target, validated rows remain reusable.

Scope limit: this sidecar resumes a completed census boundary. It does not resume an in-flight transaction or promise identical candidate ordering/attempt counts; those would require persisting the scheduler queues and history too.
