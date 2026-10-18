# Original-family frozen normalization stages

2026-10-17. Full Goal3 ACTIVE/incomplete; base d9daf0e78f.
[Source, independent audits and resource receipts](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/frozen_normalization_stages_v1/README.md).

## Complete connected capability

For provisional native3D initialization, an accepted PRIVATE first-stage candidate
can reassess the same original sensor-only P1/actual-query BL field. A material
budget discrepancy beyond its heuristic indicator permits one bounded correction
and a second private remeshing stage. Local operations see one frozen target per
stage. There is one final reader return, conservative flow/BDF/SA transfer and
accepted reference publication. Geometry, prescribed height, positivity, q>=.2,
J>=.05, full composed edge<=1.8, finer demands, fade and noise0 remain unchanged.
No testcase/solver-transition/adaptation-number dispatch or new config knob.

ComputeMetric retains original UNCLIPPED eigenframes/eigenvalues, original CFD
nodes/coordinates, selected sensor, bounds/gradation/budget and geometric authority.
Replay bypasses sensor/Hessian construction, reruns existing bounds and gradation
on the ORIGINAL CFD mesh, then restores its selected metric and diagnostics. The
original donor geometry stays fixed; only its values change at the explicit stage
boundary. Rescaling already clipped donors or refitting from candidate nodes is
not used. No predictor history, flow, turbulence or BDF state is changed by replay.

The candidate cloud retains geometry/identity-only original-publisher stencils.
Exact barycentric transport is shared with the original donor codec. Actual-point
wall geometry is prepared once; trials reevaluate current original-node P1 and
BL/current-core composition. No donor tracing, source-index rebuild or wall search
occurs DURING trials. Preparation, source verification and initial composition are
still work. Physical-source spans handle generated collars even with unused CFD
IDs. Optional eligibility is collective: a prediction/aggregate that differs on
only some ranks skips correction everywhere, rather than rejecting on job mismatch.

Limits: at most2 mesh stages/24 root callbacks,8192 points per assessment, shared
mesh-work/deadline/native-storage ceilings. The uncertainty is3*max(point SEM,
eight-block SEM) plus a first-order conditioning indicator. It is HEURISTIC, not a
proven bound or calibrated tolerance. Repeated same-scale calls do not claim more
accuracy. Hard scale bounds, unknown integration and a residual after stage2 remain
explicit. Initial successful strict policies retain their path. 2D is not replaced.

## Actual results under unchanged constraints

Original nonhomogeneous clipped/graded family: mass21.76446 ->20.00000 +/-0.04471;
independent positive-Duffy24/40 reference19.99505882, spread9.6e-14, error.0247%.
Raw frames/Hessian/CFD metric/flow/SA/BDF and source bytes remain unchanged after
replay. MPI1/2/4 global results agree within the declared2e-11 criterion. Alias,
empty publisher, storage, rank-mismatched eligibility/callback and deadline faults
pass. Numerical conditioning is scale invariant in primitive controls.

Actual initialized RANS/SA cube with deliberately injected initial sensor scales1
and10 tests both correction directions from the SAME raw family (not natural M6
integration errors). All8 accepted workflows pass: N1/M1,N2/M2,N4/M2,N4/M4,2 directions
each. Final meshes151--240 tetrahedra, work433--1374<=8192,6--7 root calls. Independent
q>=.209564,J>=.069688,L<=1.795220, exact unit volume/no overlap/conformity/height,
full actual-query field and original sensor transport pass. Current+BDF conservation,
SA admissibility/history, SU2/CGNS geometry/target reload and real CGNS flow restart
pass. A shared-cap rejection at77 operations preserves accepted CFD/reference state.
These are planar controlled workflows, not general curved-wing qualification.

Independent final complexities19.93549--19.96127 versus requested20: discrepancy
0.194--0.323% low. Final estimate errors.00107--.06307 lie inside empirical indicators
.08999--.22692. All root results remain integration_limited; no1e-6 accuracy claim.

Same corrected original field before/after ACTUAL adaptation at8192 points each:
refinement reduces indicator3.07--4.07x, observed error5.48--77.70x. Coarsening gives
indicator ratios.915--1.206 and error ratios.824--.997. It need not improve variance;
there is no monotonic-accuracy claim or additional subdivision/sampling mechanism.
See `same_field_stage_comparison.json`. No independent whole-M6 continuum reference.

## Cost, diagnosis and remaining scope

Serial correction phase.035--.040s; distributed rank0 phase.037--.044s. Serial
normalizer counters49152/57344 density evaluations,8192 geometry preparations;
source/wall queries during replay trials0. Native serial lifecycle cases24.94s
combined; independent pilot69.35s. MPI campaign219.61s includes independent audits,
with the exact sealed serial evidence reused. These tiny contended cases do not
establish scaling, speedup or unsteady affordability. Source preparation, geometry,
repartition/migration, both mesh stages, return and transfer remain event costs.

Exclusive phases and cumulative_work cover both stages; Engine/Bridge counters
mostly describe the last stage. Printed rank0 correction query counters are LOCAL.
normalization.changed describes a proposal: stages==2 establishes its application.
A shared-cap rejection can have changed=true but only1 stage and no publication.
Snapshot has a2GiB per-rank modeled ceiling; allocator/MPI/CFD memory is separate.
Deadline checks occur at integration/mesh phase boundaries, not preemptively inside
an arbitrary eigensolve/gradation sweep. Full total cost and memory need wider cases.

Initial syntax errors, work-cap fixture with missing runtime marker order, and a
Path-type reporting-wrapper failure before MPI are preserved. Fixing the fixture
required marker tag/kind/send-receive bindings, not weakened mesh checks. Exact
executed source/provenance and sealed reuse avoid repeated CFD or full regressions.
One heavy job/nice19/thread1/build-j1/MPI<=4. Largest build process2.40GiB, pilot~50MiB,
MPI campaign~48MiB process maximum; not aggregate peaks. Host~38--41GiB available,
816MiB existing swap unchanged, GNU time swaps0. Compile temporaries on4TB.

M6: historical coarsest estimate790200.50 +/-27958.45 versus100000 remains provisional
and far outside its estimated budget. A scale already at its hard lower bound cannot
be reduced by this controller; reassessment may improve the estimate, not eliminate
physical BL demand. No unchanged300s M6 retry, new CFD time advancement or cluster run.
The previous complete construction failure remains separate from integration success.

Open: accepted complete M6 and general curved/thin/interacting construction, global
error calibration/allocation, 2D correction coupling, predicted/aggregated raw-family
representation, viscous accuracy and whole-event affordability. Earlier successful
curved tests retain their own source-qualified scope. Broader donor-campaign/edge
retention and transaction/balance work is not completed by this normalization cache.

## 2026-10-18: same-field M6 private-refinement hypothesis

Original303359 versus saved REJECTED541027tets,8192samples each: estimate312124.65
+/-51830.47 versus285516.06 +/-31476.26. The indicator falls39.27%, cost rises
4.779->8.154s; source queries311551->549219. Same original selected-UPPER6 donors,
actual-query BL/profile/core/fade, no interpolation onto finer nodes and no
normalization/CFD advance. Independent existing wall-focused proposal on that SAME
field gives298571.52 +/-5765.29,32768samples/disjoint seed. Both estimates agree
within heuristic indicators; their similar observed deviations do not establish
an accuracy improvement. The tighter wall estimate remains heuristic, not an exact
reference or a demonstrated correction. MPI4 global results agree exactly.

Finite domain volume/face-incidence/ownership screens pass; no full accepted M6,
quality/overlap/transfer proof follows. This partially supports variance reduction
on an actual private refined partition, not cheaper or reliable quadrature after
adaptation. Do not replace localized original-wall integration with more cells or
claim RAW9/UPPER6 field equality. [Complete report](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/m6_same_field_reassessment_v1/README.md).
