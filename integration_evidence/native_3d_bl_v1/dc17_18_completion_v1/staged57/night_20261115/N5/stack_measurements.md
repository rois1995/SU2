# N5 serial stack measurements

Extracted from the five successful build520 `NativeD9SerialStack` face-height receipts. These are diagnostic fixture outputs, not an accepted BL mesh or production capability claim. Physical wall-face first-child altitudes and base-to-lift vertex spacings are distinct measurements and are reported separately.

## Results

| Case | Growth | Physical wall-face first-child altitude | Vertex lift spacing (separate metric) | Stack | Final mesh diagnostics |
|---|---|---|---|---|---|
| M10 n1 refresh | `G_returned=true`; 0 columns/layers/lifts/transactions; no nontrivial growth | Marker 0: 2 faces, requested 0.004, min/max/mean 0.004, max relative error 2.08e-17; 0 failures | Marker 0: 0 samples, 4 missing | 0 columns | 12/12 positive cells; q bad 6, J bad 6, 11 long edges; incomplete |
| M10 n2 | `G_returned=false`; two partial growth log layers: 2 lifts then 4 cumulative lifts; then dependency guard. G totals unavailable | Marker 0: 8 faces, requested 0.004, min 0.003600004, max 0.004, mean 0.0036500035, max relative error 0.099999; 0 failures | Marker 0: 2 measured, 7 missing; both abs dz 0.00577368 | G column stats unavailable | 101/101 positive cells; q bad 47, J bad 34, 42 long edges; incomplete |
| M10 n4 | `G_returned=false`; 12 R2 row transactions/lifts, no completed G growth layer before dependency guard; G totals unavailable | Marker 0: 32 faces, requested 0.004, min 0.003600004, max 0.004, mean 0.003612503875, max relative error 0.099999; 0 failures | Marker 0: 12 measured, 13 missing; measured abs dz 0.00577368 | G column stats unavailable | 617/617 positive cells; q bad 192, J bad 160, 174 long edges; incomplete |
| M12 control | `G_returned=true`; 50 columns, 15 layers run, 726 lifted points, 136 transactions, 20 refused | Marker 0: 32 faces at h0 .004, all .003600004; marker 3: 32 faces at h0 .006, all .005400006; both max relative error .099999, 0 failures | Both walls: 25 measured, 0 missing; marker 0 abs dz .00577368, marker 3 .00866052 | Each wall: 25 columns; max 15, mean 14.52 growth layers (initial row excluded) | 3734/3734 positive cells; q bad 280, J bad 278, 97 long edges; incomplete |
| M12 meet | `G_returned=true`; 50 columns, 17 layers run, 768 lifted points, 142 transactions, 110 refused | Marker 0: 32 faces at h0 .004, all .003600004; marker 3: 32 faces at h0 .006, all .005400006; both max relative error .099999, 0 failures | Both walls: 25 measured, 0 missing; marker 0 abs dz .00577368, marker 3 .00866052 | Each wall: 25 columns; max 16, mean 15.36 growth layers (initial row excluded) | 3914/3914 positive cells; q bad 400, J bad 406, 117 long edges; incomplete |

M10 n2 is the smallest tested nontrivial partial growth in these runs, not a mathematical minimum. Its phase log records 2 lifted points on layer 0 and 4 cumulative points on layer 1 before the production guard stopped advancement. M10 n4 has 12 completed R2 row transactions/lifts but no completed G growth layer. Neither guarded case has G column totals; they are intentionally not represented as zero.

The physical first-child wall-face altitudes above are compared with requested `h0` using the fixture's existing 0.1 relative tolerance. All five record zero face-height failures. This does not imply full mesh acceptance: all five have `complete=false` and `acceptance_claim=false`, with the separate q/J/size debt shown. All have positive cells and zero nonmanifold/unbound boundary faces.

For M12, `2T<G` control uses T=.4 and gap G=1; meet uses T=.6, so `2T>G`. The control called MakeRoom 15 times (130 needy tops, 187 region vertices, 1,128 cells, 3 logged nonempty regions of sizes 63, 62, 62; 3 commits, 0 refusals). Meet called MakeRoom 17 times (212 needy tops, 311 region vertices, 1,876 cells, 5 logged nonempty regions of sizes 63, 62, 62, 62, 62; 5 commits, 0 refusals). Guarded M10 n2/n4 partial pre-guard stats show respectively 2/1 MakeRoom calls and zero needy tops, region vertices, cells, commits, refusals, and logged nonempty regions. Their completion records also emit surface_work=0 and surface_edits=0; these are activity counters, not a distinct count of faces needing surface preparation.

`R2_max.closure_vertices` is the accepted OUTER seed-set count (0 for M10 n1; 1 for M10 n2/n4 and both M12 cases). It is not the internal expanded vertex set and cannot be read as requested growth.

## Provenance

All five face-height runs used build520, binary SHA-256 `8a77a3fb0e294ecbb143410c08f7fc995dffc88818f8c648653dbf9a3d1bfede`, one rank, `nice 19`, and the existing 600-second timeout. The source-pins receipt SHA-256 is `29dfaf2c251de2d53ea38c42f9ead09fb5ff825879e81d062d24fec1bd18101d`. Current build520 descriptor hashes, attempt paths, artifact hashes, phase timings, resource receipts, and vertex-spacing measurements are in `stack_measurements.json` and `.csv`; superseded build515 extraction pointers are isolated under `historical_previous` in the JSON. The five current case directories are recorded there under `N5/cases/`.

The test runner reported GNU `/usr/bin/time` and sampled process memory separately in the per-case records. Those process metrics are not presented as a whole-job peak. All five unit receipts report success, terminal handles, verified pins, and clean resource guards.
