# EPM Field Sensitivities Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** One adjoint replay gives sensitivity fields for every additive EPM contribution. These are `δτ` per viscous-flux site and `δP_k`, `δP_ω` per node. From these fields:
- SU2 computes the five classical `g_m` and the orientation probes;
- SU2 writes per-rank field files that are complete for a later `dJ/dR` study;
- a saved adjoint can be re-extracted without iterating.

**Architecture:** today, one scalar amplitude scales a passive correction at three hook sites. A new `FIELD` mode replaces that amplitude with persistent zero-valued AD inputs, one block per evaluation site.

- The solver loops set a site cursor before each numerics call.
- Each hook adds its site's input and stores passive state for that site.
- After one reverse sweep, the driver reads the input adjoints. It forms the classical dot products from the stored state with the existing passive helpers, and writes the files.

`SCALAR_REPLAY` keeps today's path as the reference.

**Tech Stack:**
- C++17 SU2 with CoDiPack reverse AD, no OpenMP, MPI.
- Catch2 unit tests.
- Python 3 with numpy for the smoke runner and the reader.
- CFDHub SGE cluster for all builds and runs.

**Spec:** `Docs/superpowers/specs/2026-11-20-epm-field-sensitivity-design.md` (parent: `Docs/SU2_SST_EPM_adjoint_agent_instructions.md`)

## Global Constraints

- Do not build or run SU2 locally. All compilation, unit tests and smoke runs go to CFDHub, through the `cfdhub-cluster` skill.
- Follow `Docs/CFDHUB_SST_EPM.md` for storage. All outputs stay inside the SU2 study directory on scratch. Do not touch MPI or scheduler temporary locations.
- Supported configuration: unchanged from the current `SST_EPM_POSTPROCESS` guards in `Common/src/CConfig.cpp:6136-6170`. That means reverse AD, no OpenMP, `USE_VECTORIZATION=NO`, `MGLEVEL=0`, single DRAG or LIFT objective, `UQ_DELTA_B=1`, `UQ_URLX=1`.
- Classical EPM is not modified. Do not add smoothing, averaging or target weakening.
- The ordinary primal and adjoint are bitwise unchanged when `SST_EPM_POSTPROCESS=NO`. The converged adjoint arrays are never modified by extraction.
- FIELD `g_m` equals SCALAR_REPLAY `g_m` to `1e-10` relative, or summation round-off if larger, at the same rank count and from the same adjoint.
- 2-rank against serial, both extract-only from the same serial restarts: per-key fields and `g_m` match to `1e-10` relative, with an absolute floor of `1e-10 × max|field|`.
- Field files: binary float64, size linear in the site count. They are readable by numpy using only the JSON header.
- Values are in SU2 nondimensional solver units, and the JSON states `REF_DIMENSIONALIZATION`.

## Review Focus

1. **A boundary viscous call site gets no cursor.** Its contribution then lands on a stale site, or is missed. Expected behaviour: a hard error. Pinned by the "stale cursor" test in Task 3 and the coverage assertion in Task 9.
2. **The same site is evaluated twice in one recording.** Its sensitivity must be the sum of both uses, and it must be written once. Pinned by `"Repeated site accumulates"` in Task 2.
3. **Halo edges duplicated across ranks.** The reader sums copies by global key. It must not drop them or average them. Pinned by the reader test in Task 8 and the MPI comparison in Task 10.
4. **The adjoint restart is overwritten by the extract-only job.** Expected: the input adjoint file stays unchanged. Pinned in Task 9 (hash check).
5. **ONERA-size meshes.** The writer must stream per rank and must not gather to the master. Pinned in Task 7 by the "no master gather" structure, and by the file-size assertion.

---

## File Structure

| File | Responsibility |
|---|---|
| `Common/include/CEPMExtraction.hpp` (modify) | Extraction state: mode, extract-only flag, site cursor, and an owning pointer to the field store. |
| `Common/include/CEPMFieldSites.hpp` (create) | Field store: site registry, AD input blocks, the fixed-width passive record per site, accumulation of the input adjoints. Header-only, so unit tests link without new sources. |
| `Common/src/CConfig.cpp`, `Common/include/CConfig.hpp` (modify) | Options `SST_EPM_EXTRACTION`, `SST_EPM_EXTRACT_ONLY`, and their guards. |
| `SU2_CFD/include/numerics/CNumerics.hpp` (modify) | `EPMFieldStressHook`, `EPMFieldSourceHook`: shared hook bodies called from the three numerics sites. |
| `SU2_CFD/src/numerics/flow/flow_diffusion.cpp`, `SU2_CFD/include/numerics_simd/flow/diffusion/common.hpp`, `.../viscous_fluxes.hpp`, `SU2_CFD/include/numerics/turbulent/turb_sources.hpp` (modify) | Call the hooks in FIELD mode, and keep the amplitude path in SCALAR_REPLAY mode. |
| `SU2_CFD/src/solvers/CEulerSolver.cpp`, `CNSSolver.cpp`, `CTurbSSTSolver.cpp` (modify) | Set the site cursor before every viscous and source numerics call. |
| `SU2_CFD/src/drivers/CDiscAdjSinglezoneDriver.cpp` + `.hpp` (modify) | FIELD extraction, classical dot products, extract-only run. |
| `SU2_CFD/src/output/CEPMFieldWriter.cpp` + `SU2_CFD/include/output/CEPMFieldWriter.hpp` (create) | Per-rank binary and master JSON writer. |
| `TestCases/disc_adj_rans/sst_epm_adjoint/read_epm_field.py` (create) | Reader, key merge, recomputation of `g_m`. |
| `TestCases/disc_adj_rans/sst_epm_adjoint/test_read_epm_field.py` (create) | Reader unit test on synthetic files. |
| `TestCases/disc_adj_rans/sst_epm_adjoint/check_extraction.py` (modify) | FIELD against SCALAR_REPLAY, extract-only and MPI checks. |
| `UnitTests/SU2_CFD/numerics/epm_field_tests.cpp` (create) + `UnitTests/meson.build` | Field store and hook tests, passive and reverse. |

**Running unit tests:** every "Run" step for C++ means this command, on the CFDHub build host and in the study's `src` checkout. It is the parent spec Section 13 snippet, with `epm_field_tests.cpp` added to `tests=` in both modes and `CEPMFieldWriter.cpp` added to the source list from Task 7 on:

`bash Docs/superpowers/plans/run_epm_unit_tests.sh '<catch tag>'`

Task 1 creates that script by moving the parent snippet into it. The script takes the Catch tag as `$1`.

---

### Task 1: Options, extraction state and test script

**Files:**
- Modify: `Common/include/CEPMExtraction.hpp`, `Common/include/CConfig.hpp:1287,9768`, `Common/src/CConfig.cpp:3250,6136-6170`
- Create: `Docs/superpowers/plans/run_epm_unit_tests.sh`, `UnitTests/SU2_CFD/numerics/epm_field_tests.cpp`
- Modify: `UnitTests/meson.build:21,37` (add `epm_field_tests.cpp` in both lists)

**Interfaces:**
- Produces, in `CEPMExtraction`:
  - `enum class Mode { FIELD, SCALAR_REPLAY }`
  - `Mode mode = Mode::FIELD`
  - `bool extract_only = false`
  - `enum class SiteKind : int { EDGE = 0, BOUNDARY = 1, NODE = 2 }`
  - `struct Cursor { SiteKind kind; unsigned long a, b; bool set = false; } cursor`
  - `std::unique_ptr<CEPMFieldSites> field` (forward-declared; defined in Task 2)
- Config options:
  - `SST_EPM_EXTRACTION` takes `FIELD` or `SCALAR_REPLAY`. The default is `FIELD`.
  - `SST_EPM_EXTRACT_ONLY` takes `YES` or `NO`. The default is `NO`.

- [ ] **Step 1: Write the failing tests** in `epm_field_tests.cpp`, using `[EPMField]`:
  - `"EPM field options parse"`: with `SST_EPM_POSTPROCESS= YES` and nothing else, `mode == FIELD` and `!extract_only`. With `SST_EPM_EXTRACTION= SCALAR_REPLAY`, `mode == SCALAR_REPLAY`.
  - `"Extract-only requires restart and one iteration"`: `SST_EPM_EXTRACT_ONLY= YES` with `RESTART_SOL= NO` throws. With `RESTART_SOL= YES, ITER= 5` it throws. With `RESTART_SOL= YES, ITER= 1, CONV_CRITERIA= RESIDUAL` it is accepted. Build the config through the same `CConfig(stringstream, SU2_CFD, false)` pattern as `epm_extraction_tests.cpp`. Use `REQUIRE_THROWS` around `SetPostprocessing`, or the existing guard entry point.
- [ ] **Step 2: Run** `bash Docs/superpowers/plans/run_epm_unit_tests.sh '[EPMField]'`. Expected: compile failure on the unknown `mode` member.
- [ ] **Step 3: Implement** the members and options:
  - Add the extract-only guard inside the existing `if (epmExtraction.enabled)` block. It needs `GetRestart()`, `GetnInner_Iter() == 1`, and a residual-type convergence criterion. The error message must name all three.
  - Leave every existing guard unchanged.
- [ ] **Step 4: Run** the same command. Expected: `[EPMField]` passes, and `[EPM],[EPMExtraction]` still pass (721 passive and 695 reverse assertions, unchanged).
- [ ] **Step 5: Commit** with the message `Add EPM extraction mode and extract-only options`.

### Task 2: Field store

**Files:**
- Create: `Common/include/CEPMFieldSites.hpp`
- Test: `UnitTests/SU2_CFD/numerics/epm_field_tests.cpp`

**Interfaces:**
- Produces `class CEPMFieldSites`:
  - `static constexpr int kTau = 9;` The full 3×3 block is used and only the `nDim×nDim` part is active. The full block avoids symmetric-mapping errors, because the flux uses `τ_ij` and `τ_ji` separately.
  - `static constexpr int kCases = 5;`
  - `struct Record`. All fields are `passivedouble`, in this exact order, which is also the file column order:
    - `kind, key_a, key_b`
    - `coord_a[3], coord_b[3]`
    - `rho, mu_t, k, grad[9]` (velocity gradient, row-major, `grad[i*3+j] = ∂u_i/∂x_j`)
    - `normal[3], area, volume`
    - `prod_limit, pk0, pw0, p_raw0, alfa_blended, floor_active, cap_active`
    - `S_tau[9], S_pk, S_pw`
    - `dtau[5][9], dpk[5], dpw[5]`
    - `uses`

    Unused entries are `NaN`. The exception is `S_*` and `d*`, which are `0`.
  - `void Clear()`: empties the registry and resets all AD indices.
  - `size_t Site(SiteKind kind, unsigned long a, unsigned long b)`: returns the existing index for the key or appends a new one. EDGE keys are stored as `(min, max)` of the global ids.
  - `su2double* TauInput(size_t site)`, `su2double& PkInput(size_t site)`, `su2double& PwInput(size_t site)`: persistent storage. It must not reallocate after `Freeze()`.
  - `void Freeze()`: no new sites after this. Any later `Site()` with an unknown key calls `SU2_MPI::Error("EPM field site created after registration")`.
  - `void RegisterInputs()`: sets every input to zero, then calls `AD::ResetInput` and `AD::RegisterInput`.
  - `void ExtractAdjoints()`: for each site, sets `S_*` from `SU2_TYPE::GetDerivative` of its inputs.
  - `Record& Data(size_t site)`, `size_t Size() const`.
- **Two-pass rule:** sites are discovered during the `CLEAR_INDICES` pass. That pass already runs one passive direct iteration. Then `Freeze()` is called and inputs are registered in the real recording. This keeps the input addresses stable.

- [ ] **Step 1: Write the failing tests** with `[EPMField]`:
  - `"Site keys are unique and edge order independent"`: `Site(EDGE,7,3) == Site(EDGE,3,7)`. `Site(NODE,3,0) != Site(EDGE,3,7)`. `Size() == 2`.
  - `"Freeze rejects new sites"`: after `Freeze()`, `Site(NODE,99,0)` throws, and `Site(EDGE,3,7)` returns the old index.
  - `"Repeated site accumulates"` (reverse build only, `#ifdef CODI_REVERSE_TYPE`): register the inputs and build `y = 2*PkInput(s) + 3*PkInput(s)`. Seed `y` with 1, reverse, then call `ExtractAdjoints()`. Then `Data(s).S_pk == 5` and `Data(s).uses` is unchanged by the extraction.
- [ ] **Step 2: Run** with tag `[EPMField]`. Expected: FAIL, header not found.
- [ ] **Step 3: Implement** `CEPMFieldSites.hpp`:
  - Use `std::unordered_map<std::tuple<int,unsigned long,unsigned long>, size_t>` with a simple hash.
  - Use `std::deque` or a pre-reserved `std::vector` for the inputs, so the addresses stay stable.
- [ ] **Step 4: Run.** Expected: PASS in passive and reverse modes.
- [ ] **Step 5: Commit** with the message `Add EPM field site store`.

### Task 3: Numerics hooks

**Files:**
- Modify: `SU2_CFD/include/numerics/CNumerics.hpp`, next to `EPMExtractionStressCorrection`
- Modify: `SU2_CFD/src/numerics/flow/flow_diffusion.cpp:145-154`, `SU2_CFD/include/numerics_simd/flow/diffusion/common.hpp:93-109`, `SU2_CFD/include/numerics_simd/flow/diffusion/viscous_fluxes.hpp:161-165`, `SU2_CFD/include/numerics/turbulent/turb_sources.hpp:1017-1033`
- Test: `UnitTests/SU2_CFD/numerics/epm_field_tests.cpp`

**Interfaces:**
- Consumes: `CEPMFieldSites` (Task 2) and `CEPMExtraction::cursor` (Task 1).
- Produces:
  - `template<class Mat> static void EPMFieldStressHook(size_t nDim, const Mat& gradient, su2double density, su2double viscosity, su2double k, const su2double* normal, su2double area, const CConfig* config, su2double (&tau)[3][3]);` It adds `TauInput(site)[i*3+j]` to `tau[i][j]` for `i,j < nDim`, after `AD::SetPreaccIn` on those inputs. It fills `rho, mu_t, k, grad, normal, area` the first time a site is seen in each recording (`uses == 0`), and fills `dtau[m]` for the five canonical cases with `EPMExtractionStressCorrection`, using `component = m/2+1`, `permute = m%2`, `angle = 0`, `probe_pair = 3`. Then it increments `uses`.
  - `static void EPMFieldSourceHook(const CConfig* config, su2double& pk, su2double& pw, …)`: has the same pattern. It takes the passive values the scalar path already computes (`target`, `grad`, `rho`, `mu`, `prod_limit`, `alfa_blended`, raw production). It stores the node data, the clip flags (`floor_active = raw0 < 0`, `cap_active = raw0 > prod_limit`) and `dpk[m]`, `dpw[m]` as `epm_pk − pk0` and `epm_pw − pw0`. Then it adds `PkInput(site)` and `PwInput(site)` with `SetPreaccIn`.
  - If `!cursor.set` in FIELD mode, both hooks call `SU2_MPI::Error("EPM field hook reached without a site cursor")`. Each hook clears `cursor.set` after use.
  - In SCALAR_REPLAY mode, the existing amplitude code runs unchanged.
- **Fused-edge lanes:** with the scalar-lane AD build, `Double::Size == 1`. If `Double::Size != 1`, call `SU2_MPI::Error`. Wide SIMD is out of scope.

- [ ] **Step 1: Write the failing tests** with `[EPMField]`, reusing the `Evaluate(...)` setup pattern from `epm_extraction_tests.cpp:39-180`:
  - `"Field stress hook: zero input leaves flux unchanged"`: the FIELD-mode flux equals the feature-off flux, exactly.
  - `"Field stress hook: input FD matches adjoint"` (reverse): register the inputs, compute the flux, seed one output component, reverse. Then `S_tau[0*3+1]` matches a central FD of that output with step `1e-6` on `TauInput(site)[1]`, to a relative error below `1e-7`.
  - `"Field dot product equals amplitude derivative"` (reverse): for each `m` in 0..4, `Σ_ij S_tau[ij]·dtau[m][ij]` equals the SCALAR_REPLAY `dFlux/da` at the same state, to `1e-12` relative. Do this for the scalar flux and the fused edge flux, in 2D and 3D. Add the same check for the source with `S_pk·dpk[m] + S_pw·dpw[m]`, with the cap active and with the cap inactive.
  - `"Stale cursor is an error"`: in FIELD mode, without setting `cursor`, the flux call throws.
- [ ] **Step 2: Run** with tag `[EPMField]`. Expected: FAIL, hooks undefined.
- [ ] **Step 3: Implement** the hooks and switch the four call sites on `config->GetEPMExtraction().mode`. Keep the existing `stress_evaluations` and `evaluations` counters incremented in both modes.
- [ ] **Step 4: Run** `[EPM],[EPMExtraction],[EPMField]`. Expected: all pass, and the old counts are unchanged.
- [ ] **Step 5: Commit** with the message `Add EPM field hooks to viscous flux and SST source`.

### Task 4: Site cursors in the solver loops

**Files:**
- Modify: `SU2_CFD/src/solvers/CEulerSolver.cpp` (the edge loop near `:2027`, and every `visc_numerics->ComputeResidual` call: `:5239`, `:5664`, `:6177`, `:7112`, `:7820`, plus any others `grep` finds), `SU2_CFD/src/solvers/CNSSolver.cpp` (its BC viscous calls), `SU2_CFD/src/solvers/CTurbSSTSolver.cpp:389`
- Modify: the fused kernel `viscous_fluxes.hpp:114/219/237`. It sets the cursor itself from `iEdge`, `iPoint`, `jPoint`.

**Interfaces:**
- Consumes: `cursor` (Task 1).
- Produces: a helper `inline void SetEPMCursor(const CConfig* c, CEPMExtraction::SiteKind k, unsigned long a, unsigned long b)` in `CEPMExtraction.hpp`. It is a no-op unless `enabled && active && mode == FIELD`.
- Keys:
  - EDGE: global ids of `iPoint` and `jPoint` (`geometry->nodes->GetGlobalIndex`).
  - BOUNDARY: `a` = global id of the vertex point, `b` = `config->GetMarker_All_TagBound` position in the global marker list (`config->GetMarker_CfgFile_TagBound` index), so ranks agree.
  - NODE: `a` = global id, `b = 0`.

- [ ] **Step 1: Find the call sites.** Run `grep -n "visc_numerics->ComputeResidual\|Viscous_Residual(" SU2_CFD/src/solvers/*.cpp`. List each hit in the commit message, as either "cursor added" or "unreachable under the guards" with the guard named.
- [ ] **Step 2: Add the cursors.**
- [ ] **Step 3: Compile check** on the build host. Run `ninja -C build-epm SU2_CFD_AD` in the study build dir (configured as parent spec §12.1 with `-Dwith-omp=false`). Expected: builds.
- [ ] **Step 4: Commit** with the message `Set EPM field site cursors in viscous and source loops`. Runtime coverage is checked in Task 9.

### Task 5: FIELD extraction in the driver

**Files:**
- Modify: `SU2_CFD/src/drivers/CDiscAdjSinglezoneDriver.cpp:296-303,493-591`, `SU2_CFD/include/drivers/CDiscAdjSinglezoneDriver.hpp`

**Interfaces:**
- Consumes: Tasks 1–4.
- Produces: `void ExtractEPMFieldSensitivities()`. It is called from `ExtractEPMSensitivities()` when `mode == FIELD`. The current body becomes `ExtractEPMScalarReplay()`, otherwise unchanged except for the CSV column. It returns the CSV rows in the existing format, plus a last column `extraction` with the value `field` or `scalar_replay`.
- Sequence. This is the algorithm, because the existing helpers do not determine it:
  ```
  field.Clear(); epm.active = true; SetRecording(CLEAR_INDICES)  // discovers sites
  field.Freeze(); SetRecording(MainVariables)   // SetRecording calls field.RegisterInputs() where the amplitude was registered
  same baseline-output / saved-adjoint / objective checks as today
  InitializeAdjoint; SetAdjObjFunction; AD::ComputeAdjoint(); field.ExtractAdjoints(); AD::ClearAdjoints()
  for mode m in 0..4, probe p in 0..3 (same loop and break rule as today):
      g_local = Σ_sites [canonical (p==0): S·d*[m] from the record]
                        [probe: recompute the correction from the stored passive state with
                         EPMExtractionStressCorrection / the source target, with angle=π/2, probe_pair=p-1]
      ambiguous counts from those recomputations; Allreduce g and counts (same reductions as today)
  restore exactly as today (CLEAR_INDICES, SetRecording per solver, SetDependencies)
  ```
- The probe loop runs on every rank, under the same global `ambiguous == 0` break, so the collective sequence stays the same on all ranks.

- [ ] **Step 1: Write the failing test.** Extend `check_extraction.py`. After the existing `enabled` run, add a `scalar` run with `SST_EPM_EXTRACTION="SCALAR_REPLAY"`. Assert that the rows match `enabled` by `(case, probe_pair)` with `math.isclose(field, scalar, rel_tol=1e-10, abs_tol=1e-14)`. The `extraction` column must read `field` and `scalar_replay`.
- [ ] **Step 2: Run** on CFDHub. Submit an SGE job that runs `python3 check_extraction.py $BIN <study>/cases/smoke2d --dimension 2`. Expected: FAIL, because `extraction` is missing or FIELD is unimplemented.
- [ ] **Step 3: Implement** `ExtractEPMFieldSensitivities()`.
- [ ] **Step 4: Run** the 2D and 3D jobs. Expected: PASS. Rows match to `1e-10`. The adjoint histories with the feature off and on are still identical.
- [ ] **Step 5: Commit** with the message `Extract classical EPM sensitivities from one field replay`.

### Task 6: Extract-only run

**Files:**
- Modify: `SU2_CFD/src/drivers/CDiscAdjSinglezoneDriver.cpp:157-228`

**Interfaces:**
- Consumes: the `extract_only` option (Task 1).
- Behaviour, when `extract_only`:
  1. Before the adjoint loop, snapshot all adjoint solutions with the existing `GetAllSolutions(ZONE_0, true, …)` pattern.
  2. Run the single configured iteration. `ITER=1` is enforced by Task 1. It produces the native residual and the convergence flag.
  3. Restore the snapshot with `SetAllSolutions`, so the replay seeds with the loaded adjoint.
  4. The existing convergence guard in extraction then applies unchanged.
  5. Write the measured residuals to the JSON (Task 7) as `extract_only_residuals`.
- The input adjoint restart must not be overwritten. If `SOLUTION_ADJ_FILENAME` equals `RESTART_ADJ_FILENAME`, call `SU2_MPI::Error` in the Task 1 guard. Add this case to the Task 1 test.

- [ ] **Step 1: Write the failing test.** In `check_extraction.py`, replace the `restarted` run (`rel_tol=1e-5`) with an `extract_only` run. It uses `SST_EPM_EXTRACT_ONLY=YES`, `RESTART_SOL=YES`, `ITER=1`, `SOLUTION_ADJ_FILENAME=<enabled>/restart_adj`, `RESTART_ADJ_FILENAME=restart_adj_out`. Assert:
  - every `dJ_da_at_zero` matches `enabled` with `rel_tol=1e-10, abs_tol=1e-14`;
  - the SHA-256 of `<enabled>/restart_adj.dat` is unchanged.

  Keep the old loose `restarted` comparison as well. It still documents ordinary resumption.
- [ ] **Step 2: Run** on CFDHub. Expected: FAIL.
- [ ] **Step 3: Implement.**
- [ ] **Step 4: Run** 2D and 3D. Expected: PASS.
- [ ] **Step 5: Commit** with the message `Add extract-only EPM replay from saved adjoint`.

### Task 7: Field file writer

**Files:**
- Create: `SU2_CFD/include/output/CEPMFieldWriter.hpp`, `SU2_CFD/src/output/CEPMFieldWriter.cpp`. Add it to `SU2_CFD/src/meson.build` and to the unit-test source list.
- Modify: the driver, which calls the writer at the end of `ExtractEPMFieldSensitivities()`.

**Interfaces:**
- Produces: `void WriteEPMField(const CEPMFieldSites& field, const CConfig& config, const CGeometry& geometry, const std::string& objective, const std::string& extras_json)`. `extras_json` is a pre-formatted JSON fragment, for example the extract-only residuals. Build the JSON by hand with `ostringstream`. Do not add a new dependency.
  - Each rank writes `sst_epm_field_<OBJ>_rank<r>.bin` with its own records. Each record is one row of `sizeof(Record)/8` float64 values, in the Task 2 field order, with no padding. Add a `static_assert` that `Record` is standard-layout and contains only `passivedouble`.
  - There is no gather to the master. The master gathers only the per-rank record counts.
  - The master writes `sst_epm_field_<OBJ>.json` with:
    - `"columns"`: the flat names, for example `"grad_0".."grad_8"`, `"dtau_2_5"`;
    - `"record_bytes"`;
    - `"ranks"`: a list of `{file, count}`;
    - `"nDim"`, `"objective"`, `"delta_b": 1`, `"urlx": 1`;
    - `"gap_tolerance"`, `"mpi_ranks"`, `"su2_version"`;
    - `"source_commit"`: from a `-DSU2_EPM_COMMIT=` define if present, else `"unknown"`;
    - `"ref_dimensionalization"`;
    - `"case_names"`: `["1C_KEEP","1C_SWAP","2C_KEEP","2C_SWAP","3C"]`;
    - `"kinds"`: `{"EDGE":0,"BOUNDARY":1,"NODE":2}`;
    - `"extract_only_residuals"`: residuals, or `null`.

- [ ] **Step 1: Write the failing test** `"Writer layout"` with `[EPMField]`. Create 3 synthetic sites with known values and call the writer in a temp directory. Read the `.bin` back with `std::ifstream`. Then assert:
  - file size equals `3 * record_bytes`;
  - `column[i]` of row 2 equals the value set;
  - the JSON contains `"count": 3` and the `dtau_4_8` column name.
- [ ] **Step 2: Run.** Expected: FAIL.
- [ ] **Step 3: Implement.**
- [ ] **Step 4: Run.** Expected: PASS.
- [ ] **Step 5: Commit** with the message `Write per-rank EPM field files`.

### Task 8: Python reader

**Files:**
- Create: `TestCases/disc_adj_rans/sst_epm_adjoint/read_epm_field.py`, `test_read_epm_field.py`

**Interfaces:**
- Produces:
  - `load(json_path) -> dict[str, np.ndarray]`: concatenates all ranks, one array per column.
  - `merge(fields) -> fields`: merges duplicate `(kind, key_a, key_b)` rows. It **sums** the `S_*` columns and keeps the first copy's state, `dtau`, `dpk` and `dpw` columns. Copies agree because they are computed from the same halo state. If a copy's `dtau` differs from the first by more than `1e-12` relative, it raises `ValueError`.
  - `classical_gradients(fields) -> dict[case, float]`: `Σ S_tau·dtau[m] + S_pk·dpk[m] + S_pw·dpw[m]`, computed on the **unmerged** rows. This equals the merged form, because duplicate copies carry their own `S` share. The function is also checked on the merged rows.
  - CLI: `python3 read_epm_field.py <json> --csv <sst_epm_sensitivities.csv>`. It prints the per-case relative difference and exits 1 if any value is above `1e-10`.

- [ ] **Step 1: Write the failing tests** (pytest, synthetic files written with numpy in `tmp_path`):
  - `test_merge_sums_halo_copies`: two ranks share EDGE (3,7) with `S_tau_1` equal to 0.25 and 0.75. After merging, there is one row with `S_tau_1 == 1.0`.
  - `test_classical_matches_manual`: `classical_gradients` equals a hand-computed sum.
  - `test_inconsistent_copy_raises`: copies with different `dtau` raise `ValueError`.
- [ ] **Step 2: Run** `python3 -m pytest TestCases/disc_adj_rans/sst_epm_adjoint/test_read_epm_field.py -q` (no SU2 build is needed, so this runs locally). Expected: FAIL.
- [ ] **Step 3: Implement.**
- [ ] **Step 4: Run.** Expected: 3 passed.
- [ ] **Step 5: Commit** with the message `Add EPM field reader`.

### Task 9: Smoke runner, full checks

**Files:**
- Modify: `TestCases/disc_adj_rans/sst_epm_adjoint/check_extraction.py`

- [ ] **Step 1: Add these assertions** to the `enabled`, `extract_only` and `scalar` runs:
  - `read_epm_field.py <json> --csv <csv>` exits 0.
  - EDGE site count is at least the interior edge count (the existing `edges` formula). NODE count is at least the existing source minimum. Each count equals the CSV `stress_evaluations` and `source_evaluations`, minus the repeated uses: compare against `Σ uses`.
  - The `extract_only` field files equal the `enabled` field files per merged key, to `1e-10` relative, with an absolute floor of `1e-10 × max|S|`.
  - The `.bin` size equals `count × record_bytes` for every rank.
  - Add `field_file_bytes` and `field_sites` to `summary.json`.
- [ ] **Step 2: Run** 2D and 3D serial on CFDHub. Expected: PASS.
- [ ] **Step 3: Commit** with the message `Check EPM field files in smoke runner`.

### Task 10: MPI gate on CFDHub

**Files:**
- Modify: `check_extraction.py`. With `--ranks 2 --reference <serial dir>`, run `extract_only` in both modes, from the **serial** `enabled/restart_adj`, with 2 ranks.
- Modify: `TestCases/disc_adj_rans/sst_epm_adjoint/smoke_results.json`, `README.md`.

- [ ] **Step 1: Add the 2-rank comparisons** against serial:
  - FIELD `g_m`, to `1e-10`.
  - SCALAR_REPLAY `g_m`, to `1e-10`.
  - Merged per-key `S_*`, with the same tolerance as Task 9.
  - Use `mpirun` as provided by the SGE parallel environment, replacing `--host localhost`, when `$PE_HOSTFILE` is set.
- [ ] **Step 2: Run** the 2D and 3D 2-rank jobs. Expected: PASS. If SCALAR_REPLAY fails, debug it with superpowers:systematic-debugging. The fix is in scope (spec §9.3). Add a unit test that pins the root cause before fixing it.
- [ ] **Step 3: Record** the binary SHA-256, commit, timings, sensitivities, field sizes and rank counts in `smoke_results.json`. Use the format of the existing file.
- [ ] **Step 4: Commit** with the message `Validate EPM field extraction on two MPI ranks`.

### Task 11: Documentation

**Files:**
- Modify: `Docs/SU2_SST_EPM_adjoint_agent_instructions.md` (Section 1 status, "Implemented extraction interface", §12.6, §13), `config_template.cfg` (the two new options), `TestCases/disc_adj_rans/sst_epm_adjoint/README.md`

- [ ] **Step 1: Document** the following:
  - the FIELD default and SCALAR_REPLAY as the reference;
  - extract-only usage and its guard;
  - the field file format, pointing to the JSON as the source of truth;
  - the form 1 chain-rule data (spec §8);
  - the MPI result.

  State plainly what is verified and what is still pending: nonlinear Taylor checks, continuation, and wide SIMD/OpenMP.
- [ ] **Step 2: Commit** with the message `Document EPM field extraction`, and push `feature_UQ` to the fork. The parent spec already authorizes the push.
