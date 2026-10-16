# Goal 3 — coordinated transition fronts and core coarsening

Full Goal3 remains ACTIVE/incomplete. This closes the next high-ratio curved-mesh
milestone under unchanged gates; it does not replace the full goal.
Evidence lives in `integration_evidence/native_3d_bl_v1/region_construction_v1`.

## Why change construction

The previous high-ratio case, `native_front_constant_v1`, retained its input and
rejected at work4097. Its first band costs840 tets. At least four additional
bands are needed even at the full1.8 radial allowance, exceeding4096 before core
or planning. Changing the radial step or adding another local recovery cannot
remove that topology-class obstruction. See [the preserved diagnosis](GOAL3_METRIC_FRONTS.md).

Saved complete proposals established the missing coordination. `joint_front_core_v1`
uses three-tet prism transitions with a coherent periodic chart and staggered
metric-spaced fronts. It fails near the core (q.18056/J.04389). The90 radius layouts
in v2 cannot admit this fixed topology (best shape ratio.9616). In v3, pairing42
terminal columns into21 and rebuilding all incident transition/core simplices
removes147 duplicate-ID tets. All2877 cells pass shape/full lengths, independent
exact embedding/manifold/authority/feet-height and preservation of the840-cell
first band. The prototype is separately labeled, not native acceptance.

## Native primary strategy

`CNativeRegion3D.cpp` retains the actual-metric cylinder surface sampler, exact
height first row and original sensor-only P1 composition. It records the sampled
periodic chart, estimates the full radial-prefix maximum over all chart columns,
and chooses free-front count/positions from that actual composed field. Free
fronts stagger tangentially by half an angular interval. Shared periodic/axial
ordering, including the seam, fixes every prism diagonal coherently.

The terminal front proposes pairing adjacent angular columns. Admission checks
every mapped front edge against the original full metric; stronger tangential
sensor demands can retain the fine front instead. All incident prism/core cells
are reconstructed together, with duplicate-ID simplices removed and consistent
robust orientation required. Caps follow the same diagonals. The existing
hierarchical core, physical surface/first-height/links, whole embedding and final
actual-centroid q/J/full-edge gates remain authoritative. No cylinder fallback,
scenario switch, normal reset, target smoothing or changed budget is introduced.

The work/time/memory checks cover front planning and generated cells. First-band
construction, source collars, finer demands, growth/fade, noise0, quality/length,
geometry and4096/300s limits remain unchanged.

## Complete results on the changed source

| Case | Cells | Fronts | Core regions | Charged work | qmin | Jmin | Full Lmax |
|---|---:|---:|---:|---:|---:|---:|---:|
| native_front_constant_v2, h.01/T.2/L.2 |2877|8|1|3771|.223149|.060674|1.783149|
| native_core_rotated_v13, h.05/T.2/L1 |2448|2|2|3259|.275033|.083384|1.645505|
| native_core_varying_v12, h.05/T.2/L1 |2284|2|5|3120|.214600|.052860|1.763818|

All three pass native and independent exact embedding/no overlap, connected
oriented manifold and every vertex link, wall feet/height, analytic geometry and
original-P1 sensors. The high-ratio independent NumPy reference checks all2877
actual tensors and all4727 complete edges: tensor residual8.24e-15, normalized
native length difference4.99e-9, full maximum1.7831488561463442. Numerical error
indicators are not rigorous enclosures. The varying audit independently checks
original P1 and pure-core integrals; full composed varying BL integration remains
unqualified. Inspect `core_accepted.su2` and its `.native_ref` in each case.

Changed-source high-ratio MPI2/4 native and independent geometry/source audits
pass2877 cells/work3771 with the same gate extrema. Coordinates/IDs differ;
no exact-mesh or independent full numerical metric reuse is claimed for MPI.
Actual RANS N4/M2 now passes24->621->48 with independent exact geometry,
physical current+BDF conservation/open ledger, separately replayed SA solver-floor
corrections, SU2/CGNS/reference reload and actual restart. Post-bounds SA is not
exactly conserved; pressure demand clips coarse. See sa_projection_replay_v26.
Existing full15-case/six-way RANS MPI campaign is pre-front evidence, not proof
for the changed constructor. Every case keeps source/build/binary/config/
input/artifact receipts. Failed candidates and prototype searches remain saved.

## Cost and limits

Serial phase clocks expose validation as the large measured section:43.53s of
53.86s high-ratio;66.42s rotated;59.15s varying. Transition is8.05s/1.12s/.72s.
These are local diagnostic wall times, not scaling or adaptation/CFD ratios.
Composition counters are1,870,866/611,849/1,652,774. The fixture's region-service
edge/point counters are zero because these small fields use the direct frozen
source path; this is an instrumentation-scope gap, not zero query work. A passive
perf capture attempted after varying completion reports SKIPPED and supplies no
CPU attribution. Profile an already-required lifecycle before optimizing.

The primary layout is limited to one complete inward cylinder with two planar
caps, one generation worker and one terminal pairing. Its core radius is the
heuristic `.75*(radius-thickness)` and must leave a positive BL-free core. It is
not a general surface/front mesher, interacting-layer construction, external
body capability or arbitrary-metric guarantee. Sphere behavior retains its prior
path; current results do not newly qualify it. Thin/coarse interacting support,
full gradation feasibility, viscous accuracy, repeated updates and lifecycle
performance remain full Goal3 work. Failure must preserve the accepted state;
do not add local fallback ladders or raise limits in lieu of a missing strategy.

## Closed milestone and next full-goal work

All three planned integration cases are now complete. The implementation is
published at f9a6af4a096f9796115f9fc8a04fa235aaaaaccf; this later evidence update
adds no production change. The native actual-RANS BL remesh takes11.72s locally,
with10.50s in regional validation. Metric construction/automatic sampling and
repartition/migration/return/rebuild/transfer/output clocks are saved separately;
no CFD-only ratio follows from the advance clock. Source-service counters and
fine-grained validation attribution need appropriate instrumentation before a
performance claim, not another correctness rerun solely to export counters.

Next preserve this complete accepted-mesh baseline while addressing the missing
construction geometry/interaction capability. Wider authority/feature/interacting
regions, nonconstant full composed reference, separate combined-gradation limits,
retained-BL updates, viscous/M6 validation and complete lifecycle affordability
remain the original Goal3 gates. Do not declare this cylinder pilot the full goal
or move to Goal4. No native/build/audit/replay process remains live.
