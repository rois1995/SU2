# CUT B2 exchange binding inventory (preparation only)

Scope: source binding for the six existing `Adapt` lambdas. This is not a cut, source check, or acceptance. No production/test code or existing evidence was changed; no build, test, or solver was started.

## Frozen source and design inputs

| Input | SHA256 |
|---|---|
| `Common/src/adaptation/CNativeEngine3D.cpp` | `5d95a9c8556084c7faaa10cb04603aea7e10354dc942cd89162d7f27210511a7` |
| `Common/src/adaptation/native_engine3d/EngineExchange.hpp` | `16738e08de8c69af0a5b493004b501030cbbb5ffae1de80b35f6638a7eeda3d1` |
| `Common/src/adaptation/native_engine3d/EngineExchange.cpp` | `f094321c8de30e2278629813621b35dea43f60b6e6b095f551b2ca55cc431c62` |
| `.../inputs/CUT_PLAN_8_2.md` | `c48ae8a9c59ce693a810248806e6fe6ffa76c3400924f157d6081f5f5e16492b` |
| `.../inputs/CUT_INVENTORY_PART1.md` | `034275df32e2f4b7f4ef9bb2aef0bc16453f6d0249971b90fc42ba6a33b663cd` |

Current absolute source locations: the six lambdas are `elect_trial_failure` 1647, `vote` 1651, `bytes` 1696, `real_bytes` 1703, `admit` 1704, and `import_source` 2657. These line references correspond to the source SHA above.

## Slot ownership and proposed bindings

`EngineContext` is noncopyable/nonmovable and borrows `world`, `original`, `geometry`, `composition`, `options`, and other canonical inputs. It owns the `control`, `census`, `queues`, `exchange`, `commit`, and `ownership` slot groups. `EngineSetup` owns the setup RAII and `control_prefix`; its order must remain unchanged. `ContextValue` emplaces into an optional slot at the original Adapt scope and resets it at scope exit. Thus a member binding must read the already-live slot; it must not create a value copy or extend a `ContextValue` lifetime.

| Lambda | Canonical bindings (no copy) | Callers / effects and boundaries to preserve |
|---|---|---|
| `elect_trial_failure` | `context.exchange.private_repair` (live dereference each call), `context.world` | Only called from `vote` at 1666 and 1670. In private repair, returns a local `CElectedFailure` using `world.rank`; otherwise calls `world.elect(local)`. Preserve this branch and election call count/order. |
| `vote` | `context.setup` budget source, `context.setup.control_prefix.atlas_work_initial`, `context.exchange.private_repair`, `context.control.result`, `context` stats/options/world | 132 direct invocations span setup, census/layer/measure, and round/slot paths (first at 1709; last at 6440). It polls the live time callback only in private repair; snapshots work/peak stats; overlays sticky work/backend/time failures; elects once, and elects the original local failure a second time only for a sticky refusal. It writes result diagnostics/validity and stop reason. It is conditionally collective: `world.elect` except while the live private-repair flag is true. No timer scope or catch is inside it; callers retain existing catches and timer scopes. |
| `bytes` | Exact existing operands: `options.caller_bytes`, wall/region maps; `control.accepted_bytes`, `batch_bytes`, `diagnostic_bytes`, `checkpoint`, result split/collapse vectors; `ownership.working`; `commit.batch_changed/batch_touched/batch_supports`; borrowed `original`; `census.initial`, `eligible_extensions`, `selection`, `retired_edges`, `campaign_edges`, `field_identity`; `queues.inactive_blocked_edges/links` | 57 direct calls across setup, admission, import, directory operations and trial paths. Pure capacity/map accounting; no collective, timer, or catch. Keep operand sequence, capacity-based helpers, and overflow behavior exactly. No omitted or newly counted slot. |
| `real_bytes` | `bytes()` plus live setup `atlas_geometry` and `control_prefix.atlas_scratch` | 5 direct calls (2483, 2636, 2889, 6085, 6378). Returns live bytes less atlas scratch only when atlas geometry exists and bytes are not `SIZE_MAX`. No collective/timer/catch. |
| `admit` | `bytes`, live `exchange.source` and `exchange.directory`, `options.memory_ceiling`, `control` stats/result via `vote`, setup atlas geometry/budget | 40 direct calls across initial admission, setup, measurement and rounds. Computes peak from bytes + source resident + directory resident + extra; records maximum; submits severity-3 failure through `vote`; after successful vote only, updates the live atlas caller/maximum byte window. Its collective behavior is inherited from `vote`; preserve that ordering. |
| `import_source` | live `exchange.source`, `exchange.directory`, `options`, `bytes()` | Exactly two call sites: layer donor query at 2761 inside an `Import` timer; measure batch at 2960 inside the caller’s `Import` timer and `measure`’s `Census` scope. It increments layer or donor batch count, then calls `source.ImportCells(cells, memory_ceiling, Add(bytes(), directory.ResidentBytes(), extra))`. The `requests` parameter at 2658 is intentionally ignored, including the `&requests` passed at 2960; do not forward it. Preserve surrounding `SetCavity`/clear and retry behavior. |

## Lifetime, ordering, and hazard notes

- `private_repair` is created false by `ContextValue` at 1643 and referenced at 1644. It is set true in repair blocks at 5890 and 5934, then false at 5914 and 5937. `vote`/`elect_trial_failure` must inspect that live optional value on every invocation; capturing a bool at member construction would change the local-vs-collective choice.
- Directory/source lifetimes are activated earlier via `ContextValue` in `EngineExchange` slots. `source` refers to canonical `working` and `World`; directory retains the geometry/world references. Do not move/copy/reconstruct them as part of method extraction.
- `RoundState` and `SlotState` hold cross-block/slot trial data. They do not own or snapshot the B2 exchange state. The six proposed bindings consult current `EngineContext` slots at invocation time.
- `vote` and `admit` are collective boundaries on ordinary paths but local during private repair. Keep every caller-side `vote`/`admit`, the possible second election, and their original conditional/catch placement. `bytes`/`real_bytes` are accounting helpers only. `import_source` reaches the import service and existing timer scopes remain at its two callers.
- No new timer, catch, allocation, storage slot, forwarding wrapper, or accounting operand is proposed. Existing `requests` non-forwarding is a required behavior. Source inventory’s N4-wide 22 readings remain controlling; this binding note makes no new proposal or gate judgment.

No unapplied code draft is included: the methods have cross-unit dependencies and their literal extraction should be reviewed against the live `EngineContext` call signatures by the integration owner.
