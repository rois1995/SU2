# Report setup corrections

- Cause: the first Markdown generation loop appended the second source-accounting row after the child-parent header, then emitted that header twice.
- Correction: regenerated the summary with a complete two-row source-accounting table before the single physical/child-parent table. Checked table column counts and both run rows against `g3_pair_audit.json`.
- Cause: the existing CSV writer emitted CRLF line endings, producing Git whitespace noise.
- Correction: normalized the newly generated `wall_rows.csv` to LF without changing field content; the archived historical CSV was left untouched.

Primary follow-up: set the unpinned report helper CSV writer lineterminator to LF, so later reports do not recreate the CRLF Git-style issue. The previous helper is preserved as report_wall_runs_before_LF.py; past raw evidence remains unchanged. This report helper is outside the active full workflow closure.

- Cause: primary source-semantic review found that `ObserveMeasuredFace` increments `required_faces_` only when a measured `Selection::Add` face matches a pending child. The prior audit labeled this measured count as total registered children and treated unmeasured/unknown buckets as potentially overlapping.
- Correction: audited `CNativeWallDiagnostics3D.hpp` (SHA256 `854a7474a0e3fd0de5edb7444c8604504c87012aef829105f096e1a38cbb425a`, lines 162–180, 181–208, 290–302). Re-labeled `new_faces` as measured matched children; reported pending `children_unmeasured` separately. Parent-known and parent-unknown are a partition of measured faces. Derived registered counts as measured + pending (unmatched=0): 2924=4×731 and 2868=4×717. Updated JSON/Markdown semantics only; preserved raw extracted rows and archived the exact prior JSON/Markdown bytes plus SHA256 manifests before correction.
