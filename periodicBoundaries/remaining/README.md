# Periodic follow-ups — 2026-10-07

Baseline develop: `6db10127d1`. The original four-part series remains A rotation, B axis/geometry, C stencils, D streamwise/auxiliary. E–H are additional focused follow-ups.

| Part | Branch | Evidence |
|---|---|---|
| B | `fix_periodic_axis` | [geometry](geometry/) |
| C | `fix_periodic_counts` | [stencils](stencils/) |
| D | `fix_streamwise_periodic` | [auxiliary](auxiliary/) |
| E | `fix_periodic_transforms` | [transforms](transforms/) |
| F | `fix_periodic_implicit` | [implicit](implicit/) |
| G | `fix_periodic_support` | [support](support/) |
| H | `fix_periodic_wall_distance` | [wall_distance](wall_distance/) |

Combined release checks: 10 cases / 3175 assertions serial and OpenMP2; partitioned MPI2 and MPI2×OMP2 pass on both ranks (2334 / 2238 assertions). Actual reverse linear-solve callback finite differences pass serial/MPI2 for translation and helical rotation (4 assertions/rank). Float Krylov solution checks use sqrt(float epsilon); matrix products retain 1e-5 bounds. Whole CFD/geometry adjoint, ALE/GCL, full regression/reference changes and rotational/dependent-lattice wall images remain open. Wall distance is a separate draft.

`validation/develop-reproducer.patch` changes tests only. Apply it to clean develop, build the existing unit target, and select the relevant Catch2 case. The support configurations are complete BOX inputs. The flamelet unit exercises the real metadata and gradient helper without requiring an external chemistry table. Published logs identify actual fail/pass assertions; no crash from a malformed fixture is counted as a bug.

Combined validation source checkpoint: `2d3622408c1d369ef5408406c472d60760d2d248` on `codex_periodic_remaining_20261007`. Exact delivery heads are in `validation/branch-heads.json`.

Initial CodeFactor findings on B/C/D/F were extra assignment spaces in regression scripts. All flagged lines are corrected and pushed; Python AST equivalence and pre-commit checks pass. Numeric source/tests are unchanged from the validated checkpoint. CI is rerunning; full-suite success is not claimed.
