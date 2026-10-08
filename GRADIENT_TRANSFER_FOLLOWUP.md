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
