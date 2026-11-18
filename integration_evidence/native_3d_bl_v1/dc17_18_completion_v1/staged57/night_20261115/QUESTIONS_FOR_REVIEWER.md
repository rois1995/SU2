
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
