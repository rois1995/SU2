# Source findings

Baseline d162016382f0c122433000c114e88d808b48f7a7, no production patches.
Far-field flux accumulation precedes final Euler-wall processing. Stationary walls
project old solution, residual and truncation momentum; current remains unchanged.
Far-field writes characteristic primitive cache and uses scalar CUpwRoe_Flow.
The interior flux remains SIMD Roe. The explicit update visits all owned points
and all four components, reads old (after boundary processing), and writes current.
A fresh private build passed 545 assertions / six selected tests. Production
floating-point and object/geometry initialization correspondence remain open.

The scalar Roe source reads the stored interior primitive enthalpy. The candidate
therefore carries `insideEnthalpy` explicitly and requires its agreement with the
independent thermodynamic formula for the refinement. Primitive preprocessing is
not silently assumed proved. `CPoint` initializes periodic volume to zero;
`CVariable::Set_OldSolution` uses parallelCopy; `CFlowVariable` initializes
truncation and timestep storage. Those source facts justify the stated caller
requirements but are not allocation/initialization proofs.

No newly claimed production defect or repair is part of this milestone. Semantic
mutants are disposable validation fixtures, not proposed production patches.
