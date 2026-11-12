# Open obligations

1. External approval of this new specification and independent cold evaluation of the frozen candidate.
2. Joint stationary wall/far-field transformation of old state, residual and truncation; discharge `finish`.
3. Actual initialization/copy, heap allocation/layout and full update-loop refinement.
4. Mesh validity, endpoint orientation, edge enumeration, SIMD padding and unpacking refinement.
5. Actual first-order SIMD Roe flux consistency and all selected branches.
6. Pseudo-time selection, admissibility and stability under justified restrictions.
7. IEEE-754/fast-math/compiler semantics and quantitative rounding analysis.
8. Only after the configured Euler path closes: moving meshes, multigrid, viscous/RANS,
   implicit solves, MPI and adaptation as separately reviewed extensions.
