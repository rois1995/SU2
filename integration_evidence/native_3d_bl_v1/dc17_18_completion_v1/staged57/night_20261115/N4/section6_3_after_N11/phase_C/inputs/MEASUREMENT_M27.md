# N17 = measurement M27: where the exact wall queries of the quality stage go

Written 2026-11-17 by the reviewer, for the main agent. This is the file I8A of work
order 2 (section 4, item 1a). It is a print-only item with identical results.

Source that the reviewer read: commit `ad64feb822`. Phase C moves code, so this text
names functions and unique strings, not line numbers. Find each place by its name.

## 1. Why (facts from the N14 full run, build 552, rank 0, event 1)

The exact wall query with a cost is the union query,
`MappedFacetReference::BoundPhysicalSurface`. It has a memo (`FaceMemo::union_values`).

- Quality stage: about 104,000 union queries. The memo answers 59 % of them. About
  42,500 are computed. One computed query costs about 7 ms. The answered ones are cheap.
- The memo becomes full in the quality stage (`memo_bytes=50128600` of
  `memo_limit=50128648`; no eviction before the quality stage). At the end of the
  event: `memo_stores=96009`, `memo_evictions=54938`, `memo_union_entries=28005`.
- The rank has about 18,000 wall faces. A rank makes about 3,700 quality commits
  (mean of the six ranks, M26).

So most computed queries are not the one proof of a committed face. They are one of:
(1) a face that was computed before and that the memo lost;
(2) a candidate face that is never committed;
(3) a face that is known with another kind of answer.
No counter separates these today. Design step 8.4a (N16) depends on their shares.
M27 measures them. It also measures the chain of exact checks in `SmoothBoundary`.

M27 does not evaluate the wall law, and it makes no new geometry query.

## 2. Rules of this item

- Identical results. Rules R10 and R11 of the work order apply.
- No new timer operation, no new `SU2NativeTiming3D::Scope`, no new call of a geometry
  query, no call of `AtlasOps::Work` or `AtlasOps::Storage`, no change of
  `budget.work`, no new field in `Session`, `CellState` or a row that travels.
- All new state is in `SU2NativeD9::Data` (thread-local, alive only under
  `options.timings`, as the fields of N15). When `SU2NativeD9::active` is null, the new
  code does nothing.
- The new functions do not throw. They catch all exceptions, set a flag
  `union_observer_failed` and stop observing.
- The new lines have new names (`NATIVE3D_M27_...`). Do not change the text of an
  existing line (`D9_QUERY`, `D9_MOVE`, `NATIVE3D_STAGE`, ...).
- The names in section 3 are a proposal. You can change a name or a place to fit the
  files after phase C. You cannot change what is counted.
- The table of seen faces is real memory, not modeled memory. Expected: below 10 MB
  for each rank. Print its size (line `NATIVE3D_M27_TABLE`).

## 3. What to implement

### 3.1 In `Common/include/adaptation/CNativeD9Profile3D.hpp`

```cpp
// M27. The source of a union query. An "Other" value holds the queries of a call
// site that are outside its guidance calls.
enum class Part {
  None, GuidanceOther, GuidanceOld, GuidanceFresh, SmoothOther, SmoothOld, SmoothCandidate,
  Shell, Coupled, Census, Finish, Count
};
constexpr size_t PartCount = size_t(Part::Count);
constexpr size_t StageCount = 4;  // other, quality, final_proof, surface = Phase() + 1

// M27. The kind of answer that a union query wants or gives.
enum class Answer : uint8_t { Classified = 1, Magnitude = 2, CutOff = 4 };

// M27. Miss classes: 0 = new face, 1 = repeat (the wanted answer was computed before),
// 2 = repeat with another kind of answer.
struct UnionRow {
  uint64_t queries = 0, hits = 0, bypass = 0;
  std::array<uint64_t, 3> misses{}, work{};
  std::array<double, 3> seconds{};
  uint64_t faces_reached_census = 0, work_reached_census = 0;
};

// M27. One face that a union query computed, or that the memo answered from an
// earlier event (stage == StageCount).
struct SeenFace {
  uint8_t stage = 0, part = 0, answers = 0;
  bool reached_census = false;
  uint64_t first_work = 0;
};

// M27. Exact wall checks of the wall-vertex searches (section 3.5).
struct SmoothChains {
  uint64_t calls = 0, checked = 0, passed = 0, moved = 0;
  std::array<uint64_t, 5> checked_bins{};  // 1, 2, 3, 4, 5 or more
  std::array<uint64_t, 4> passed_bins{};   // 0, 1, 2, 3 or more
};
```

New members of `Data`:

```cpp
  std::array<std::array<UnionRow, PartCount>, StageCount> unions{};
  std::unordered_map<uint64_t, SeenFace> seen_faces;
  uint64_t faces_from_earlier_event = 0;
  bool union_observer_failed = false;
  SmoothChains chains;
```

New thread-local value and scope:

```cpp
inline thread_local Part current_part = Part::None;

// M27. Names the source of the union queries inside its scope. Restores the outer one.
struct PartScope {
  Part previous = current_part;
  explicit PartScope(Part value) { if (active) current_part = value; }
  ~PartScope() { current_part = previous; }
  PartScope(const PartScope&) = delete;
  PartScope& operator=(const PartScope&) = delete;
};

// M27. The part of a call site when no guidance call is open.
inline Part SitePart(Site site);  // Guidance -> GuidanceOther, SmoothBoundary -> SmoothOther,
                                  // ShellBoundary -> Shell, CoupledBoundary -> Coupled,
                                  // Census -> Census, Finish -> Finish
```

Three observer functions, all `noexcept`, all return at once when `active` is null or
`union_observer_failed` is set. The row is
`active->unions[size_t(Phase() + 1)][size_t(current_part)]`.

1. `ObserveUnionBypass()`: `++row.bypass`.
2. `ObserveUnionHit(uint64_t face, Answer wanted)`: `++row.queries; ++row.hits;`
   If the face is not in `seen_faces`: insert it with `stage = StageCount`,
   `answers = wanted`, `first_work = 0`, and `++faces_from_earlier_event`.
   Then do the census mark (below).
3. `ObserveUnionMiss(uint64_t face, Answer wanted, Answer given, uint64_t work, double seconds)`:
   `++row.queries`. Find the class:
   - face not in `seen_faces`: class 0. Insert it with the stage and the part of this
     query, `answers = given`, `first_work = work`, and
     `reached_census = (current_part == Part::Census)`.
   - face found and `(answers & wanted) != 0`: class 1.
   - face found and `(answers & wanted) == 0`: class 2.
   For a found face: `answers |= given`.
   Then `++row.misses[class]; row.work[class] += work; row.seconds[class] += seconds;`
   For a found face do the census mark.

Census mark (one small function): when `current_part == Part::Census`, the face is in
`seen_faces`, `reached_census` is false and `stage < StageCount`: set
`reached_census = true`, and for the row of its FIRST computation
(`unions[face.stage][face.part]`): `++faces_reached_census;
work_reached_census += first_work;`. For a face with `stage == StageCount`: only set
`reached_census = true`.

Print functions, called from `Root::~Root()` after `PrintPrepare(data)`:

```
NATIVE3D_M27_QUERY rank= stage=<other|quality|final_proof|surface> part=<name>
    queries= hits= bypass= miss_new= miss_repeat= miss_other_kind=
    work_new= work_repeat= work_other_kind=
    seconds_new= seconds_repeat= seconds_other_kind=
    faces_reached_census= work_reached_census=
NATIVE3D_M27_SMOOTH rank= calls= checked= passed= moved=
    checked_1= checked_2= checked_3= checked_4= checked_5plus=
    passed_0= passed_1= passed_2= passed_3plus=
NATIVE3D_M27_TABLE rank= faces= from_earlier_event= observer_failed=
```

One `NATIVE3D_M27_QUERY` line for each stage and part with `queries + bypass > 0`.
Each line is one text line. Part names: `none, a_other, a_old, a_fresh, b_other,
b_old, b_candidate, c, d, e, g`.

### 3.2 In `MappedFacetReference::BoundPhysicalSurface` (`CNativeAffineReference.cpp`)

Include the D9 header. Add one small static helper in this file:

```cpp
// M27. Identity of a union query without its purpose: support, tolerance, triangles.
// FNV-1a over the bytes of the words, in this order: facets.size(), each facet,
// triangles.size(), the bits of the tolerance, the 9 coordinate bit words of each triangle.
uint64_t UnionFaceWord(const std::vector<Id>& facets, double tolerance,
                       const std::vector<std::array<Point, 3>>& triangles);
```

`wanted` is: `Answer::Magnitude` when `magnitude` is true (this includes `limited`),
else `Answer::Classified`. `given` is: `Classified` when `magnitude` is false;
`Magnitude` when `magnitude` is true and (`!limited` or `result <= stop_above`);
else `CutOff`.

Hooks (each only when `SU2NativeD9::active` is not null):

1. At each of the two returns with the text `"bypass"`: `ObserveUnionBypass()` before
   the query.
2. After the line `const auto& query=canonical.empty()?physical:canonical;`:
   compute `face_word = UnionFaceWord(facets, tolerance, query)`.
3. In the block `if(found){...}` before its `return`: `ObserveUnionHit(face_word, wanted)`.
4. Around the call `QuerySurfaceUnion(facets,query,...)` of the miss path: take the
   time with `SU2NativeD9::Clock` before the call. After the line
   `const auto result=receipt.mesh_to_model;`:
   `ObserveUnionMiss(face_word, wanted, given, budget.work-before, seconds)`.
   When the query throws, nothing is observed.

Do not move, add or remove an `op.Work`, `op.Storage`, a memo `Find` or a memo `Store`.

### 3.3 In `D9QueryScope`

Add one member `SU2NativeD9::PartScope part;` that is built with
`SU2NativeD9::SitePart(value)`. Nothing else changes in this struct.

### 3.4 The guidance calls of call sites (a) and (b)

Add one helper next to `D9QueryScope`:

```cpp
// M27. Names the patch that a guidance query measures. It does not change the query.
template <class... Arguments>
GeometryGuidance GuidanceOf(SU2NativeD9::Part part, Arguments&&... arguments) {
  SU2NativeD9::PartScope scope(part);
  return GeometryGuidanceExcess(std::forward<Arguments>(arguments)...);
}
```

Replace exactly these six calls of `GeometryGuidanceExcess` by `GuidanceOf` with the
same arguments:

- In `geometric_change` (the function that holds the text
  `"Native3D candidate does not preserve/improve its frozen geometry error."`, two times):
  the two calls with the patch `old` get `Part::GuidanceOld`; the two calls with the
  patch `fresh` get `Part::GuidanceFresh`.
- In `MeshOptimizer::SmoothBoundary`: the call with `patch` (the one that sets
  `geometric_before`) gets `Part::SmoothOld`; the call with `candidate` gets
  `Part::SmoothCandidate`.

All other calls of `GeometryGuidanceExcess` stay as they are.

### 3.5 The chain counter in `MeshOptimizer::SmoothBoundary`

```cpp
// M27. Counts the exact wall checks of one wall-vertex search in the quality stage.
struct SmoothChain {
  size_t checked = 0, passed = 0;
  bool moved = false;
  ~SmoothChain();  // records only when Phase() == 0, active is set and checked > 0
};
```

The destructor adds to `active->chains`: `++calls; checked += checked; passed += passed;
moved += moved; ++checked_bins[min(checked, 5) - 1]; ++passed_bins[min(passed, 3)]`.

In `SmoothBoundary`:
- declare `SU2NativeD9::SmoothChain d9_chain;` directly before the loop
  `for(Point target_point:targets)`;
- `++d9_chain.checked;` directly before the guidance call with `candidate`;
- `++d9_chain.passed;` at the statement `best_score=score;best=std::move(candidate);`;
- `d9_chain.moved=true;` at the statement `++report.boundary_moves;`.

## 4. Gates of this item (one commit)

Before the first edit: one source check (R4). A contradiction on a counter is written
and does not stop the item.

1. Build with the same options. No new warning in the touched files.
2. G1, G2, GQ, G4 and G3 (rules a and b) against the references of work order 2,
   section 2. All old rows are equal. `fixedstorage=4120`. The timer calls are equal.
   The `NATIVE3D_M27_...` lines are new rows; your compare script skips them (R3).
3. Then the G5 run of the end of phase C, with this commit inside. It gives the numbers.
   Its gates are the gates of G5. No extra full run for M27.

If gate 2 fails: R5 (one diagnosis, one correction of a wiring error, else revert N17 and
go on with the G5 run without it).

## 5. Report

File `R/night_20261115/N17/reports/M27_<build>.md`, with a CSV of the raw lines of all
ranks and both events. Keep the raw log lines.

For each event:

1. Table Q (stage `quality`): one row for each part. Columns, as the sum of the six
   ranks, and the smallest and largest rank: queries, hits, miss_new, miss_repeat,
   miss_other_kind, the three work sums, the three second sums, faces_reached_census,
   work_reached_census. One more row with the totals.
2. Table W (stage `quality`, as the sum of the six ranks): for each part,
   `miss_new - faces_reached_census` and `work_new - work_reached_census`. These are
   the first computations of faces that no census query asked for later.
3. The same table Q for the stages `other`, `final_proof` and `surface`, totals of
   the six ranks only.
4. Table S: the `NATIVE3D_M27_SMOOTH` line of each rank and the sum.
5. `NATIVE3D_M27_TABLE` of each rank.
6. A cross-check, report only: for each rank and each site a, b, c, d, e of
   `D9_QUERY phase=quality`: the sum of `queries + bypass` over the parts of that site
   beside `upper_calls`, and the sum of `hits` beside `memo_union_hits`. Write the
   differences. A difference is not a failure (the scopes can nest).
7. The values `memo_stores`, `memo_evictions`, `memo_union_entries`, `memo_bytes` and
   `memo_limit` of the last `NATIVE3D_STAGE` line of each rank and event.

No conclusion and no proposal for a fix in the report (R2). The reviewer reads the
numbers and writes the text of N16.
