
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
