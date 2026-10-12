# CFD-owned curved exterior sensor coverage

Working repository: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch: codex/native-3d-curved-local; Goal 2 active, all four integrated gates pending.

The actual CFD geometry importer now generates finite exterior sensor cells before
freezing the field. Bump boundary faces produce vertical triangular prisms; sphere
faces produce radial frusta. Each prism/frustum uses three compatible tetrahedra,
with globally agreed primitive transforms and disjoint transient identities. New
nodes copy the original boundary-node sensor tensors exactly. Original physical P1
retains priority wherever covered; extension never composes or spreads BL tensors.
The existing exact layered edge integration supplies consistent exterior segments.
Generation and storage/identity overflow checks precede publication. Admission or
orientation failure preserves the previous reference/output and accepted CFD state.

Source records now carry their original CFD parent. Repartitioning co-locates each
exterior record with that parent for M=N and M<N; the parent-routing map is admitted
using the original destination count, not the already moved import container.
Engine and reader field construction associate received exterior records with the
supplied analytic authority. Complete private projected-candidate source footprints
and query-specific component handling remain work to finish before runtime support.

The sensor arrays in the fixtures are prescribed on real CFD-owned geometry. This
is not a solved CFD-derived-metric adaptation cycle. Bump tests preserve the original
inside query/weights, exact exterior nodal values, reverse edge integration, finite
envelope refusal and collective budget rollback. Sphere tests exercise one original
CFD tetrahedron and its twelve automatically generated exterior tetrahedra. Actual
backend repartition/M<N reaches the existing analytic runtime refusal without
publishing a replacement. Existing planar driver/transfer/output/restart controls
remain in the selection; their success is not curved-transfer proof.

Evidence: generated_sensor_v1 and generated_sensor_v2. The independent generated
source auditor checks positive exact volumes, exact separating-axis no-interior-
overlap, shared identities/coordinates/tensors, parent boundary-face provenance,
finite transform and MPI1/2/4 physical geometry, marker/component footprint and
sensor consistency. It writes directly inspectable generated_bump_source.su2 and
generated_sphere_source.su2 beside the source JSON/native reference. These are
metric coverage envelopes, not adapted CFD grids. Existing private operator grids
also receive independent shape/sliver and signed-skin closure audits.

The initial auditor incorrectly required numeric facet IDs invariant between MPI
sizes. Its source, failed log and receipt remain in v1; the corrected auditor uses
physical provenance across runs and retains exact identity checks within each run.
The immutable v1 receipt also contains stale scope prose saying automatic generation
is unimplemented; SCOPE_CORRECTION.md records the correction without rewriting
its source/binary/log pins. V2 fixes migration-map admission. No geometry, metric,
quality, BL/fade/NOISE0, solution-transfer, MMG or existing 2D gate was relaxed.

Next substantial implementation step: geometry-driven selection/completion,
actual projected-candidate metric routing and nonlocal clearance, exercised together
on one end-to-end curved runtime case. Cylinder, mixed curved/planar expansion,
actual curved MPI publication/state/history/output/restart and solved Euler metric
cycles remain pending. No cluster, profiling, scaling or affordability claim.
The permanent AGENTS.md cadence groups these connected changes and reserves broad
regression for integration milestones; small edits do not each create a campaign.

Final v2 result: PASS,97test cases per rank on MPI1/2/4, full68-object linked driver;
independent generated_source_validation.json and private_shape_validation.json PASS.
publication_integrity.json verifies every original controller artifact after compact
archival, and v2 source pins against the current worktree. Build/test timings are
receipt data only, not a performance assessment. All owned processes are terminal.
