# Report setup corrections

- Cause: the first Markdown generation loop appended the second source-accounting row after the child-parent header, then emitted that header twice.
- Correction: regenerated the summary with a complete two-row source-accounting table before the single physical/child-parent table. Checked table column counts and both run rows against `g3_pair_audit.json`.
- Cause: the existing CSV writer emitted CRLF line endings, producing Git whitespace noise.
- Correction: normalized the newly generated `wall_rows.csv` to LF without changing field content; the archived historical CSV was left untouched.

Primary follow-up: set the unpinned report helper CSV writer lineterminator to LF, so later reports do not recreate the CRLF Git-style issue. The previous helper is preserved as report_wall_runs_before_LF.py; past raw evidence remains unchanged. This report helper is outside the active full workflow closure.
