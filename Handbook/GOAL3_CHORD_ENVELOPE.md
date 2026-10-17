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

## Larger-field result and scalar/strict partition policy

Complete-field attempt global_envelope_seed_np4_v1 is terminal stopped/124 after
205.88s:9442/303359 cells,8469368 density calls,14 unknown contributions/infinite
global indicator. Prefix C30649.51946 is NOT the whole-field mass. All positive
approximations and counters remain. Verified only our GNU timeout was signalled;
all4 workers exited and terminal receipts flushed. Source commit94307269bb.

In particular622/2331 consume8192 queries before completing their initial19-chord
band seed; the earlier original cut policy had finite estimates there. Other unknown
cells can also involve unresolved angular support. This is a scalar-budget regression,
not proof of a changed physical field or a geometry/height infeasibility. It must not
be hidden by reporting the faster geometric preparation alone.

Next connected adjustment: the SAME finite-feature lower envelope supplies physical
distance-band roots for provisional scalar integration. Nearest-feature transitions
remain mandatory for STRICT mesh-edge acceptance. Scalar's full density callback,
parent retention, support guards, error indicators and8192-query ceiling remain.
Scalar heuristics do not certify edge acceptance; both evaluate the SAME original
P1/geometric BL field. Suppressing extra partition points is not an Euler/BL case
switch, target change or increase in work budget. Its global error still needs evidence.

The scalar/strict partition adjustment builds PASS. distance_band_scalar_v1:
2 focused feature/scalar cases1154 assertions PASS;8 existing2D composition cases95
assertions PASS. Original88/157/329 full and decomposed VALUES return bit-for-bit to
the previous band-only results, with unchanged reference errors;157 removes8 redundant
queries. Cold/warm values/indicators/counts remain identical. Cold88 takes2.087s,
so the earlier4x local cold reduction must NOT be carried over as an equal-accuracy
speedup: that earlier cut set completed less angular refinement within8192 points.

On the same2555 original field cells, the earlier strict-cut scalar experiment
reduces summed preparation573.431s->138.981s (4.126x), but queries1471704->2390484
and5 finite estimates become unknown. These are summed CELL/STAGE elapsed times,
not CPU times, critical-path elapsed or equivalent-accuracy throughput. Full dataset
and exact-source receipt: global_envelope_seed_np4_v1/same_cell_comparison.json.

New exact-source seed failures now retain finite BL-aware estimates with the same
8192 ceiling:622 C12.48272431/raw indicator.000915602/2132 calls;2331 seedC3.67129618/
indicator.00824171/740 calls, boundedC3.66396840/indicator.00300851. This resolves
the demonstrated INITIAL seed regressions, not every unresolved cell/full-field error.

Independent positive tensor-Gauss references use the same native point law but no
native adaptive controller or band cuts.2331 order48 apex0/3 gives3.66321778175/
3.66321794832; chart span4.55e-8 relative, last-order changes7.80e-6/6.55e-5.
622 is NOT converged: order48 gives12.48838234/12.48398387, order96 gives12.48246230/
12.49305721. Larger order does not monotonically reduce chart disagreement. Stop
raising its reference order: neither the96 mean nor its apparent difference from
the estimator certifies that cell's small raw indicator. All stages/raw scalar
receipts preserved; deterministic query arrays are removed after successful stages.

Current normalization_distance_bands_v1 repeats the connected MPI1/4 check on3
original fields: exact agreement C10.103643924254474, guarded uncertainty.22167625143,
logScale1.8868171230877364,8 trials,56632 density calls,23 cell estimates,554 prepared/
744 reused chords,133352 accounted bytes,37 reductions/rank. Status remains correctly
integration_limited at1%, not an exact root.25 controller assertions and owner-only
malformed input collective rejection pass. This is not production regradation or a
complete-M6 MPI normalization proof. Production CSolver normalization is unchanged.

## Resources and next capability

Only UnitTests/test_driver was rebuilt for these replays; the installed SU2_CFD
executable is NOT current-build evidence. Rebuild/check it before any actual CFD
validation. The existing construction-only helper uses the current test driver.

All owned jobs terminal. One owned heavy job at a time; recheck available RAM/CPU
before follow-up. The serial qualification batch uses49.78 user+8.97 system seconds,
66.11s elapsed,59788KiB maximum PROCESS RSS; the622 reference extension uses30.15+
1.69 CPU seconds/33.50s elapsed/215564KiB max process RSS, zero swaps. These GNU time
figures are validation work (including helpers), not adaptation-stage timing, total
host RSS or peak whole-mesh cache qualification. Host had~40GiB available throughout.
No concurrent owned CFD/MPI/compilation job was run.

The complete frozen-field integral/calibrated global uncertainty remain OPEN. Do not
replace infinity with an arbitrary small number or use an unconverged local reference
as ground truth. Further work should spend the global error allowance by absolute
contributions, with defensible finite bounds/indicators for unresolved small cells,
rather than insisting on uniform local accuracy or adding subdivision/limit ladders.
A geometry-only lower bound on unavoidable BL mass remains unmeasured; requested100000
is NOT established feasible or infeasible by current sensor-only/prefix estimates.

A cheap next CONSTRUCTION diagnostic is the exact saved first-layer edge, before
another348s driver setup: export its original donor support from Coarse.su2 and
construction_first_trial_v1/sensor_original.bin (owner UPPER6, matching remesher input),
retain real node IDs and exact coordinates, orient donors positively, filter using
existing exact IntersectsSegment, respect the256-donor limit and call existing
FrozenField::TargetEdgeLength with unchanged native geometric composition/cuts.
The saved edge is(.619448337998914411173,1.02515983674473298848,-.0153284912029800698963)
to(.619865041411222006396,1.02046045941819452096,-.015806993408904391335), IDs18209/61331.
Its original failure is composed-edge budget at piece0/parameter.5446777343755232.
Reuse the hidden faceted replay/FrozenField facilities; no CFD advancement or point
fallback. Verify source/geometry/core provenance and support completeness before
crediting a pass; it would resolve one failed edge, not establish an accepted M6 mesh.
Only then decide whether another full construction-only backend attempt is warranted.

No accepted M6 mesh, solution transfer, calibrated global normalization or lifecycle
speedup is claimed. The broader query/index/transaction performance package remains
deferred as recorded in POST_COMPLEXITY_QUERY_TRANSACTIONS.md.
