# B4 primary scope correction — source only

PrepareSurface deliberately omits shape measurements during validity-first construction. Reusing q/J cannot remove its principal old geometry-guidance cost. Selection::CellState currently saves raw q/J and aggregate geometry debt plus six edge-weighted geometry_scores, not the four per-face complete magnitude bounds (Engine3D:808–855). Replacing GeometryGuidanceExcess with an aggregate or edge priority would change face order and early rejection arithmetic.

For old shape in streamed measure_private (Engine3D:4290–4408), both published census and private measurement resolve the same frozen target. Any reuse must still preserve source support validation/import and exact endpoint/target identity; do not skip donor validation merely because q/J are cached. A bounded cached-shape input to MeasureMetricCells can replace only the centroid Measure after checking the current cell/version/coordinates and scope; all fresh rows continue through the old measurement. The final PatchMetric assembly and shape-debt arithmetic must keep original order and definitions.

For OptimizedRepairPatch, avoid changing the existing OptimizeOptions callback API unnecessarily. The adapter owns local IDs and can capture its local mesh and an explicit local-to-original-cell map. Its existing by-ID callback can compare the actual current local cell with the immutable original cell snapshot before asking the outer Selection for raw q/J. Fresh/replaced cells or any coordinate/topology/version/target mismatch return false. This preserves other BL callback callers. Prove actual target equivalence first; field identity alone is not a persistent certificate.

If geometry census reuse is added, retain actual complete per-face magnitude bounds and their exact geometry contracts, then seed the existing GeometryGuidanceMemo. Do not add another aggregate merit implementation. Current low-level face memo already reuses identical numeric geometry queries, so measure the incremental benefit rather than assume a gain. Extra CellState storage and migration/checkpoint serialization must be accounted together with B6.

No code changed or tests run for this note.
