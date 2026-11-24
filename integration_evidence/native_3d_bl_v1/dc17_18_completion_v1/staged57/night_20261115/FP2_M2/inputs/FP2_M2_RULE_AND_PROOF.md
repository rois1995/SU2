# FP2-M2: measurement of the geometry distance rule and of the proof cost (lab test code only)

Author: reviewer, 2026-11-24 (source facts and review by Codex). For the main agent. The user sends it; it is a
user order. Copy this file with its SHA256 into `R/night_20261115/FP2_M2/inputs/`.
Code base: `0957977801` ("FP2-M1: measurement done; default path not yet proven"). Production code = build664
(`af5844543d`). This order changes TEST CODE ONLY. No production source file changes (R2): no file under
`Common/` and `SU2_CFD/` is edited. If a step of this text seems to need a production edit: do not do it, write
it in the report (R4) and do the rest.
Short names: AR = `Common/src/adaptation/CNativeAffineReference.cpp`; ARh = its header; BP = `Common/src/
adaptation/CNativeBoundaryParameters3D.cpp`; SETUP = `UnitTests/SU2_CFD/adaptation/fixtures/
native_fast_path_stage2_setup.hpp`; VH = `native_volume_handover.inc`; OBS = `native_c3_lab_observers.inc`;
LAB = `CNativeLocalCoreLab_tests.cpp`.
Source lines in this text are those of `af5844543d`; the lab files moved a little in `0957977801`.
"The lab" = the lab of FP2-C3 continuation 1 (build664) with its runner, comparator and one-trial evaluator.
The BASELINE, ANATOMY and TOLERANCE trials of this order are the serial case N1 with the M1 observer OFF. The
options of this order are OFF in the inherited serial, two-rank and curved fixture runs (the curved fixture
keeps its own trace setting and checker). The new print-only rows are gated by the lab reporting
(`stats.report`) together with the options of this order.

R1 and this order. The user ordered this measurement (2026-11-24). Some trials run with another geometry
tolerance than 1e-4. Such a trial is a MEASUREMENT. It is never an acceptance, never a reference and never a
candidate. The gate "geometry 1e-4" does not change. No config of the repository, no reference file and no
pinned value of another test changes.

## 1. Why

Result of FP2-M1 (N1): the mean support of stage 1 fell from 239.39 to 20.46 facet ids, with the same proofs.
The stage-1 replay was 882.77 s and the stage-1+2 replay 883.07 s, against 924.04 s of the full forwards of the
same run. Both estimates use the cache history of that run. So a smaller support gives only a small saving.
Two things are open, and the user wants numbers for both before a design:
- THE PROOF. Where does one query on a large face spend its work? The source has these places: the physical
  traversal (AR:2455-2533: root face, upper bound from one chord or from a projected footprint, then a sample
  and a four-way split), the build of the joined witness (`BuildPlanes`, on a scope miss), the per-facet
  chords, the support selection, and the direct path. Your run S of build664 gives only a ratio over all
  callers (`NATIVE3D_STAGE` rows of rank 0: `traversal_seconds` is about 85 % of `upper_seconds`,
  `scope_build_seconds` about 12 %).
- THE RULE. The tolerance 1e-4 is `ADAP_HAUSD` of the M6 config (default of the option: 0.01). The model is a
  set of cubic PN patches on the triangles of the FIRST mesh. The user asks what the rule decides on the wing,
  what a larger tolerance gives, which faces and edges an angle limit would mark, and how smooth the model is.
Limits of this measurement, to write at the top of the report too: the same frozen sensor supplies the input
tensors in each trial (the pass makes the tensors of inserted points as usual). A change of the tolerance
moves the serial geometry thresholds together: 0.7 tol, 1.4 tol, tol/8, the support padding by tol, and the
first chord ceiling tol/16 (the margin 3 tol of the remote contact belongs to the MPI path; N1 does not use
it). The angle tables describe the measured meshes; they do not predict the mesh of an angle rule.

| Assumption | State |
|---|---|
| On the N1 path the only reader of the lab tolerance is `PrepareIN1` (SETUP:39-56), which gives the constant `native_fp2_fixture::Tolerance` (SETUP:13) to `WithEntityGeometry`. `LabInput::tolerance` (from `ADAP_HAUSD` of `run.cfg`) has no reader on the N1 path. The pass, the observers, the strict hand-over and the rebuilt proof read the tolerance of the state | from the source (read by Codex). Check it in your R4 source check |
| The trace `SU2_NATIVE_GEOMETRY_WORK_TRACE=1` (AR:148-157) only prints to stderr. It adds no work charge, no byte admission and no input of a decision. It adds elapsed time before later time checks | from the source. The identity check of 3.4 tests the equality of the inherited repeat projection |
| The physical traversal is the main cost of a step-C query | measured only as a ratio over all callers of run S. Measured here for step C (2.2) |
| How many physical nodes one query prepares, how deep, how often the root is enough, how far the root bound is above the tolerance, and how the work splits between coverage, lift, sample and the rest | NOT KNOWN. Measured here (2.3) |
| A larger tolerance gives clearly fewer surface faces and cells | NOT KNOWN. Measured here (2.4) |
| The cost of ONE proof stays about the same at a larger tolerance (each face is again near the limit) | NOT KNOWN (reviewer's guess). Measured here (2.4, trace off in all four trials) |
| The lab qualifies at a larger tolerance | NOT KNOWN. A trial that does not qualify is a finding, not a fault of yours (3.5) |
| The model normal is continuous across the edges of the reference facets | NOT guaranteed by the source: equal edge controls give position continuity only. Measured here (2.6) |
| `EvaluateSurface(side, binding)` gives a value for each corner of each hand-over face | it can throw when the side is not incident (ARh:170-183; AR:1978-2012). Counted (2.5) |

## 2. The measurement (test code; new lab options, off in each other test)

2.1 Lab tolerance from the config (option T). Give `PrepareIN1` a defaulted parameter for the tolerance. Only
the N1 lab with option T on supplies it, with `LabInput::tolerance` (the parsed `ADAP_HAUSD` of the trial's
`run.cfg`). Each other caller keeps the constant. If your source check finds another reader of a tolerance on
the N1 path that cannot follow without a production edit: write it (R4) and do not run 3.5.

2.2 Stage counters of step C (rows `FP2_M2 STAGE`). In `LabBoundObserver::WithinTolerance` take two snapshots
`state_.Geometry().GeometryStatsSnapshot()` (default `reset=false`), one immediately before and one
immediately after the forwarded production call. `seconds_forward` is the wall time between the two
snapshots; the writes of the marks of 2.3 and the bookkeeping of the bins are outside it. Sum the differences
for each pass by the five bins of the longest physical edge of your table 3.1 (FP2-C3) and by
`direct_eligible` 0 / 1 (the flag of FP2-M1; a direct-eligible face can still go into the union query). One
row for each bin and flag:
`FP2_M2 STAGE rank= pass= bin= direct_eligible= calls= seconds_forward= direct_calls= direct_seconds=
upper_calls= upper_seconds= classification_calls= classification_work= facet_hits= facet_misses= scope_hits=
scope_misses= scope_builds= scope_build_work= scope_build_seconds= support_calls= support_facets=
support_seconds= facet_build_work= facet_build_seconds= piece_selection_work= piece_selection_seconds=
traversal_work= traversal_seconds= selected_pieces= support_pieces=`.
No extra query is made. The rows are made in each trial.
Isolation. The snapshots, the data and the format of the marks, and the bookkeeping of the bins are isolated
from the forward (as `Measurement::Record` does in the C3 observer). If a part of the observation fails: record
that measurement as incomplete and still call the inherited forward exactly ONE time; return its original
answer or let its original exception pass. An opened `Q` mark of 2.3 is closed on unwind through a path that
cannot throw. The existing gates of the trial apply to the run as it then is.
Observer time. The inherited Bound clock of step C and the pass clock enclose the observer. Record the time of
the new observer work (snapshots, bins, marks, the capture of metric lengths of 2.5) apart
(`FP2_M2 OBSERVER rank= pass= seconds=`) and add it to the existing `measurement_seconds` of the lab where the
pass report already subtracts it. In the report the inherited clocks of step C are named "instrumented"; the
cost of one forward is compared with the trace-off `seconds_forward` only.
In the report, for each bin and flag: (a) the sums of `direct_seconds`, `support_seconds`, `upper_seconds`,
each divided by the sum of `seconds_forward`, and the remainder; (b) the sums of `facet_build_seconds`,
`piece_selection_seconds`, `scope_build_seconds`, `traversal_seconds`, each divided by the sum of
`upper_seconds`, and the remainder; an empty share when the denominator is zero; (c) for the largest length
bin: each summed seconds field and each summed work counter, and each sum divided by `calls`.

2.3 Anatomy of the physical traversal (rows of the trace; the ONE trial of 3.4). All trace rows go to stderr:
- outer `NATIVE_SURFACE_QUERY authority= budget= start= tolerance=<hex> support= triangles=<hex>` (AR:2095-2110)
  before the memo lookup, and `NATIVE_SURFACE_QUERY_RESULT authority= budget= start= cache=hit|miss|bypass
  work= bound=<hex>` (AR:2112-2115) on a normal return. Pair the two by (`authority`, `budget`, `start`). An
  exception leaves a query without a result row. A memo hit prints both rows and one work receipt of the
  `BoundPhysicalSurface` scope;
- inner `NATIVE_SURFACE_QUERY tolerance=<decimal> model= mesh= facets= points=` (AR:2272-2282), only for one
  physical triangle and at most 64 facets, once for each resolution attempt; it can come without an outer row;
- `NATIVE_SURFACE_MODEL pieces= witness= full= prepared= work=` (AR:2450-2454) after the preparation;
- `NATIVE_SURFACE_PLANES triangles= groups= accepted= invalid= storage_refused= boundary_trimmed=
  trimmed_triangles=` (AR:567-568) near the normal end of each `BuildPlanes`;
- `NATIVE_SURFACE_GRAPH coverage= bound= coverage_work= lift_work=` (AR:330-336), one for each footprint
  attempt (several are possible for one node); its two work fields are inside `upper_work` of that node;
- `NATIVE_SURFACE_DOMAIN model= facet= depth= upper= lower= upper_work= distance_work= parameter=`
  (AR:2473-2494), one row for each PREPARED node, before it enters the queue. A prepared node is not always
  popped: a row with `upper <= tolerance` is "a prepared node within the tolerance", not "a proved leaf".
  `lower` is the running largest lower bound; a sample is computed only when the upper bound before it is
  above the tolerance;
- `NATIVE_GEOMETRY_WORK operation= total= exclusive=` (AR:153-157) at the end of each traced operation scope;
- `NATIVE_SURFACE_PLANE_REJECT`, `NATIVE_SURFACE_BOUND`, `NATIVE_SURFACE_REFINE`, `NATIVE_SURFACE_UNRESOLVED`:
  count them by kind; keep the `UNRESOLVED` lines in the evidence.
The direct path (`SurfaceError`) prints nothing.
Marks. Printed only when the trace is on, with `std::fprintf(stderr, ...)` (the serial child has one stderr
stream, so the order against the production rows is kept, the receipts of destructors included):
`FP2_M2 Q begin ordinal= bin= direct_eligible= longest_edge= sheet=` immediately before each step-C forward and
`FP2_M2 Q end ordinal= answer= seconds=` immediately after it, also when the forward unwinds with an exception
(`answer=threw`); `FP2_M2 PHASE name=collapse_report begin` / `end` around `ReportCollapseFaces` (it queries
the pass authority through `WithinTolerance`), `name=handover_strict` around `ObserveLabStrictFaces`, and
`name=proof` around the whole serial `ProveLabSurface`, `PrintLabC2Diagnosis` included.
The stream. Take stderr of the test child through its OWN pipe at the launch of the child and drain it during
the run. Rows that begin with `NATIVE_` or `FP2_M2 Q` / `FP2_M2 PHASE` go to the aggregator. Of these rows,
store only: each `NATIVE_SURFACE_UNRESOLVED` line (in the evidence of the attempt), and the rows of the first
300 forwards of each length bin (a sample file). Each other stderr line, timeout diagnostics included, goes to
the inherited log of the trial. The process ownership and the supervisor, terminal-process, Catch, exit and
memory checks stay as they are.
The aggregator gives each forward (the rows after its begin mark) exactly one class, tested in this order:
- `incomplete`: the forward threw, or has no matching end mark, or has an outer row without its pair, or has
  more than one outer query. Count them; keep them out of each cost table;
- `direct_only`: a closed forward that returned normally and has no outer query row;
- `hit`, `miss`, `bypass`: a closed forward that returned normally with exactly one paired outer query, from
  the `cache=` of its result.
For a `miss` or `bypass` forward it records: the count of support ids; the `work` of the result row;
`pieces`, `witness`; the `PLANES` rows; the `GRAPH` rows with the sums of `coverage_work` and `lift_work` and
the count with a finite `bound <= tolerance`; the count of `DOMAIN` rows with `model=0` (= prepared physical
nodes) and the count of them with `upper <= tolerance`; the largest `depth`; `root_ratio` = `upper` of the
first `model=0` row with `depth=0`, divided by the tolerance (missing when there is no such row); the sums of
`upper_work` and `distance_work`, for the root and for the other nodes apart; the sum of `exclusive` for each
`operation` name; `root_enough` = 1 when there is exactly one `model=0` row and it has `upper <= tolerance`.
Tables, by the five length bins, for `direct_eligible=0` (and one line for `direct_eligible=1`). First the
calls by class. Then, over the `miss` + `bypass` forwards of the group (print each denominator):
- prepared physical nodes for each forward: mean, median, 90 % value, maximum; the share with `root_enough`;
- `root_ratio` in the classes `<= 1`, `(1, 2]`, `(2, 10]`, `> 10`, not finite, missing: each count divided by
  the forwards of the group;
- largest depth: median and maximum; `witness` and `pieces`: mean for each forward;
- the share of forwards with a `PLANES` row; the mean of each `PLANES` field for each emitted `PLANES` row;
- `GRAPH` attempts for each forward (mean); attempts within the tolerance divided by all attempts;
- the summed result `work` divided by the forwards, and divided by the prepared physical nodes;
- shares of the summed result `work`: summed `coverage_work`; summed `lift_work`; the rest of the summed
  `upper_work` of the `model=0` rows (= that sum minus the two sums before: chord search, candidates, node
  preparation); summed `distance_work`; the rest of the result `work`;
- the ten operations with the largest summed `exclusive`, each divided by the sum over all operations of the
  group; ties in the order of the operation name.
Percentiles in this order: nearest rank on the sorted finite values (rank = ceiling of p times n); print n;
leave the summary empty when n = 0.
For each phase (`collapse_report`, `handover_strict`, `proof`) and for the rows outside each mark, one line:
the counts of the row kinds; the paired outer queries by `cache=` (count each pair: a caller can make two);
the sum of `exclusive` by operation.
The aggregator writes its tables at the end of its input, also when the stream ends early.

2.4 Trials with a larger tolerance (option T on, trace off). The trial's `run.cfg` is DERIVED from the pinned
config of N1: exactly one assignment `ADAP_HAUSD` changes, to `1E-4` (unchanged), `2E-4`, `5E-4`, `1E-3`.
Nothing else changes: same mesh, same sensor file, same `.native_ref`, same lab options. The rows of 2.2 and
the tables of 2.5 are made in each of the four trials.

2.5 Hand-over tables (two CSV files for each trace-off trial; serial).
Where the data come from (no new `EntityFaceError` / `WithinTolerance` call is made for the tables):
- before `pass_mesh.reset()`: save `Mesh::Length` of each physical surface edge in lab-owned storage, with
  the sorted pair of point ids as the key;
- after the inherited proof checks: keep `result.state` (the rebuilt state), and move the existing
  `LabEntityCensus::bounds` and the serial `local_answers` of the strict check out of their scopes; join them
  by the sorted triple of point ids;
- after the inherited cell digest (the last inherited check): make a separate
  `local::StateSurfaceGeometry(reference.original->WithParameterState(rebuilt_state), table_budget)` for
  `Samples(face)`, and use `rebuilt_state->Geometry().EvaluateSurface(...)` for the corners. `table_budget`
  is a budget of the tables only; check the deadline of the tables yourself around `EvaluateSurface` (it has
  no budget argument).
The kept data, the sampler and the CSV work count in the existing resource limits and in the watchdog of the
child. If the headroom for the tables is not there, or the table work throws or reaches its deadline: print
`status=incomplete`, stop the table work, and keep each inherited exit, deadline, memory and identity result
as it is. A failure of the supervisor counts also when the table work caused it.
`handover_faces_<tolerance>.csv`, one line for each physical surface face of the rebuilt state:
- `ordinal`: from zero, in the key order of `state.Surface()`; the three point ids in the orientation of the
  state; body, sheet, marker; inherited facet;
- `bin` of the centroid: for marker 2 the wing bins of OBS:8-21 (0 to 4, tip bin 4 first), and 5 for eta < 0.98
  with 0.10 <= c <= 0.90; bin 6 for each face of another marker;
- centroid x, y, z; area; unit face normal; physical length and metric length of the edges in the oriented
  order (0,1), (1,2), (2,0); `m_max` = the largest of the three metric lengths, empty unless all three joined;
- the four distances of `Samples(face)` (centroid and the three edge midpoints), each with its finite flag,
  and the `unconverged` count; `sample_max` = the largest finite one (empty if none);
- `strict_within`: the saved answer of the strict hand-over check for the face (0 / 1, empty if none);
- `rebuilt_classification_bound`: the saved number of the proof census for the face (empty if none);
- for each corner: `side` (the facet id used, empty if none); `eval=ok|none`; the model normal; `curvature1`,
  `curvature2`; `direction1`. Rule: collect the incident facets of the binding with `ForFacets(binding)` and
  keep those of the entity of the face; try the inherited facet of the face first if it is in that list, then
  the others in ascending facet id; the first `EvaluateSurface(side, binding)` that returns is used; an
  exception goes to the next one; none gives `eval=none`;
- `normal_dev_deg` = the largest angle, over the corners with `eval=ok`, between the unit face normal and the
  model normal (empty if no corner has a value).
`handover_edges_<tolerance>.csv`, one line for each physical surface edge: the sorted pair of point ids; the
ordinals of its incident faces (`IncidentFaces(edge)`) and their (body, sheet, marker); `feature` = 1 when the
edge is a feature trace (`Coverage().traces`), independent of the class; `class` = the first that applies of:
`open` (not exactly two incident faces), `feature` (a trace), `entity_border` (the two entity keys differ),
`interior`; physical length; metric length; `turn_deg` = the angle between the two oriented unit face normals
(0 = flat; empty for `open`); `bin` of the midpoint: 6 unless the two incident faces have the same marker,
then the rule of the faces for that marker.
One row `FP2_M2 TABLE rank= status=complete|incomplete faces= edges= corners_ok= corners_none=
edges_without_metric= seconds=`.

2.6 The model alone (one serial fixture section on the pinned M6 `.native_ref` of the lab; no mesh pass).
Load the reference state with the pinned N1 byte ceiling and call `PrepareIN1` (the 60-degree mobility policy
and the marker roles of N1). Then take `*reference.original->Parameters()` AGAIN (the helper replaces the
view) and use `Entity(facet)` of that state for the edges and for the facets. Without the mobility policy the
sheet of a facet is its own id and no edge would have two facets of one entity.
Coordinates: use the straight chords of the reference vertices for the edge length, the edge midpoint, the
facet centroid and the longest edge of a facet. Use the cubic patch only for the points on the seam, the
derivatives and the differential values.
`model_edges.csv`, one line for each edge of the reference triangulation that is shared by exactly two
reference facets with the same (body, sheet, marker): the two vertex ids (lower id first); the two facets; the
entity; midpoint x, y, z; length; the bin of the midpoint (rule of 2.5 for the marker of the entity); and at
the edge parameters t = 0, 1/4, 1/2, 3/4, 1 (t = 0 at the lower vertex id) the angle in degrees between the
unit normals of the two patches at that point. Each normal is the cross product of the two first derivatives
of its patch (`TriangleDerivatives` on the nets of `Curves()`, `CNativeCurvedReference.hpp:357-373`), with the
orientation of its facet. A normal of zero length or not finite makes that angle missing (empty field).
`kink_max` = the largest of the finite angles, empty if none is finite. Count the edges with a missing angle
and the edges with no finite angle.
Pin (R3 if it fails): for each line, the two patches give the SAME point at each of the five parameters, and
at t = 0 and t = 1 the point of the named reference vertex, to 1e-12 times the diagonal of the bounding box of
the reference vertices.
`model_facets.csv`, one line for each reference facet: id; entity; bin of the centroid; longest edge;
`control_deviation` = `StructuralEstimate(facet).control_deviation` (the largest displacement between a curved
control point and its degree-elevated flat control point, AR:2233-2239); `curvature1` and `curvature2` from
`EvaluateSurface(facet, FacetPoint(facet, 1/3, 1/3))` and from `EvaluateSurface(facet, Vertex(id))` at the
three corners (an exception gives empty fields and a count).
Edges between two different entities, and edges with one facet: counts only.

## 3. The runs

A new build number and new receipt folders under `R/night_20261115/FP2_M2/`. Build `UnitTests/test_driver` only,
no new warning. The receipt shows: the production source pins and the hash of the `SU2_CFD` binary are those of
build664; the new test binary; the changed files and helpers of this order are pinned. One owned heavy job;
the host preflight of R1 before each launch.
Runner. Do not reuse the command builder of FP2-M1 as it is (it forces the M1 observer on and the trace off).
Write an FP2_M2 adapter around `evaluate` of `FP2_C3/run_c2_lab.py`: M1 observer off; the trace set for each
trial; for the derived config of 2.4, verify the original pins first, then replace only the config bytes of
the input tuple (one changed assignment) and give it to the inherited preparation, which seals the hash of
the new `run.cfg` beside the original pins; one receipt folder for each trial and for each retry; the stderr
pipe of 2.3 for the traced trial. All gates of `evaluate` and the outer checks of one trial stay.
Watchdog, for each trial: the wrapper timeout is 3,261 s (as in FP2-M1 for N1). Retry only a wrapper-only
timeout, one time, with the same value, after all owned processes are terminal; the receipt names the retry;
keep the partial evidence of both attempts. A result, modeled-memory, job-memory or in-process deadline
failure that was seen wins over the watchdog.
Status of a part. A failure that was seen before the watchdog wins over the watchdog.
- BASELINE and ANATOMY: `done`; `OPEN` after a second wrapper-only timeout; `FAILED` for an inherited failure
  or a confirmed identity difference. When the projection is not complete, the identity is "not evaluated".
- ANATOMY also reports the completion of its tables: `complete`; `partial` (the stream was interrupted; the
  tables say how many forwards they hold); `failed` (the collector or the parser cannot give the tables). A
  `failed` completion makes the ANATOMY part `FAILED`, also when its identity passed.
- A TOLERANCE trial that throws, fails an inherited gate, or reaches the watchdog two times is a `finding`. It
  is never `OPEN` and never `FAILED`.
Attempts. For each named trial the report uses ONE attempt: the last authorized one. Name its receipt. Earlier
attempts and their partial files stay in their own receipt folders. Each table comes from one attempt only.
If the last attempt is partial: show the counts that it has and leave the other fields empty.
R5 in this lab-only item: one diagnosis pass, one correction
of a wiring fault of your test code, one repeat; there is NO revert of a failed measurement: keep the failing
test code and the receipts. A failed fixture or pin: R3 / R6, record it, go on with the work that does not
depend on it. Update the progress file after each sub-step. Stop only after all independent trials and the
report are done.
1. The focused fixtures of FP2-C3 continuation 1 (9 serial, 3 two-rank) and the curved fixture of FP2-M1, all
   options of this order off: all pass with the Catch counts of build666.
2. The model fixture of 2.6: the two CSV files and its pin.
3. BASELINE: option T on, `1E-4`, trace off, the rows of 2.2, the tables of 2.5. ONE trial.
   - Identity: verify the sealed trial 1 of build664 of N1 as in FP2-M1; then `repeat_differences`
     (`FP2_C3/compare_lab.py`) between the new trial and that trial: equal, including the compared
     `modeled_peak` fields. This checks the equality of the inherited repeat projection.
   - A difference or a failed inherited gate: R5 as above; if it stays, the part is FAILED: keep the
     evidence, write the first row, field and pass. The trials 3.4 and 3.5 still run; the report says that
     their base differs.
4. ANATOMY: option T on, `1E-4`, trace ON with the marks and the stream of 2.3, the rows of 2.2. No CSV tables.
   ONE trial. Its seconds are not used for a time comparison.
   - Its own identity check as in 3.3 (equality of the inherited repeat projection). Report the completion of
     the anatomy apart from it (the status rules above).
5. TOLERANCE: option T on, `2E-4`, `5E-4`, `1E-3`, trace off, the rows of 2.2, the tables of 2.5. ONE trial
   each, in this order.
   - No identity check (the tolerance differs). For each trial report three things apart: the result of
     `evaluate` (qualified or the failed gate), `qualified` of the proof, `accepted` of the proof.
   - A `finding` (the status rules above; a trial that does not qualify is one too): keep its rows and tables
     as far as they exist. One look of at most 15 minutes: which gate, the first row and pass. No correction
     of production code. Go on with the next value.
The numbers of section 2 are a REPORT. They have no gate.

## 4. The report

`R/night_20261115/FP2_M2/reports/FP2_M2_<build>.md` and `.json`; the CSV files and the sample file of 2.3
beside them (commit them). Write the limits of section 1 at the top, and the status of each part.
A. The proof: the `STAGE` tables of the BASELINE trial (2.2 a, b, c); the tables of 2.3 with the completion of
   the anatomy; the seconds of both trials beside build664.
B. The tolerance, one column for each of the four trace-off trials:
   - the three results of 3.5 (for 1E-4: of 3.3); pass seconds; the instrumented seconds of steps L, D, C and
     of the Bound clock of step C; `seconds_forward` with calls and mean milliseconds, total and for each
     length bin; the `STAGE` shares; the `OBSERVER` seconds;
   - cells out; surface faces at hand-over, total, by entity and by bin; surface points;
   - step L splits; step D splits with its `d_*` and `left_*` counters; the rows `FP2_D REFUSED_REGION`
     (`captured` by bin, and `total_refused` one time for each observer; they are the capped captures of the
     refusals at the end of step D); step C removals and each `c_refused_*` counter;
   - the proof rows: both directed bounds for each entity; faces classified above the tolerance; the seconds
     of the proof parts; `modeled_peak`.
C. From the hand-over CSV files, for each tolerance and each bin (print each denominator and each count of
   missing values):
   - faces; area; median and 90 % value of the longest physical edge;
   - the share of faces with `m_max` below 0.5, and below 0.9 (denominator: faces with `m_max`);
   - of the faces with `sample_max`: the shares with `sample_max` above 0.35, above 0.6 and above 0.7 of the
     tolerance, and the median and 90 % value of `sample_max` / tolerance; apart: the count of faces with a
     sample that is not finite, with no finite sample, and with `unconverged > 0`;
   - the histogram of `turn_deg` of the `interior` edges, with the classes `<= 1`, `(1, 2]`, `(2, 5]`,
     `(5, 10]`, `(10, 15]`, `(15, 20]`, `(20, 30]`, `(30, 45]`, `> 45` degrees, with median, 90 % value and
     maximum; the counts of the other edge classes;
   - the histogram of `normal_dev_deg` with the same classes, median, 90 % value and maximum; `corners_ok`
     and `corners_none`.
D. The model (2.6), for each bin: edges; the histogram of `kink_max` with the classes `<= 0.01`, `(0.01, 0.1]`,
   `(0.1, 0.5]`, `(0.5, 1]`, `(1, 2]`, `(2, 5]`, `(5, 15]`, `> 15` degrees, with median, 90 % value and
   maximum; median and 90 % value of `control_deviation` and of the longest edge of the facets; the counts of
   the edges and values that were left out.
Write no rule and no recommendation; the reviewer reads the tables and the CSV files.

## 5. The order of the work and the labels

0. One R4 source check of this text (the statements of the table of section 1 included).
1. The test code, the runs 3.1 and 3.2. Commit and push: "FP2-M2: fixtures and model table done; default path
   not yet proven". If a fixture or the pin of 2.6 failed after R3 / R6: "FP2-M2: fixtures and model table
   FAILED; default path not yet proven", and go on with the trials that do not depend on the failed part.
2. The trials 3.3 and 3.4. Commit and push: "FP2-M2: baseline and proof anatomy done; default path not yet
   proven"; with a FAILED part: "FP2-M2: baseline and proof anatomy FAILED; default path not yet proven"; with
   no FAILED part and an OPEN part: "FP2-M2: baseline and proof anatomy OPEN; default path not yet proven". In
   each case go on with step 3.
3. The three trials 3.5 and the report. Commit and push: "FP2-M2: measurement done; default path not yet
   proven". A `finding` of 3.5 does not change this label; the report names it. If a fixture, the BASELINE or
   the ANATOMY is FAILED, or a table of the report is missing without a recorded reason: "FP2-M2: measurement
   FAILED; default path not yet proven". Else, if a part is OPEN: "FP2-M2: measurement OPEN; default path not
   yet proven". Update the progress file, `HANDOFF_Codex.md`, `HowToProceed.md` and
   `Handbook/ADAPTATION_TIMINGS.md` with the numbers of parts A and B. Then STOP and wait for the next user
   order.
The rules R1, R3, R5 (as section 3 reads it), R6, R7, R8 and R13 hold as in each item.

## 6. Not in this order

A change of the proof, of test 11, of the tolerance of a production run, of `ADAP_HAUSD` in a config of the
repository, of the metric, or of the model. An angle rule. A run S, G1, the FP1 cohorts, N6, N6P. A trial
beyond the named trials and the watchdog retry or R5 repeat that section 3 permits. A rule or a
recommendation from the numbers: the reviewer and the user decide after the report.
