# Queued leading-edge target/delivery investigation

2026-11-10 user instruction. Part of the existing curved surface-BL cycle, not a
competing implementation. Trigger: its FIRST representative saved actual mesh/patch,
including a meaningful rejected result. No current BL mesh has been produced.
Keep frozen-run discipline and let the active work-attribution outcome drive work
until that trigger. Preserve qualified Euler293 checkpoints as controls.

## Paper review and limits

[Coulaud, High-order CAD-based surface mesh adaptation with the log-simplex method](https://coulaudolivier.fr/docs/Articles/Coulaud_2025.pdf)
was read through focused primary-PDF extracts on2026-11-10. Sections3.1–3.3 construct
geometry demand in a physical tangent frame using a local surface-gap function.
Section4.1 treats geometric curves separately. Section4.2 extends a surface metric
in its normal direction; equation21 chooses the larger tangential eigenvalue there.
Section4.3 assumes known parameter coordinates, interpolates parameters for inserted
vertices and avoids repeated CAD projection. Section5.1 evaluates geometry error
with quadrature in an L2 norm. These are design references, not measured benefits
or equivalent acceptance criteria for our implementation.

Our interpretation: the normal extension does not prescribe a BL first height.
Its aggregate quadrature error cannot replace our strict geometry certification.
Known surface parameters may help surface operations; they do not supply a closest
foot/distance certificate for arbitrary off-surface volume queries. CAD dependence
and high-order mesh elements are outside the authorized change. The2024 log-simplex/
DG paper is supporting context only, not a reason to switch L1/L2 or Hessian recovery.
L2 recovery/projection and an Lp adaptation error objective are different choices.

Completed329 target evaluation is provisional/outside500k estimated bounds.
[Current metric-stage diagnosis](ADAPTATION_CURVED_BL_METRIC_STAGE_DIAGNOSIS_20261110.md)
records inherited tangent-policy evidence and unresolved attribution. The saved
initial flow VTU is still the original mesh and exports sensor-only floor tensors;
it is not a representative generated BL mesh or its effective composed metric.
The queued complete-patch investigation remains pending that first actual result.

## Bounded evidence plan

Use existing diagnostics, actual saved fields and complete supports first. Choose
smooth leading-edge patches at representative span stations plus genuine-feature
and root/TE transition controls. Keep each patch's adjacent volume cells and full
reference support; do not inspect isolated surface triangles alone. A representation
or CAD-patch seam is not automatically a physical feature. An association on a
reference edge alone is not evidence of a genuine geometric curve. Use declared physical
features, body/association rules and sided geometry. Retain exact reference identity,
coordinates, bindings, configuration, build/runtime and saved-flow provenance.

1. **Geometry demand.** Evaluate existing no-CAD differentials at valid associations:
   principal curvature magnitudes/directions and their orientation relative to the
   around-nose and swept-span directions. Treat nearly repeated/zero eigenvalues
   as directionally ambiguous. Determine tangential resolution required by the
   existing geometry tolerance using current certification facilities. A local
   curvature/sagitta estimate is a diagnostic scale, not a conservative substitute
   for full-patch certification where curvature or support changes.
2. **Combined target.** At identical physical points, record flow, geometry and
   requested-height contributions separately, then actual composition and gradation.
   Use the production FIRST_ROW full/base/blend evaluator where applicable. Label
   nodal sensor tensors separately: their VTU restriction alone need not represent
   the complete composed BL target. Inspect tangent-restricted eigenpairs, normal
   demand, exact production directional edge quadrature and gradation ratios. Test
   whether short around-nose spacing and longer spanwise spacing survive each stage;
   do not infer pointwise target stability from regional distributions.
3. **Realized mesh.** Compare directional metric lengths, orientation and q/J on
   complete affected surface/volume patches and neighbors. Classify poor placement,
   actual feature/body/height constraints, insufficient connectivity, missing support
   or opportunity, true geometry regression and conservative certification refusal.
   Reuse existing collapse/reconnection/relocation and rejection evidence before
   considering any new facility. Include a successful/clean control.

Assess requested first height and curved-normal alignment, normal progression,
coverage/outer transition and tangential flow conformity. Recheck actual mesh
properties after splitting, coarsening, relocation, reconnection and migration;
construction registries alone do not prove surviving coverage. Report justified
feature/junction transitions explicitly rather than requiring perfect orthogonality
everywhere. Record complete-patch/neighbor geometry failures and excess, q/J failures
removed/introduced, metric conformity, point counts, work, useful progress/time and
memory. A smaller mesh or a higher cache-hit count is insufficient.

## Frozen330 capture-readiness audit

Read-only source check while330 remains live: NativeSelectedFailureWorkflow3D
installs FailureCapture only on the first rejected engine exit. It preserves raw
OwnedCell records (including transported sensor tensors), cached q/J/geometry/edge
diagnostics and the reference. FailureCapture explicitly does not recompute metric
or geometry values. Its tensors therefore are NOT an export of the effective BL
composition. An absent/unknown cached value must remain unknown.

The existing ActualCubicBLPolicy3D point probe evaluates the raw wall model and
certified closest query; it does not reconstruct the full/base OptionA production
TargetPolicy. NativeSavedMetric3D recomputes the whole metric initialization and
is not an inexpensive extraction of this run. Do not launch either as a competing
campaign or label raw wall eigenvalues as the combined target. Use cached exact
edge diagnostics where available. If bounded point reconstruction is needed after
terminal, retain sensor/source coordinates, original authority, exact support,
full/base profile and D1/blending identity, frozen core and constraints, then reuse
the actual TargetPolicy composition. Do not silently substitute a sensor-only or
wall-only tensor. No pinned code or helper was changed by this readiness audit.

## Decision and return

If geometry target deficiency is demonstrated, test ONE connected correction with
the existing no-CAD authority/metric facilities. If the target is sound, address the
demonstrated construction or constrained-repair limitation instead. Retained
parameters must obey exact coordinate/association/support/dependency invalidation;
an accepted association is not a distance certificate. No projection-idempotence
assumption or sampling-only certification.

Keep geometry1e-4,q>=.2,J>=.05,maxedge1.8,requested heights, positivity, body separation,
validity-first construction and every work/time/memory budget unchanged. No new
scheduler, geometry representation, high-order elements, permanent columns, fixed
boundary coordinates, norm/sensor change or mandatory CAD. Record alternatives in
the backlog. Qualify any demonstrated change at the established cadence, then
immediately return to the complete same-seed workflow and all transfer/CFD/restart
obligations. Current paper-derived benefits remain hypotheses.

Read-only extract receipts: Parallel extract_9fa2b025eb7e718db13ff02ac452a187 and
extract_d47e55bbf05ea791583089e42e366ff2, exact URL above. Focused excerpts, not a
claim of independently verifying every theorem or published result.
