# F3b frozen-source audit

Audit baseline: git HEAD `856b89ccb40f92d9a6fe7bccaf4dc3278f48bb02`. Four changed files only: timer-tree header, engine, distributed directory, and timer fixture. This was read-only; no build/test/job was run.

## Collective preservation and counter behavior

A source parser compared every scalar reduction argument tuple and order in `Adapt` with the HEAD snapshot. HEAD has 78 invocations across 77 source lines (two calls share line 1215). The frozen edit has 77 `TimedAllreduce` calls plus the single direct join call; all 78 original `(value, op, communicator)` tuples and their source order are preserved. All arguments are scalar expressions, matching `CPassiveComm::Allreduce(T value, Op, Communicator)`.

The unique `private_work_used` reduction remains at engine line 5809 inside `JoinWait`; the enclosing legacy `joined` timestamp and `stats.quality.join_wait_seconds` update remain at lines 5805 and 5811. `stats.quality.private_tracked_collectives` accounting remains unchanged at line 5804. The helper only opens `Communication` and calls the original scalar reduction; it does not touch `World` collective/election counters. No edits to those counters or the legacy join counter were found.

## Scope and operation audit

The only new operation names are `JoinWait` and `Clearance`. Requested F3b scopes are present: `Census` over progress/census work at engine 1819–1824; `Candidate` at the `validate_private` lambda entry; contact validation/list assembly at 5823–5845; `Clearance` at `Directory::Clearance` and `Directory::ClearanceBatch` entries (distributed 344 and 357). The `ClearanceBatch` legacy collision timer remains intact and is nested beneath the new scope. Communication instrumentation for direct reductions is the only other operation wrapper added. No numerical or resource-counter code changes were found in these four-file diffs.

## New function/lambda sizes and largest functions

New callable bodies are small: `TimedAllreduce` has a 2-statement body (engine 88–91); the join IIFE spans 4 lines with a 2-statement body (engine 5807–5810); the operation-name test spans 6 lines with two assertions (fixture 162–167). The Census, Candidate, and contact scopes were inserted into existing functions/lambdas; no new broad callback or large helper was introduced.

For context, the ten longest function definitions in the two changed `.cpp` files are listed by inclusive source-line span (signature, body, and closing brace; nested control flow is included):

| Lines | Function | File lines |
|---:|---|---:|
| 4,871 | `Adapt` | Engine 1183–6053 |
| 216 | `Directory::ClearanceBatch` | Distributed 355–570 |
| 204 | `MeshOptimizer::SmoothBoundary` | Engine 6550–6753 |
| 152 | `Directory::LookupImpl` | Distributed 616–767 |
| 139 | `MeshOptimizer::Smooth` | Engine 6754–6892 |
| 139 | `Directory::CorrectCollapse` | Distributed 768–906 |
| 134 | `Directory::PrepareParameters` | Distributed 1441–1574 |
| 118 | `Directory::FetchBatch` | Distributed 979–1096 |
| 99 | `MeshOptimizer::CoupledBoundary` | Engine 7127–7225 |
| 90 | `Directory::Prepare` | Distributed 166–255 |

These are existing bodies, not newly added implementations. The new instrumentation itself stays within the short bodies above.

## Formatting dry-run

`clang-format-18 --dry-run --Werror` passes for the current timer header and fixture; their HEAD snapshots also pass when checked via stdin with their original repository paths as `--assume-filename`. The engine and distributed source files fail dry-run both at HEAD and in the frozen current state, showing substantial existing formatting debt (HEAD/current diagnostic output lines: engine 101,592/101,574; distributed 13,887/13,887). No formatter was applied. This comparison does not treat the existing source-wide failures as newly introduced by F3b.

## Audit pins

SHA-256 recorded at audit start and rechecked at completion:

| File | SHA-256 |
|---|---|
| `Common/include/adaptation/CNativeTimerTree3D.hpp` | `d6c090372903b9107634a9f208644762803c8b0671a622dace64a7b8bc449709` |
| `Common/src/adaptation/CNativeEngine3D.cpp` | `ecd69bf8b42a2f2679695c3ae6f084e9033758b1448c13e71ae3126276db8ea0` |
| `Common/src/adaptation/CNativeDistributed3D.cpp` | `07e62d504e2251865140522404a06ef4eeec2bf5c651e5d93ecacc51d0f9c8a5` |
| `UnitTests/Common/adaptation/fixtures/native_timer_tree3d_tests.inc` | `be0a2841ce1e10bde71bf443cb4d03532fab24d041a00a4f9a015399d955bc55` |
