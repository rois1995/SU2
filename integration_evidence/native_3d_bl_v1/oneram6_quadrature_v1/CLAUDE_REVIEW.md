## Native3D thin-BL column integration: read-only review

**Status first.** The session notes say the column design hasn't been tested, but the evidence folder shows a finished run. `columns_validation.log` exits 0 with "Passed all 7 test cases". The replay reports `complete=1 points=1419 normal_integral=6.22412553e-5 seconds=2.92`, and `normal_integral_check.json` shows `passed: true` with relative error **1.593e-4**. `columns_provenance.json` still says `COMPILED_VALIDATION_RUNNING`. I couldn't check hashes, so I can't confirm these files came from the current uncommitted source. The findings below assume they did.

**Overall:** I found no Jacobian, volume or orientation bug in the new face-to-apex column code (`CBoundaryLayerMetric.cpp:1761-1974`). The (1−t)² factor, the 3V scale, the radial measure and the midpoint split that halves the fraction all check out. The real problems are that the error estimate can falsely accept, the convergence audit doesn't actually vary anything, the reference can't settle the question, and the whole-mesh cost.

### Findings, ranked

**1. The error estimate under-reports on the target cell itself. (Evidence; very likely real, pending finding 3.)**
- **Code:** a cell is accepted once the summed estimate is ≤ 1e-4 × total (`CBoundaryLayerMetric.cpp:1941`). Each panel's estimate is the gap between the retained 7-point rule and the 12-point comparison rule, plus the radial gaps (`:1917`).
- **Evidence:** the replay's callback (`BoundaryLayerMetric_tests.cpp:1563-1566`) is the same normal density the reference uses (`check_normal_integral.py:36-38`; pointwise agreement is asserted at :46-47). So the integrator claimed ≤ 1e-4 on exactly that quantity, but the independent error is 1.593e-4. It only passes because the gate is 5e-4, five times looser than the integrator's own target.
- **Why:** the rule kept only 28 rays, i.e. 4 angular panels. All 28 rays start at the same first-sample barycentric b3 = 5.018e-5 in `replay_rule_150.csv`. Two neighbouring-degree rules (degree 5 vs degree 6) agreeing is not a bound for a non-polynomial angular integrand. On top of that, the lower-order rule is the one kept: 7-point angular at `:1915`, and 3-point (or 5-point) radial at `:1876`/`:1882`.
- **Next action:** have the replay print the final estimated error. Gate on "independent error ≤ reported estimate", not only on 5e-4.

**2. The band-ratio convergence audit doesn't change anything. (Bug, certain; it predates this change.)**
- **Code:** every call clamps the ratio to at least 2 (`:1346`, `:1602`, `:1846`; baseline had the same at `before_…cpp:1378/1633/1833`). So 1.5 and 1.25 both become 2.
- **Evidence:** the two replay runs produce the same rule file hash `c756c8f6…` (`normal_integral_check.json:10,18`), the same point count and the same integral. The production value 1.5 (`CSolver.cpp:3109`) is also really 2.
- **Consequences:**
  - The ratio loops in the tests at `BoundaryLayerMetric_tests.cpp:1005`, `:1083` and `:1556` run the same rule twice.
  - The ratio-agreement check at `:1095` (concave "far vertex" case) can never fail.
  - The script's `len(native)==2` requirement counts one result twice.
- There is no other convergence knob on the native side: the 1e-4 and 1e-5 tolerances are hard-coded.

**3. The independent reference only shows angular convergence, and its radial structure is shared with the native rule. (Certain.)**
- **Only angular order varies:** 32/48/72 (`check_normal_integral.py:65`). The radial rule is fixed: cuts at h0·1.5^k/height with 10-point Gauss per band (`:56-62`).
- **Misplaced cuts:** `height` is the apex's nearest distance, 0.0024513 to face 37327 (`independent_distance_probes.json`, mask 8). Near the base the actual distance is d = 0.0022459·t; the replay CSV gives d/t = 1.127e-7 / 5.018e-5. So the density kink at d = 0.5·h0 sits at t ≈ 4.45e-4, inside the reference's second interval (cut at 4.08e-4), not on a cut. My rough estimate of the resulting error is ~1e-5. That probably doesn't explain the 5.9e-5 excess, but it is unmeasured.
- **Same chart:** the base face is the wall face 37760 (mask 7) and the apex is 31725 in both codes. The comment at `:55` ("Different coordinates from native clipping") is no longer true. Anything specific to the face-to-apex chart is common to both.
- **Next action:** add a radial-refinement sequence to the reference, and compute it with each of the four vertices as apex. The integral can't depend on that choice, and wall-touching apexes change the singularity structure completely. This settles finding 1 at no source cost.

**4. Whole-mesh cost and memory. (Measured on one cell, extrapolated.)**
- **Measured:** cell13 takes 2.9 s with 20,121 density calls for 1,419 points (debugoptimized build).
- **No reuse across trials:** with a callback, visible rules are always marked sensor-dependent (`:1422`). The cache key — the four nodal tensors plus the frozen core (`CSolver.cpp:3101-3106`) — changes on every scale trial, so every rule is rebuilt every trial. Baseline kept geometry-only visible rules (`before_…cpp:1331,1453`).
- **Cache key itself is fine:** it captures every input of the density callback. I found no stale-reuse bug.
- **Rough extrapolation:** 38,800 wall faces means at least ~38,800 wall-adjacent cells. If cell13 is typical, that's ~31 CPU-hours per complexity trial. Storage is ~350 B per point, so ~0.5 MB per cell and ~20 GB in total, against the 2 GiB per-rank cap (`CSolver.cpp:3115`).
- **Every cell goes through this path:** all owned cells are queued (`CSolver.cpp:2904`). Cells with no wall support still run the 19-ray adaptive path with a nearest-face tree query per sample.
- **Likely hotspot (unprofiled):** `EdgeBreakpoints` is now called per ray per panel evaluation. Each call loops over every face in the chord box padded by the thickness, for every level and feature (`:2121-2158`). Each candidate root then costs a nearest-face tree query (`:1997`, `:2028-2038`).

**5. The evidence covers one easy cell category. (Certain.)**
Cell13's base is a wall face, the neighbouring face bends away (convex), and distance is linear in t from the base on every ray. That is the best case for this chart. None of the hard categories are tested against a reference:
- **Wall edge or vertex on the base:** the angular integrand has log singularities. Depth 32 and the 16,384-step work budget are at risk; this hasn't been measured.
- **Two wall faces or all four vertices on the wall** (trailing edge, tip): base selection ties and falls back to vertex 0 by index (`:1764-1772`).
- **Cells inside the band with no wall vertex.**
- **Concave nearest-face switches:** the only test is `:1075`, and its check is vacuous (finding 2).
- **The full runtime tensor density with the real frozen sensor:** the replay controls only the normal density.

Volume and moment checks (`:1592-1596`) are exact by construction for this rule, so they confirm the Jacobian code, not integration adequacy.

**6. Changes of nearest feature aren't cut, and the hidden-support safeguard never fires on M6. (Code fact; impact speculative.)**
- `EdgeBreakpoints` cuts only at band levels. Switches between face, edge and vertex regions are left to the 3/5/7-point radial adaptivity and the angular two-rule comparison. That's harmless for convex walls (distance is smooth enough), but at concave edges the normal and tangent frame jump. The sensor-intersected density is then discontinuous, and two rules agreeing proves little.
- The unresolved-support check is per wall (`:1919-1920`). With M6's single wall, any sample with positive weight disables it for that panel.

**7. Minor.**
- The volume-proportional tolerance allowance uses the running total (`:1871`, `:1879`). It is zero in the first panel and shrinks when a parent panel is removed (`:1947`), so tolerance depends on processing order and the first panel is over-strict.
- Comparison-rule rays share the work budget (`:1885`).
- The header comment (`CBoundaryLayerMetric.hpp:211`) still describes distance-band clipping.

### Is the strategy adequate? The smallest complete fix

The face-to-apex column design is a sound choice for wall-adjacent cells, and nothing above shows it is wrong. It is incomplete in three connected ways, and no new fallback is needed:

1. **Conservative acceptance.** Keep the stronger rule that is already evaluated: the 12-ray angular rule (positive weights, degree 6) and the higher accepted radial order. The existing gap then over-estimates the kept rule's error, at no extra density calls (storage grows roughly 1.7–2.8×). Also return or print the estimated error.
2. **Reuse geometry across trials.** Memoise ray breakpoints and point samples per cell, keyed by exact ray and point barycentrics, so each trial only redoes the adaptive decisions and density calls. The output is bit-identical to a rebuild. Profile first to confirm `EdgeBreakpoints` is the hotspot.
3. **A real convergence knob.** Make the audit ratio or tolerance genuinely tightenable for tests.

**Decisive check on the difficult geometry.** Do one full-mesh quadrature pass over all owned M6 coarse cells:
- Use the actual frozen sensor and core from `oneram6_frozen_v1` at one fixed scale. No remeshing, no CFD.
- Record, per cell: points, callbacks, time, estimated error, and any failure. Also record the total trial time and peak storage per rank.
- Take every failure plus the worst few cells in each category from finding 5. For those, compute an independent reference with radial refinement and all four apex choices, and check that the independent error ≤ the reported estimate ≤ 1e-4.

This tests budgets, cost and estimator honesty together, which more single-cell checks cannot.

### Limits of this review
- Static reading only; I ran nothing and verified no hashes, so the evidence files may not match the current source.
- I didn't diff the shared physics functions against baseline beyond the ratio clamp.
- I didn't read the parallel-chord or cylinder hidden-support paths in depth, the spherical path, or the remesher's query side.
- The cost, memory and reference-error figures are estimates from one cell.
- The concave and edge-singularity risks are reasoned, not shown.
- Plan mode asked for a plan file and a final hand-off call, but I had no write tool or hand-off tool this session, so this answer is the whole deliverable.
