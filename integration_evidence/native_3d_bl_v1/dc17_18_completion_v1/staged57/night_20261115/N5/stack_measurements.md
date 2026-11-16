# N5 serial stack measurements

This is an extraction of five completed `NativeD9SerialStack` receipts. It records observed fixture outputs; it does not establish an accepted BL mesh or production capability. The checked JSON is the full machine-readable record and the CSV is its compact numeric index. Height column names explicitly identify vertex-lift spacing, not a face-height measurement.

## Findings

| Case | Growth result | Base-to-lift vertex absolute-dz spacing (wall marker 0 / 3; not physical first-layer height) | Stack by wall (columns; max / mean layers) | Final mesh diagnostics |
|---|---|---|---|---|
| M10 n1 refresh | `G_returned=true`; zero columns/layers/lifts/transactions. No nontrivial growth occurred. | 0/4 samples (unavailable) | 0; no stack | 12 cells, all positive; q bad 6, J bad 6, 11 long edges; incomplete |
| M10 n2 | `G_returned=false`; guard stopped growth: “BL growth surface dependencies changed; current fronts need reconstruction before advancement.” Partial log records 2 lifts on layer 0 and 4 total on layer 1. G total statistics are unavailable and intentionally left null. | 2/9 vertex lift spacings, both 0.00577368 | unavailable | 101 cells, all positive; q bad 47, J bad 34, 42 long edges; incomplete |
| M10 n4 | Same guard; 12 R2 row transactions/lifts completed, but no completed G growth layer is recorded before the guard. G total statistics remain unavailable. | 12/25 vertex lift spacings, all 0.00577368 | unavailable | 617 cells, all positive; q bad 192, J bad 160, 174 long edges; incomplete |
| M12 control | `G_returned=true`; 50 columns, 15 layers run, 726 lifted points; 136 transactions, 20 refused. | 25/25 vertex lift spacings at each wall: marker 0 = 0.00577368; marker 3 = 0.00866052 | each wall: 25 columns; max 15, mean 14.52 | 3734 cells, all positive; q bad 280, J bad 278, 97 long edges; incomplete |
| M12 meet | `G_returned=true`; 50 columns, 17 layers run, 768 lifted points; 142 transactions, 110 refused. | 25/25 vertex lift spacings at each wall: marker 0 = 0.00577368; marker 3 = 0.00866052 | each wall: 25 columns; max 16, mean 15.36 | 3914 cells, all positive; q bad 400, J bad 406, 117 long edges; incomplete |

The height figures above are measured base-to-lift vertex spacings. They are not wall-face/opposite-face prism heights and do not demonstrate the configured `h0` height gate. A later build520 face-height measurement pass is planned; if its new descriptors and receipts complete, those measurements will supersede these preliminary spacing values. For both M12 cases the control used thickness 0.4 and the meet used 0.6. In both, wall marker 0 has configured `h0=0.004`, growth 1.2; wall marker 3 has `h0=0.006`, growth 1.15. The reported stacks count growth layers and exclude the initial row. Columns stop on thickness or clearance refusal; neither case is accepted.

M12 control called MakeRoom 15 times (212 needy tops, 311 region vertices, 1,876 cells, 5 nonempty logged regions with sizes 63, 62, 62, 62, 62; 5 commits, 0 refusals). M12 meet called it 17 times (212 needy tops, 311 region vertices, 1,876 cells, the same five logged sizes; 5 commits, 0 refusals). These are reported counters, not proof that the distributed contact-clearance test was removed. M10 n1 recorded no MakeRoom calls. Guarded M10 n2 recorded 2 MakeRoom calls and n4 recorded 1; both have zero needy tops, region vertices, cells, commits, refusals and logged nonempty regions in the partial pre-guard report. These are observed counters, not missing data.

The final quality counts remain separate: `q_bad` uses threshold 0.2, `j_bad` uses threshold 0.05, `size_long_edges` counts size failures, and `height_failures` is zero in each recorded mesh. All five report positive cells and zero nonmanifold or unbound boundary faces, but all have `complete=false` and `acceptance_claim=false`. Validity/topology is necessary, not sufficient for final acceptance.

`R2_max.closure_vertices` is an accepted OUTER seed-set count. It is not the internal expanded vertex set and is not a growth-count substitute. Values were 0 (M10 n1), 1 (M10 n2/n4 and both M12 cases). M10 n2/n4 `G_returned=false` is a production guard outcome, not a zero-column result.

## Provenance and timing

All runs used build `build515`, one rank, `nice -n 19`, `/usr/bin/time -v`, and the 600-second wrapper timeout. Test binary SHA-256 for all five: `30b74f56ca10162687447283e006fa93ab5dfb0a18e9e55df6f0f134f4130a72`. Pinned fixture hashes: `native_first_row_dc10.inc` = `c9d61107b81b590a20159a38e2dbb587c28c8a95081799c161c53650779012b2`; `native_first_row_dc12.inc` = `bd92e320ff2ffddb6a1b6f2b882114d140f3c38fda816091a67cad2df6c52c05`. The shared source-pins receipt SHA-256 is `cb2050ae0ed98e5396d78876444ee870025fdc32c228fefdd48a12ef03bdcacc`. Per-case descriptor, measurement JSON, phase-log hashes, runner wall duration, GNU time elapsed and GNU time maximum RSS are in `stack_measurements.json` / `.csv`.

GNU `/usr/bin/time` maximum RSS is the wrapper's reported value (35,548–44,608 KiB); resource sampler's largest individual-process RSS is separately recorded (0.59–44.75 MB). Neither field is a whole-job peak-memory measurement. All receipts ended terminal with exit code 0 and verified pins. Phase timings are recorded separately from wrapper duration.

## Evidence locations

Case directories are under `N5/cases/` and have the corresponding names in the JSON/CSV. Each includes `d9_stack_measurement.json`, `d9_stack_phase_log.txt`, `run.log`, `command.json`, `pins.json`, `resources.jsonl`, and `status.json`. Receipt summaries are under `N5/unit_measurement_receipts/`.
