# Latest selected M6 inspection: build225

Updated2026-11-06. Actual selected first-event rejected mesh:
`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/m6_runs/euler_build225_entity_shell_real_cfd50k/inspection/`.

- `rejected_mesh.vtu`: COMPLETE193855points/987865tetrahedra, with all193855nodal
  `captured_nodal_metric` tensors, nine row-major components and availability flags.
- `rejected_mesh.su2`: same COMPLETE mesh and exact point coordinates, fallback
  boundary markers Marker_0/1/2. These are visualization/export labels.
- All full-mesh q/J/geometry measurements complete:24200q-bad/21065J-bad/
  1784geometry-bad/0size-bad. This is UNAPPROVED; the event rejected its time budget.
- `bad_cells.vtu`: bounded PARTIAL32768of37353all-gate bad cells,4585omitted.
- `cavities.vtu`: bounded PARTIAL262144cells,177089neighboring cells omitted.
- `reference.vtu`: COMPLETE48954facets; exact controls in reference_records.jsonl.

Independent meshio5.3.5 reads all4VTUs/SU2, verifies every array length, full counts,
finite symmetric positive-definite tensors and exact SU2/VTU point agreement.
Artifact hashes/read receipts: attempt/inspection_artifacts.json. Export428.700427s,
reader3.378982s; all solver/export handles terminal. No ParaView GUI qualification.
The unchanged qualified binary Float64 writer avoids the historical ASCII NaN issue.
Surface extraction inherits the same3Dtransported nodal field; tangential restriction
is used for boundary movement. This is not proof of immutable continuous-background
metric fidelity. No accepted mesh, adapted transfer or disk-restart proof exists.

[Terminal outcome and next analysis](ADAPTATION_BUILD225_OUTCOME_20261106.md).
This was selected as a substantial coupled-boundary implementation checkpoint;
ordinary attempts and individual trials are not automatically exported.

## Previous selected corrected-target219

Its complete mesh remains saved in
`staged57/m6_runs/euler_build219_local_metricL1_real_cfd50k/inspection/`:
197550points/1011596tets, nodal metric complete;10286q/6872J/1753geometry-bad.
Full mesh/bad set complete; only its bounded cavity view is partial. That run also
failed the strict budget; it supplies same-seed correctedL1comparison evidence.

# Historical build217 inspection

Updated2026-11-06. Historical selected first-event rejected mesh:
`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/m6_runs/euler_build217_real_cfd50k_inspection_metric/inspection/`.

- `rejected_mesh.vtu`: complete159281points/790537tetrahedra, with all nodal target tensors.
- `rejected_mesh.su2`: same complete volume mesh, fallback marker labels.
- `bad_cells.vtu` and `cavities.vtu`: PARTIAL42bad/685neighboring cells.
- `reference.vtu`: reference visualization; exact controls in reference_records.jsonl.

The event naturally failed the unchanged native time budget. Final cache classification
is unavailable on5ranks/589332cells, so missing q/J/geometry values are IEEE NaNs;
zero partial EXIT q/J counts do not establish closure. Geometry gates remain strict.
Full mesh and all159281captured_nodal_metric tensors ARE complete. This M6 run uses
current_nodal/full_target1; the captured tensor is the current transported nodal target.
Surface extraction retains the same3Dmetric, with surface motion using its tangent
restriction; there is no separately fabricated surface field.

The user reported ASCII NaN reader failures. Floating arrays now use inline binary
VTK encoding; FieldData is at UnstructuredGrid scope. Original ASCII artifacts remain
as *.vtu.ascii-original. Corrected mesh/metric values and connectivity are unchanged.
Twelve exporter tests pass. Installed meshio independently reads the actual files;
artifact hashes and array/mesh checks are in inspection_artifacts.json beside the
inspection directory. VTK GUI itself was not available in this environment.
This is an UNAPPROVED diagnostic mesh, with no adapted transfer/restart proof.

# Historical build202 inspection

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


## Connected witness cycle — build206 scoped qualification passed

The selected diagnostic now reconstructs the complete current boundary using the
existing feature-path and parameter-state facilities. An initial100m work allowance
was insufficient for the diagnostic. Using the production per-face reservation,
204then hit its unchanged600s fixture limit in repeated whole-map resource-size
callbacks, before evaluating a collapse. These are preserved diagnostic failures,
not evidence that the M6 repair is infeasible.

205replaces those repeated size walks with exact checked retained-byte totals and
four phase-boundary full-scan invariants. Same allocations, overlap admission,
arithmetic, work charges and geometry decisions. Focused projection_paths passed
34cases/2666assertions, including one-byte-below-peak refusal and exact-peak
admission. Actual captured_cavity_20261101T152837_2113440 passed38872assertions in
90.230s:760current genuine traces validated, all52cachedq/J reproduce bit-exactly,
maximum metric edge error2.4042183174866061e-17. No ordinary-M6 speedup is established
by this setup result.

The4502→181199364collapse constructs a positive12→9cell replacement, but parameter
preflight rejects genuine feature coverage before fresh metric/guidance evaluation.
New trace174173910/181199364 belongs to an already retained physical edge. Its
unchanged companion face181199364/173349490/174173910 in cell143424774/facet16300
is outside4502's complete vertex star. The directed proof sees changed faces but
omits this newly induced trace use on the unchanged face. The strict unsigned
whole-curve proof remains necessary.

206implements a narrow correction: when a previously untraced accepted physical
edge receives a genuine trace, include its unchanged accepted incident face's
read-only directed contribution. No companion volume/face edit, degree update or
vertex move; explicit bounded scratch; existing non-equal old-route coordination,
exact unsigned feature coverage, topology, revisions and reservation unchanged.
The endpoints are already in a valid candidate's full-vertex footprint. Current
replicated parameter state supplies the accepted companion and immutable entity.

A self-contained bent-feature regression covers a4→2cell collapse, a companion on
another rank, an empty sixth rank, lost-arc refusal, clearance/publication and
revision/stale-revision checks. The actual witness requires replacement and rejects
its missing-arc variant; fresh metric results explicitly report whether measured.
206build passed261.348s with1298pins unchanged. Serial publication153assertions/
4cases, actual captured witness38878assertions/1case, and ONE MPI6 milestone23cases
on EACH of six ranks all passed. All owned handles are absent. Actual witness
passes construction/replacement/metric/guidance with zero geometry excess; q/J-bad
remain1each, debt1.99999995486→.15423814413. This removes a demonstrated refusal,
not the full repair burden. Actual global contact/publication is not checked here;
the self-contained distributed regression checks them separately. Evidence is in
staged57/feature_proof206_qualification.json. Relevant205source/helper snapshots
were recovered and SHA-verified against original pins for resumable history.
Return immediately to normal full two-event M6 with saved CFDseed/50k/5400s. Do not
capture every rejected run:202complete saved mesh remains available.
Final geometry/shape/size/heights and transfer/continued-CFD/restart gates unchanged.


The connected source checkpoint12a14dc955 is pushed. Normal full two-event M6
`euler_build206_real_cfd50k_feature_companion` naturally rejected event1 at the
unchanged time budget:809684cells/281q-bad/341J-bad/2129geometry-bad/0size-bad.
All6 EXIT measurement/census flags are complete. Three quality stages and two
construction returns were exercised; neither small intermediate shape tail closed.
Natural failure and pins/handles were preserved before the exact owned-launcher
stop. All10exact handles are absent and1403pins unchanged; HANDOFF_Codex.md is
current authority. The selected202mesh remains the inspection artifact. Use the
terminal counters and existing geometry facilities before selecting another fix.

## Next selected export: captured metric tensors (2026-11-06)

The user requests the metric in the next substantially useful selected rejected
mesh, not a new mesh dump per attempt. The offline exporter now writes
`captured_nodal_metric` as nine row-major symmetric tensor components in PointData
and `captured_nodal_metric_available` as UInt8. Missing captured values are NaN;
conflicting shared-node tensors fail export instead of being averaged. Full mesh,
bad-cell and cavity VTUs carry the field. Extracting the surface retains it.
The existing selected202files have not been rewritten.

This is the **current transported nodal sensor**. Current M6 receipts report
`full_target=1`, so it is the nodal target for this run. It does not reconstruct
original continuous-field values or any query-time composition in other modes.
Surface and volume use the same3Dtensor; permitted surface displacements see its
tangential restriction. Geometry, body, feature and requested-height checks remain
separate constraints. No fabricated independent surface metric is exported.

All12offline-export tests PASS, including anisotropic off-diagonal tensor order,
cross-rank shared-node consistency, missing tensors and existing partial-capture
behavior. Receipt: `staged57/metric_vtu209_qualification.json`. These files are
outside live build209pins; all1301frozen build inputs remained unchanged.
