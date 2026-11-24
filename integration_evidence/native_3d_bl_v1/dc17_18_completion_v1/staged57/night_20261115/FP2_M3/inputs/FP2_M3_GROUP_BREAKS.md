# FP2-M3: measurement of the cause of the footprint group breaks (print-only trace rows + lab test code)

Author: reviewer, 2026-11-24 (source facts and review by Codex). For the main agent. The user sends it; it is a
user order. Copy this file with its SHA256 into `R/night_20261115/FP2_M3/inputs/`.
Start. Only after the final checkpoint of FP2-M2 ("FP2-M2: measurement done", "... FAILED" or "... OPEN", each
with "; default path not yet proven") is recorded and each owned job of FP2-M2 is terminal. That commit is the
code base; write its hash in the report. (`74a05f17dd` is the baseline / anatomy checkpoint, not the final one.)
Short names: AR = `Common/src/adaptation/CNativeAffineReference.cpp`; M2 = the order `FP2_M2_RULE_AND_PROOF.md`
(your copy in `R/night_20261115/FP2_M2/inputs/`); the other names as in M2. Source lines are those of
`af5844543d`.

Scope. This order adds PRINT-ONLY rows at THREE places of AR (2.1) and lab test code. The new production lines
run only when the trace switch `SU2_NATIVE_GEOMETRY_WORK_TRACE=1` is on (AR:148-152), as the other trace rows of
that file. This is the written exception to R11 for this item: the switch is the trace switch, not
`options.timings`. No other production line changes. The new lines leave the proof data, the charges of work
and storage, and each decision unchanged; they add no persistent field, no timer operation and no modeled byte
(R10). Trace output and its collection can add elapsed time and real memory. So the identity with the switch
on is a gate that is executed (3.4); it is not taken from the form of the change.
Deferred gates. `BuildPlanes` and the union query are shared production code. For FP2-M3 ONLY, this user order
explicitly defers the shared-engine executions that AGENTS.md section 2 and the standing instruction of
2026-11-14 ask for a shared-code change, and also G2, GQ, G3, G5 and a run S. Reason: the new lines run only
with the trace switch on, and none of these runs sets it. List each deferred check as "not evaluated" for this
build in the report. Section 3 is the complete gate set of this trace-only checkpoint. No gate threshold
changes. The deferred checks run at the item that changes the proof, on the code with these lines.

## 1. Why

The ANATOMY trial of FP2-M2 (N1, 70,482 uncached step-C queries) shows two kinds of query. 73.5 % prove the
root face with one prepared node. The other 26.5 % prepare about 400 nodes each (7.58 M nodes) and hold nearly
all the work; in the largest length bin 60 % of the work is `coverage_work`.
The reviewer counted in your sample file `anatomy_sample.log` (the first 300 forwards of each length bin; the
forwards of the length bins 2, 3 and 4 with `direct_eligible=0`, a `PLANES` row and a root `DOMAIN` row): 649
forwards have a root bound at or below the tolerance, 549 of them with ONE plane group; 185 forwards have a
root bound above 10 times the tolerance, ALL of them with two or more groups. This is a relation in a sample.
A root with one group can also be above the tolerance after a successful coverage (one such row is in the
sample).
Source (read by Codex): `BuildPlanes` (AR:492-568) joins two chords directly only when they have an edge with
the SAME two vertex ids (ids by equality of the stored coordinates) and the same projection (axis, sign);
another path of such joins can still connect two chords. The chords of a facet come from four-way midpoint
subdivision until the error of a leaf is at most resolution / 16 (`prepare_facet`, AR:2319-2327); no step makes
them conforming. So a coarse edge (A, B) beside two fine edges (A, M), (M, B) gives no direct join
("T-junction"), between two facets or inside one facet. The crop of the witness (AR:2374-2404), the border of
the support, and a different (axis, sign) end a group too. A piece is tested against ONE group (AR:296-380).
If all upper-bound attempts stay above the tolerance, the traversal splits the piece, or proves from a lower
bound that the face is outside the tolerance, or refuses it at the approximation floor (AR:2473-2531).

| Assumption | State |
|---|---|
| A root fails mainly because the face lies across two or more groups | a relation in a sample (above); mechanism from the source. Measured here for the uncached forwards with a usable witness batch (2.2); the other forwards are reported apart |
| The groups end mainly at T-junctions of the chord subdivision | NOT KNOWN. The reviewer has only an estimate from the control deviations of the model table (14.5 % of the mid-chord model edges); the model table has no leaf depth. Measured here |
| The other causes (crop inside the face, support border, axis / sign, same segment without equal ids) are rare | NOT KNOWN. Measured here |
| The T-junctions are between two facets, or inside one facet | NOT KNOWN. Measured here |
| The stored chord point k is the middle of the control of the parameter corner k of its leaf (`UnionChord`, AR:230-238; `net`, AR:2302-2311); the local corner k of a facet is `reference->Get(facet).v[k]`; the index of a leaf in the retained list of its facet is its order of acceptance in `prepare_facet`; the position of a chord in the joined list is its index in `BuildPlanes` | from the source (read by Codex). Check it in your R4 source check; the fixture of 3.2 pins the first statement |

## 2. The measurement

2.1 New trace rows (production, print-only, each inside `if (AtlasOps::Trace())`). Rules for all of them: no
`op.Work()`, no `op.Storage()`, no write to a variable that later production code reads, no new member; the
containers that they need are local to the trace branch and have a bounded size (one facet build, one witness,
one `BuildPlanes` call). A failure inside the new lines (an exception, for example of an allocation) must not
change the outcome of the function: catch it there, print `NATIVE_SURFACE_TRACE_INCOMPLETE site=`, and go on
with the unchanged code. A failed write is not an exception; the collector sees it as a missing row. The
existing rows stay as they are. Readable code (R9): small helper functions, not inline blocks. Numbers of type
double are printed as the hex word of their bits (as the outer `NATIVE_SURFACE_QUERY` row does); `authority` is
the pointer of the authority as in that row.
(a) Leaves, in `prepare_facet` (AR:2319-2327):
- at the start of a call: `NATIVE_SURFACE_FACET_BEGIN authority= facet= resolution=`;
- when a leaf is accepted (`model.push_back(piece)`): `NATIVE_SURFACE_FACET_LEAF authority= facet= resolution=
  ordinal= depth= parameter=<nine words: the three barycentric numbers of each of the three corners> points=
  <nine words: x, y, z of the three stored chord points> error=`, with `ordinal` = the count of leaves of THIS
  call that were accepted before it (from zero);
- at the normal end of the call: `NATIVE_SURFACE_FACET_END authority= facet= resolution= leaves=<count>`.
(b) Witness. Where the joined witness is built on a scope miss (AR:2430-2434), before the `TriangleUnion` is
made, ONE row: `NATIVE_SURFACE_WITNESS authority= resolution= chords=<count> list=<facet:piece,facet:piece,...>`
in the order of the joined list (`facet` = `facets[selected.first]`, `piece` = `selected.second`).
(c) Groups, in `BuildPlanes`, before the unchanged `NATIVE_SURFACE_PLANES` row:
- for each group, in the ascending order of its root index after all unions:
  `NATIVE_SURFACE_PLANE_GROUP index= axis= sign= status= chords=<the chord indices, ascending, with commas>`,
  `index` from zero in that order. `status`: `single` (one chord); else `zero_sign`; else the outcome recorded
  at the existing branch of that group: `storage_refused`, `invalid`, `trimmed` (success after the retry; add
  the field `kept=<the kept chord indices>`), `accepted` (success without the retry). Record the outcome in a
  trace-only list at each branch; the totals `invalid`, `refused`, `trimmed` cannot give it later;
- for the chord edge keys (sorted pair of the vertex ids of this function) count the DISTINCT chords of each
  key (the map `edges` keeps only the first chord; build the complete count in the trace branch from `ids`).
  Exactly ONE row for each call, also with an empty list:
  `NATIVE_SURFACE_PLANE_OPEN open_count= split_count= edges=<chord.k,chord.k,...>` with each key that has
  exactly one chord, in the sorted order of the keys (`k` = the local edge: the points k and (k+1) mod 3 of
  that chord; if one chord has the key on two of its edges, the smallest k). `open_count` = the count of these
  keys; `split_count` = the count of the keys of the next item. Both counts come from the complete key table,
  before a row is printed;
- for each key with two or more chords whose final roots are not all equal:
  `NATIVE_SURFACE_PLANE_SPLIT chords=<count> triangle= edge= group= other_triangle= other_edge= other_group=`
  for the first pair, in the order of the chord indices, that has different final roots (the same rule for
  `edge` and `other_edge`).

2.2 Collection and analysis (test helpers).
Collection, during the TRACE-ON trial. The stream handling of M2 2.3 stays (marks, own stderr pipe, the M2
aggregator with its own classes, cohorts and arithmetic). The collector writes the rows of this order (the
kinds of 2.1), the marks, the paired outer `NATIVE_SURFACE_QUERY` / `_RESULT` rows and the
`NATIVE_SURFACE_PLANES` rows in their order to ONE compressed file (the "provenance stream") and does no
analysis of them during the run. It does not copy the `DOMAIN` rows. At the close of each forward it adds one
short record with the values that the M2 aggregator already has for it: `FP2_M3 FORWARD ordinal= class=
root_enough=` (`root_enough` exactly as in M2: one prepared `model=0` node, and that node within the
tolerance). A forward without this record is "not classifiable" for the tables of this order. A row of this
order that is malformed or missing never changes the classes or the numbers of the M2 tables. The collector
keeps draining when a write to that file fails (then the measurement of this order is incomplete).
Do not commit the provenance stream; write its size and SHA256 in the report and keep it in the receipt folder.
Analysis, after the trial ended, from the provenance stream:
- Leaf tables. Stage the leaf rows between a `FACET_BEGIN` and its `FACET_END` with the same (authority, facet,
  resolution). Publish a table only when the end row is there and the ordinals are exactly 0 .. leaves-1; never
  publish a part. A later complete table of the same key replaces the old one (report how often its content
  differs: not expected).
- Numbers. Decode each hex word to binary64 and use its exact rational value. For each leaf check: each corner
  has three weights with sum one, and each weight is a whole multiple of 2^-depth. A leaf that fails makes its
  table not usable.
- Batches. A `WITNESS` row opens a batch; the next `NATIVE_SURFACE_PLANES` row of the same forward closes it.
  A new `WITNESS` row, the end of the forward or the end of the stream before that close: the open batch is
  "incomplete" (count it). Join each chord of the witness list to the published leaf table of its (authority,
  facet, resolution of the witness) at that point of the stream. A closed batch is "usable" when ALL hold: no
  `TRACE_INCOMPLETE` row is in it; `chords=` of the witness, the length of its list and `triangles=` of the
  `PLANES` row are equal; there are exactly `groups=` (of the `PLANES` row) group rows and their chord lists
  are a partition of 0 .. chords-1; there is exactly one `PLANE_OPEN` row, with `open_count` different entries;
  there are exactly `split_count` `PLANE_SPLIT` rows with different keys; each index is in range; each chord
  has a leaf. Inside a marked step-C forward a `NATIVE_SURFACE_PLANES` row without a `WITNESS` row before it
  is a build that is "not usable". Outside the step-C marks such a row is allowed and is not a batch. The same
  packet rules hold inside the production M1 marks of the curved fixture (3.2).
  A closed batch says that the trace packet is complete. Whether the forward returned normally comes from the
  class of the forward (M2).
- Facet neighbors, from `facet_vertices.csv` (2.3): two facets with the same (body, sheet, marker) are
  neighbors across a reference edge when a local edge of each has the same two reference vertex ids. The
  position on a reference edge is t = the barycentric weight of the corner with the HIGHER vertex id.
- "Overlap" of two leaf edges = they are on one line and their common part has a positive length (a common end
  point alone is no overlap). On one line, two dyadic leaf edges that overlap are equal, or one is a proper
  part of the other.
Class of each open edge e (chord X on facet F, parameter segment S). Take the first that applies:
1. `tj_intra_coarse`: an edge of another chord of the batch on the same facet F is a proper part of S;
   `tj_intra_fine`: S is a proper part of such an edge; `same_segment`: such an edge is equal to S;
2. S is on a local edge of F (one weight is zero on both ends). Let I be its interval on the reference edge:
   `entity_border`: F has no neighbor there; `support_border`: the neighbor G is not in the `support=` list of
   the outer query row of the forward; else, with the edges of the chords of the batch on G that lie on that
   reference edge: `tj_inter_coarse`: one of them is a proper part of I; `tj_inter_fine`: I is a proper part of
   one of them; `same_segment`: one is equal to I; `crop_inter`: none overlaps I (also when G has no chord in
   the batch);
3. S is inside F (not on a local edge): `crop_intra`: no edge of another chord of the batch on F overlaps S;
4. `other`: none of the above (count them; keep the first 50 as examples).
A `PLANE_SPLIT` row has the class `key_split`, with `axis_sign` when its two named groups differ in
(axis, sign), else `same_projection`; these two words describe the printed pair only. Report apart the keys
with `chords` above 2.
The classes describe the witness. They do not prove by themselves why the bound of a root was above the
tolerance; the tables below show the relation.
"Inside the face": take the tolerance and the three physical points from the ONE paired outer
`NATIVE_SURFACE_QUERY` row of the forward. The test is made for each open edge, and for ONE edge of each
`PLANE_SPLIT` row (its `triangle`, `edge`); a split key counts one time. The midpoint of the two stored chord
points of the edge, projected on the plane of the physical triangle, has all three barycentric coordinates in
[0.02, 0.98], and its distance to that plane is at most 10 tol. A forward without valid geometry is "not
classifiable" (count it).
The measured population is: the completed forwards of class `miss` or `bypass` (the classes of M2 2.3) with a
usable batch. For each of them: the length bin; `root_enough`; from the batch: groups, groups by `status`,
open edges by class, split keys by sub-class, and the edges INSIDE the face by class (open edges and split
keys).
Tables, by length bin and by `root_enough` 1 / 0. Print each denominator. A forward with no batch, with an
incomplete batch or one that is not usable, or not classifiable is counted apart and is never a zero:
- forwards with 0 / 1 / more batches (more than one is not expected: report it); groups for each batch (mean,
  median, maximum); the share of groups by `status`; open edges for each batch by class (mean); split keys
  for each batch (mean);
- for each class (`key_split` included): the share of forwards with at least one edge of it inside the face;
  the share with no edge of any class inside the face;
- for `root_enough=0`: the complete histogram of the SETS of merged classes inside the face, the empty set
  included (merged classes: `tj` = the four T-junction classes; `crop` = `crop_intra` + `crop_inter`; `border`
  = `support_border` + `entity_border`; `same_segment`; `key_split`; `other`);
- T-junction pairs: count an unordered pair of chord edges of a usable batch ONE time when one parameter
  segment is a proper part of the other (intra: same facet; inter: on a common reference edge) and at least
  one of the two is an open edge whose midpoint is inside the face. For each forward: the count of such pairs
  (mean, median, 90 % value), intra and inter apart; over all pairs: the histogram of the depth difference
  (the depth of the leaf of the finer edge minus the depth of the leaf of the coarser edge: 1, 2, more).
For the new tables of this order: the median and the 90 % value are nearest-rank values on the sorted
observations, at the one-based ranks ceiling(0.5 n) and ceiling(0.9 n). Print n; leave the value empty when
n = 0. The old M2 tables keep their own arithmetic.
Leaf tables of the whole stream (pass authority; one published table for each facet and resolution; write
also how many of them were built inside step-C marks), for each resolution that occurs: for each bin of the
facet centroid: facets; the histogram of the largest depth of a facet; the share of facets with leaves of
different depth. For each bin of the edge midpoint of the sealed `model_edges.csv` of FP2-M2, over the model
edges whose two facets both have a published table at that resolution: the share where the two sorted lists
of leaf intervals on the shared edge are not equal.
The old tables of M2 2.3 are made again by the M2 aggregator, with their own cohorts.
Examples: in the order of the ordinals, the first 20 completed forwards of the largest length bin with
`root_enough=0` and a usable batch: the group rows, the open edges and the split rows with class and inside
flag, and the leaves (facet, depth, parameter) at the edges inside the face. If fewer exist, keep all and
write the count.

2.3 Facet table (lab test code, no production edit), written after the cell digest of the TRACE-OFF trial
with the isolation and status rules of M2 2.5: `facet_vertices.csv`, one line for each reference facet:
`facet`; body, sheet, marker (the entity of the state with the mobility policy of N1); the three reference
vertex ids in the local corner order (`surface->State().Geometry().Reference().Get(facet).v[k].id`); `bin` of
the centroid (M2 2.6, straight chords); `retained_pieces` = `StructuralEstimate(facet).prepared_pieces` from
`surface->State().Geometry()`. `retained_pieces` is the largest leaf count that the cache holds for the facet
at this moment over its resolutions; 0 = no retained entry now (never built, evicted or not admitted); the
pass and the rebuilt state share the authority, so later uses are in it. Report it only as a cache snapshot
(zero and positive counts for each bin); it is not used for the classes.

## 3. The runs

A new build number and receipt folders under `R/night_20261115/FP2_M3/`. Build and pin the new `SU2_CFD` and
`UnitTests/test_driver`, no new warning. One owned heavy job; the host preflight of R1 before each launch.
Runner. Adapt the FP2-M2 runner: keep its frozen-input, runtime, process and artifact checks; replace its
check "production sources and `SU2_CFD` equal to build664" by a source-diff check that permits only the
authorized lines of AR, and freeze the new binary hashes. Verify the sealed trial 1 of build664 of N1 by
itself as the comparison reference. The watchdog (3,261 s, one retry of a wrapper-only timeout), the attempt
rule, the partial-evidence rule and the precedence "a failure that was seen wins over the watchdog" are those
of M2 section 3.
Trace switch: set `SU2_NATIVE_GEOMETRY_WORK_TRACE=0` explicitly for G1 and for the focused serial and two-rank
fixtures; set it to `1` for the curved fixture and for the TRACE-ON trial.
Observers of the two N1 trials: option T on; the M1 shadow observer off; the hand-over CSV exporter of M2 off.
The facet export of 2.3 is on only in the TRACE-OFF trial; the collector of 2.2 only in the TRACE-ON trial.
Status. Report each result gate and the completion of each measurement apart.
- A failed RESULT gate of the candidate (G1, one of the two N1 identity checks, an inherited gate of a trial):
  R5. Keep the first difference; one diagnosis of at most 60 minutes; a confirmed transcription or wiring
  fault: one correction and one repeat of that check. If it fails again or the change itself is the cause:
  revert the production edit in one commit, write the hash; the part is FAILED.
- A defect of a fixture, of the collector, of the analysis or of an export (test code): R3 / R6. If a required
  measurement is still missing or wrong after that work: the measurement is FAILED; this is no failure of a
  production result gate and needs no revert.
- A second wrapper-only timeout: OPEN, unless a failed gate was seen. When the projection is not complete the
  identity is "not evaluated".
- A required part that could not run because another part is missing (example: no `facet_vertices.csv`, so
  the classes of 2.2 cannot be made; the inherited checks of the TRACE-ON trial still run) is "not evaluated"
  with its reason: the item is then OPEN.
- Label rule at both checkpoints: FAILED if a required part failed; else OPEN if a required part is OPEN or
  not evaluated; "passed" / "done" only when each required part is complete. Go on with each part that does
  not depend on a failed one.
1. G1: the table of cases, pass / fail and failed assertions is identical to the reference (375 / 114 / 228).
2. The focused fixtures of FP2-C3 continuation 1 (9 serial, 3 two-rank) with the Catch counts of build666. The
   curved fixture of FP2-M1 with its checker unchanged (change it only if it shows an incompatibility). Two
   new checks of test code:
   - Pin of the leaf rows: take the first complete witness batch of the curved fixture that is inside a
     production `FP2_M1 TRACE_BEGIN` / `TRACE_END` pair. For each leaf of its chords and each corner k:
     evaluate the pinned cubic of the facet at the parameter corner k (exact rational arithmetic, or floating
     arithmetic with a stated rounding bound) and require that its distance to the stored point k of the leaf
     row is at most resolution / 16 plus that bound (resolution / 16 is the chord-error limit of the
     production code, not a new tolerance). Report the witness, the corners checked, and the depths and
     ordinals that occurred. No such witness: the pin is "not evaluated".
   - Self-check of the analysis on a small written stream: a facet with several leaf ordinals, a build that
     is cut before its end row (must not be published), a T-junction inside a facet, one between two facets,
     a cropped neighbor, and a border.
3. TRACE-OFF trial: N1, option T on with `1E-4`, trace off, the table of 2.3. Identity: `repeat_differences`
   against the sealed trial 1 of build664 of N1: equal, including the compared `modeled_peak` fields.
4. TRACE-ON trial: N1, option T on with `1E-4`, trace on with the marks and the stream of M2, the collection
   of 2.2. Identity as in 3.3 (equality of the inherited repeat projection). Then the analysis of 2.2. To
   report, not a gate: the numbers `forwards`, `prepared_nodes`, `root_enough` and `result_work` of each
   length bin beside those of the ANATOMY trial of FP2-M2.
The numbers of section 2 are a REPORT. They have no gate.

## 4. The report

`R/night_20261115/FP2_M3/reports/FP2_M3_<build>.md` and `.json`, with `facet_vertices.csv` and the example
file. The status of each part; the list of the deferred checks as "not evaluated"; the diff of the production
change; the tables of 2.2 and 2.3; the observer effect (trial seconds beside FP2-M2, trace rows by kind, size
of the provenance stream). Write no rule and no recommendation.

## 5. The order of the work and the labels

0. One R4 source check of this text (the statements of section 1 and of 2.1 included).
1. The code, G1, the fixtures. Commit and push, by the label rule of section 3: "FP2-M3: trace rows and
   fixtures passed; default path not yet proven", or "FP2-M3: trace rows and fixtures FAILED; default path not
   yet proven", or "FP2-M3: trace rows and fixtures OPEN; default path not yet proven". Write the real status
   of each part. Go on with the work that does not depend on a failed part.
2. The two trials, the analysis and the report. Commit and push, by the same rule: "FP2-M3: measurement done;
   default path not yet proven", or "FP2-M3: measurement FAILED; default path not yet proven", or "FP2-M3:
   measurement OPEN; default path not yet proven". Update the progress file, `HANDOFF_Codex.md`,
   `HowToProceed.md` and `Handbook/ADAPTATION_TIMINGS.md`. Then STOP and wait for the next user order.
The rules R1, R3, R5, R6, R7, R8, R9, R10 and R13 hold as in each item.

## 6. Not in this order

A change of the grouping, of the chords, of the coverage test or of a bound. A production change outside the
three print places of 2.1. The deferred checks named in "Scope". N6, N6P, a trial with another tolerance. A
rule or a recommendation from the numbers: the reviewer designs it after the report.
