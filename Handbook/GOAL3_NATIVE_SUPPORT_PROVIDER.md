# C++ support provider and actual original-donor ownership

Full Goal3 remains ACTIVE/incomplete. Previous pushed base d76612c36cd4c9b5b941c922fbc0de918374f498.
All relative evidence paths below are in
`integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/native_support_provider_v1`.
This is a qualified production-library API and original-field replay, not an
activated CSolver normalization policy or an accepted adapted mesh.

## Implementation and unchanged field

`Common/include/adaptation/CNativeSupport3D.hpp` implements bounded faceted-wall
support sampling using the house CADT box index. Two-sided face prisms, edge
sectors and enclosing vertex lunes cover the finite-triangle tube. Full mixture
PDFs sum ALL covering components, including different walls. Stable common-plane/
half-angle frames avoid the earlier coplanar degeneracy. The same deterministic
stream can be extended by prefix ranges, allowing later bounded accuracy work.
Tangential geometry weights and reciprocal-height profiles affect importance
only; actual target density still comes from the unchanged native BL composition.

`CBoundaryLayerMetric::PrepareCorrectionSupport3D` builds the provider from its
actual model/search triangles. It requires faceted3D walls, growth>=1 and T>=h0.
Analytic authorities explicitly retain their existing integration path: no proxy
substitutes for the physical model. Storage, candidate and total work checks throw
rather than silently truncating PDFs. Maximum stream extent1048576, per-query
candidate ceiling32768; tested replay uses80M work/2GiB sampler budget. These model
payload/work, not total CFD/process RSS or an outward floating-point error bound.

`DonorPatch::source_rank` identifies the actual publisher of each returned cell,
in the source's World and aligned with canonical cell order. Ownership must be
rebuilt after source migration; it is not the working-mesh partition. The existing
admitted donor response carries this metadata without changing donor sensor values,
strict coverage, tensor interpolation or canonical ordering. Result-allocation/
metadata errors use the existing final vote, adding no collective per import.
The wire adds one integer and a local result copy; full-remeshing cost is not yet
measured. The replay now routes through returned owners, with deliberately shifted
storage(id+1)%ranks, so the old ID/rank assumption cannot silently pass.

The same batch votes root baseline-output failure before further collectives and
checks normalization controls agree across ranks. Fault coverage is denied/open
failure and one-rank control disagreement, not every disk-full/close-time failure.
No geometry/height/positivity/quality/transfer/full-edge acceptance gate changed.

## Evidence

`summary.json` pins the complete campaign.21 serial controls/10129 assertions cover
new square tube/zero coplanar diagonal, overlapping walls, growth1/near1/1.2/2,
stream-prefix identity, invalid geometry and work/storage refusal plus affected
3D/2D composition and existing normalization controls.4 donor/regional controls
pass serial and MPI4 (125 assertions per rank), including actual shifted owners.

Every one of8192 points per native preparation is independently checked against
the Python proposal using its different KD-tree broad phase:

| Case | Native components(face/edge/vertex) | Maximum relative PDF difference |
|---|---|---:|
|Finite square|2/4/4|2.220e-16|
|M6 seed2641|38800/58305/19506|3.636e-12|
|M6 seed9919|38800/58305/19506|3.630e-12|

Native square proposals also match the independent complete tube volume. Its
original2I field and every normalization trial agree with independent coarea
integration. Planar MPI1/4 query rows agree exactly. M6 MPI2/4 query/source/density/
weight rows and normalization agree exactly for seed2641. Every5391/5428 retained
inside-source query of the M6 seeds has bitwise-identical direct/cached composed
tensors at the unscaled preparation field. Raw hosting-rank9 coefficients are kept;
identity metadata is only the locator, never the physical sensor field.

The M6 correction estimates801696.2557/816508.4502 have empirical SE9422.0561/
9416.5831. Their mean plus the independent sensorQ18 is962096.5539. The normalizer
uses the full analytic-background interval instead of treating Q18 as exact: its
midpoint totals are950043.0/964855.2, with heuristic total uncertainty~34190.
These are the SAME baseline field but different background point estimates.
At selected factor1.148698355, target1M gives985771.9921/1000964.5020 and heuristic
uncertainty35658.8908/35661.6554. Both meet the explicit experimental5% criterion in
3 trials. This is no proven global bound or chosen production default tolerance.

Both focused fault cases terminate collectively in0.87/1.46s, no timeout and no
normalization after failure. Native/Python PDF validation and original-field
queries use the saved M6 wall/source; no CFD, new reference solutions or mesh
construction runs occurred.21/4 controls are a connected focused batch, not a
full repository regression or unsteady lifecycle qualification.

## Work and resources

M6 seed2641:798785 preparation work,14.711M indexed PDF candidates,126567576 bytes
retained by the sampler. Native preparation0.4892s/PDF4.6782s. The independent
Python preparation/PDF validation of those SAME points takes23.70s. Native invoke
also includes physical-wall model build, point generation and output; do not
present the kernel ratio as complete-remeshing speedup. Second-seed receipts
remain separately inspectable. This is a measured integration-infrastructure
improvement, not donor-campaign retention/shared metric-edge caching/batched edits.

Original23-object build11:07.55/peak process2.36GiB. Final donor object/archive/link
refresh34.06s/2.38GiB; it was necessary because final-vote consolidation followed
that source's first compilation. Both completed BEFORE testing. Campaign2:14.62/
667MiB maximum process, recorded swaps0. One owned heavy job/nice19/thread1/
build-j1/MPI<=4, host38–40GiB available. Process peaks are not aggregate MPI memory.
Host pre-existing swap changed727->747MiB during this interval; no attribution to
owned jobs is established. All owned handles are terminal and foreign jobs untouched.
Root/tmp pressure is avoided using build-integrated-v2/compile-tmp on4TB.

Execution proofs pin native source/binary/config/inputs/command parameters.
Verification helpers are separately fingerprinted, so a changed assertion can
recheck saved native output without rerunning valid execution. Saved campaign.py
runs cases sequentially and is resumable. Restore omitted128MB original data with
`support_correction.py domain`, whose digest/manifest preservation was validated.

## What remains

CSolver still uses its old strict per-cell integrator and scale search. Next connect
actual bounded/graded trial values to retained geometry-only source stencils;
homogeneous cached sensor answers are insufficient when clipping/gradation changes.
Define the global accuracy/work policy from evidence, then explicit frozen remesh/
original-field reassessment/correction stages. An incomplete or outside-budget
estimate must remain visible. No5% default, silent target change, coarse-cell wall
spreading, hard-normal reset, accepted M6 grid or lifecycle speedup is established.
Do not rerun unchanged300s construction or add a subdivision/sample-budget ladder.

Separate source observation for the later composed-field audit: faceted3D wall
samples pass curvature=0 to WallMetric3D; only analytic authorities currently
supply a directional curvature cap. Shape is sampled at cell centroid while full
metric edge integration is checked separately. Whether rapid normal rotation near
sharp finite edges limits construction/cost is UNTESTED; it is not a diagnosis of
the saved stall. Investigate before changing the field, and never change the
original frozen metric to make this integration experiment pass.
