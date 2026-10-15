# Goal3 — bounded original-field service for complete regions

Worktree NativeIntegrated, branch codex/native-3d-bl. Full Goal3 active.
Published cold/removal checkpoint b943fd5635 is the preceding baseline.
The connected field-service/source-refresh workflow passes serial and MPI2/4.
Full Goal3 remains active and incomplete.

## Missing capability and connected design

The accepted1368-cell CFD mesh exceeds the region constructor's256-donor whole-box
admission once its sensor source is refreshed. Retaining the old24-cell frozen oracle
cannot establish repeated actual CFD rebuilding. The current batch decouples region
metric callbacks from resident whole-source storage and adds a bounded field service.

All ranks call the service together on their existing MPI thread. Only rank0 runs
the admitted private constructor. A cache miss emits a matching query command;
other ranks serve original donor imports until rank0 closes the protocol after success
or any exception. There is no extra MPI thread and no global original-donor gather.
Small original sources retain their existing preloaded path.

`DonorImport::ImportSegments` routes closed point/segment queries through the existing
rank/local indexes, exact tetrahedron intersection and bounded payload protocol.
It imports touching donors and preserves canonical primary/extension priority, without
requiring the full segment AABB's donor union to fit256 records. Each footprint and
resident cache stay<=256. Per-segment source limits still apply; arbitrary source or
segment size is not claimed.

Point memoization stores only sensor tensors at identical coordinates; geometric
BL composition runs again at each actual point query. A cached strict interior
physical donor can answer locally because the admitted original source is embedded.
Boundary ties and extension samples require a certified complete point footprint;
a missing physical donor must never lose priority to an extension. Every new segment
is certified by an import before full original-P1/geometric-band integration. Scalar
edge reuse is limited to identical endpoints and the same immutable event policy.

The engine's complete topology/geometry/height/quality/full-length and versioned
publication gates stay unchanged. Source/cache/import counters accompany existing
field/donor timings, to expose communication/build overhead before any speedup claim.
The geometry constructor remains one visible radial/caps region on one worker;
this field service does not establish parallel region generation or exterior topology.

## Required validation for this batch

- Large distributed source whose diagonal segment box exceeds256 donors, but its
  exact segment footprint does not; affine original-P1 integral and point values.
- Repeated point composition, bounded resident storage, query refusal and a subsequent
  valid service invocation without mismatched collectives.
- Existing difficult curved target remains fully accepted under unchanged constraints.
- Complete cold construction followed by source refresh from the actual1368-cell
  mesh, sensor-only removal, source refresh again and BL rebuilding. Use mildly varying
  affine sensors, preserving finer demands; check actual target residuals and full
  strict gates. Do not use replicated far-away donors as a substitute for this source.
- Serial first, then MPI2/4 after the connected capability works. Keep donor records
  distributed and input/reference signatures unchanged. No unqualified CFD transfer,
  restart, accuracy, high-ratio front or performance claim.

## Qualified source-refresh workflow

Final connected build passes. The750-cell analytic source controls pass serial and
MPI2/4: point P1 interpolation, repeated actual composition, closed diagonal segment
integration, reverse-edge reuse, bounded residency, refusal and subsequent recovery.
The diagonal segment box contains all750 cells; its touching footprint fits256.

The complete workflow first builds1368 BL cells from the24-cell source, then imports
that actual saved mesh through SU2's reader/geometry importer with affine sensors
`diag(1+.02*z)`. It releases the BL, imports the actual released mesh as a fresh source,
and rebuilds the BL. No replicated distant donors or retained coarse oracle stand in
for either refreshed source.

| Ranks | Release source -> cells | Rebuild source -> cells | Rebuild qmin / Jmin | Target residual max |
|---|---|---|---|---|
| 1 | 3072 ->72 | 288 ->1368 | .331359107 / .095726905 | 1.29e-15 |
| 2 | 3072 ->72 | 288 ->1368 | .331359317 / .095726959 | 1.73e-15 |
| 4 | 3072 ->72 | 288 ->1368 | .331359327 / .095726975 | 1.67e-14 |

All final meshes pass native full topology/geometry/no-overlap/height/quality/length
acceptance. A separate read-only service checks analytic affine-P1/composed targets
at every cell centroid and all complete native edge integrals. This is not fresh
independent geometry or independent full-edge quadrature evidence. Maximum lengths
are1.127594938 released and1.795194884 rebuilt, below unchanged1.8. Prescribed height
.05, growth1.2, thickness.2, fade, geometry tolerances, q>=.20/J>=.05 are unchanged.
Each refreshed event retains4096-work/300s limits; the whole multievent test is not
claimed to share one4096 allowance. Release work191/192/180; rebuild1612/1620/1616.

Peak resident donors256. Rebuild imports2529/2567/2539, with912 sensor memo hits,
2527..2539 strict-interior hits and1284 identical-edge hits. Counters are exposed in
runtime profiles. This is bounded storage/query correctness, not evidence of low
communication cost or scalable geometry construction. Host contention affected the
campaign; walltimes must not be used as lifecycle performance or scaling comparisons.

Final evidence is under `region_construction_v1/native_refreshed_source_v2`,
`native_refreshed_source_mpi2_v2`, `native_refreshed_source_mpi4_v2` and the MPI2/4
`native_region_field_controls_*_v2` folders. Serial analytic controls precede final
wire/profile cleanup; their exact source/binary receipt is preserved separately.
`refreshed_released.su2` and `refreshed_BL.su2`, each with `.native_ref`, are actual
accepted output meshes. No CFD solution was fabricated.

The original1402-cell difficult target also passes with the final binary:1368 cells,
zero oversized edges, work2940. SU2/ref bytes and original sensor/authority records
match prior independently audited results; explicit reuse is preserved in
`native_field_compat_v2/independent_reuse.json`.

[Complete provenance](../integration_evidence/native_3d_bl_v1/region_construction_v1/distributed_field_summary.json).

## Remaining scope

This admits distributed original sources larger than256 records, with a256-donor
limit on each actual closed point/segment footprint and resident cache. It does not
remove that bound, parallelize region construction, solve general exterior/multiwall
geometry or establish high-ratio metric-driven multiple fronts. Current+BDF/SA flow
transfer/restart on this curved refreshed-source workflow, actual RANS evolution,
stronger fields, viscous accuracy and full lifecycle cost remain full Goal3 gates.
The next connected step is actual curved RANS/current+BDF transfer and restart,
using these refreshed sources rather than a fixed coarse oracle.
