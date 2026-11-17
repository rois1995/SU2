# First G3 report corrections

- Cause: source review showed `ObserveMeasuredFace` increments `required_faces_` only when a measured `Selection::Add` face matches a pending child. The old report incorrectly labeled `new_faces=2916` as the full registered count and implied possible overlap between unknown-parent and unmeasured children.
- Correction: using `CNativeWallDiagnostics3D.hpp` SHA256 `854a7474a0e3fd0de5edb7444c8604504c87012aef829105f096e1a38cbb425a` (lines 162–180, 181–208, 290–302), relabeled it as measured matched children; separated pending registered children; and recorded parent-known/unknown as a partition of measured children. For this run, 2916 measured + 8 pending = 2924 = 4 × 731 required accepted splits; unmatched=0. Updated audit JSON and Markdown only; raw wall rows were unchanged.
- Exact prior `first_g3_audit.json` and `first_g3_audit.md` are preserved compressed with original SHA256/byte lengths in `pre_child_semantics_correction_20261117/manifest.json`.
