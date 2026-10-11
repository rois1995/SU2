# Canonical planar reference lookup — 2026-10-11

Published implementation/evidence checkpoint: `31df202a90d549fc834fa8d9b16a38edef4c7f36`, origin/codex/native-3d-core; full remote SHA verified.

Branch codex/native-3d-core, predecessor e99dee967611ec960a4e9368bcd7df8b7aba1b3d. Physical binding previously scanned every original facet and repeatedly tested the same certified canonical patch. FindFace now searches canonical same-marker patches: singleton markers use direct exact containment; multiple-patch markers use SU2's existing local CADTElemClass with conservative two-corner LINE boxes, followed by unchanged exact finite containment and orientation. Lowest canonical identity remains deterministic. A missing patch leaves the caller's output unchanged; nonfinite query coordinates throw before output mutation.

Geometry certification and every q/J/L/default512/noise0/target/operator guard remain unchanged. Nonconvex patches retain original triangle constraints; the index cannot admit a triangle across a gap or fill a hole. Search does not modify original sensors or geometric BL composition. The global256-original-triangle, private64-cavity/256-donor and1MiB-sidecar limits remain enforced: this checkpoint removes an expensive binding scan as a prerequisite to lifting the global limit, not a claim that larger meshes are admitted yet.

Resident maps/vectors and tree capacity are counted. ConstructionBytes combines existing conservative component scratch with the house tree predictor; ImportGeometry admits construction, retained authority and single-rank query candidates/traversal growth before binding. The construction map/scratch model remains conservative and requires allocation calibration; this is not a peak-RSS equivalence claim. Rejection preserves published authority and imported output.

reference_lookup_regression_v1 builds the actual complete68-object test driver.67 native/reference/2D+3Dbridge cases pass per MPI1/2/4 rank, plus serial MMG2D/3D48 assertions. The new test compares direct/indexed results to exhaustive existing ContainsFace across a128-triangle tiled planar patch, disconnected same-marker patch and another plane/marker; all source faces, a spanning triangle, crossing/gap queries, reversed orientation, nextafter off-plane input, unknown marker, nonfinite input, unchanged output and requested-storage bounds are covered. No full AD build/run or all-SU2-test-suite claim.

Two actual configured n4 wedge three-event cases pass the independent exact geometry/positive-volume/incidence/overlap-gap/domain-marker/q/J/L/original-P1/current-BDF/SU2CGNS audits and six corruption controls each. All26 raw geometry/source/state/nodal-readback/frozen-readback files are byte-identical to their vertex_clip baselines, and accepted rank-local edit counts match. Source changes are limited to the boundary header/implementation, import caller and targeted boundary test. Nine production members rebuild through that header: native3D components and the single-zone factory; external/MMG/2D members remain pinned. All candidate cases and full regression use the same qualified production archives/members.

| Ranks/workers | Baseline event-parent maxima(s) | Candidate maxima(s) | Accepted edits |
|---|---|---|---|
| 1/1 | 1.280673 / 2.694163 / 1.196627 | 1.093911 / 2.404417 / 1.171225 | 101.0 / 71.0 / 165.0 |
| 4/2 | 1.193571 / 2.694954 / 1.082925 | 4.178617 / 8.702100 / 3.208181 | 101.0 / 71.0 / 156.0 |

These are single contended pilots, not sustained speedup/scaling measurements. Serial import-face samples were135.26/128.32/55.49ms before and9.04/6.12/2.18ms after; they include incidence/authority/binding work, not an isolated query timer. The CSV retains closed exclusive and separately labelled nested phases and useful accepted-work rates. Packed bytes include self traffic and collective elapsed is not pure wait. Full communication/private-wait/memory calibration, detail-OFF overhead and cluster repetitions remain open.

Requalify without rerunning meshes:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 integration_evidence/native_3d_core_v1/analyze_reference_lookup.py
```

The qualifier verifies all saved receipts, artifact hashes, production/source dependencies, configured regression,26 byte-identical files and phase closure. Inspect reference_lookup_n4_n1_m1_v1 and reference_lookup_n4_n4_m2_v1/envelope_s0/1/2.vtu plus final SU2/CGNS and state/source JSON. Test executables, objects and caches are excluded from publication; historical evidence is retained.

FullGoal1 ACTIVE. Next: reduce repeated collinear boundary constraints while retaining exact convex certification/nonconvex fallback, then replace the256-facet global cap with memory/count admission across constructor/import/decode/restart. Retain the independent1MiB-sidecar limit unless explicitly reconciled with memory admission. Add a closed same-marker/multiple-patch case as size grows, profile authority construction/search and establish first practical operator/memory/rank limits. Complete full accounting/overhead and prepare closed SGE repetitions/scaling. No curved geometry,robust3DBL,physical unsteady3D,full sensor-source persistence or CFD affordability certificate.
