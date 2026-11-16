# Failed N1 M18/M20/M21 counter inventory

**Status: all values below are UNQUALIFIED.** The source run is `staged57/p7p8_r3_20261115`, which failed the required event-payload identity gate. `first_result_failure.json` records event 1 payloads byte-identical to P6c, but event 2 `adapted_mesh_adap_00002.su2` differs first at line 2 (`NELEM=855828` vs `855760`); event 2 reference mesh and restart also differ. The stated cause was changed retained prepared-cache history altering event-2 geometry partition weights; exact contributing facets were not logged. N1 was reverted. Therefore these are extracted diagnostic observations from a failed/reverted run, not accepted measurements and not evidence for a performance/result claim. No B7 condition was computed.

## Evidence and extraction

- Main failure receipt: `../p7p8_r3_20261115/first_result_failure.json`, SHA-256 `d796e47441a8fc98b23ecf9f398b948ffcd859c741e77c028abd220dde055afb`.
- Raw streams: `../p7p8_r3_20261115/full_workflow/rank_output/1/rank.{0..5}/stdout`. Each physical rank stream contains both events. CSVs retain source-relative path, physical rank, line number, and stream SHA-256 for each observation.
- Event split for the two event windows: first 20 then last 12 `NATIVE3D_QUALITY stage=window` rows in each rank stream. The reset in emitted quality census sequences confirms the split (event 1: censuses 3,4,7–22,24,27; event 2: 3–14).
- `failed_N1_vs_F3b_summary.json` provides aggregate context only: event-1 N1 remesh root 2220.388028168 s and peak sum 14,007,463,936 bytes; event-2 1451.015039598 s and 17,343,528,960 bytes. It labels the comparison failed N1 vs accepted F3b and warns that rank-summed exclusive timer seconds are not job wall time. These aggregate values are also UNQUALIFIED for adoption.

## M18 — geometry-cache demand

The per-rank final `NATIVE3D_STAGE stage=exit` row of each event reports `distinct_facets`, `prepared_facet_mean_bytes`, `distinct_scope_keys`, and `scope_builds`. All 12 rank/event rows are in [failed_N1_M18_rows.csv](failed_N1_M18_rows.csv). These are direct printed counters; the CSV line numbers identify the exact row. The data describes cache demand as emitted by this failed N1 run only.

## M20 — private quality work and busy time

The raw streams emit `private_work_used` and `busy_seconds` for every quality-window record. [failed_N1_M20_rows.csv](failed_N1_M20_rows.csv) contains all 192 observations (2 events × 6 ranks × 20/12 windows), with exact rank, event, census, round, source line and stream SHA. Extracted ranges, still UNQUALIFIED:

| Event | Window records | `private_work_used` min–max (sum across rows) | `busy_seconds` min–max (sum across rows) |
|---|---:|---:|---:|
| 1 | 120 | 0–41,635 (1,879,372) | 0.000000925–63.7358465 s (2,472.942365 s) |
| 2 | 72 | 0–36,560 (961,920) | 0.000019032–44.371448614 s (1,347.674433 s) |

The row data allows rank/window comparisons, but these numbers do not establish that one work unit costs the same on every rank. The sums are diagnostic arithmetic over logged rows, not CPU time or elapsed wall time.

## M21 — ownership-transition memory

[failed_N1_M21_rows.csv](failed_N1_M21_rows.csv) records each rank/event final stage-exit `job_peak_rss_lower` field for traceability; the per-event values are repeated across ranks (event 1: 7,917,252,608 bytes at the construction snapshot, then run peak 14,007,463,936 bytes; event 2: 15,888,314,368 bytes at construction snapshot, then run peak 17,343,528,960 bytes). These are emitted lower-bound/cumulative peak fields, not current RSS sampled immediately before/after ownership transfer.

The requested M21 evidence is **not emitted**: no before/after `/proc/self/statm` current RSS samples, no transition-local `ProcessPeakRSS3D()` pair, and no exchange-bytes-sent / `max_exchange_work_bytes` values are present in the event-boundary rows. `NATIVE3D_STORAGE stage=ownership_staged` does print component byte estimates and `native_peak_bytes`, but those are allocator/component estimates, not RSS samples and not proof of which buffers caused the reported run-level rise. Therefore the requested transition delta and buffer attribution are unavailable. Do not infer them from the stage estimates.

All M18/M20/M21 entries in this report and CSVs remain UNQUALIFIED because the same N1 workflow failed event-2 identity and was reverted. No accepted measurement status is assigned here.
