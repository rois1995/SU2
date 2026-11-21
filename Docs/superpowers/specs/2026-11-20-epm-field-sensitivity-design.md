# SST–EPM field sensitivities from one adjoint replay — design

Date: 2026-11-20. Branch: `feature_UQ` (head `39bca1f`). Parent spec:
[SU2_SST_EPM_adjoint_agent_instructions.md](../../SU2_SST_EPM_adjoint_agent_instructions.md).

## 1. Intent

Today, `SST_EPM_POSTPROCESS=YES` gives the five classical EPM sensitivities
`g_m = dJ/da` at `a=0` (1C_KEEP, 1C_SWAP, 2C_KEEP, 2C_SWAP, 3C). It records one
direct iteration per case and per eigenspace probe, with one scalar amplitude `a`.
That is 5 to 17 recordings. It writes one number per case to `sst_epm_sensitivities.csv`.

The user wants two results from the same converged primal and adjoint:

- **Form 2 (this paper):** the five classical `g_m`, exact, plus the orientation
  probes. This must give the same numbers as today.
- **Form 1 (a later paper):** a node field `dJ/dR_ij(x)`. This is the sensitivity of the
  QoI to a Reynolds-stress perturbation. It is useful for maps, other
  perturbations, and linearized worst-case studies.

**Why:** the expensive runs (primal and adjoint convergence on CFDHub) must run
only once per case and QoI. Form 1 must later be a pure postprocessing step on
files we write now. It must not need new SU2 runs.

## 2. Core idea

The five scalar values come from one reverse sweep. That sweep can give the sensitivity
to *every* local additive contribution at once. These are the contributions the scalar
amplitude currently scales:

| Field | Where it is added today | Meaning |
|---|---|---|
| `δτ_e` (sym. nDim×nDim) | `tau += a*correction`, every viscous-flux evaluation: fused edge kernel `diffusion/common.hpp:addEPMExtractionStress`, scalar `flow_diffusion.cpp:SetStressTensor` (boundaries and other scalar paths) | Additive viscous-stress perturbation. It also enters the energy work through `tau`. |
| `δP_k,i` | `pk += a*(…)` in `turb_sources.hpp` SST source | Additive k production per unit volume, after the clip. |
| `δP_ω,i` | `pw += a*(…)` same place | Additive ω production, after the clip. |

All are registered as AD inputs with value zero, in one recording. The saved coupled
adjoint is seeded, and one reverse sweep gives the sensitivity fields `S_τ(e)`, `S_k(i)`, `S_ω(i)`.

Because the current scalar path is linear in these contributions, the following is exact:

```
g_m = Σ_e S_τ(e) : Δτ_m(e) + Σ_i [ S_k(i) ΔP_k,m(i) + S_ω(i) ΔP_ω,m(i) ]
```

The terms are as follows:
- `Δτ_m` is today's `EPMExtractionStressCorrection`.
- `ΔP_k,m = clip(P_EPM,m) − pk`.
- `ΔP_ω,m` is the matching ω term.

All are evaluated passively at the same states the replay used. The clip stays inside
each endpoint, as the parent spec requires.

## 3. Goals

1. One recording and one reverse sweep per adjoint give all field sensitivities.
2. SU2 computes the five classical `g_m` and all orientation probes from those fields
   with passive dot products. It uses the existing `ComputePerturbedRSM` code, so no
   extra recordings are needed.
3. The new `sst_epm_sensitivities.csv` matches the current scalar-replay CSV. The
   tolerance is `1e-10` relative, or summation-order round-off if larger. Check this on
   the 2D and 3D smoke tests, serial and with 2 MPI ranks.
4. Write field files that contain everything form 1 needs later:
   - sensitivities;
   - the passive baseline state at each evaluation site;
   - clip status;
   - global identifiers.
5. **Extract-only mode (user decision, 2026-11-20).**
   - It loads the saved primal and adjoint restarts and runs zero adjoint updates.
   - It performs only the field replay, so a later re-extraction is a short job.
   - Its results are identical to round-off with an extraction done at the end of the
     original adjoint run.
   - Convergence guard: evaluate the native adjoint residual once from the loaded
     adjoint. Do not apply that update. Require the residual to satisfy the case's
     configured adjoint criterion. Record the value in the JSON. Seed the replay with the
     loaded, not updated, adjoint.
   - It must accept restarts written by the current head binary (`39bca1f`) and by the
     new binary.
6. Provide a small Python reader. It recomputes the five `g_m` from the files alone and
   compares them with the CSV. This proves the files are complete for later postprocessing.

## 4. Non-goals

- Computing the form 1 `dJ/dR` map, choosing its node-to-edge mapping, or its policy
  where the clip is active. Those belong to the later paper.
- Nonlinear continuation, Taylor checks, wide SIMD/OpenMP, new physics coverage.
- Changing classical EPM targets, orientations or endpoint controls.

## 5. Anti-goals (failure even if it "works")

- Form 2 values that differ from the scalar-replay values beyond round-off.
- Field files that miss data form 1 needs, so the cluster cases must run again.
- Any change to the ordinary SST primal/adjoint when the feature is off, or to the
  converged adjoint arrays.
- Spectral smoothing, averaging, or weakening of EPM targets.

## 6. Constraints

- Same supported configuration as the current extraction (parent spec, Section 1 overlay):
  - reverse AD without OpenMP;
  - `USE_VECTORIZATION=NO`;
  - no multigrid;
  - the same guards.
- Builds and SU2 runs happen on CFDHub (see [CFDHUB_SST_EPM.md](../../CFDHUB_SST_EPM.md) and
  the `cfdhub-cluster` skill). Do not build or run SU2 locally.
- Mesh size: ONERA M6 can have millions of edges. Field output must be binary and in double
  precision. Its size must scale linearly with the edge count.
- MPI: a halo edge can be computed on two ranks, and each copy drives a different owned node.
  The true sensitivity of that edge is the **sum** of the copies. Key every site by global
  identifiers and sum the duplicates during postprocessing. The internal dot product is a
  plain sum over all local evaluations, followed by an Allreduce. That sum is already correct.
- If one site is evaluated more than once in a recording, its sensitivity is the sum
  over all evaluations. This is consistent with the scalar path.

## 7. Approaches considered

1. **Contribution fields now, form 1 later by chain rule (chosen by the user).** Exact form 2.
   Form 1 needs no rerun. Form 1 design choices stay open.
2. Keep the 5 scalar replays and add fields later from restarts. No code now, but every case
   needs another cluster job later with the identical binary.
3. Register a node `δR` field directly. Clean maps, but it fixes form 1's node-to-edge mapping
   and clip policy now. It also does not reproduce the edge-evaluated, clipped classical
   values exactly. Rejected for this paper.

## 8. Design decisions

- **Mode switch:** keep `SST_EPM_POSTPROCESS=YES`. Add `SST_EPM_EXTRACTION= FIELD | SCALAR_REPLAY`,
  default `FIELD`. `SCALAR_REPLAY` keeps today's path unchanged as the reference check.
  *(Builder's call on exact option naming; the semantics are fixed.)*
- **Evaluation sites** are numbered per recording. There are three kinds:
  - `EDGE`: fused kernel, keyed by global `(i,j)`;
  - `BOUNDARY`: scalar viscous flux at a boundary vertex, keyed by marker tag and global
    point; any other scalar viscous-flux use gets its own kind;
  - `NODE`: SST source, keyed by global point.

  The existing coverage counters must equal the number of written sites.
- **Field inputs** live in persistent `su2double` arrays owned by the extraction state.
  They are set to zero, reset and registered before dependencies, exactly like the amplitude
  is today. Each use site calls `AD::SetPreaccIn` on its own entries.
- **Passive data stored at each site during the recording,** taken from the same values that
  build the correction today:
  - EDGE / BOUNDARY: `ρ`, `μ_t`, `k`, the velocity gradient used (nDim×nDim), the face
    normal and area.
  - NODE: `ρ`, `μ_t`, `k`, velocity gradient, volume, `prod_limit`, baseline `pk` and `pw`
    before the addition, raw baseline production, `alfa_blended`, clip flags (floor active,
    cap active).
  - Coordinates of the points involved.
  - Output units are SU2's nondimensional solver variables. The header must state the
    nondimensionalization.
- **Classical values:** after the reverse sweep and extraction of the field adjoints,
  loop over the stored sites. For each case and probe, compute the passive corrections
  with the existing helpers and accumulate the dot products. Then Allreduce. The probe
  logic, ambiguity counts and CSV columns stay as today. Add a column
  `extraction=field|scalar_replay`.
- **Files:**
  - `sst_epm_field_<OBJ>_rank<r>.bin`: one per rank, with a fixed binary layout.
  - `sst_epm_field_<OBJ>.json`: written by the master. It holds the layout (field names,
    dtypes, counts per rank and kind), nDim, objective, `UQ_DELTA_B`, `UQ_URLX`, gap
    tolerance, rank count, source commit, nondimensionalization, and SU2 version.

  *(Builder's call on the exact binary layout, as long as numpy can read it with the JSON
  alone.)*
- **Reader:** `TestCases/disc_adj_rans/sst_epm_adjoint/read_epm_field.py`. It loads all
  ranks and merges duplicate global keys by summing. It recomputes `g_m` for the canonical
  cases by reimplementing only the dot product, and reads the corrections that SU2 also
  writes per site per case (see the next bullet). It compares the result with the CSV.
- **Per-site corrections:** to keep the reader independent of a Python EPM reimplementation,
  also write `Δτ_m`, `ΔP_k,m` and `ΔP_ω,m` for the five canonical cases per site.
  This is about 5× the stress storage. It is acceptable. It also lets form 1 compare its
  maps against the classical directions.

## 9. Verification

1. Unit tests (existing `epm_extraction_tests.cpp` framework):
   - the field sensitivity of one site matches a finite difference of that site's input;
   - the five dot products equal the scalar amplitude derivative at the same state.
2. Smoke tests (`check_extraction.py`), 2D and 3D:
   - FIELD CSV against SCALAR_REPLAY CSV;
   - coverage counts equal the written site counts;
   - the reader reproduces the CSV;
   - the adjoint and primal restarts are unchanged.
3. The same checks with 2 MPI ranks against serial, on CFDHub. Use extract-only mode
   from the *same* saved serial primal and adjoint. The merged per-key fields and `g_m`
   must then match serial to round-off: `1e-10` relative, with an absolute floor scaled by
   the field's max norm. A 2-rank SCALAR_REPLAY run is also checked. That path has never
   run on more than one rank. Diagnosing and fixing it is **in scope**, because every
   campaign run uses MPI.
4. Extract-only from a saved adjoint reproduces the end-of-run extraction files to round-off.

## 10. Left to the builder

- Discovering every caller of the scalar viscous numerics. Each one becomes a site kind,
  or it is shown to be inactive for the supported configurations. Start with the
  boundary conditions in `CEulerSolver.cpp` / `CNSSolver.cpp`.
- Option naming, including the extract-only switch.
- The binary layout.
- How the site index reaches the scalar numerics (for example, a cursor in the extraction
  state set by the solver loop before each `ComputeResidual`).
- The memory layout of the input arrays.
- Code organisation inside the driver.

## 11. Deviations after implementation

Recorded 2026-11-21. The approved text above is unchanged. Two user rulings (2026-11-20) change it in practice.

**(a) Extract-only from adjoint restarts written by MPI runs is rejected (§3.5).** With more than one rank, a
converged discrete adjoint has nonzero halo values. The adjoint restart stores owned points only, and native SU2
does not restore the halo part on restart. On several ranks the extract-only guard measures the total adjoint
(owned value plus halo copies) and rejects such restarts. To re-extract from them, run a short re-converge: a
restarted adjoint run with `SST_EPM_POSTPROCESS= YES`, which extracts at the end. No native SU2 change was made.

**(b) Option A replaces the strict 2-rank check (§9.3).** The adjoint fixed point depends on the partition,
through the finite accuracy of the transposed linear solves and the rank-local ILU. The `S_*` and `g_m` depend
only on the adjoint of the residual, and only when the loaded adjoint is the fixed point of the same map that is
replayed. So the serial adjoint is not a converged adjoint of the 2-rank iteration. The 2-rank checks are:
- field vs scalar replay on the same 2-rank run, to `1e-10`;
- the converged 2-rank run vs serial, and the 2-rank restarted re-extraction vs its end-of-run fields, at a
  convergence tolerance derived from the adjoint histories.

The §9.3 check (serial adjoint into 2 ranks, `1e-10` against serial) is a guard-decided diagnostic. The runner
enforces it if the guard accepts and records the guard values if it rejects. On CFDHub (job 664831) the guard
rejected it in 2D and 3D, and all other checks passed. Details: parent spec, Section 1, "MPI result".

**(c) Final review fixes (2026-11-21).** The stored `EDGE` normal now points from `key_a` to `key_b` on every rank
(it followed the local edge orientation before, so halo copies could have opposite signs); a `BOUNDARY` normal
points outward. The JSON records `byte_order` and the normal convention. A site used twice in one recording must
have the same passive state (error otherwise); every smoke run has one use per site. In practice extract-only works
serial-into-serial: both multi-rank inputs are rejected by the guard (see (a), (b)). §3.5 compatibility with
adjoint restarts written by `39bca1f` is untested; the restart format did not change. Details: parent spec,
Section 1 and Section 13.
