# F3 event 1: source map for unattributed time

This is a source map, not a residual-time attribution. `event1_gap_attribution.json` reports quality-phase unattributed time of 619.987–773.172 s/rank (rank 4 highest), construction 52.156–58.582 s/rank, and root residual 9.852–10.015 s/rank. Complete-event root coverage is 68.36–74.36%. The receipt gives no call stacks or per-function elapsed time for the residual, so none of these seconds can be assigned to the source candidates below.

## Concrete quality work outside named operation scopes

- `validate_private` is called throughout the quality proposal paths (definition at `Common/src/adaptation/CNativeEngine3D.cpp:4478`; callers include 4536, 4653, 4947, 5102–5104, 5351, 5405, 5472, and 5481). Its callees already contain nested spans: `geometric_change` has a `GeometryBound` timer at 4057–4058, donor retrieval has `Import` at 4386–4391, and target construction/planning has `MetricNormalization` at 4396–4405. The specific local gap is the outer `validate_private` setup/aggregation at 4479–4487 and post-validation bookkeeping/observers from 4509 onward, which has no enclosing operation scope. Nested timers may cover substantial sub-work, so this gap alone does not show how much time remains uncovered.
- After private proposal work, final repair contact construction and checks at 5808–5830, including per-proposal dependency/version scans, have no enclosing named operation scope. The `world.maximum(needs_contact)` reduction at5831 has the shared World communication scope. `ClearanceBatch` itself is also outside a local scope at 5832–5833. The later reservation starts a `Communication` timer at 5853–5857. This is another concrete scope gap; the receipt does not establish its cost.
- The explicit full mesh quality sweeps in `MeshOptimizer::Run()` are already under `Phase::Quality` and `QualityOptimization` from 7210–7214 through return at 7259. They are not an uninstrumented candidate to explain the ledger residual. Rank 1 records 85.922 s exclusive for `quality_optimization`; other rank rows do not put this operation among their largest rows, which is not proof it was absent or cheap.

## Existing counters and what they measure

- At `observe_progress` (1802–1815), `Selection::Progress` and the completion reduction happen before the later `Output` scope (1817–1819). The output scope covers its reporting block, not those progress/census calls.
- `emit_quality_census` times its own loop over `selection.cells` / `selection.repair_users` and adds it to `diagnostic_seconds` at 1709–1754. Final `NATIVE3D_QUALITY ... stage=exit` records show only 0.02476–0.02973 s in the last reported window (538.886 s window); rank 0 is 0.02834 s. This small last-window diagnostic measurement does not explain that interval; total diagnostic cost across every prior window is not reconstructed here. It also does not measure the surrounding `Progress`, reductions, or other unscoped quality work. The records show `queue=0`, `blocked_features=0`, `stuck_shape=0`, `stuck_required=0`, and `missing_features=0` at exit; these are state counters, not elapsed-time attribution.
- The `NATIVE3D_STAGE` exit row exposes many stage counters and accumulated legacy timers (including update, guidance, smoothing, and collective seconds), but those totals overlap other ledger scopes and do not provide a complete exclusive decomposition. They cannot be subtracted to assign the gap. The gathered rank rows in `run.log` also report the existing `NATIVE3D_DETAIL stage=refine` collision service timer:

  | Rank | collision seconds | builds | candidates | checks |
  |---:|---:|---:|---:|---:|
  | 0 | 250.0793 | 322 | 124,891,620 | 13,127,392 |
  | 1 | 250.0749 | 336 | 126,424,143 | 13,589,029 |
  | 2 | 250.0736 | 255 | 120,011,423 | 6,601,107 |
  | 3 | 250.0764 | 272 | 121,416,178 | 11,287,829 |
  | 4 | 250.0723 | 350 | 123,681,727 | 14,397,882 |
  | 5 | 250.0781 | 224 | 124,006,037 | 7,157,416 |

  `Directory::ClearanceBatch` is backed by the legacy `Timer(stats.collision_seconds)` in `Common/src/adaptation/CNativeDistributed3D.cpp:355`; its interval has no local F3 operation scope, although World child communication can separately record protocol work. The collision timer is an inclusive service counter and overlaps the F3 ledger, so it cannot be directly subtracted from the unattributed residual.

## Old cavity measurement scope check

The receipt records `old_cavity_measurement` at 178,067–333,516 calls and only 0.168–0.333 s/rank (rank 4: 333,516 calls / 0.333 s). `OldMerit` is scoped at 6295–6300, and explicit old-cell shape-debt loops are scoped at 6568–6569 and 6768–6769. The coupled-boundary pre-edit quality counts are scoped at 7186–7188. The old-edge maximum scan in interior smoothing (`Estimate(x0, px)`) at 6771–6773 is outside that scope, as are old-edge `Estimate(pa,pb)` checks such as 6461–6463. Those are real omissions from the old-cavity label, but the measured scoped work is tiny; the evidence does not support treating these omissions as the source of the quality residual.
