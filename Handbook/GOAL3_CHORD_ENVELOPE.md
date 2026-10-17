# Faceted chord distance partition — connected experiment

2026-10-17; NativeIntegrated/codex/native-3d-bl. Full Goal3 remains active.
Cold M6 scalar-volume preparation dominates even with cheap integration and geometry
reuse across scale trials. Pure background subtraction changes329's error but not
its171 cold chords;88/157 get no query reduction. Repeated nearest-wall admission
of candidate facet/edge/vertex band roots is the structural target.

Construct finite-feature squared-distance quadratics restricted to their valid
chord intervals: face projection inside the triangle, segment projection within
both endpoints, vertices everywhere. Take the GLOBAL lower envelope; facet Voronoi
planes alone are insufficient. Every triangle contributes its finite boundary
features. ADT chord box plus wall thickness provides conservative candidates for
all points where that wall can be active. Solve distance-level crossings on each
winning piece and retain non-identical nearest-displacement transitions. Coplanar
tiles with identical displacement functions must not create artificial cuts.

This changes geometry preparation only. Actual samples still use original sensor
P1 plus the original authoritative nearest-wall evaluator/intersection/finer/fade.
No wall tensors/densities are interpolated from coarse mesh samples. Exact donor
predicates, mesh constraints,65536 cut bound and strict edge integration tolerances
remain. Per-wall envelopes preserve ordered multiwall composition. Analytic geometry
keeps its existing finite-rim/cylinder/sphere path;2D retains its geometry kernel.

A new helper bounds retained features/pieces at65536 and overlap comparisons at
1048576 per chord; exhaustion explicitly rejects. Long-double coefficients/domain
and root arithmetic are numerical, NOT outward-rounded certificates or exact-sign
distance enclosures. Physical query law and exact donor predicates are unchanged.
No claim that merely adding cuts guarantees convergence of composed integration.

First connected qualification:20 cases/437753 assertions PASS, including finite
feature domains/global competition, constant densely tiled chord (still2 cuts),
microscopic/hidden walls, strict volume/curved/rotated and planar tangent edge.
chord_envelope_v1 preserves exact sources, binary/build pins and original3-cell
cold/warm full/decomposed results. No pointwise tensor law changed.

Results are MIXED under unchanged8192-query/.001 absolute controls:

| Cell | Old full cold s | Envelope cold s | Old reference error | Envelope error | New raw indicator |
|---|---:|---:|---:|---:|---:|
|88|1.997|.5055|.7682%|4.9486%|.295921|
|157|.1079|.03762|.09178%|.09173%|.000640711|
|329|.1009|.07127|.14414%|.23676%|.00120343|

Cold preparation88 1.893s->.4209s, but additional winning-feature cuts consume
more density budget and leave fewer completed angular refinements: prepared chords
201->53. Larger raw indicator reflects that loss.329 also now exhausts8192;
157 uses3676->4060 queries. The original decomposition benefit does NOT transfer
unchanged to this new cut representation. Warm/cold values/indicators/counts stay
bit-identical within each method. These are local tradeoffs, not global error or
lifecycle conclusions.157's raw indicator still underestimates its numerical error.

The bounded-geometry follow-up builds PASS and2 focused cases/1152 assertions PASS
(chord_envelope_bounds_v1). It changes exhaustion only: strict edges still reject,
while scalar complexity retains a complete positive parent/seed with infinite
uncertainty and explicit limited/unknown flags. No failed BL contribution is
classified accurate. Earlier successful arithmetic/physical paths remain unchanged;
20-case evidence is explicitly before this exhaustion-only delta, not an unchanged
binary claim. Exact earlier sources are retained to support scoped reuse.

Complete frozen-field cheap-band integration is now LIVE session76946, folder
oneram6_uncertainty_v1/global_envelope_seed_np4_v1,4 ranks/600s ceiling. Check the
handle/process before another heavy job or restart. Each completed original cell
is flushed with indicator/work/preparation/density counters; resumable receipts
are written at terminal completion or timeout. No field resampling, CFD or remesh.

Next: complete frozen-M6 global contributions/uncertainty using cheap band seeds;
do not require every small cell to meet.001 absolute. Large local relative errors
may be globally small, but this requires the complete sum and independent audits.
Strict first-layer construction is a separate remaining replay; no accepted M6
mesh or transfer is claimed. No integration limits have been raised.
