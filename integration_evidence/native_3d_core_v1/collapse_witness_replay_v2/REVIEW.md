# Persistent coarsening rejection-witness replay

2026-10-12. Repository `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`;
branch `codex/native-3d-transport-accounting`; starting checkpoint
`1bfb136dbd6c3655db20afe73db96f670d076c8a`. Goal1 ACTIVE; production unchanged.

**A bounded bad-chord cache can identify some still-invalid retries after other
parts of a vertex star change. This offline eligibility evidence does not prove
512-round completion or justify a production filter yet.**

## Evidence and eligibility rule

The retained source-matched full_star_facet_trace_n8_v1 trace contains940complete
vertex-star proposals:428in the completed first coarsening event and512in the
second event truncated by the production limit. Earlier exact directional-bound
analysis reconstructs all proposals and matches every one of the709accepted
replacement topologies. Its45/69original nodal tensors give exact original-P1
lower bounds for changed chords; those source/trace/analysis pins are reverified.
Current boundary/metric code still matches the source required by that analysis.
This is not a fresh current-source SU2 execution.

Current `Selection::Touch` reactivates a previously rejected directed collapse
when its removed vertex star changes. The full-star trace established no exact
unchanged-star retry. A smaller obstruction can nevertheless persist: with fixed
retained/neighbor endpoints, a surviving cell containing removed+neighbor but not
retained necessarily introduces the same chord after collapse. If that chord's
exact frozen-field length lower bound exceeds1.8, the proposal remains invalid.
A single unchanged supporting tetrahedron is sufficient; the adjacency rule also
covers its replacement by another tetrahedron with the same relation.

Modelv1 uses exact-bound evidence from any rejected proposal. Modelv2 tightens
learning to the recorded oversized-edge rejection category. Production checks
fresh shape before integrating edges, so an early shape failure has no complete
length measurements to learn from. This distinction reduces the optimistic
v1eligibility counts; both models and their source pins are retained.

Modelv2 also stores bounded bad chords by endpoint pair, allowing the same chord
to identify a different directed collapse that would introduce it. It requires
that the chord is new relative to the admitted old patch, matches both endpoint
coordinates exactly, and was learned in the same immutable-target event. Dead
incident keys are discarded when a vertex is removed. LRU eviction is conservative:
forgotten evidence merely stops suppressing a proposal. Caps16/128entries and1/4
learned chords per rejection are exercised; cell-witness mode stores one supporting
quadruple per chord. Cache contents are eligibility proofs, not new nodal tensors,
metric interpolation, target regularization or field spreading.

## Results on the unchanged recorded trajectories

| Model,16-entry/one-witness control | First event: identified rejects of156 | Truncated second: identified rejects of75 |
|---|---:|---:|
| v1directed collapse, persistent adjacency | 7 | 23 |
| v2length-failure-only directed collapse, one supporting cell | 7 | 16 |
| v2length-failure-only directed collapse, persistent adjacency | 7 | 19 |
| v2length-failure-only shared bad-chord memo | 7 | 20 |

Every configuration retains all272+437accepted proposals. The shared memo's
second-event20consist of18same-direction and2different-direction matches;
maximum live entries12. Raising capacity to128or learning4chords does not
improve these counts. The first event reaches the16entry cap with22LRU evictions;
a128entry cache still identifies only7rejects. Sixteen entries are enough for
this observed benefit, not a universal sizing recommendation.

Dropping those20rejected records leaves492recorded attempts at the same historical
private geometry. This is **not** a new492-round adaptation: real scheduling,
reserved cell identities, query/context lifetimes, reactivation and MPI ownership
can change after a filter is introduced. The historical private exit still has
266pending directions. The separately certified full second event needs615
attempts and the third1002; this940-row trace does not cover their tails or predict
whether the cache would bring either below512. Actual repeated cost/scaling is
also absent. Do not relabel the valid-but-incomplete private output as success.

## Conservative integration idea and missing proof

If later evidence warrants an experiment, use a fixed-capacity **worker-local,
engine-call-local bad-chord memo** learned from successful measurement of a
rejected coarsening proposal. Check an owned surviving cell as a negative
counterexample during existing valid-field preselection; do not assume a partial
local star can certify success. Match canonical IDs and physical endpoint
coordinates, reset at a new target, and ignore composition callbacks unless a
separate invariant proof exists. Existing sensor-only edge reuse already provides
an analogous frozen-coordinate restriction. Shape failures cannot automatically
supply complete edge measurements. Keep geometry/dependency/transaction/final
q/J/L/state gates unchanged, including invalid-field repair.

The replay uses precomputed exact global nodal bounds as certificates. Its choice
of a strongest bound is not necessarily the actual measured longest chord.
Production needs conservative floating error treatment, bounded learning without
gathering all global tensors, local/remote ownership and lifetime tests, and
accounted cache bytes/lookups/scan costs. A worker-local memo may miss a proof
learned by a different worker; broadcasting it adds cost and is not proposed
without evidence. All metric queries that are actually made must retain the
existing actual-position sensor/geometry composition behavior. No cache feature,
extra collective, public config or CPP implementation is introduced here.

The modest7/20record benefit supports a targeted hypothesis, not promotion over
batching/ordering or measured donor optimizations. Before implementation, gather
source-matched full1024coarsening trace evidence only if needed for this decision,
with compact rejection witnesses rather than dumping all candidates by default;
then qualify actual new trajectories and MPI1/2/4 rejection/state behavior.
The existing standard-versus-rotating cluster comparison remains independent.

## Verification and preserved scope

Self-checks cover unchanged/replaced/lost support, moved endpoints, uncertified
bounds, bad-chord reuse by another collapse, no learning from a shape-only failure,
and explicit refusal to suppress a saved accepted proposal. Every replay checks
cache caps and all709accepted outcomes. Both analyses verify complete original
trace/analysis/source hashes before consuming data. V2 retains raw stdout/stderr;
V1's execution and exact output are recorded from the tool transcript in its
receipt. No source trace, CPP, normal executable, accepted CFD state, budget,
geometry, target, quality or cluster package changes. OneCPU/nice19/thread1
correctness analyses, no MPI/SU2 build/run, local profiling or cluster submission.
Goal1 performance/weight/wait/memory/overhead/default512qualification remains open;
Goal2 is not activated and no curved/3DBL/physicalunsteady/affordability claim follows.
