# RAE2822 RANS-SA from the user-provided Euler-type seed

Stopped by user after851.186seconds during the first native remesh. Read
user_stop.json: exit1 is cancellation, not a returned mesh-quality verdict.
No adapted RANS mesh was accepted. Initial grid:3592points,6952triangles.

The initial solve ended at iteration847 and met its configured
density and SA residual criteria. Exact mesh/restart coordinates, all scalar
VTU/restart pairs and conserved-flow admissibility checks PASS.
The initial wall y+ min/median/max is[2.367554931473752, 109.24504688607145, 198.6745731667757].
This is a coarse, unresolved-wall initial solution, not final RANS validation.
See initial_solution_inspection.json and flow_adap_00000.vtu.

The metric, rather than the CFD solve, triggered the expensive phase.
See ../BL_COMPLEXITY_DIAGNOSIS.md: one tip node contributes89% of complexity.
The stopped in-memory candidate could not be saved retrospectively; do not
substitute the input grid for a rejected adapted RANS grid.
