# Fixed parts, item FX2: the mobility policy is built one time and then replayed (identical results)

Author: reviewer, 2026-11-22 (checked against the source and your evidence; the design was reviewed by a second
model). For the main agent. The user sends it; it is a user order. Copy this file with its SHA256 into
`R/night_20261115/FX2/inputs/`. One whole-item source check (R4) before the first production edit; questions R13.
This is an item with IDENTICAL RESULTS (R10): each output file, each integer row, the modeled bytes and the timer
calls stay equal; only seconds change. Code state: commit `c843610249` (build640, "FP2-B: accepted").
Short names: `BP` = `Common/src/adaptation/CNativeBoundaryParameters3D.cpp`, `AF` =
`Common/src/adaptation/CNativeAffineReference.cpp` (with its header), `SG` =
`Common/src/adaptation/native_local/CNativeLocalSurfaceGeometry3D.cpp`.

## 1. Why

Your FP2-B runs show that the validating constructor of `BoundaryParameterState` builds the mobility policy again
only to compare it with the one that it got (`BP:250-253`): about 16.8 s each time, on all ranks, at the surface-state
build of the core (`BuildSurfaceState`, `SG:846-851`) and at the transfer (`Rebind`, `BP:717-736`). That is 33.6 s in
event 1 and 33.9 s in event 2 of the native time of run S (650 / 447 s), and about 17 s in each event with both
options NO. (The 17 s in the preparation of event 1 are the FIRST build, `PrepareMobility`; they stay.) All these
sites use the same authority object in both events.
In FX1 you refused part P for a valid reason: `BuildMobilityPolicy` charges work and storage to the budget of the
constructor, and a plain skip changes the budget that is left for the later validations. This item keeps the budget
exactly: the first build of a process records its work, and a later constructor that gets a policy for the same
authority object charges the same storage and the same work without the build ("replay"), and compares the encoded
bytes as today. The policy is a pure function of the immutable authority, the angle and the roles. What the item
gives up: the constructor no longer derives the policy again at each call; it compares with the recorded derivation
of the same object.

## 2. The change (production)

1. The memo: a function-local static in `BP` (anonymous namespace), protected by a mutex:
   `std::map<std::weak_ptr<const MappedFacetReference>, std::vector<PolicyMemoEntry>, std::owner_less<>>` with
   `PolicyMemoEntry { const MappedFacetReference* object; double angle; std::map<int, int> roles; int rounding;
   unsigned sse_control; std::vector<char> encoded; size_t work; }` (`BoundaryParameterState::Entry` exists
   already; do not use that name).
   `encoded` is `EncodeBoundaryAtlas({}, policy)`; `object` is the stored pointer of the authority (two different
   objects behind one control block must not match); `rounding = std::fegetround()` and, under `#if defined(__SSE2__)`,
   `sse_control = _mm_getcsr() & 0xFFC0u` (the control bits: exception masks, rounding, flush-to-zero,
   denormals-are-zero; without the sticky flags), otherwise 0; both are read at the build. Add `#include <cfenv>`
   (and `<xmmintrin.h>` under `__SSE2__`) to `BP`.
   Each access first removes the expired keys and then looks up. Never record when `geometry.use_count() == 0`.
   The memo is best effort: an exception while an entry is made (allocation, the encoder) is caught, no entry is
   stored, and the work goes on as today.
   Written exception to the storage model (by this order): the memo is not in the modeled bytes. Report its size as
   `BoundaryAtlasBytes({}, state.Mobility())` of the M6 state plus `sizeof(PolicyMemoEntry)` and the roles (print it
   in F5), times the count of live entries.
2. The match of an entry: the same control block, the same `object` pointer, `EqualDouble(entry.angle, angle)`
   (`BP:27`: the bits, so +0.0 and -0.0 differ), `entry.roles == roles`, and `rounding` and `sse_control` equal to
   their values now.
3. Record: right after a `BuildMobilityPolicy` returned true, reached through `WithMobilityPolicy` (`BP:592`) or
   through the build path of the constructor (`BP:252`; before the encoded comparison, also when that comparison then
   fails), and only when the budget has no deadline (`!budget.within_time`): store or replace the entry that matches
   by item 2, with `work = budget.work after the build - budget.work before it`. Do not record on a hit.
   The record has no side effect on the caller: read `const size_t before = budget.work;` before the build. After
   the build returned true, inside `try { ... } catch (...) {}`, compute `EncodeBoundaryAtlas({}, policy)` with its
   default arguments (its own local budget, `BP:192-198`; never pass `budget`, `reservation`, a ceiling or a
   caller), then lock the mutex only for the prune and the store. The record must not move or change the policy,
   the budget, `reason` or any member. In the constructor, the comparison of today
   (`EncodeBoundaryAtlas({}, expected) != EncodeBoundaryAtlas({}, mobility)`) stays unchanged after the record. On
   a hit, copy `work` and `encoded` out under the lock and release it before `ChargeMobilityPolicy`. The lock is
   never held during a build, a replay or an encoding.
4. S: one public const member `size_t MobilityPolicyStorageBytes() const` of `MappedFacetReference` (`AF`, beside
   `BuildMobilityPolicy`; its value is what the build charges at `AF:1466`: (facets + edges) x 512). The build, the
   replay and the hit test use it. A member function adds no data member.
5. Replay: `bool MappedFacetReference::ChargeMobilityPolicy(size_t work, AtlasBudget& budget, std::string& reason)
   const`, beside the build. Body: `SU2NativeTiming3D::Scope timer(SU2NativeTiming3D::Operation::MobilityPolicy);
   try { AtlasOps op{budget, "BuildMobilityPolicy"}; op.Storage(MobilityPolicyStorageBytes()); op.Work(work);
   reason.clear(); return true; } catch (const std::exception& e) { reason = e.what(); return false; }`.
6. The constructor, at the place of the build call (`BP:250-253`, after the storage accounting of the constructor),
   one small function for the hit test (R9), evaluated BEFORE a timer scope is opened or the budget is touched:
   an entry matches by item 2, AND the budget cannot limit the charge:
   `!budget.within_time && !budget.time_exhausted && !budget.work_exhausted && budget.work <= budget.maximum_work &&
   entry.work <= budget.maximum_work - budget.work && Add(budget.caller_bytes, S) != SIZE_MAX &&
   Add(budget.caller_bytes, S) <= budget.maximum_bytes`.
   On a hit: `ChargeMobilityPolicy(entry.work, budget, reason)` (a `false` is treated as a failed build: the same
   `invalid_argument` and message; by the hit condition it does not occur), then compare `entry.encoded` with
   `EncodeBoundaryAtlas({}, mobility)`; a difference throws the same exception with the same message as today.
   In each other case: the build and the comparison of today (and item 3).
   Comment at the function: the policy is a pure function of the authority object, the angle and the roles; the
   replay charges what the build charged; each case where the budget could stop the build takes the build, so that a
   failure is the same failure.
7. No change of a caller, of `Session`, `CellState`, a row that travels, a timer operation, a data member of the
   policy or of the authority (their `ResidentBytes()` count `sizeof(*this)`), or of the serialisation.
In your R4 list confirm that no production code assigns to or replaces a live `MappedFacetReference` object (its
public members are const, but the implicit copy assignment exists). If one does: do not make the item; write it.

What stays equal, and why (these are the checks of the fixtures):
- budget `work`: one `ChargeWork(W)` gives the same `work`, flags and `next_time_check` as W charges of 1 when no
  deadline is set;
- `peak_bytes` and the modeled bytes: the build has one storage charge (`AF:1466`); the replay gives the same
  `caller_bytes + S` at the same place;
- the timer calls: one `mobility_policy` call at each site as today, no nested scope; only its seconds fall;
- the files: the same objects and bytes; nothing else reads the memo.
Timer coverage (`AGENTS.md` 2a asks the text to say this): this item removes about 34 s of NAMED time from each
event of run S (about 17 s with both options NO) and does not change the time without a name. Projection from the
rows of build640: S lowest 98.24 % -> about 98.15 % (event 1) and 98.93 % -> about 98.84 % (event 2); OFF
98.995 % -> about 98.98 % and 99.527 % -> about 99.5 %. The gate of 95 % does not change; measure it on each rank.

## 3. Fixtures (test code)

F1 to F4 are hidden test cases (tags `[.][NativeFX2Memo]`); F5 is the existing hidden B1 case. G1 runs the default
set and must keep 375 / 114 / 228 records.
F0 (baseline, BEFORE the first production edit). Write F1 to F5 (test code) first, build the test driver on the
    production code of `c843610249`, run F1 to F5, and keep their printed rows as the baseline. Each fixture prints,
    for each compared call: `work`, `work_exhausted`, `time_exhausted`, `next_time_check`, `caller_bytes`,
    `maximum_bytes`, `peak_bytes`, the exception type and message (or none), the SHA256 of `Snapshot()` and of
    `Payload()`, and the calls of each timer operation by phase under an active timer session. After the production
    edit the same rows must be byte-equal to the baseline except seconds. This is the identity proof of the fixtures.
F1. An authority A without curves and without components, built with the `MappedFacetReference` constructor (as
    `UnitTests/Common/adaptation/CNativeGeometryBackend3D_tests.cpp:42`; then `ResidentBytes()` does not depend on the
    query history). Its policy comes from a build alone (`BuildMobilityPolicy`, which does not record). Call 1: the
    constructor with that policy (a miss; it records). Call 2: the same constructor with a fresh equal budget (a
    hit). A second object A' with the same content: call 1 on A' (a miss). Check before each compared call that
    `A.ResidentBytes() == A'.ResidentBytes()`. The rows of call 2 on A equal the rows of call 1 on A' and the
    baseline. Two more cases: an entry recorded with angle +0.0 is not a hit for a policy with -0.0 (the result
    equals the baseline); two authorities x (content X) and y (content Y, different policies) in one
    `std::make_shared<TwoAuthorities>`, used through the aliasing pointers
    `shared_ptr<const MappedFacetReference>(owner, &owner->x)` and `(owner, &owner->y)`: after a record for x, the
    constructor on y passes with the policy of Y and throws with the policy of X (as the baseline). And one
    `FromPayload` call on an authority with an entry (a hit): rows equal to the baseline.
F2. A policy that was changed (one feature removed, or the identity or one sheet changed) is refused with the same
    exception type and message on a hit and on a miss.
F3. On A with an entry, each of: `maximum_work = work + W - 1`; `budget.maximum_bytes = budget.caller_bytes + S - 1`
    at the constructor entry; `within_time` set (one callback that always accepts, one that refuses);
    `time_exhausted` set; `work_exhausted` set. Also the two equal limits `work + W` and `caller_bytes + S`, which
    must succeed. W = the `work` of a build alone on a fresh budget. Rows equal to the baseline. For the bytes case
    also print `peak_bytes > maximum_bytes` after the failure (the value that the catch of the decoder reads).
F4. A1 with content X records; destroy every owner of A1. Make A2 with content Y (one facet coordinate changed so
    that the two policies differ; check that their encoded bytes differ). On A2 the constructor with the policy of Y
    passes and with the policy of X throws the refusal.
F5. B1 (`native_fast_path_stage2_strict.inc:42-100`, called at `CNativeFastPathStage2_tests.cpp:281`) with one new
    print, `reservation.work` after the constructor. Against the F0 baseline run of B1 on `c843610249`:
    `modeled_peak_bytes`, `reservation.work`, `snapshot_equal` and the other printed integers are equal. The
    constructor seconds must be at least 10 s lower than the baseline: this proves that the hit path runs on M6 (the
    load records the entry and the measured constructor call is a hit).
The present fixtures that use the policy must pass without a change.

## 4. Gates and the order of the work

1. F0 (test code only, on `c843610249`). Then the production edit. Build `SU2_CFD` and `UnitTests/test_driver`
   without a new warning. Fixtures F1 to F5 against the baseline; all fixtures of FP1; the cohort of
   `R/night_20261115/FP2_B/scoped/build640/fixtures.json`; G1 identical (375 / 114 / 228). Commit and push ("FX2:
   fixtures passed; full runs pending").
2. G2 and GQ with the comparator, the scheduling check and the references of build640
   (`R/night_20261115/FP2_B/scoped/build640/prefix/G2_comparison.json`, `GQ_comparison.json`); G3, G4 and the
   safeguards as `FP2_B/scoped/build640/scoped.json` and `safeguards.json`. (These are regression gates here.)
3. The lab runs N1 and N6P of FP2-B on this binary (they use `BuildSurfaceState`): each field of the rows
   `NATIVE3D_LOCAL_SURFACE`, `FP2_B state` and the proof row (`LAB_S_PROOF` in N1, `proof` in N6P), except the fields
   whose name starts with `seconds_`, equal to `rows_by_rank` of
   `R/night_20261115/FP2_B/cases/build640/{N1,N6P}/measurement.json`; write `seconds_build` beside build640
   (expected: about 16 s lower).
4. One G5 run with `ADAP_NATIVE_LOCAL_CORE= NO` and `ADAP_NATIVE_LOCAL_SURFACE= NO` (the default): all standing
   gates; the six files identical to the N21 W reference. For each rank, event, root, phase and operation: the timer
   CALLS and the modeled-storage rows equal to `R/night_20261115/FP2_B/G5/build640/OFF/full640_run1`, compared by a
   new script (test code) `R/night_20261115/FX2/compare_full.py`. It uses `PARSER.parse_timer_stream(path)` with no
   cutoff from `N11/section6_8_preparation/drafts/compare_timer_census_delta_69.py` (the parser of
   `N22/compare_prefix.py`), and the `NATIVE3D_STORAGE` scan of `compare_prefix.py` with no line limit. Exact: the
   number of snapshots and, for each rank, snapshot, root, boundary, phase and operation, `calls` (also `census`);
   `fixedstorage=4120`; every `NATIVE3D_STORAGE` row in order. Report only: seconds, the `calls` of
   `communication`, the fields `C_FIELDS` (`AGENTS.md` 2a). Self-check before use: build640 S1 against build640 S2
   must give no exact difference (the reviewer's check: 0 differences in 6 x 16 snapshots and 69 storage rows for
   each rank). The isolated restart has only its standing gates. `transfer / mobility_policy`:
   exclusive seconds below 1 s on each rank and event (the proof that the replay ran; if not: R5, a wiring error).
5. One run S (both options YES; the config of `FP2_B/G5/build640/S1/full640_run1/run.cfg`, SHA256 `07b91a94...` as
   in `on_surface_reference.json`; the runner: a copy of `R/night_20261115/FP2_B/run_fp2_b_g5.py` in
   `R/night_20261115/FX2/`): all standing gates; the six files identical to
   `R/night_20261115/FP2_B/on_surface_reference.json`. `compare_full.py` against
   `FP2_B/G5/build640/S1/full640_run1` gives no exact difference; each field of the rows `NATIVE3D_LOCAL_SURFACE_STATE`
   except `rss_before`, `rss_after` (observations; the job peak gate stays) and the fields that start with
   `seconds_`. `construction / mobility_policy` and `transfer / mobility_policy`: exclusive seconds below 1 s on each
   rank and event. `output / mobility_policy` on rank 0 stays at about 18 s (a miss; not in this item).
   (A run CORE is not necessary: it reaches no site of this change that the run of item 4 does not reach.)
6. Report (not a gate): for the two runs and each event the native maximum beside build640 (OFF:
   `R/night_20261115/FP2_B/G5/build640/OFF/run_receipt.json`, 1,757.338 / 1,017.567 s; S: 649.954 / 446.547 s), the
   seconds of `mobility_policy` by phase, the seconds of the surface-state build and of the transfer, the timer
   coverage of each rank. Expected: S about 34 s lower in each event; OFF about 17 s lower in each event.
7. If a gate with "identical" fails: R5 (one diagnosis; a transcription error is corrected one time; else revert the
   item in one commit to `c843610249` for `Common`, `SU2_CFD`, `UnitTests`, rebuild, check that the source pins
   equal those of build640 (`FP2_B/G5/build640/default_proven.json`), mark "FX2: FAILED", keep the evidence).
8. Close: `R/night_20261115/FX2/reports/FX2_<build>.md`, the progress file, `HANDOFF_Codex.md`, `HowToProceed.md`,
   `Handbook/ADAPTATION_TIMINGS.md`. Commit ("FX2: accepted"), push, verify that no owned job runs, stop and wait.

## 5. Not in this item

The self-check decode of the reference output on rank 0 (it builds a new authority object, so it is a miss here;
about 18 s of job wall for each event outside the native time; a later item). The first build in the preparation.
A change of a result. A change of the engine or of the core.
