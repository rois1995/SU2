# Larger wedge envelope — 2026-10-11

Published implementation/evidence checkpoint: `9dc21db012be53a1e723b3f01e7649c7cad2b5ab`, origin/codex/native-3d-core, full remote SHA verified.

Branch codex/native-3d-core, baseline 7bc3869babb19171fe402fdfb0e96f745b769703. LARGER_WEDGE_MATRIX.json froze four n4 wedge cases before execution: MPI1/M1, MPI2/M2, MPI4/M2 and MPI4/M4. Input:75 points,192 tetrahedra,128 original boundary triangles, exact volume1/2, six marker patches including the oblique x+y=1 face and bottom x=.5 junction. Three events use scales16/4/16, aspect1.5, spatial strength1+.125x and rotating direction. Guards remain q>=.20,J>=.05,L<=1.8,512 rounds,noise0.

All four actual configured-binary cases pass independent exact geometry, positive volume, incidence, overlap/gap, domain/marker, original-P1 tensor, manufactured current/BDF, SU2/CGNS write/readback and six corruption controls. They reuse the same qualified production source, binary, six runtime test objects, archives and all thin-archive members as restart_control_box_n1_v1. The retained complete68-object restart_regression_v1 covers that unchanged production; no redundant numerical regression was run for this size-only expansion. analyze_larger_wedge.py rechecks all receipts, artifact hashes and source/object/archive consistency.

| CFD ranks / workers | Accepted cells after events0/1/2 | Accepted edits0/1/2 |
|---|---|---|
| 1 / 1 | 289 / 52 / 308 | 101.0 / 71.0 / 165.0 |
| 2 / 2 | 289 / 42 / 309 | 101.0 / 72.0 / 155.0 |
| 4 / 2 | 289 / 42 / 309 | 101.0 / 71.0 / 156.0 |
| 4 / 4 | 301 / 46 / 291 | 98.0 / 74.0 / 168.0 |

Across the matrix:min q=.2474003062,min J=.07957138227,max L=1.7987750917934346; manufactured state residual<=3.86440184e-16; transported/reloaded tensor error0. Partitions produce different valid trajectories, so this matrix is not a fixed-work scaling comparison. Inspect accepted grids in larger_wedge_n4_n{1,2,4}_m{1,2,4}_v1: envelope_s0/1/2.vtu, final SU2/CGNS and their nodal/frozen-source readbacks, donor/accepted JSON and states.

374 closed phase rows are retained in larger_wedge_rank_phases.csv. Exclusive engine/import/partition phases close against their own parents. Summed rank-event work is labelled as work, never elapsed; parent elapsed uses event maxima. For serial coarsening, metric validation totals1.5732s and seed measurement.8339s of rank-event work. Edge evaluation2.5151s contains exact trace2.4829s but interpolation only.001743s. The trace hotspot motivated the separately validated vertex-clip optimization. MPI4/M4 coarsening has routing11.2647 rank-event work seconds; a local arithmetic improvement cannot establish distributed affordability.

Requested native buffer bound8MiB; observed maximum lifetime process RSS32,567,296bytes (31.06MiB). These are distinct, incompletely calibrated quantities. Single contended pilots only: four foreign FSI ranks were busy and left untouched. Packed record bytes include self traffic and are not network bytes; wrapper collective elapsed is not pure wait. Complete communication coverage, true cache-miss exports, private-wait attribution, memory calibration and detail-OFF overhead remain open.

The hard original-reference bound is256 triangles (4*MaximumCavityCells), enforced in CNativeImport3D.cpp, PlanarReference and DecodeReference; sidecar bytes are separately limited to1MiB. This is a global geometry authority limit, not the64-cell private cavity or256-cell donor budget. The n4 wedge fits128 triangles; its success does not remove that scalability bottleneck. The next size step must address global authority storage/search/admission without weakening exact finite facet coverage or marker checks. No curved geometry,3D BL generation,PDE/unsteady accuracy or CFD affordability claim. Full Goal1 remains ACTIVE.
