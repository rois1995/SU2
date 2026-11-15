# Adapt cut: source check and gate ledger

N4 follows the SHA-sealed CUT_PLAN_8_2.md and both inventories under
`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/inputs/`.
The complete plan and both inventories were read before any production edit.
The reference implementation is accepted F3b `48f07ad1a8`, restored by `becb4927c9`;
`6941fcc41c` adds only the completed N3 review. P7/P8 failed and are absent.

The missing capability is a readable decomposition of Adapt with its existing
behaviour intact. Acceptance is C's G1–GQ/G5 schedule, every integer counter,
six P6c payload hashes, strict physical/resource gates, and the final size report.
Affected scope: CNativeEngine3D.cpp, new private engine units, their build inclusion,
and the existing native tests/runners. Selection and MeshOptimizer remain unchanged.

## Whole-item source check: one consolidated list

1. **Reference reading used.** The inventories describe the P7/P8 working snapshot.
   Their line numbers and new counter/cache blocks do not describe restored F3b.
   Follow names and existing F3b statements; do not reintroduce rejected N1 code.
2. Session construction updates a thread-local timer ledger, with no MPI. Destruction
   finalizes and prints local rows. Moving its lifetime changes counters/nesting;
   keep its exact construction/destruction points (TimerTree header, Session).
3. Directory construction/destruction has no collective. Its methods do. It borrows
   World and parameter state; preserve those identities and lifetime ordering
   (Distributed3D cpp constructor/destructor).
4. CavityMetricSource construction stores World/Owned references, with no collective.
   DonorImport construction is local too. Import/build methods are collective.
   Both must continue to refer to canonical state, never a copied working map.
5. transaction_observer is NOT guaranteed equal, even in presence, across callers.
   SetTransactionObserver installs an instance-local callback. Existing observation
   maxima do not establish callback equality. Preserve the original guards and votes;
   do not add an agreement check or repair this old behaviour during the cut.
6. Selection writers of stats are PrioritizeGeometrySplits, Queue, Block, Touch,
   FilterCollapse, LeavesFacet and Forbidden; FieldCounts writes outside Selection.
   The edge-history rounds assignment reads stats, rather than writing it.
   Moving only measured-cell insertion does not isolate all stats mutation.
7. Construction order is interleaved across future unit ownership. Canonical objects
   must be created at their current sequence points; typed references must not copy
   or move them. Context and unit states will delete copy/move. Original private
   by-value options mutations must not reach caller options or copy callbacks again.
   State grouping cannot justify advancing allocations, votes or timer construction.
8. Selection borrows geometry, stats and options.wall_heights; source borrows working;
   Directory borrows geometry parameters. Preserve all aliases. SetCavity's vector
   pointer survives through per-slot commit, while batch measurement has no cavity.
9. **Inclusion reading used.** Selection is private to the original anonymous
   namespace. C requires private file pairs, not separate translation units. Private
   implementation inclusion in that namespace can retain the class unchanged;
   no new public API or Selection split is authorized by N4.
10. All eight loop donor-import/FrozenField sequences are NOT textually identical
    up to variable names: accounting lifetimes, stack versus unique_ptr ownership,
    diagnostic labels, retry/argument structure and their surrounding imports differ.
    Keep all eight moved sequences. The two setup instances are additional, not part
    of the proposed eight-copy merge. No merge is approved by the conditional rule.
11. A status-returning extraction may shorten RAII lifetimes. CavityScope, lane,
    slot and repair timers must remain at caller sequence points, including return
    finish paths. Restart must not increment round; boolean short circuits retain
    their exact order and collective targets.
12. vote must read the LIVE private_repair flag. bytes/real_bytes must count precisely
    the original containers and capacities. reserve/clear/swap and admission order
    are result-sensitive, not cleanup opportunities.
13. Per-slot and batch commit conflict rules differ; the three expand-cavity paths
    also differ. Move separately. JoinWait stays first in the batch commit.
14. The always-streamed block's apparently dead wrapper still performs
    reserve_optimization(false), trial_maximum(repair_budget>repair_work), and
    release_optimization in the collective path. Those calls cannot be removed.
15. **G3 budget reading used.** C492 is an existing time-bounded BL diagnostic
    (388.8 s). Its observed total split count has varied in historical evidence.
    The specific G3 requirement wins: retain the unchanged fixture/budget, compare
    the first 336 edits AND total split count AND integer rows strictly. Do not
    replace its stop by a fixed-work prefix, tune duration or waive a difference.
    C's far-from-binding warning also applies to the unchanged 5400 s M6 checks.
16. Ownership uses measured elapsed/rebuild costs. Byte/counter identity is therefore
    an execution gate, not a source theorem. Preserve the original cost policy;
    no deterministic-cost redesign is allowed to obtain a passing cut.
17. An early parameter-setup rejection occurs before geometry/context and does not
    call finish. Keep that return distinct. finish runs exactly once on each normal
    finish path, with observe_progress("exit") and votes at their original points.
18. G2/GQ logs can contain rows from several logical ranks in one physical stdout.
    Compare paired physical streams and each logical rank's bounded prefix, retaining
    row order; asynchronous lines after a rank's anchor are not part of its prefix.
    Require all six anchors. No comparison of integer work/call/storage counters is
    waived by excluding measured seconds, RSS and floating debt values.

No unresolved result contradiction has yet been identified. These readings constrain
implementation; they do not waive any result gate. Reference G2/G3/GQ must complete
before the first production edit. This ledger will add sub-step outcomes in place.

## Found during the cut (preserve, do not fix)

- Acquisition can throw outside its existing catch region.
- import_source's requests argument is ignored.
- Receipt sort has ties whose input order matters.
- Callback presence is not guaranteed symmetric across ranks.
- Retained prepared geometry can affect later partition weights (N1 failure).

## Written exceptions

EngineLegacyWall is the explicit C exception to the 80-line rule. No other new
exception has been authorized. The three growth copies remain separate.

## Current state

Reference rebuild PASSED870.265s/347actions, sourcepinsunchanged. Tool63730
and controller1870291/start27335226/ninja1870303/start27335244 are terminal;
host no-heavy receipt verified. No production edit. G2 capture PASSED:317.194s until stop,326.125s sealed;
90,809,869 work, exact480/473/346/263/469/464, zero refinements; peak sum
5,201,391,616B. G3 C492 reference PASSED543.547s:473splits,305015positivecells,
first336edits match P6c, zero refinements and unchanged388.8s budget.
The fresh C reference is473; historical F3b462 is recorded, not substituted.
Future G3 checks must match473 and every integer row. GQ pending.
No new implementation is accepted by this source check.
