# Metric lifecycle and convergence scope across branches

Checked 2026-10-08: metric branch 16624403df9b9fb859dab1987c96bf6da99a069f
against codex/native-unsteady-performance at
1f7e60012ee09552930a1996d51dd35c90984937. Both worktrees were clean.
This is a source comparison; no additional solver runs or production edits.

The Postprocess and RemeshFromMetric bodies are identical. In both branches,
steady Postprocess computes the final metric and RemeshFromMetric computes it
again before consuming the same solution. Those call sites date to commits
617f99af67c and 60fea3c83e9, before the recent GG/QR recovery work. The normal
unsteady adaptation loop calls remesher->Remesh directly with the completed
window metric, so that path does not have this particular duplication.

The entire native gradation/complexity evaluator/root solve and the subsequent
corner correction/final metric region are byte-identical across these branches.
Both retain synchronous full-field gradation, an 80-sweep cap, fresh grading
inside each complexity trial and repeated scaling solves after corner updates.
The cap originates in shared integration history (0dc58cd8751), not the later
simplex GG or wall-quartic commits. Workload counts depend on the metric, mesh
and partition; the 78 trials / 6015 sweeps / 18-19 seconds measured in the matched
cycle must not be asserted as timings for the newer branch without a new run.

The GG simplex correction defaults off for ordinary flow-gradient callers.
Adaptation sensor/Hessian calls explicitly enable it. New WLS neighbor ordering
also selects only GRADIENT_ADAPT/HESSIAN communications. Earlier shared WLS
numerical scaling can affect ordinary CFD callers, but both actual cycle
executables have identical WLS kernel hashes
(efd86dddf287d6da9954a1ad72a1407d03544242849fd8ff1ffdfc4adced92f2).
The recent cycle did not compare changed convective or viscous gradient operators.

Recovery can affect CFD convergence indirectly: changed Hessians change the
metric, remeshing and transferred starting state; the same flow-gradient formula
then acts on a different mesh, with different truncation error and conditioning.
The 4.72x lower SA residual is one observed mesh/continuation result. Density
residual is 1.223x higher and neither SA criterion passes. No general improvement
in nonlinear convergence, forces or flow-gradient accuracy is established.

Eliminating duplicate work or accelerating equivalent metric solves should
reduce wall-clock cost, not change residual convergence per iteration. Any
change that instead resolves remaining gradation violations changes the metric
and needs fresh matched mesh/flow checks. Simply reducing the 80-sweep cap is
not a demonstrated fix. Implement these as shared integration improvements and
preserve the newer branch's unsteady restart/output/lifecycle changes.
