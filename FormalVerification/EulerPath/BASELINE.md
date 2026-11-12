# Baseline ledger

The new baseline is actual upstream `https://github.com/su2code/SU2.git`, branch develop,
fetched and checked on 2026-11-12. Commit:
`d162016382f0c122433000c114e88d808b48f7a7`; commit date
`2026-11-11T22:46:00+01:00`; tree `6fed259adfd100172b212d944d9e0ed12af352b9`.
The user's fork's develop was not substituted. Production patch list: **empty**.
The candidate branch is based directly on this commit; only FormalVerification files
are added. The historical accepted pin and both accepted projects remain unchanged.

| Item | Verdict on new baseline | Evidence and disposition |
|---|---|---|
| Scalar AddSolution | Unchanged, not a new defect | Fresh extraction of the unique unsigned-long/unsigned-long/su2double overload matches accepted raw method. C2DContainer row-major accessor and actual explicit caller reinspected. Reuse accepted proofs. |
| Moving slip-wall density/residual/energy correction | Historical **CONFIRMED**, validated repair missing upstream; current missing terms **BY READING** | Historical e0d3ee8 versus parent 371fcae6 has independent fixed/parent native and Lean evidence. Current CFVMFlowSolverBase.inl still projects momentum residual/truncation without density times wall-normal speed and lacks the corresponding state/energy coupling. Open upstream PR #2945 contains e0d3ee8; no duplicate repair. Dynamic grid is outside the selected fixed-mesh path. No new moving-grid production run is claimed. |
| Related multigrid/truncation coupling | Historical validated repair missing upstream; current impact **BY READING** | Reinspected wall truncation loop and MG projection helpers against accepted patch. For zero grid speed the added density and energy-work terms vanish. MGLEVEL=0 executes no restriction/prolongation, although the flow integrator class is CMultiGridIntegration. Historical repaired-vs-parent test is preserved, not rerun or relabeled as an upstream result. |
| Sharp-edge wall normals | Separate local validation; excluded geometry | Workspace `todo/bug-wall-normal-sharp-edges.md`, local fix branch `fix_sharp_edge_wall_normals`; no sharp intersection of two wall planes exists in this flat-wall case. No production patch imported. |
| Pressure positivity/underrelaxation | Historical algebraic concern and optional local repair; excluded implicit path | `todo/bug-underrelaxation-positivity.md`; actual explicit arm does not call implicit underrelaxation. No arbitrary explicit-update positivity claim. |
| HLLC moving-grid / 3D Roe wave strengths | Known separate repair work; excluded configuration | Existing PRs #2943/#2944; chosen path is 2D SIMD Roe on a fixed mesh, not HLLC or 3D legacy Roe. No duplicate fix or new defect claim. |
| Source-slot combinations, turbo Riemann, excessive MGLEVEL, multi-species gradients, incompressible reductions | Existing #2945 work; excluded switches | No sources/turbomachinery/species/incompressible/MPI, MGLEVEL=0. No inference that unrelated branches are correct. |
| EPM/SST, outlet reconstruction, mesh motion/adaptation, AD, implicit linear solves | Not selected | Deferred explicitly. |

PR #2945 was read with its full discussion/review timeline and 25-commit list on
2026-11-12. It was open, unmerged, head `de29b5852ac35eb2cb14a52d0ee20575315865c4`.
Its reviewed follow-up replaces the original MGLEVEL clamp with rejection; the ledger
uses the current discussion, not the obsolete initial patch. This milestone neither
approves that entire PR nor imports its unrelated changes.

Upstream references: [moving-wall/shared fixes #2945](https://github.com/su2code/SU2/pull/2945),
[wall follow-up](https://github.com/su2code/SU2/pull/2945#issuecomment-6141825713),
[HLLC #2943](https://github.com/su2code/SU2/pull/2943).

## Validation scope

`build_baseline.py` creates a standalone clone with private Git metadata. It restores
pinned Meson, Ninja, Eigen and MEL sources, builds SU2_CFD and the unit driver with two
threads, and checks that production files remain unchanged. Official preconfigure runs
only in this private clone with all dependency acquisition disabled. No shared worktree
submodule metadata is touched.

The compiler is GCC 13.3.0 (Ubuntu 13.3.0-6ubuntu2~24.04.1), C++17 LP64 passive double.
MPI/OpenMP/AD/CGNS/TecIO/CUDA are disabled. Release retains upstream -O3 and fast-math
flags; complete commands and Meson options are in evidence. Focused upstream tests
`[Vectorization],[LinearAlgebra]` passed **545 assertions in six test cases**.
Integration, native source controls and Lean outcomes are recorded separately in
`evidence/` and `RESULTS.md`. Timeouts or missing prerequisites are failures/inconclusive,
never a passing baseline qualification. This ledger makes no whole-codebase bug-free claim.
