# Rejection dependencies — connected experiment, not yet qualified

Base d4d531c0f6e20516178be98c7f954f715b2de075. Full Goal3 active/incomplete.
Queue insertions during progressive measurement currently ignore recorded rejected
directions; a rejection arriving before local edge measurement is discarded. Fix
both in existing blocked-edge bookkeeping, retaining changed-star reactivation and
pre-admitting new blocker storage. No target, geometry, profile, caps or mesh gates
change. Exact saved320-cell control must exercise skipped queue insertions; then
use the saved difficult N4M2 repeat and meaningful MPI/current-BDF compatibility.
No accepted repeat or speedup claim before complete validation. Preserve failures.

First build PASS. Initial serial check stops at existing immutable-corner timing
assertion: other_seconds=-4.27e-5 because query-reuse support imports nested in
seed/update were also subtracted as exclusive donor time. This is a real reporting
bug, not mesh failure or a relaxed assertion. Preserve controls/serial and build.
Connected correction gives the existing RAII timer an optional reported parent;
the nested import is charged once to donor and excluded from seed/update. V2 source
pins/build log and controls_v2 preserve both stages. No mesh algorithm change.

V2 timing invariant passes. The broader legacy control then stops at an obsolete
exact string: it expects "movement proposal leaves an incident immutable facet"
but the existing geometric rejection is "movement leaves incident reference facet 0".
The debugger receipts preserve the same incident-reference failure, not a source
coverage error. Update only that diagnostic assertion and add INFO; all rejected-
input, invalid-cache, filter, timing and mesh assertions remain. V3 rebuild/test
receipts are separate. The first optimized debugger breakpoint mapped to test entry;
only diagnostic_finish_v2 establishes the actual last local rejection.
