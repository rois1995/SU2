# Read-only Opus5.5 medium controller review

Advisory review of the supplied diff only, tools disabled. It is not validation.

# Review: coupled angular/radial queues in `CBoundaryLayerMetric.cpp`

Findings are ordered by severity. Things I checked and found correct are listed briefly at the end.

## 1. The pre-existing `rule` content is no longer counted in the budget or the tolerance

The old code set `retained=rule.size()` and added `Σ w·probe` over the existing `rule` points into `totalValue`. The new code sets `retained=0` and drops that sum, but still appends to `rule` at the end.

If `rule` can be non-empty when this code is entered (the old initialisation suggests it can):
- **Budget:** the returned rule can exceed the 65536-point limit by `rule.size()`.
- **Tolerance:** the denominators of `1e-4*totalValue` and `1e-5*(totalValue+totalComparison)` now cover only the boundary-layer part. That tightens the tolerance, which changes the work and failure behaviour on the saved fields.
- **Validity check:** `!(totalValue>0)` can now throw on cells that previously passed because the existing points contributed positive value.

**Fix:** keep `retained=rule.size()` and carry the existing rule value as a constant offset in `totalValue`. Alternatively, assert `rule.empty()` if it is always empty here.

## 2. Non-finite values can exit the loop silently and corrupt the heaps

The finiteness check runs only after the initial `add` calls. Any NaN produced by later refinement points propagates into `panel.value` or `angularError`, and then:
- every `>` comparison in the `while` condition is false, so the loop exits "converged" with garbage totals;
- NaN keys in `angularPriority` and `radialPriority` violate strict weak ordering, which is undefined behaviour in `std::priority_queue`.

The old code had the same exit, but each panel's errors now pass through two heaps on every `publish`, so the exposure is larger.

**Fix:** in `publish`, throw unless `panel.value`, `panel.comparison`, `angularError` and `radialError` are all finite.

## 3. Radial work is discarded on angular splits and competes for the shared work budget

The radial branch always runs first whenever the radial criterion fails, including on panels that are `unresolved` or angularly dominant and will be split next. On a split:
- `panel.rays={}` throws away all of the parent's radial refinement;
- both children restart from the depth-0 bands, so the cell's radial error can jump back above tolerance after each angular step.

Each radial bisection increments the same `visited<=16384` counter as angular additions. This pattern is a likely route to the work-budget failures on fields 88/157/329.

**Fix:** do one of the following:
- skip radial refinement on panels with `unresolved` set;
- choose the branch by comparing estimated reductions, e.g. prefer angular when `totalAngularError > totalRadialError` or the top angular panel is unresolved;
- only enforce the radial criterion once the angular criterion and `unresolved==0` are met.

## 4. Memory is no longer bounded by the point budget

Every live panel now keeps, for all 19 rays:
- `points` and `comparisonPoints` for every interval that has not been upgraded;
- all points of the 12 comparison rays.

Each of these carries its own `SamplePoints` payload. The old code discarded the coarse points after each `add`.

The budget checks do not cover this storage:
- `retained` counts only `points` of rays 0–6;
- `comparisonCount` is checked per panel, not globally;
- `comparisonPoints` are never counted.

Peak memory is therefore several times the fine budget in typical cases. In the worst case it is about live panels × 65536.

**Fix:**
- For comparison rays, store only `value`, `comparison` and a per-interval wall-hit count (needed for the `ray.sampled` decrement), and drop the point vectors.
- For fine rays, consider recomputing the 5-point set on upgrade instead of storing it.
- Alternatively, add a global budget that counts all stored points.

## 5. Vertex values lose their per-ray accuracy control (check the consumers)

`vertexValues[k]` comes from `rays[k+4].value`. Under an aggregate cell budget, a single small ray can carry a radial error that is large relative to itself; per-ray accuracy was the old behaviour that was removed. If `vertexValues` drives the split-edge selection (in the elided lines between the hunks) or anything downstream, it now has no relative accuracy control.

**Fix:** confirm that its consumers only need it as a heuristic. If they need accuracy, keep a loose per-ray floor for rays 4–6.

## 6. Minor robustness points

- **Empty ray queue:** `panel.rays[k].priority.top()` is undefined behaviour if a ray has no intervals. This is safe only if `EdgeBreakpoints` always returns at least two cuts. Add an assert or guard.
- **Drift in running totals:** `totalAngularError` and `totalRadialError` are maintained by repeated add, subtract and clamp-at-zero. Near the tolerance, drift can leave the loop selecting panels whose current radial error is zero. Each such iteration bisects a zero-error interval, which burns one unit of the work budget or ends at the depth-32 throw. A cheap safeguard is to recompute the sums exactly from `panels` when the loop is about to continue on a panel with zero error, or before terminating.

## Estimates vs. bounds

The only rigorous properties in this change are structural:
- **Positivity:** all returned weights are positive. Gauss weights, `faceWeights`, `length>0`, `chart.area*fraction` and `(1-t)²>=0` at interior nodes are all non-negative, and only rays 0–6 are returned.
- **Point budget:** the returned rule respects 65536 points, subject to finding 1.
- **Hidden-wall guard:** this holds only to the extent that `MayIntersect*Band` is conservative.

Everything else is an indicator, not a bound:
- `|Q5−Q3|` and `|Q7−Q5|` are attributed to the lower-order value. This is a pessimistic heuristic, but both rules can miss a feature jump inside an interval.
- `|fine7 − comparison12|` is an angular indicator only. It also mixes in radial error, so adding the radial sums double-counts some error. That is conservative as an indicator but still not a bound.
- The integrand `sqrt(det(intersection))` is only piecewise smooth, so none of the Gauss-difference estimates have guaranteed asymptotics.

The function and its error messages should keep calling these "estimated", as they do now.

## Checked and correct

- **Stale-entry handling:** revision matching in `select` plus `find` on erased ids works, and ids are monotonic.
- **Wall-sample counting:** the counts are symmetric across upgrade and bisection.
- **Guard rerun:** the geometry guard reruns on any later `publish` where a support wall becomes unsampled.
- **Budget timing:** `publish` checks the budget after `removeTotals`.
- **Exception safety:** no partial rule is returned on a throw.
