# Accepted N1 counter CSVs

These three CSVs are concise projections of `section6_1/accepted_counters.json` from the accepted two-event N1 run. Rows retain event, rank, record kind/status, original source path, source SHA-256, and source line. They are reports from already accepted evidence; no workflow logs were read to create these CSVs.

- `M18_accepted_counters.csv`: one row per rank/event, fields from the last engine-exit record; missing fields stay empty and are named in `missing_fields`.
- `M20_accepted_counters.csv`: quality rows only where `stage=window`, plus separate `NATIVE3D_PRIVATE_WORK` round rows. Other `NATIVE3D_QUALITY` lifecycle markers are excluded.
- `M21_accepted_counters.csv`: every before/staged/committed memory record; absent boundaries remain explicit `not_emitted` rows with blank measurements.

The N5 table summarizes these records without deriving B7. That condition remains for primary review before section 6.2.
