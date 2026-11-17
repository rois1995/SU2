# B7 frozen B3 binding audit

Source commit: `ce25219f04cb01285bfeb8b33dba1e6a3496782a`; SHA256: `806e13eccc41df3a6e19602b38e1297dc8b9852fd4a6a8804af7a8b67d4d80e5`.
Controlling source check SHA256: `3fff8559d3f67b42c1314f2388fb8ba9d510fa064e625f53ef99df0d7f17f8a1`.
Plan/PART1 SHA256 values are recorded in `binding_audit.json`. This is read-only; no production or execution files were changed.

| Lambda | Current lines | Raw body | Formatted member preview + live aliases | Free outer identifiers |
|---|---:|---:|---:|---|
| `begin_length_generation` | 3079–3094 | 16 | 42 | `attempted_at_window`, `construction_claims`, `construction_generation_changed`, `deferred_at_window`, `directory`, `field_identity`, `phase_start_round`, `progress_at_window`, `selection`, `stats`, `working` |
| `surface_ready` | 3095–3098 | 4 | 7 | `selection`, `world` |
| `enter_phase` | 3099–3159 | 61 | 134 | `attempted_at_window`, `bidirectional`, `construction_changed`, `construction_claims`, `construction_entry_reported`, `construction_generation_changed`, `construction_handoff_progress`, `deferred_at_window`, `directory`, `field_identity`, `generation_window_count`, `geometry`, `improvement_changed`, `initial`, `length_blocks_dirty`, `metric_cleanup`, `options`, `phase`, `phase_start_round`, `progress_at_window`, `repairing`, `result`, `retired_edges`, `seed_slot`, `selection`, `started`, `stats`, `surface_target`, `working`, `world` |
| `discard_checkpoint` | 3160–3167 | 8 | 15 | `checkpoint`, `directory` |
| `capture_checkpoint` | 3168–3194 | 27 | 52 | `checkpoint`, `directory`, `geometry`, `options`, `private_parameters`, `result`, `selection`, `source`, `stats`, `working`, `world` |
| `rollback_checkpoint` | 3195–3233 | 39 | 69 | `campaign_edges`, `checkpoint`, `construction_changed`, `construction_handoff_progress`, `directory`, `improvement_changed`, `private_parameters`, `result`, `retired_edges`, `selection`, `stats`, `working`, `world` |

## Canonical bindings

### `begin_length_generation`

- `attempted_at_window` (auto&) → `*context.control.attempted_at_window`
- `construction_claims` (auto&) → `*context.exchange.construction_claims`
- `construction_generation_changed` (auto&) → `*context.setup.control_prefix.construction_generation_changed`
- `deferred_at_window` (auto&) → `*context.control.deferred_at_window`
- `directory` (auto&) → `*context.exchange.directory`
- `field_identity` (auto&) → `*context.census.field_identity`
- `phase_start_round` (auto&) → `*context.setup.control_prefix.phase_start_round`
- `progress_at_window` (auto&) → `*context.control.progress_at_window`
- `selection` (auto&) → `*context.census.selection`
- `stats` (auto&) → `context.control.result->stats`
- `working` (auto&) → `*context.ownership.working`
- Explicit parameters: `bool collapse`

### `surface_ready`

- `selection` (auto&) → `*context.census.selection`
- `world` (auto&) → `context.world`

### `enter_phase`

- `attempted_at_window` (auto&) → `*context.control.attempted_at_window`
- `bidirectional` (bool&) → `context.bidirectional`
- `construction_changed` (auto&) → `*context.setup.control_prefix.construction_changed`
- `construction_claims` (auto&) → `*context.exchange.construction_claims`
- `construction_entry_reported` (auto&) → `*context.control.construction_entry_reported`
- `construction_generation_changed` (auto&) → `*context.setup.control_prefix.construction_generation_changed`
- `construction_handoff_progress` (auto&) → `*context.setup.control_prefix.construction_handoff_progress`
- `deferred_at_window` (auto&) → `*context.control.deferred_at_window`
- `directory` (auto&) → `*context.exchange.directory`
- `field_identity` (auto&) → `*context.census.field_identity`
- `generation_window_count` (auto&) → `*context.control.generation_window_count`
- `geometry` (const auto&) → `context.geometry`
- `improvement_changed` (auto&) → `*context.setup.control_prefix.improvement_changed`
- `initial` (auto&) → `*context.census.initial`
- `length_blocks_dirty` (auto&) → `*context.queues.length_blocks_dirty`
- `metric_cleanup` (auto&) → `*context.setup.control_prefix.metric_cleanup`
- `options` (auto&) → `context.options`
- `phase` (auto&) → `*context.setup.control_prefix.phase`
- `phase_start_round` (auto&) → `*context.setup.control_prefix.phase_start_round`
- `progress_at_window` (auto&) → `*context.control.progress_at_window`
- `repairing` (auto&) → `*context.control.repairing`
- `result` (auto&) → `*context.control.result`
- `retired_edges` (auto&) → `*context.census.retired_edges`
- `seed_slot` (auto&) → `*context.census.seed_slot`
- `selection` (auto&) → `*context.census.selection`
- `started` (const auto&) → `*context.setup.control_prefix.started`
- `stats` (auto&) → `context.control.result->stats`
- `surface_target` (auto&) → `context.surface_target`
- `working` (auto&) → `*context.ownership.working`
- `world` (auto&) → `context.world`
- Explicit parameters: `EnginePhase next`, `bool refine_geometry`, `Observer& observe_progress`

### `discard_checkpoint`

- `checkpoint` (auto&) → `*context.control.checkpoint`
- `directory` (auto&) → `*context.exchange.directory`

### `capture_checkpoint`

- `checkpoint` (auto&) → `*context.control.checkpoint`
- `directory` (auto&) → `*context.exchange.directory`
- `geometry` (const auto&) → `context.geometry`
- `options` (auto&) → `context.options`
- `private_parameters` (auto&) → `*context.setup.private_parameters`
- `result` (auto&) → `*context.control.result`
- `selection` (auto&) → `*context.census.selection`
- `source` (auto&) → `*context.exchange.source`
- `stats` (auto&) → `context.control.result->stats`
- `working` (auto&) → `*context.ownership.working`
- `world` (auto&) → `context.world`

### `rollback_checkpoint`

- `campaign_edges` (auto&) → `*context.census.campaign_edges`
- `checkpoint` (auto&) → `*context.control.checkpoint`
- `construction_changed` (auto&) → `*context.setup.control_prefix.construction_changed`
- `construction_handoff_progress` (auto&) → `*context.setup.control_prefix.construction_handoff_progress`
- `directory` (auto&) → `*context.exchange.directory`
- `improvement_changed` (auto&) → `*context.setup.control_prefix.improvement_changed`
- `private_parameters` (auto&) → `*context.setup.private_parameters`
- `result` (auto&) → `*context.control.result`
- `retired_edges` (auto&) → `*context.census.retired_edges`
- `selection` (auto&) → `*context.census.selection`
- `stats` (auto&) → `context.control.result->stats`
- `working` (auto&) → `*context.ownership.working`
- `world` (auto&) → `context.world`
- Explicit parameters: `Observer& observe_progress`

## Size and split

`enter_phase` is the oversized body. Extract its early wall-repair branch (source lines 3100–3112), bidirectional direction setup (lines 3114–3122), and normal-entry tail (lines 3136–3159). Formatted previews are 41 lines for `EnterSurfaceRepair`, 24 for `PreparePhaseDirection`, 52 for `CompletePhaseEntry`, and 54 for `EnterPhase`; each includes current capture aliases.

These are existing ordered blocks. The wall-repair branch remains before the normal transition; direction setup stays before construction-entry reporting and state transitions; the final measurement/priority/ownership/observer tail stays after admission and optional length-generation setup. `begin_length_generation`, `surface_ready`, `discard_checkpoint`, `capture_checkpoint`, and `rollback_checkpoint` fit under 80 lines with their current required aliases in the preview. Rebind if B5 changes `observe_progress`; keep the observer synchronous and passed by reference.

The method previews measure formatting and borrowed aliases only. They are not generated C++ bodies and make no token-equivalence claim. Exact line counts and canonical bindings are in `binding_audit.json`.
