# Build293 no-BL reference extraction

Global mesh results and Adapt summary times come from the combined `run.log`; rank-local markers and counters come from `rank_output/1/rank.N/stdout`, which avoids combined-stream interleaving. All detail rows for `NATIVE3D_DETAIL stage=refine` are emitted in rank0 stdout. Per-row source paths and line numbers are in `reference293.json`. Phase intervals use `measurement_20261114_v2/analyze_B.py` `intervals_for_invocation` on isolated rank stdout, retaining marker scopes and no profiler clocks.

Rank0 first collapse marker: see JSON line provenance. First two split snapshots: 241.1358 s / 5047 accepted, then 274.1028 s / 9949 accepted. Adaptation 1 accepted: 149051 points / 736323 tets. Adaptation 2 accepted: 172001 points / 870422 tets. Native-inner refine totals: 3640.719 s and 2314.855 s. Combined solver summary `Adapt [s]` is broader: 3819.85 s and 2440.84 s.

JSON contains all six per-rank refine detail rows for each adaptation, all six final stage exits, plus marker-bounded phase intervals. Directory election counters come from refine details; World election/upper/geometry from final stage exits. `other` and `update` are refine detail fields. Interpret phases using each interval's parser semantics; setup and transition intervals are not pure timers.
