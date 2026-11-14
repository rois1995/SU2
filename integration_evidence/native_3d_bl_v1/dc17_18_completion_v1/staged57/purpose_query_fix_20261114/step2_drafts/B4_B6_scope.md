# B4/B6 source review — scratch only

No implementation or validation. The full P5 workflow owns the frozen source.

B4 safe reuse must preserve the original arithmetic and ordering:
- Selection CellDebt q/j are separate normalized hinge squares. OldMerit uses NormalizedShapeHingeDebt; substituting their sum is not equivalent.
- Raw q_value/j_value exist, with measured flags and Diagnose version checks. Reuse these through the existing OptimizeOptions.mesh_quality hook only when the complete target, current owned version and exact ordered cell match.
- OptimizedRepairPatch renumbers imported old cells to local IDs before OptimizeMetricMesh. A callback must map those local IDs to original IDs, and must not return stale quality after an optimizer publication changes a local cell.
- OptimizedRepairPatch is called on a candidate patch (Engine ~4171-4182), which may already be fresh, not accepted census input. A local-id callback needs an exact local-to-original mapping and current-local-cell coordinate guard; many candidates have no valid census row.
- Fresh changed cells still need fresh measurements. Existing edge certificate reuse is not a q/J certificate.
- CellDebt.geometry is a cell aggregate in physical-slot order. GeometryGuidanceExcess is a face sum in patch.surface order, with private parameter context. geometry_scores are edge priorities, not face errors.
- Therefore aggregate census geometry cannot replace old-face scoring. Exact reuse requires matching per-face facet, vertex order/coordinates, bindings, authority and tolerance, plus preserving the original sum order. Existing numeric memos already cover repeated identical low-level geometry questions; do not invent an aggregate identity.

B6 needs more than serializing migration rows:
- CachedRow/CellState serialization and Restore already exist. Restore checks cell/version/vertices and rebinds edge certificates to the current event-local FrozenMetricIdentity.
- FrozenMetricIdentity is an event-local token, not a persistent content fingerprint. StageProgressView explicitly says its borrowed composition/breakpoint functions are not a serializable field specification.
- A resumed census must match the complete frozen target, normalization/profile/stage, current geometry/parameters and owned input, and relevant acceptance/measurement controls. Sensor tensors alone are insufficient.
- Reuse existing complete checkpoint provenance and reconstruction rather than defining a second target representation. Only issue the new event identity after that contract is verified.
- Preserve partial measured flags and unknown rows; a partial saved census cannot become complete on load.
- Do not promise exact scheduler replay from census rows alone. Rebuild queues deterministically or save the additional required state and test its scope explicitly.
- Regression acceptance must compare fresh and resumed complete measurements/decisions, reject changed target/geometry/controls or corrupted records, and demonstrate that validated census work is not repeated.

Primary source locations: CNativeEngine3D.cpp Selection CellState/CellDebt ~289-313, Diagnose ~421, Add ~787-855, Restore ~975; OptimizedRepairPatch ~1115; RowQuality/OldMerit ~6195-6242. CNativeEngine3D.hpp StageProgressView ~58-83. CNativeField3D.hpp FrozenMetricIdentity ~26.
