
## 2026-11-17T22:07:06+01:00 · N4 phase C · C1

Can B02 use the same entry point as B07–B29 while leaving B03–B06 at their original scope and lifetime boundaries? The options were one wrapper spanning selection and lane setup, or a pre-selection entry for B02 plus the ordered decision function for B07–B29. Used the pre-selection entry. B02 can finalize before the lane timer, while later finalization must run with that timer alive. Keeping B03–B06 at the call site preserves their lifetimes and leaves acquisition code in its planned step. C1 base commit `ad64feb822`.

## 2026-11-17T22:32:54+01:00 · N4 phase C · C2

Where should the Topology timer shared by B35e–g live when those blocks become named functions? Separate stage timers or one scope in the Exchange unit were possible. Used one Exchange scope with the original features and retained-cell state kept alive through expansion and receipt service. This preserves the original timer call and nesting. C2 base commit `f7017771e8`.

## 2026-11-17T22:55:10+01:00 · N4 phase C · C3

Does “join stays first” move the join ahead of ReportPrivateWork? The alternatives were the first post-slot action and the first post-slot collective. Kept the private work observation immediately before the unchanged first collective because C requires the original order and timer scope. C3 base commit `c7b7664853`.

## 2026-11-18T01:11:11+01:00 · N4 phase C · C6

Should each private donor call take the imported cavity as an argument, or should the trial switch borrow it once? Used a const reference to the live per-slot Imported object. The former lambdas captured that same object and read its cells at each call, so this keeps the old bindings and ownership. C6 base commit `227187f952`.

## 2026-11-18T02:43:34+01:00 · N4 phase C · C9

Where should the immediate inputs to the legacy strip, apex and growth searches live? The alternatives were caller-local assembly and moving the E18–E24 block together. Moved E18–E24 together to preserve the original scopes and retain the three reservation policies. C9 base commit `f3beeb21d8`.

## 2026-11-18T04:41:41+01:00 · N4 phase C · G5

Which complete log should supply the full integer and timer baseline? The options were the original phase A comparison input and the adopted N14 full workflow. Used the N14 log for the existing C §1 comparison, and the work order §2 P6c/P7 file hashes for payload identity. The two receipts serve different checks and the phase A comparator validated itself against the sealed N14 logs. Phase end pre-GQ checkpoint `e3444aa51d`.

## 2026-11-18T04:53:47+01:00 · N17 · M27 source check

Does the table size mean the number of faces alone or the instrumented table footprint? Kept the specified face count and added estimated bytes for entries, node links and buckets. The 10 MB expectation refers to memory, and allocator overhead has no portable exact value. N17 base commit `167e353ca7`.

## 2026-11-18T07:25:44+01:00 · N4 phase D · D1

Does deleting the non-streamed metric block remove its outer timer scope as well? Kept the scope, field accounting and elapsed-time subtractions, because they execute after the dead condition and contribute to the old results. D1 base commit `cba3cfe577`.

## 2026-11-18T07:39:38+01:00 · N4 phase D · D2

Should the extracted Adapt helpers group lifetime guards in named methods or share a new owner with its own teardown? Used named methods that retain the original guards until the continuation returns. This preserves construction order, reverse teardown, and the original scope at each Finish call. Draft source base commit `cba3cfe577`.

## 2026-11-18T10:34:02+01:00 · N16 · A–C source check

How should the packed controls and scaled window handle existing full-width inputs? The options were literal unchecked arithmetic and preserving the specified agreement and floor within the existing range. Used invalid-pair sentinel packing with the same two votes, the exact old result when widths match, and overflow-safe scaling when they differ. Phase D base `c9176e9439`.

## 2026-11-18T10:34:02+01:00 · N16 · C3 and reference prefix

Which accepted repair fixture and integer comparison should be used? The explicitly parameterized incomplete or intentionally invalid fixtures cannot prove final acceptance. Use the existing successful concurrent repair fixture with the construction width made explicit and a wide repeat at ranks 2 and 4. For the prefix use the established numeric-field convention, requiring modeled/work/count identity and validating measured RSS separately. Literal all-token comparison would make two accepted, already identical workflows fail on measured RSS. Phase D base `c9176e9439`.

## 2026-11-18T10:34:02+01:00 · N16 · B report fields

Should the maximum-choice report be derived from window totals or measured per round? The totals do not recover the maximum. Append the actual count to the timings-only private-work row at its existing output site, with no additional scope. Phase D base `c9176e9439`.

## 2026-11-18T14:20:45+01:00 · N6 · Remesh

Where should the moved reporting clock end, and which function name should moved failures report? The alternatives were the new helper names and clocking after report-buffer teardown, or keeping the public Remesh identity and the previous lifetime boundary. Used the public caller value and read report time before its summary buffers unwind. Base `3276cbb862`.

## 2026-11-18T14:29:01+01:00 · N6 · MeshOptimizer

Should the interior and boundary ring operators each keep a copy of triangulation scoring and traversal? Used two shared named helpers for the source-identical scoring and traversal loops. The existing memo, retry and diagonal certification state remains at each call site, with separate interface and boundary commit rules. Base `3276cbb862`.

## 2026-11-18T14:29:01+01:00 · N6 · Selection

Should census and queue state have separate lifetimes or remain in one Selection owner? Used two value members within the existing owner, preserving the existing references, the mixed operations and the explicit swap and reset lists. Base `3276cbb862`.

## 2026-11-18T15:48+01:00 · N6 · Optimizer GQ evidence storage

Which local data should release disk space for the existing 2 GB preflight? The choices were moving active case material, turning down the guard, or archiving earlier locally copied binaries. Archive the terminal phase C and D backup executables losslessly in the workspace, verify each original SHA256 through decompression, and retain exact raw access at the original paths through a verified copy on the root filesystem. The active case material and the guard stay in use. Base commit `4946b2d126`.

## 2026-11-18T18:26:12+01:00 · N18 · M28 report

Should a quality migration mean the evaluation requested one or the ownership change committed? Count `quality=1 committed=1` for migrations and `quality=1 evaluate=1` for evaluations. A requested migration can fail before the ownership change. Should the last stuck row include a cell from an older census when a rank's last census has none? Use the last actual complete quality census in the rank log, even when it has no stuck lines. Search earlier prints for the same cell ID across ranks in that event to cover migration, using round and census serial to order them. Report any limit or interruption visible in the last census alongside the raw lines. N18 base `7d8f538c00`.

## 2026-11-18T18:25+01:00 · N18 · E1 and E2 binding

Which live feature and print guard supply M28 after N6? The alternatives were the slot's observer feature or the round choice copied into the proposal, and the census call alone or a separate timings guard. Used the round choice and an explicit timings guard after the existing printer. The census can run without timings, and proposal routing reads the copied choice. Base `7d8f538c00`.

## 2026-11-18T18:25+01:00 · N18 · E3 active candidates

Do oversized choices still occupying an index count as reservation losers? Used the live choice and valid dependency conditions, which identify the candidates eligible for the reservation. The real conflict counter already excludes oversized refusals. Applied the specified cell-overlap rule even though the real reservation can also conflict through shared vertices. Base `7d8f538c00`.

## 2026-11-18T18:25+01:00 · N18 · F purpose

The written rounded sum ceiling is 3,288.032 s, and multiplying the two printed T0 times by 0.90 gives 3,288.0321 s. Used the explicit confirmed ceiling 3,288.032 s and the two printed individual ceilings. Base `7d8f538c00`.

- 2026-11-18T21:51+01:00, N19 A, exit hook. Should M29 run at the outer public completion flag or the final worker exit? Used final worker rejection after the complete exit census, because the outer finalizer no longer owns the census, and restricted the hook to the worker call used by remesh events. Commit reference: N19 A checkpoint (recorded in progress).
- 2026-11-18T21:51+01:00, N19 A, edge order. Should lengths follow the canonical global-ID map order or a fixed local tetrahedron order? Used local vertex pairs 01,02,03,12,13,23 and looked up the canonical key per pair, because that is the engine's fixed enumeration. Commit reference: N19 A checkpoint (recorded in progress).
- 2026-11-18T21:51+01:00, N19 A, mobility. Should the cross-rank flags describe local face incidence or all constraints? Used global physical-face and requested-height apex incidence for unconstrained motion, including the IDs in the second plain exchange, so the same vertex has the same flag including at the outer fringe. Commit reference: N19 A checkpoint (recorded in progress).
- 2026-11-18T22:49+01:00, N19 A, print-guard fixture reading. Should the one timed, intentionally rejected G1 remesh event suppress its empty M29 pair because the text describes all gates as complete, or follow the printed event result? Used `complete=0` as the operative condition and corrected the checker to correlate the pair and refusal. Commit reference: N19 A checkpoint (recorded in progress).

N19 A commit reference for its implementation questions: `c6b42a748f91dfcb7331a967fb89e93b6871f002`.

- 2026-11-19T00:44:17+01:00, N20 A1, constraint projection. Should the search return the projected champion or the raw point that produces it? Use the raw champion as the placed target, so the existing plane/constraint sequence reproduces the already measured point without adding a second effective projection. Commit reference: N20 A checkpoint (recorded in progress).
- 2026-11-19T00:44:17+01:00, N20 A2, overlapping stuck-cell features. Reopen while scanning or mark all requested vertices first? Mark first, since reopening a common vertex would hide a later stuck cell from the shared M28 predicate. Commit reference: N20 A checkpoint (recorded in progress).
- 2026-11-19T00:44:17+01:00, N20 A3, priority. Should the fine request override metric cleanup or the independent observation-window stop? Keep the written insertion point and both stop conditions. The N18 post-cleanup quality entry resets the window start at the known stall. Commit reference: N20 A checkpoint (recorded in progress).
- 2026-11-19T00:44:17+01:00, N20 A4/W, fine acceptance receipt. Should accepted identify private validation or eventual publication? Use the existing private validation flag and report the publication evidence separately, because the requested print point precedes contact and commit. Commit reference: N20 A checkpoint (recorded in progress).

N20 A implementation question commit: `01d2bbd24f2504a35406ebc529978c97af29c647`.

- 2026-11-19T07:45:51+01:00 — N21 P1/P2, root count. Should the kept count be
  returned by the backend beside its certificate, or derived from the entity's
  reference count? Used the latter. Entity and backend membership agree and
  the modulo count leaves the cache result and caller-byte charges unchanged.
  Commit: N21 P source checkpoint, recorded in the N21 report.
- 2026-11-19T07:45:51+01:00 — N21 P/P5.7, restart. Does the Part P run require the
  separate isolated event-two restart used by some prior G5 campaigns?
  Used the single two-event run for P under I8T and C §2. The isolated restart
  is explicitly required and executed under W §6. Commit: N21 P qualification
  checkpoint, recorded in the N21 report.

- 2026-11-19T08:12:09+01:00 — N21 P3/T4, fixture access. Should the extracted join be
  declared in a new private engine header or declared directly in the single
  test translation unit? Used a declaration in the test so T4 calls the same
  real World function without adding a header or extending the public engine
  API. Commit: N21 P source checkpoint, recorded in the N21 report.
- 2026-11-19T08:12:09+01:00 — N21 P4, diagnostic stream. Should proof rows go on the
  timer ledger's stdout or rank-local stderr? Used stderr to keep the new
  diagnostic family out of the old timer parsing path. The proof collector and
  report bind stderr and stdout to the same rank and event. Commit: N21 P
  source checkpoint, recorded in the N21 report.

- 2026-11-19T10:00:46+01:00 — N21 P1–P5.7 commit reference for the root-count,
  restart, fixture-access and diagnostic-stream entries above: `08a8d7c7ff`.

- 2026-11-19T13:37:52+01:00 — N22 M, ownership timing statistic. Should the last
  world.seconds read stay inside ShouldMigrate for the declined-exchange
  statistic, or move to its caller? Used the caller so the decision function
  meets the literal clock-free source check. The statistic is unchanged in
  purpose, and only its call boundary moves. Commit: N22 M checkpoint.

- 2026-11-19T14:45:10+01:00 — N22 C, purpose reference. Does the ordered N21
  pair apply throughout both parts, or is it the entry reference that §5.1.7
  advances after Part M? Used the ordered pair for M and the qualified M clocks
  for C. The pinned text names M as C's reference explicitly, and the latest
  order changes C's disposition for a purpose-only miss. Commit: N22 report.

- 2026-11-19T15:53:22+01:00 — N22 M, checkpoint association for the ownership
  timing statistic entry above. The immediate caller retains the statistic
  and the decision routine reads no clock. Commit: `484f29315f`.

- 2026-11-19T17:50:00+01:00 — FP0, source. Should the experiment use the checkout
  after N22 or the accepted source when FP0 was ordered? Used the latter on an
  isolated view of N21 `4fe083d460`, as the pinned measurement text specifies.
  The test-only patch applies to the current branch. The existing N22 solver
  remains active. Commit: FP0 checkpoint.

- 2026-11-19T17:54:00+01:00 — FP0 A7/A8, candidate scope. Is an edge interior when
  neither endpoint is on a physical face, or when the edge itself is absent
  from the skin? Used two interior endpoints to preserve the marked surface.
  A8 tries the lower endpoint first and defers conflicting endpoint stars
  within the one pass so the incidence used by each collapse remains current.
  Commit: FP0 checkpoint.

- 2026-11-19T17:54:00+01:00 — FP0 A9, private options and field. Should the
  region diagnostic retain the 160-trial production cap or scale by region?
  Used production cap, operator switches, cache and validity-only admission,
  with the ordered 900 s wall bound. Used indexed frozen P1 samples and the
  endpoint-linear edge integral. The production cavity's current-nodal scan
  assumes small donor sets. Both ball centres use the vertex with most A5
  q/J failures, with vertex ID breaking ties. Commit: FP0 checkpoint.

- 2026-11-19T17:54:00+01:00 — FP0 A11, allowance. Should strict single-rank
  proof use an unlimited diagnostic or the event-model budget? Used the
  12,288 MiB native ceiling, 5,400 s event limit and production work formulas
  for the pinned cell count. The reverse pass and full proof share the
  allowance and the reverse bounds. Commit: FP0 checkpoint.

- 2026-11-19T17:54:00+01:00 — FP0 A12, stored anchor. How should a cubic be
  evaluated for a realized face whose corners lack a common chart on its
  stored anchor? Used forward parameters where available. For other faces,
  projected each realized sample onto the flat stored anchor for barycentrics,
  then evaluated that anchor's cubic. The report gives each population and
  its distances separately. Commit: FP0 checkpoint.

- 2026-11-19T17:54:00+01:00 — FP0 B, exit target and options. Should the donor
  argument use the archived input donors or a snapshot of the exit tensors?
  Used the exit tensors to preserve the requested target. The input donor
  capture remains provenance. Used accepted N21 width and the saved active
  engine time/work limits, so the replay can differ from Part P's default
  width comparator. Commit: FP0 checkpoint.

- 2026-11-19 21:02 Europe/Rome, FP1 A, split sweep (base `a59380df18`). Which neighborhood inherits the stamp deferral? The collapse rule refers to stars of ends that can be removed, and a split removes neither end. The choices were the split ring alone or the stars of both eligible split endpoints. Used both endpoint stars because section 7 says to carry over the same stamp rule and checks for earlier edits around the ends.

- 2026-11-19 21:40 Europe/Rome, FP1 C, G2 OFF reference (at `68d28afad0`). Which construction communication count should qualify OFF? N14 reports 340,843 and accepted N22 C reports 333,799 per rank, with Work order 3 section 2a naming only preparation for the substitution. The options were the explicit N14 construction comparison or extending the preparation exception to construction. Used the explicit comparison because R1 holds the gate fixed and OFF has no production core statement. The numeric discrepancy is in the FP1 report.

- 2026-11-20T10:05:08+01:00, FP1 resume 2 §2 / §5.1, base `931b306263`. How should build/write timing and completed rank rows be collected when the NRR scope names only PrintLocalPass? Options were to omit those observations or add report-only bookkeeping at the existing RunLocalCore build, write and print boundaries. Used the latter under R4 because those boundaries own the two measured operations and the existing vote. The operation and failure order are preserved.

- 2026-11-20T12:22:54+01:00, FP1 H2 and H5, base `9940a85314`. The exact refusal is cacheable, while F2 separately forbids caching an entry with an alternative capacity refusal. Used the existing capacity exception to retain the dependency on the live-cell count. For candidate invalidation, include the replacement plan's old-only surviving vertices as well as fresh vertices because both receive the epoch in the existing Commit.

- 2026-11-20T14:21:42+01:00, FP1 H6 R4. Does the engine reference import aggregate progress as well as the frontier test? Options were the general engine admission expression and the explicit one-edge inequality in Resume 4 §1. Used the explicit inequality because the order restricts this split trial to its one removed edge. Base 1391148436.

- 2026-11-20T15:03:32+01:00, FP1 H6 replay. Which observed row anchors the 600 s clock? Options were the initial partition rows and the first completed sweep. Used the latest observation of the six initial partition rows because the core follows the partition directly and the first sweep omits its setup. The watchdog also records first-sweep onset estimates and its one-second polling cadence. Base c9ba8b3cb5.

### 2026-11-21T07:45:49+01:00 — FP2-0 G2 failure locations

Question: how should trailing-edge and tip failures be counted when the order specifies locations but gives no predicate? Options: omit attribution, or use the frozen authority geometry to define reproducible predicates. Used the latter. Trailing edge uses sheet 27868 and the shared physical feature chain 908; the tip uses the outer-span station 1.196299999999894 from facet 27868. Counts may overlap. Base commit f2d4dfda7d.

### 2026-11-21T09:52:00+01:00 — FP2-0b split refusal scope and proof timing

Does §1.4's later-pass exclusion reset between L and D? The options were a reset at the step boundary or a run-wide exclusion. Used one run-wide set because §1.5 excludes refused edges from marking in either phase of later scans.

Can P4 seconds be attributed to each entity while retaining the ordered public calls? The choices were aggregate clocks with per-entity bounds or independent timed proofs that change the workload. Used the aggregate clocks and marked per-entity seconds unavailable because the public calls return bounds but expose no per-entity time. Base `0167ad274f`.

### 2026-11-21 — FP2-A R4 fixture access and affine components

For the one-parent test, the options were a test friend across the existing private boundary or widening the production surface split API. Used the fixture friend to call the same Prepare implementation without relaxing the two-parent ring contract.

The FP2-0b float walk directly indexes the cubic net, while supported entity states may have disconnected flat components. The choices were a flat projection over the existing facet vertices or treating a missing net as a failed walk. Used the same affine triangle as the authority for the flat branch, leaving curved iteration and applicability as specified. Base `abc9450b86`.

### 2026-11-21T14:06:00+01:00 — FP2-A Correction 1 qualification scope

Does the instruction to repeat Part B on the corrected binary apply to the superseded full-volume lab attempt as well as the selected §10.7 alternative? The choices were to rerun the timed-out full-volume lab or to re-execute the required MPI reconstruction gate on the permitted two-cube case. Used the two-cube case for the gate because it is the selected §10.7 fixture and verifies the required owned-face checks and exact snapshot agreement at both rank counts. The full-volume attempt is preserved as a timeout. Base `f7ec841a9a`.

The corrected Part A and Part B test campaigns have one exact pinned source manifest and binary. Reuse the freshly repeated FP1/G1 results from that binary for the identical inherited gates and run both Part B MPI cases separately. The alternative was duplicate execution of the same binary and selectors. The pre-correction receipts are historical for this qualification.

- 2026-11-21 16:40 CET, FP2-A Correction 3, step D. Does a feature edge with an unknown chord count as a candidate when an above-limit face selects it as its longest edge? The options were to exclude it everywhere under the old feature-chord wording or exclude it from chord marking only. The latter follows the new item 2, which selects the longest edge of any kind and lists its three tests. An above-limit feature chord blocked by the length floor retains priority and supplies reason `short`. Base `e056028b0a`.
- 2026-11-21 16:40 CET, FP2-A Correction 3, sweep output and build626 OFF. The existing sink printer formats only volume refusal fields. The options were to omit the new detailed fields from solver sweep rows or to append them at that printer for surface rows. Use the second option to record all requested reasons via the existing sink. Its call site in `NativeRemeshRun.inc` is outside the allowed old-OFF diff, so the old run remains historical and OFF repeats on the corrected build. Base `e056028b0a`.

## 2026-11-22 · FP2-B · A1 source check

Should the V sweep comparator and validation follow only the prose keys and successive checks, or B0's full tuple and per-cell early checks? Used the B0 tuple with slot as the final tie and kept the interleaved checks, as ordered for exact Gate E. Base commit `bb352600c5`.

## 2026-11-22 · FP2-B · default proof source check

Does the reference to FP2-A 11.2 also repeat the old LAB-S and rank check after S1? Used the FP2-B §5 replacements and the same-binary scoped fixture and G1 receipts. This keeps the original default prefix and workflow checks and follows §5's explicit replacement. Base commit `bb352600c5`.

## 2026-11-22 · FP2-B correction 1 · R4

Does A2's exactly-two shortcut or the correction's ordered first nonmatching face govern the path, and how should the diagnosis cover more than 20 short edges? Used the explicit corrected traversal and a separate diagnostic census alongside the original 20 shortest rows. Base commit `457822008c`.

## 2026-11-22T08:32:38+01:00 · FP2-B D1 · R4

How should the report cover all edges with five ranked rows, and which label applies to the closing commit? Used the exact bins and visible pairs with omitted counts. Used the temporary diagnosis label for the code checkpoint and the explicit section 6 label for the restoration. Base `240ca96db8`.

## 2026-11-22 10:15 CET, FP2-B correction 2, source check

- Which census anchors the exact shifted edge comparison? P1 is measured by the core and P4 by the engine, with a last-digit difference in the rank-2 event-1 minimum. Use P1 for exact core equality and report P4 beside it. See `6415b6dab9`.
- Does the event-2 lab rebuild the loaded geometry? Its v9 sidecar already has the configuration's prepared mobility and geometry tolerance. Use the loaded state after verifying those values, as production `PrepareMobility` does. See `6415b6dab9`.
- What index and batch size belong in an orientation exception before the donor vector is filled? Use the count of successful predecessors and size of the input cell batch. See `6415b6dab9`.

## 2026-11-22 10:32 CET, FP2-B correction 2, baseline comparison

- Do unlisted census values disqualify N6P? Use the exact minimum and IDs and the named short bins from §5.2. Carry the additional values and their differences as measurements. The D1-source baseline is based on `6415b6dab9`.

## 2026-11-22 13:34 CET, FP2-B correction 3, source check

- How can the pre-edit fixture identify converged placements without a walk field? Build638 refuses every unconverged sheet walk before it can return Accepted. Use the first three accepted nonfeature edges as the converged pins. See `4ba64e9f8b`.
- Can the before-V class history and construction geometry bins be reproduced from the build638 logs? Those logs have before-V totals and the construction geometry count. Mark their missing breakdowns unavailable. In the corrected lab use the existing quality-sweep sink for the before-V classes, and in S1 use the existing geometry-repair entry histogram. See `4ba64e9f8b`.

## 2026-11-22 19:50 CET · FX2 · R4

- How should F1 report the reservation owned by FromPayload? Considered exposing it through a production test hook or recording only observable public results. Used explicit unavailable fields for the internal reservation, with exact bytes, exception and timer-call comparison. Base c843610249.
- Are F5's integer RSS samples part of the identity gate? Considered comparing sampled RSS exactly or reporting it beside the exact modeled bytes and logical fields. Used the observation reading from the order's full state-row rule. Base c843610249.

## 2026-11-22 22:57 CET · FP2-C1 · R4

- The pass-1 sweep equality rule includes rows whose printer adds an elapsed-seconds field. Used exact comparison of all non-clock fields, applying the timing exclusion of the neighboring pass-1 surface rule. S0 `7e6953e721`.
- The earlier correction-3 controller also has an above-tolerance count alarm and a post-shift edge census. Used the C1 §4 list as the gate and retain the older measurements alongside it. S0 `7e6953e721`.
- The engine emits the collapse elapsed clock from entry and prints rounds at the boundary. Used that elapsed clock with setup included, since a separate exclusive collapse clock is unavailable. S0 `7e6953e721`.

## 2026-11-23 01:33 CET · FP2-C2 step 0 · probe timing

- The inherited `FP2_B pass` row uses the post-writeback exact cell count. Considered moving that row or leaving it at its established boundary. Kept it in place and ran the dry probe after the pre-writeback rows, excluding the probe interval from `seconds_pass`. C1 base `4730bd59ec`.
- The strict-query order spans two captured passes on a production-partition rank. Used the same ascending metric length and ID-pair order across those records, preserving that order within each pass. The one-in-ten sample ordinal is pass-local. C1 base `4730bd59ec`.
