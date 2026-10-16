# Native3D BL integration on cells 157, 329 and 88: review

This review is based only on reading the line ranges you listed. I didn't run or change anything.

## Ranked assessment

**1. Reusing the existing parallel-chord path for all faceted support is valid geometry but probably isn't enough.**

I found nothing in the chart math that depends on the cell being outside the band (hidden):
- **Entry/exit classification** (`CBoundaryLayerMetric.cpp:1475-1478`) is correct for any projection direction. Since v = d×u, (u,v,d) is right-handed, so a positive projected area together with a positive orientation means an inward normal with a positive d component, which is an entry face.
- **Pairing faces that share an edge** (`:1490-1516`) gives the exact triangle where two projected faces overlap, for any convex tet. The exact-zero-area skip at `:1476` makes sure the edge side test at `:1501` is never zero. The guards at `:1512/1514` don't depend on the cell being hidden.
- **Source barycentrics** (`:1480-1488`) are exact because the projection is affine. The cosine floor of 1e-3 at `:1458` limits the condition number.
- **Jacobian**: projected area × (exit−entry)·d (`:1753-1754`, `:1843`) is the correct dV for orthonormal (u,v) and a unit d. Length is affine, so it can only be zero on the shared edge. The anchor choice (`:1562-1570`) and the interior Gauss points keep it positive, so `:1842` won't fire on a valid chart.
- **sensorDependent**: at present the hidden path reports `true` even without a density callback (`:1299`), while the visible path reports `bool(density)` (`:1422`). If you route every active support through this path, use `bool(density) || hiddenSupport`.

The real problems with using it for visible support are these:
- **(a) The direction is arbitrary.** It is the normal of `support.front()`, and that is just the face with the lowest ADT id (`:1289`, `:1433-1436`). It isn't the nearest face or a representative one. Judging by the coordinates (and assuming the standard M6 planform: 30° leading-edge sweep, root chord about 0.806, trailing-edge sweep about 15.8°):
  - 157 sits near the tip leading edge, on the lower surface.
  - 329 sits just behind the root leading edge, on the upper surface.
  - 88 straddles the trailing edge at y≈0.085, where x_TE≈0.83.

  At all three locations the support normals vary a lot; at the trailing edge, upper and lower normals are nearly opposite.
- **(b) The footprint partition only aligns faces whose normal is close to d.** It cuts along the Voronoi boundaries for the support faces: offset-0 lines for the face/edge boundaries and endpoint-normal lines for the edge/vertex boundaries (`:1528-1537`). Those lines are only exact for faces whose normal ≈ d. For every other face it still adds 21 cuts that don't help alignment. The ±T and ±0.9T offsets exist for the case where the band enters the cell from the side, which is a hidden-cell situation. With many visible support faces this risks hitting the 4096-piece cap (`:1555`) as a new failure.
- **(c) Chords still cross other faces' Voronoi planes at an angle**, exactly as Duffy rays do. So I expect this reuse would at most help 157, and only if the front face happens to be the nearest one. That's a prediction, not something I've checked.

**2. Likely mechanism: nearest-feature density jumps that no current chart aligns with.**

With the anisotropic sensor, the density sqrt(det(M_BL∩M_s)) depends on the nearest-feature normal. It therefore jumps across the Voronoi planes of the faceted wall. With a unit sensor, the determinant doesn't change under rotation, so those jumps disappear. That fits cell22 passing.

The budgets you reported point to the radial direction:

| Cell | Failure | Retained points | Panels | Points per panel | Points per retained ray (7 per panel) |
|---|---|---|---|---|---|
| 157 | point limit | 65411 | 652 | ≈100 | ≈14 |
| 88 | point limit | 64957 | 202 | ≈320 | ≈46 |
| 329 | work limit | 22193 | 188 | ≈118 | — |

For 329, the 16384 work count over 188 panels is about 87 visits per panel. That counter includes radial bisections, and each panel runs 19 rays (7 kept for the rule, 12 comparison rays that are discarded).

So the cost sits mostly in the radial direction, not the angular one. Within a ray, a jump converges only at O(L), and the per-ray tolerance of 1e-5 (`:1888`) forces deep bisection at every crossing.

A second possible source in the fade region (88/329) is the kink where the faded BL eigenvalue crosses the sensor eigenvalue. Its location depends on the sensor, so no geometric breakpoint captures it. A kink converges at O(L²), so it costs less than a jump.

**3. Smallest connected design: put nearest-feature crossings into the existing charts.**

Keep the current routing (Duffy for visible, parallel chords for hidden, analytic/cylinder unchanged). Add one step:
- Build the support faces' Voronoi planes:
  - prism sides: through each wall edge, containing the adjacent face normal;
  - vertex planes: perpendicular to each incident edge.
- Insert each chord's crossings of those planes as radial breakpoints, kept only when the nearest-feature identity actually changes, in the style of the `validRoot` check.

Once radial integration is exact piecewise, the ray integral I(θ) is smooth except along curves where a plane meets the ray endpoints. Those curves only produce kinks. The angular partition along those curves, using the same `convex_clip` code as `:1538-1557`, is a possible second step, and only if the experiment shows it's needed.

This one design covers all three classes. Point/edge/face contact Duffy charts only target the vertex contact in 157, so I'd rank them last.

One honest limit: in concave wall regions, the face–edge and face–vertex Voronoi boundaries are parabolic, not planar. They need to be detected and explicitly rejected, not retried.

## Decisive experiment (offline, same three fields, caps unchanged)

1. **Reference.** Integrate each cell without caps, using dense uniform rules in at least two coordinate systems: all four Duffy apexes, plus parallel chords along the nearest-face normal. Confirm the expected h-convergence rate; for jumps that is first order, so don't use Richardson extrapolation beyond that rate.
2. **Instrumented current rule.** At the cap, record the truncated value, its estimated error, and the true error against the reference. Classify every refined radial and angular panel by cause:
   - an EdgeBreakpoints base cut (count base cuts per ray separately, since over-generated cuts from non-nearest faces would show up here);
   - a nearest-feature identity change between its Gauss samples;
   - an intersection-branch switch;
   - none of these.
3. **Reading the results:**
   - If work concentrates in the "none" class and estimated error ≫ true error, the error estimate is false.
   - If base cuts dominate, breakpoints are being over-generated.
   - If feature changes dominate, coordinates are the problem. Confirm by injecting the exact Voronoi crossings as radial cuts (a test hook only, no formula change). Passing within caps with error under 1e-4 against the reference validates the design above.
   - If branch switches dominate, the density has a kink that no chart choice will fix.
   - Also run the forced parallel path with both `front()` and the nearest-face normal, and record the piece counts against the 4096 cap. That answers question 1 empirically.

Keeping the 3/5/7-point Gauss comparison does not bound the error on panels that contain a jump. Only the reference comparison does that.

**Recommended next step:** build that instrumented offline replay, including the Voronoi-crossing radial-cut hook, before changing any production chart.

## Limitations

- I didn't read `:1319-1409` (radial fans), `:1589-1746` (the cylinder branch), the rest of `EdgeBreakpoints`/`validRoot`, `MayIntersectWallBand` or `SamplePoints`. So I can't confirm whether nearest-feature roots are already emitted.
- The wall-location placements are inferred from the M6 planform, not from the wall mesh.
- The per-panel point counts are averages from your reported totals.
- The jump-versus-kink attribution is a hypothesis for the experiment to test.
- `four_chart_audit.json` covers the normal-only density on a different cell.
