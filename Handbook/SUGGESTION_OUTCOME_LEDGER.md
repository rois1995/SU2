# Suggestion outcomes — current index

Full Goal3 is active/incomplete. This index separates measured scoped results from
unimplemented work and whole-event claims. Detailed successes, failures, rejected
experiments, accuracy/work/budget/memory/communication records and source provenance
are preserved in the [complete chronological ledger](SUGGESTION_OUTCOME_LEDGER_HISTORY_2026_10_18.md).
That archive stays in the same directory so its existing relative links remain valid.
The [initial suggestion snapshot](SUGGESTION_INITIAL_SNAPSHOT_2026_10_16.md) is retained.
This is not a completed-goal final report.

| Suggestion | Current outcome | Quantified scope and remaining limits |
|---|---|---|
| Match normalization accuracy to integration uncertainty | Worked in controlled3D workflows | At most24 correction trials/two frozen stages using the SAME original unclipped family; eight planar RANS workflows independently0.194--0.323% below budget. M6 initialization now freezes295068 +/-5769 against300k, explicitly integration_limited. No exact-root claim. [Stages](GOAL3_FROZEN_NORMALIZATION_STAGES.md), [M6 evidence](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/adaptive_budget_evidence_v1/README.md). |
| Allocate absolute error globally | Partial | Owned reductions, global sensor/correction indicators and bounded8192->32768 evidence stages resolve M6 profile classification. No calibrated general tails, sequential confidence bound or distributed per-cell error-priority allocator. [Runtime policy](GOAL3_RUNTIME_COMPLEXITY.md). |
| Reuse geometry across scale/profile trials | Worked within bounded scopes | Original geometry/source stencils retained; current profile changes refresh actual wall samples. Latest identical-sensor background cache reduces46.72M->6.67M determinants (-85.7%), +1.077MiB/rank; observed total6.552->7.515s gives no event speedup. [Report](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/background_trial_reuse_v1/README.md). |
| Integrate sensor plus BL correction | Worked for difficult faceted3D initialization | Same-config saved M6 metric phase415.97->16.47s; coarsest790200.50 +/-27958.45 retained. Correction depends on the actual sensor-plus-BL composition; unresolved mass is not dropped. Not whole-remeshing speedup. [Scope](GOAL3_RUNTIME_COMPLEXITY.md). |
| More integration subdivisions | Mixed/negative evidence | Cell329 accuracy nonmonotone;88 work/time increases. More cells alone did not reliably improve accuracy/cost. Do not restore a subdivision/limit ladder. [History](SUGGESTION_OUTCOME_LEDGER_HISTORY_2026_10_18.md). |
| Geometric root screening | Withdrawn | Cell88 .2003->.2090s; no useful gain and unchanged estimates. Source/negative receipts retained. [History](SUGGESTION_OUTCOME_LEDGER_HISTORY_2026_10_18.md). |
| Actual adaptation should ease integration | Worked in controlled cases; M6 unresolved | Same corrected ORIGINAL field on refined partitions reduces indicators3.07--4.07x and observed error5.48--77.70x; coarsening does not guarantee improvement. No adapted-node donor refit. Saved rejected M6 finer partition now reduces the8192-point heuristic indicator39.27%, but assessment costs70.64% more; accuracy improvement is unproven. [Exact-field comparison](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/m6_same_field_reassessment_v1/README.md). [Stages](GOAL3_FROZEN_NORMALIZATION_STAGES.md). |
| Opt-in budget-aware BL relaxation | Scoped complete workflows qualified | Mandatory firstheight/protected/finer sensor/geometry retained; only declared outer-normal growth changes. M6 permitted growth2/protect2e-6:100k sampled-profile conflict,300k selectslambda.875; final estimate295068 +/-5769. No proof of exact infeasibility or automatic physical accuracy. [Policy](GOAL3_BUDGET_AWARE_BL.md). |
| Preserve geometric localization and fix strict source-point coverage | Worked in recorded arithmetic/geometry scope | Exact affine rounding/at most8 corners fixes captured outside source midpoint without loosening predicates. Complete N4M2 conservative RANS/SA/BDF/reload/restart controls pass. Historical owner-UPPER6 M6 still rejects~300s. [Construction](GOAL3_PROGRESSIVE_MEASUREMENT.md), [field identities](GOAL3_M6_FROZEN_FIELDS.md). |
| Retain repair-campaign donor support/indexes | Partial; broad retention open | Canonical owner sampling and physical-interior hypotheses qualify;1.407M hints in historical M6. Whole-donor preparation retention failed; broad campaign support/index retention is not implemented. No net event gain established. [Outcome](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/query_campaign_reuse_v1/README.md). |
| Extend shared-edge integral reuse | Worked as exact reuse infrastructure | Full composition/field identity and exact endpoints required. Historical campaign adds6420 edge hits; existing reuse already covers most. No whole-event speedup. [Current evidence](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/query_campaign_reuse_v1/README.md). |
| Batch independent transactions | First-row gains; general faceted concurrency now passes controls |64-parent first-row stage12.7%/23.2% lower at2/4 ranks; later ranking removes~42% metric work (overlapping gains, do not add). New complete-proposal reservations permit4independent commits/epoch; serial9/735, MPI4 and actual opt-in/default N4M2 independent workflows pass. Exact frozen M6 reaches3simultaneous commits,326764->280568kernel calls/rank (-14.14%),1707->1955private edits (+14.53%), but still rejects300.609s with no clear residual improvement. No event speedup/accepted wing. [Current experiment](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/concurrent_composed_proposals_v1/README.md), [transaction scope](POST_COMPLEXITY_QUERY_TRANSACTIONS.md). |
| Avoid unchanged failed searches | Existing behavior retained; broader extension open | Dependency-triggered reactivation exists. New final reservation conflicts defer without blocking the unchanged feature; general repeated reconstruction-search memoization is not implemented. [Transactions](POST_COMPLEXITY_QUERY_TRANSACTIONS.md). |
| Share transfer geometry across fields/histories | Existing facilities retained | Correction stages transfer once after final acceptance and share original reassessment/return source. Additional overlap-weight sharing/performance campaign not attempted. |
| Measured-cost balance/mid-remeshing migration | Not implemented | Existing N/M lifecycle qualified. Migration, index rebuilding, return and solver/transfer cost must enter break-even; historical2D lower imbalance could be slower. No new balance/scaling claim. |
| Apply successful policy in2D | Foundations retained; extension incomplete | Existing strict2D composition/integration controls retained. Opt-in profile selection/current correction provider is3D-only.30P30N2D input prepared, not yet adapted. Supplied3D hex case exceeds native tetra support. |

Additional accounting requests remain in the archive: cold preparation versus
reused work, geometry/source/placement versus density, peak/transient memory versus
retained-byte models, and nested/synchronized timing attribution. A residual is
not automatically pure geometry or MPI; maxima of nested clocks cannot be summed
as an exact critical path. No clean whole-event affordability/scaling conclusion
follows from contended correctness runs.

The previous complete selected-profile M6 attempt is [recorded here](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/selected_profile_construction_v1/README.md):
300.434s atomic rejection,1707private later edits, no transfer or accepted mesh.
Current concurrency work preserves its exact original sensor/profile and unchanged
mesh/time/work gates, avoiding another Hessian/normalization build. Construction
feasibility and estimated budget feasibility remain separate.


Latest original-edge witness: five original tetrahedra recover the persistent
reported obstruction face and an exact centroid match, outside configured BL
support; sensor edge2.60882>1.8. This motivates a small generic cavity replay,
not proof of late-cavity equality or a cause for the entire wing failure.
[Data and viewable original patch](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/concurrent_composed_proposals_v1/original_edge_witness/README.md).

Original-star native replay now establishes local feasibility: existing joint
placement passes unchanged geometry/Refine/q/J gates in3trials, q=.273986,
J=.086470, changed-edge excess.654190->.023844; trial work.1202s. Balanced
insertion fails J=.040146<.05. This does NOT resolve M6 or establish late-cavity
equality. The observer capture atomically rejects300.641s, retaining64exact
contexts/14distinct features; all input snapshots unchanged. Captures include
evolved volume stars and expanded wall/transition patches. No new placement
fallback or weakened gate is justified.
[Replay and inspectable private grids](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/cavity_operator_replay_v1/README.md).

No new production improvement is claimed by this diagnostic checkpoint. Replay
the captured contexts before deciding whether reconstruction or coordination lacks
a capability. The original-star result specifically argues against another local
placement fallback. Exact runtime/build/input/helper provenance and verifier are
in the linked report. All owned jobs terminal.

Current connected follow-up:10EXACT evolved volume stars pass9/10; trial phases
sum1.818s/no source failures. The remaining7-cell star and negative full-fan probe
are preserved. Retaining the initial blocker enables bounded targeted star
admission when the old cover cannot fit64cells. Serial10/748, MPI4 and independent
opt-in/default N4M2 RANS workflows pass. M6 still rejects300.994s;167dependency
expansions demonstrate path use, not a construction fix or net speedup.
[Evolved evidence and full rejected grid](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/cavity_operator_replay_v1/evolved_volume_v1/README.md).

**A sufficient obstruction is now diagnosed.** Current SYMMETRY edge211--3481 lies
between distinct exact near-coplanar facets. Its only interior binary64 point is
the midpoint; neither child has another. Full frozen composed lengths12.796parent,
11.572left/1.225right cannot satisfy1.8, while sensor-only parent is.325. The lattice
part is exact; metric values are native fixed-accuracy measurements, not interval
bounds. Under exact-facet/conforming/binary64 rules this frozen case cannot finish;
more cavities/ranks cannot repair it. This is not physical BL or budget infeasibility.
[Certificate and explicit assumptions](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/cavity_operator_replay_v1/evolved_volume_v1/full_state_v1/geometry_lattice_v1/README.md).

Complete terminal working mesh541027tets/139884points and exact versioned owner
records are preserved. Shared-node coordinate and unique ownership screens pass;
full mesh acceptance/transfer does not. The earlier volume witness edge/vertex are
absent, confirming that last intermediate rejections cannot alone diagnose final
blockage. Further unchanged M6 construction runs are unwarranted. Geometry-reference
preference is pending; no authority, target, profile or acceptance gate was changed.
Other full Goal3 work stays active. All owned jobs terminal.

Strict exact-facet diagnosis is now integrated on the mandatory composed seed path:
serial11/760, MPI4 eleven cases/rank and independent opt-in/default N4M2 workflows
pass. The actual captured M6 crease MPI1/2/4 elects the same conflict with immutable
input. At most two extra child integrals, no new collective/gather or sensor-only
import requirement. Preliminary broader diagnostic was narrowed after identifying
a compatibility risk; its source/controls and failed helper compile are preserved.
This does not produce an accepted wing or establish event speedup.
[Final evidence](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/strict_geometry_preflight_v1/README.md).
