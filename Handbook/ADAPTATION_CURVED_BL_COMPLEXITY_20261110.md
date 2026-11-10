# Curved BL complexity integration — connected design

2026-11-10. Same curved-BL integration cycle. Euler293 checkpoints/restart remain
preserved. Full319 terminal1/5457.353s; all10 actual identities absent, source/input
pins and mapped runtime verified. No normalized target or BL mesh was produced.

## Missing capability and evidence

The first original cell cannot complete strict curved target quadrature within
the original5400s native allowance.316/319 clear captured chord failures, and319
reduces isolated chord preparation time by about29%, but the complete target still
fails. This justifies reconsidering normalization integration rather than another
cache variant. Full wall time includes collective joining; CPU includes MPI polling.

Existing CNativeComplexity3D separates original-P1 background integration from a
retained wall correction proposal. It evaluates the actual composed metric, retains
geometry across scale trials, reports uncertainty and refines under existing limits.
CSolver already uses this for nonplanar faceted walls. Curved authorities are
excluded because PrepareCorrectionSupport3D is qualified only for triangle tubes.
Scalar EstimateComplexity3D also calls costly chord preparation; selecting it alone
does not address the demonstrated limitation.

## Proposed connected capability and scope

Qualify the existing finite-triangle tube sampler as a conservative PROPOSAL for
mapped cubic walls. The constructor already verifies exact complete original
component coverage and matching facet order, and bounds every cubic-minus-linear
Bernstein control coefficient by outward proxy_error. For a corresponding cubic
point C and linear point L, |C-L|<=proxy_error; hence distance(x,C)<=support implies
distance(x,proxy)<=support+proxy_error. Include realization/computed-distance
margins and outward addition. Preserve analytic-authority refusal until its own
support contract is qualified. A proxy is never the wall distance, normal or metric.

Use the existing support/PDF/work/memory/owner routing and full-domain continuation
mixture. True SamplePoints/ApplySample or shared FIRST_ROW TargetPolicy must evaluate
every retained sample using the original cubic authority. Every overlapping PDF
contribution stays present. Cached/recomputed current tensors must agree exactly.

Audit target fidelity before enabling: strict CSolver currently composes full
nativeLayers, while bounded assessment accepts firstRowTarget's base/A blend.
Normalization must integrate the same shared target used by construction and
reassessment. Record any corrected mismatch explicitly; sensor, norm, target
complexity, requested profile and construction policy remain unchanged.

Affected sites: CBoundaryLayerMetric SupportsCorrectionSupport3D and
PrepareCorrectionSupport3D, their header contracts; CSolver selection/diagnostic
wording and any demonstrated target-fidelity correction; existing
CNativeComplexity3D preparation/assessment callers and tests. No new config or
sampling framework, no strict edge tolerance change, no new geometry representation.

## Acceptance and risks

The existing correction uncertainty is EMPIRICAL, not a rigorous error bound.
Retain its insufficient-hit/infinite-uncertainty and normalization refusal/refinement
logic; never promote sampling to a geometry certificate. Padded coverage alone does
not prove statistical resolution: cubic sag much larger than h0 can cause poor
proposal efficiency. Actual M6 must establish useful target progress within unchanged
work/time/memory limits. If variance remains unresolved, refuse and preserve evidence.

Focused checks must include a true curved-band point outside the unpadded triangle
tube; open edges/seams, separate components, genuine features, continuation support,
fresh-versus-retained tensors and scale/profile invalidation. Check malformed support
and exhausted storage/work/time. Reuse existing tests and run ONE connected MPI
milestone including empty ranks and dependency/publication behavior. Analytic guards
must remain. Then promptly full SAMEseed M6, not another strict-cell baseline campaign.

All geometry1e-4,q>=.2,J>=.05,maxedge1.8, requested heights, positivity/body/feature
constraints and validity-first construction stay strict. Actual BL suitability needs
current-mesh curved-normal height/alignment, progression, coverage, outer transition,
tangential flow conformity and junction/LE/TE behavior after general operators and
migration. Then TWO RANS500k events, conservative admissible flow+SA transfer,
continued CFD and actual isolated adapted disk restart. No current BL acceptance.

##320 implementation under build — unvalidated

Four compiled files changed: CBC header/source, CSolver diagnostic, existing BL
metric test. SupportsCorrectionSupport3D admits only complete mapped cubic support
(already constructor-verified), retaining analytic refusal. The proposal radius is
outward thickness+proxy_error+maximum_realization_error+1e-6*h0; all distance/normal/
tensor calls remain unchanged authority queries. Existing CSolver selection now
reaches CNativeComplexity3D with its shared FIRST_ROW TargetPolicy. This corrects
normalization's use of the older full-only callback relative to construction; record
trajectory/normalization differences, not an identical-integral timing comparison.
No new estimator/caps/physical profile/construction operator.

Added checks in existing cubic fixture: active curved-band point beyond unpadded
planar tube, nonzero padded PDF, exact direct/retained tensors, work/storage refusal,
retained bounded assessment across changed sensors/cores and exact return-to-original
assessment. Rank0-only source ownership deliberately leaves other ranks empty.
Build320 currently frozen; no passing qualification or actual M6 benefit yet.

Build320 passed246.232s/all11 observed actual host identities absent. Before any
qualification, review identified two gaps addressed in321: pre-admit temporary
proposal vector copies and deduct their peak allowance from sampler admission;
exercise mapped cubic shared FIRST_ROW base/A target plus outer-continuation mixture
and cached/direct profile/core changes with empty source ranks. Build321 frozen;
no tests or actual M6 benefit claimed. Keep empirical uncertainty explicit.
