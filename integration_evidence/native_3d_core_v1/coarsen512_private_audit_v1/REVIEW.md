# Independent audit of the production 512 private rejection — 2026-10-12

Repository: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch: codex/native-3d-transport-accounting. Starting checkpoint:
b34877d709b9de2ad00150f439e001d400f6db10. No production/config/guard change.
Goal 1 remains active; Goal 2 is not started.

## Result

The retained full_star_facet_trace_n8_v1/coarsen_rejected_s1.su2 private export
passes a fresh independent exact geometry and frozen original-P1 metric audit:

| Quantity | Independently measured |
|---|---:|
| Tetrahedra / points / physical triangles | 517 / 153 / 222 |
| Exact domain volume | 1/2 |
| Minimum mean-ratio quality | 0.20433538142016777 |
| Minimum scaled Jacobian | 0.08825346092225352 |
| Maximum integrated metric-edge length | 1.7946315664648058 |
| Cached invalid cells at the original exit | 0 |
| Pending coarsening directions at the original exit | 266 |
| Original attempted / accepted / rejected operations | 512 / 437 / 75 |

The unchanged physical criteria are q>=0.20, J>=0.05 and L<=1.8. The wrapper
explicitly asserts L<=1.8 after the existing auditor, rather than relying on its
roundoff allowance. Positive exact volumes, tetrahedral SAT no-overlap, complete
oriented incidence/skin, sphere/disk vertex links, convex domain containment,
finite facet coverage and physical marker areas pass. Original-P1 interpolation
and full chord integration use the preserved second-event frozen source. There
is no original-facet or sensor modification to obtain these bounds.

This converts the earlier cached-valid observation into independent mesh/metric
evidence. The failure is queue completion under the bounded coarsening search,
not a demonstrated violation of these physical mesh gates at the private exit.
The source header describes CoarseningMetricEdge=.9 as candidate hysteresis,
while final upper/shape gates remain strict. Engine completion additionally
requires its coarsening queue to empty; it rejects when the 512 rounds end with
266 directions pending. Pending directions need not all be admissible, and this
audit does not test them or demonstrate how many more edits are necessary.

The separate 1024 diagnostic's eventual 457-cell/133-point accepted result remains
a different fully completed outcome. This 517-cell partial private mesh was never
installed in CFD. It has no newly transferred solution/BDF history, actual CGNS
readback or accepted driver replacement certificate. Its valid geometry alone
does not justify calling the original adaptation successful or complete.

## Input authority and exact conversion

The original source-matched bb87 trace's analysis and complete artifact hashes
are verified before conversion. The retained export is copied byte for byte as
coarsen512_diagnostic_original_rejected.su2. The existing capcheck.read_su2 parser
reads the actual tetrahedra and markers. audit_geometry_bridge.su2_signature
independently verifies positive orientation and its complete simplex/marker
signature. Local node/cell IDs are reconstructed without coordinate changes;
all points are retained original source points.

The trace labels physical markers reference_marker_K, where K is the marker
index, not the reference facet ID. Original marker names are retained from
envelope_s1_initial.json. This particular fixture has one convex planar facet
group per marker; the converter checks that condition explicitly and assigns
each original group root. Both geometric simplex counters and the renamed full
marker signature must exactly match the actual export. A deliberate wrong
marker assignment changes the signature and is rejected by the self-check.

coarsen512_rejected.json contains the reconstructed mesh, untouched reference
facets/groups/names and complete original frozen donors. Existing audit_coupled.frame
and its original dependencies perform the full independent audit; none is edited.
The generated coarsen512_rejected.su2 is inspectable and uses that auditor's side_K
labels. original_marker_names and marker counts in independent_audit.json explain
both labels. Source exports/certificates are hash-checked unchanged after auditing.

The audit's extra reference normals/points exist only in its returned in-memory
object. The saved JSON retains the original reference representation. All tools,
inputs, outputs and original trace certificates are pinned in independent_audit.json
and validation.json. This is a fresh audit of retained historical private data,
not a fresh execution of the current c1bdef2a binary's 512-transaction trajectory. No historical
source pin is silently presented as current-source runtime proof.

## Implication for the next implementation decision

First decide how bounded coarsening progress should be handled, rather than
assuming this private output needs a geometric repair. Keeping the existing
queue-exhaustion contract requires more useful proposals per bounded transaction
or better ordering/filtering. The earlier exact-bound experiment only identifies
some doomed proposals and does not prove completion within 512 rounds. A valid mesh with unfinished
coarsening might support an explicitly reported partial-progress policy, but that
would change the existing completion/rejection contract and require separate
specification, distributed validation, complete state transfer and forced-budget
controls. This audit does not implement or validate that policy.

The current source performs at most one private proposal per worker per round.
For an already strict mesh, accepted direct collapses remove one vertex apiece,
and rejected proposals consume rounds too. A fixed 512-round cap cannot represent
a mesh-size-independent full-coarsening envelope. The measured practical limit
must therefore remain explicit; this observation does not authorize increasing
the default or hiding incomplete coarsening behind a success status.

Runs use one CPU/nice19/one numerical thread after actual host/memory/process
checks. Four unrelated FSI ranks were active. All owned work is terminal. This
is independent correctness analysis, not a local profiling or calibrated timing
campaign. No SU2 executable, MPI job, cluster submission or accepted state was
changed. Repeated cluster cost/scaling/overhead/rank/weight/wait/memory qualification
and production completion limits remain open before full Goal 1 completion.
