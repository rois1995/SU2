# Future transfer to convective and viscous gradients

User request, 2026-10-08: after validating adaptation-gradient/Hessian recovery,
assess whether its numerical improvements can also improve convective and
viscous gradient robustness on low-quality grids. Keep this as an explicit
follow-up; adaptation accuracy does not establish flow-solver robustness.

Start by auditing shared kernels and distinguishing changes already shared
(e.g. numerical scaling) from adaptation-only boundary or stencil policies.
Compare each transferable change separately against the existing reconstruction
and viscous operators. Use stretched, rotated, skewed and nearly degenerate
cells, multiple connectivities, curved walls, and mixed-cell meshes in 2D/3D.

For convective reconstruction, check limiter interaction, positivity, shock
resolution, conservation and residual convergence. For viscous gradients, check
wall-normal/tangential derivatives, shear stress, heat flux, diffusion consistency
and solver convergence. Scalar sensor boundary rules cannot simply be copied to
velocity/vector/turbulence fields or applied as physical wall conditions.

Measure exact manufactured derivatives first, then Euler/laminar/RANS flow
convergence, forces and wall outputs at identical meshes and cost budgets.
Include MPI partition consistency, OpenMP, AD/adjoint compatibility, CPU time,
memory and communication costs before accepting a production flow change.

Current boundary-recovery work remains restricted to adaptation call sites.
The flow-gradient transfer and its validation are not implemented yet.

## Element support distinction and cycle comparison (2026-10-08)

User explicitly asked to retain this distinction. WLS stabilization/reuse,
point-based QR fitting/noise/MPI stencils and metric tensor algebra are not
inherently simplex-only. Their availability does not certify accuracy on every
mixed topology. The new adaptation GG P1 recovery is specifically for complete
triangle/tetrahedron stars. A vertex touching a quad, prism, pyramid or hex
retains legacy GG. Its measured boundary improvements apply to tested simplex
meshes; the hexahedral check certifies fallback preservation, not an improvement.

For prism/hex boundary layers, compare existing point-based WLS/QR with
element-aware recovery using the appropriate shape functions and quadrature.
Test interfaces, wall-normal derivatives, MPI and cost before extending the
recovery or transferring it to convective/viscous flow gradients.

Immediate work: reduce repeated simplex geometry evaluation while retaining
deterministic owner accumulation and mixed/degenerate fallback; compare
higher-order boundary reconstruction on the same manufactured fields before
accepting a wall-bias policy. Do not infer high-order wall accuracy from affine
consistency or extrapolate simplex gains to prism/hex meshes.

Adaptation-cycle candidates should first run on codex/metric-robustness, where
the recovery changes exist, paired with the integrated-main baseline at matched
inputs, settings, complexity and execution resources. Pin both revisions and
separate recovery changes from other branch differences. After integration,
repeat the accepted cycle checks on the merged main revision. Never validate
an unmerged improvement using main alone or treat a branch result as proof
that the final merged executable behaves identically.

At this checkpoint the other agent's NativeIntegrated worktree is on
codex/native-unsteady-performance at 99aa0d7f72, ahead of our common integration
base 2abbd11769. It contains newer native partition/worker/performance work and
uncommitted audit/handoff edits. Before attributing adaptation-cycle differences
to recovery, stage the validated metric changes on the same accepted remesher
revision as the control (rebase or isolated integration candidate). Keep the
other agent's active worktree unchanged. Comparing these diverged heads directly
would mix metric and remesher changes.

Matched-cycle follow-up (2026-10-08): both native RAE2822 GG recovery cases passed
independent mesh/BL/flow-output checks on the shared 2ab remesher. SA residual
is 4.72x smaller in the improved run after 2000 iterations, with similar cell
counts; neither case meets the SA stopping criterion. This is not force accuracy
or flow-gradient-transfer validation. See Papers/ADAPTATION_CYCLE_RECOVERY.md.
Measured next performance targets are duplicate steady metric construction and
repeated gradation inside complexity trials. Keep newer-remesher paired checks
and post-merge checks pending; mixed cells and 3D remain separate test scopes.
