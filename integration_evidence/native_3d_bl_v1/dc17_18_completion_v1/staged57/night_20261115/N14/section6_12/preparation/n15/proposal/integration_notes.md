# N15 lane-census proposal notes

Status: unapplied proposal only. This is a print-only M26 extension of the existing N12 `Slot` / `Trial` / `Counters`. It adds no timer operation, Session field, CellState field, traveling field, RepairProposal field, or result decision. It does not implement design step 8.5ab.

## Proposed files

- `CNativeLaneCensus3D.hpp.proposed`: proposed full header replacement.
- `native_lane_census3d_tests.inc.proposed`: proposed full fixture replacement. It changes the existing expected output-line count from 8 to 9 and adds focused class/proof/seconds/commit/disabled cases.

## Intended engine binding sequence

1. On the quality local-wall proposal path, use the already-produced `BoundaryParameterState::Changes` vector and `old` / `fresh` patches to call `DescribeWall` before `Directory::Publish`. Reuse the already-needed `SameSkin(old, fresh)` result and existing empty coverage-delta test; neither result is a new decision input. `link_local` is supplied by the engine's local indexed face check. Pass the actual accepted ladder kind.
2. The `Trial` destructor computes the existing elapsed duration once and adds that same `double elapsed` to both N12's row and `Slot::seconds`. There is no extra clock query. The class totals receive those slot seconds only at a successful quality commit.
3. On the existing successful quality commit receipt at the current lane `Commit(true, ...)` call, call `CommitWall(true, lane_slot)` after the successful publish/reservation path. Keep construction and shared-wall commits out of M26. The four M26 class counts then cover exactly the quality/local-wall subset; compare `wall_record.commits` to `records[1][1].commits` in the engine-side reconciliation.
4. Append one `NATIVE3D_WALL_LANE_CENSUS` row after the eight existing N12 lines in `Counters::Print`. Counters are already constructed with `options.timings`; disabled mode remains silent.
5. N14 may set `required_primary_split` and `checked_primary_split` in `Slot` as diagnostic booleans. These fields are deliberately not read by M26 classification, counting, or decisions.

The fixture proposal includes synthetic data only. It checks S/W1/W2M/W2O classification, old-face orientation in original order, the local-link and kind bins, totals over only explicitly committed slots, class seconds, reuse of exactly the recorded N12 trial duration, and disabled output. It has not been compiled or run.

## Sources read and SHA256

- Archived `NIGHT_WORK_ORDER.md` (6.11/6.12): `0c446a618cda84b707a66d27551bdd8344db1166206811ee3fa84c0ec9432464`.
- N14 connected `source_check.md`: `c8655c756c5e73702fd285d14442c650616847ae70335e4d097b0162f8ba9edb`.
- N15 `source_binding_inventory.md`: `3cb521d45885f6e01900aa0dc1ea8ba3895624db2015afa0cf1766897083bd67`.
- Current `CNativeLaneCensus3D.hpp`: `d26379e1aee755659c4bedfe12445d70dc23ceae51e130b7e05c09f2e1530b08`.
- Current `native_lane_census3d_tests.inc`: `651c77ea923cc2cf914d00d5ad2c4c375f229d73c02d7f7e5185e39e0a950421`.
- `CNativeBoundaryParameters3D.hpp`: `13e6c5d2f3d08b26d0a000e140457b3c9e1bc74eb6bc60f1fa69cb1c7de24421`.
- `CNativeMesh3D.hpp`: `42e2d7a305622bf8da8bc6da7685209cdd1619d7265723fee2136edd5b5c4aee`.
- `CNativePredicates3D.cpp`: `9adaa29e0b90112bd30e82208f3b4063249065056a8af06692de60d9d794a012`.

## Proposal artifact hashes

- `CNativeLaneCensus3D.hpp.proposed`: `6e4c4590fa4a66fca24c32d28353ad0fba54f1fe0b33c5a2c41d8d38ced34a8d`
- `native_lane_census3d_tests.inc.proposed`: `4aba6354a520b23eef4c590784266b86be27bafee19ea338f132bffc1b264cf3`
