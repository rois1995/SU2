# FP2-P1 build670 first-collapse diagnosis

The closure-on safeguard failed at `upper_work_sum=111921030` against the unchanged ceiling of 108459426. This exceeds the ceiling by 3461604 work units. The closure-off safeguard used 90809869 units. The paired launches used the same frozen binary, configuration and mesh, reference and restart inputs. Trace was off and all six opt-in process markers were present. Both runs reached the controller's first-collapse boundary with the same six accepted counts, zero upper refinements and terminal processes.

| Existing stage counter | Closure off | Closure on | Change |
| --- | ---: | ---: | ---: |
| Upper work | 90809869 | 111921030 | 21111161 |
| Upper calls | 22889 | 22889 | 0 |
| Scope builds | 12494 | 12494 | 0 |
| Scope build work | 2645341 | 13433251 | 10787910 |
| Traversal work | 80858515 | 91181766 | 10323251 |
| Piece selection work | 4218009 | 4218009 | 0 |
| Facet build work | 1447418 | 1447418 | 0 |

The two changed counters account for the entire increase in upper work. The scope-build counter spans the authorized closure and the subsequent joined target construction. The traversal counter measures proof work on that target. The counters give the work change in those two parts without attributing individual units to the closure's chain search.

The read-only source check revisited the work-charge sites and query placement. The input chord, inspected edge, entered chain iteration, structurally eligible adjacency and visited child each use the specified existing work charge. Closure executes once on a curved scope miss after the original witness trace and before constructing the joined target. The trace-only scan is outside this path. The observed scope and call counts match across settings. No transcription or wiring error was found. Under the signed closure-on failure rule, keep the default-off production candidate, retain the failed check, and record the dependent S workflow as not evaluated.

[Per-rank counters and evidence hashes](first_collapse_r5_670.json)
