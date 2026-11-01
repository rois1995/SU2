# Selected build202 M6 mesh inspection

Updated2026-11-01. The full Euler goal remains active; no adapted mesh has been
accepted. This is the deliberately selected substantial diagnostic snapshot,
not a policy to save ordinary attempts or individual trials.

## Actual saved mesh

Attempt directory:
`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/m6_runs/`
`euler_build202_real_cfd50k_inspection/`.

- [Complete SU2 mesh](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/m6_runs/euler_build202_real_cfd50k_inspection/inspection/rejected_mesh.su2)
- [Complete VTU mesh](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/m6_runs/euler_build202_real_cfd50k_inspection/inspection/rejected_mesh.vtu)
- [Bad cells](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/m6_runs/euler_build202_real_cfd50k_inspection/inspection/bad_cells.vtu)
- [Complete neighboring vertex stars](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/m6_runs/euler_build202_real_cfd50k_inspection/inspection/cavities.vtu)
- [Reference visualization](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/m6_runs/euler_build202_real_cfd50k_inspection/inspection/reference.vtu)

There are **164526points/810163tetrahedra**, independently parsed from SU2. Target
metric complexity remains50000. All6rank shards and measurements are complete;
2476bad cells and35426cavity cells were retained with zero omissions. Whole-field
counts agree with the natural EXIT:295q-bad/294J-bad/2110geometry-bad/0size-bad.
These meshes are unapproved and do not constitute an adapted solver restart.

Full SU2 marker labels are explicit fallbacks: Marker_0=FARFIELD,
Marker_1=SYMMETRY, Marker_2=WING3D. Ordering was verified against the original
version7native_ref header. No coordinate/connectivity repair was made. The
extractor reports810163positive floating-point determinants; that diagnostic is
not a replacement for the engine's exact validity predicates. Reference VTU uses
flat triangles; exact immutable controls/metadata remain in reference_records.jsonl.

Extractor exec17281 terminal0; wall437.704s including receipts, sampled summed
process RSS720207872bytes, no swaps. All31input hashes reverified unchanged. All
three extraction handles are absent. `inspection_artifacts.json` hashes all10
outputs; `inspection_analysis.json` preserves standalone parsing and selected
witnesses. Binary/mesh outputs remain local and are deliberately excluded from Git.

## What the terminal evidence changes

The final q-or-J-bad union is450cells:243have physical faces,187have boundary
bindings without physical faces,20have no captured boundary binding. Thus430/450
are demonstrably boundary constrained. Absence of a binding is not a proof of
freedom from every surrounding constraint. All2110geometry-bad cells have physical
faces. Geometry-bad remains failure of conservative admission, not a demonstrated
Hausdorff-distance violation for every cell.

All450shape-bad cells have an edge below.5. However, **32948of32950nearby cells
with all captured bad flags clear also have such an edge**. The32950controls
comprise9586surface and23364non-physical-face cells. These are neighbors in the
same complete cavity export, not matched anisotropy/variation samples or an
independent geometry recertification. Short-edge incidence alone therefore does
not distinguish the remaining bad cells or establish a coarsening defect.

Complete all-rank stage counts show the phase interaction explicitly:

| Stage/round | Cells | q-bad | J-bad | Geometry-bad |
| --- | ---: | ---: | ---: | ---: |
| Initial quality entry21582 | 727818 | 54081 | 53392 | 3770 |
| First quality exit24343 | 804776 | 48 | 26 | 2988 |
| First rebuilt quality entry24477 | 806186 | 571 | 666 | 2699 |
| Second quality exit24613 | 807745 | 38 | 22 | 2554 |
| Second rebuilt quality entry24728 | 809072 | 521 | 642 | 2219 |
| Actual EXIT24746 | 810163 | 295 | 294 | 2110 |

Every row above has complete shape and geometry measurements. Construction's
intervening partial q/J counts must not be substituted for these. The existing
continuation ran twice and shape repair resumed each time. This establishes new
repair burden after rebuilding; it does not establish repeated local cycling or
identify the exact operation that created every final bad cell.

The last full quality window used2925ladder trials for128publications and net
60q/62J removals. Collapse/reconnect accounted for2000trials and2publications,
removing net1q/4J; coordinate and optimizer stages accounted for the rest.
Category timers overlap. Counts and directions are retained in the original
service/ladder logs; private gain is not publication gain.

## Selected actual witness and next test

Cell143424775 has q=4.512914130441782e-9,J=2.642376203797157e-13,geometry excess0.
Its physical face contains4502,174173910,181199364, all mobility1, nearly collinear
on a genuine feature. Its other vertex174150582 has no boundary binding. All14
recorded repair demands are blocked. Raw current state, complete vertex-star
neighborhoods, and historical rejections are available; a historical rejection
cannot automatically explain a final cell.

An exact integer determinant test on the captured binary64 coordinates found
that4502→181199364 eliminates this cell while preserving positive orientation of
all surviving cells of4502's12-cell star. The opposite direction is obstructed.
This is **only a necessary positivity result**, not a legal collapse: surface
topology, feature traces, geometry, whole-patch metric merit, heights, contact,
dependencies and publication have not been checked for this fresh candidate.
`witness_143424775_collapse_positivity.json` records every tested neighbor direction.

Existing source already supports feature chains across original vertices and
contains `CollapseVertex`, `RebuildFeatureTraces`, `BoundaryParameterState` and
current-cavity metric imports. Do not add a competing operator or assume that
binding dimension0 means immobility:4502has mobility1. The next bounded test should:

1. Load the exact selected owned records/metrics and immutable original authority.
2. Reuse the existing feature reconstruction and parameter-state facilities;
   validate reconstructed current associations, feature paths and topology.
   Missing capture traces cannot be treated as empty authoritative traces.
3. Reproduce old q/J and relevant metric edge evaluations with the production
   current-cavity field before interpreting candidate merit. Euler has no BL
   composition here, but that does not prove every cached evaluation is recreated.
4. Exercise the existing directed collapse and complete affected-cavity gates,
   recording the actual first rejection. A rebuilt-state experiment is a local
   feasibility test, not historical trial reproduction or distributed acceptance.
5. Select one connected correction only if this identifies a demonstrated blocker;
   qualify it, then return immediately to full two-event M6 with the same saved
   real CFD seed and unchanged gates/5400s budget.

The provisional next target is **repair power and geometry/shape reclosure**.
Bulk shape repair is productive, but it leaves a predominantly constrained tail
and reconstruction creates further shape work. Certification cost remains
relevant; a geometry optimization should follow actual unresolved/expensive query
witnesses. There is presently no evidence supporting a construction q/J preference
or another coarsening pass as the selected correction.
