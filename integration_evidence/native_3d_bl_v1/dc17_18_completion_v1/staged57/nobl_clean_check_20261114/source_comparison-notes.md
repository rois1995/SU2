# Build 293/build 494 upper-bound source comparison

Exact source contents match both pin manifests; matching commits identify file contents, not build IDs. Primary interpretation supersedes the initial ordering preserved in commit 31680e8469.

1. `76f10c8310`: `EntityFaceError` → `BoundPhysicalSurface` → `QuerySurfaceUnion*` requests ErrorMagnitude above tolerance and refines the covering upper/lower gap. This is the leading candidate for more queries and work per query.
2. `35e173b043`: introduced the follow-up certificate query relative to build 293; this ancestor was later modified by `76f10c8310`, so it is not an independent additive cause.
3. `54d2ca2bd4`: adds diameter/interval arithmetic and sampled-distance Lipschitz tightening. This adds work per node but may reduce refinement; its net cost direction is unestablished.

First-collapse rank 3: 2,794 → 4,674 calls, 4,555,668 → 830,252,222 logged work units, 28.65 → 934.26 seconds, with 263 accepted collapses in both. Aggregate counters do not identify which query or commit caused this growth. No causal tests or fixes were performed.

The load threshold below 1.0 was explicitly waived after 24.5 minutes of rechecks; launch load was 2.72 and the CPU-use gate passed. No strict quiet-host claim is made. The running job was not changed by this source comparison.
