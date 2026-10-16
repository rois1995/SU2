# Common Newton–Krylov qualification, 16 October 2026

The steady primal CPU implementation now has one controller for mean flow,
SA/SST, existing SA-BC/LM transition and basic species transport. Selection and
grouping are independent. Both segregated and coupled solves retain refreshed
physical derivatives, independent true-defect checks, globalization and bounded
recovery. The final audit closes the original ten requirements within the
capabilities below. Historical sections of `NewtonKrylovRefactor.md` describe
earlier checkpoints; this document records the final qualification.

Numerical source is `a3fcc53d2e6fd0831c1a332f90e02a9b1db0a6a5`.
The subsequent production change `b621a12bf473115ae4b66a40da20c5048342ba8b`
fixes empty-rank collective output. Object hashes establish that every solver,
linear algebra and geometry object is unchanged. Final binaries are recorded in
[NewtonKrylovQualification.json](NewtonKrylovQualification.json), together with
source and evidence hashes. Later reference/configuration-comment updates do
not change any solver control or tolerance.

[The performance report](../TestCases/newton_krylov/PERFORMANCE.md) contains the
144-run original-controls comparison, medians, physical agreement, phase
profiles, memory measurements and separate adaptive-control experiments.
Segregated loop time improves by 29–34% serial and 12–17% on the tested 2x2
SA/SST/LM fixtures; native still wins those fixed-CFL comparisons. Coupled LM
retains a substantial iteration/runtime regression. These small cases do not
establish production scaling. Additional results here are correctness samples,
not new performance medians.

## Architecture and detecting checks

| Requirement | Implementation and evidence |
|---|---|
| One algorithm | `CNewtonIntegration` owns forcing, Krylov, true defects, Armijo, recovery and diagnostics. The separate SST controller is removed. `NewtonController`, `NewtonFlowPolicy`, `NewtonProduction`. |
| Selection/grouping | Configuration validates available equations; factory and native dispatch give each group one owner. Flow-only, auxiliary-only and selected grouped modes are exercised. `NewtonProduction`, `NewtonExplicit`. |
| Cross derivatives | Install all trial unknowns, then refresh dependencies and evaluate the full residual. Independent centered references and step-size sweeps cover cross-block effects, including mixture density. `NewtonEquations`, `NewtonLinearization`, `NewtonPeriodicProducts`. |
| Preconditioning | Native per-system diagonal/lower blocks precondition the full operator. Lower-block products do not replace its cross derivatives. `NewtonLinearization`, `NewtonPeriodicPreconditioner`. |
| Coordinates/mass | Scalar coordinates use fixed base density; the residual retains live trial density. Native dense/projected pseudo-time mass, scaling and prescribed rows are tested. `NewtonMass`, `NewtonPeriodicMass`, `NewtonPeriodicAxis`. |
| Trial/rollback | Capture primary/old values, numerical history, masks, relaxation and configuration bookkeeping; rebuild live dependencies and preserve physical-time history. `NewtonBoundaries`, `NewtonController`, `NewtonResidualFailure`, `NewtonRepeatability`. |
| Admissibility | Signed SA, SST/LM bounds, composition and live thermodynamics/transport have model-specific checks. Feasible finite differences retain their requested direction. `NewtonBounds`, `NewtonBoundProbes`, `NewtonProperties`, `NewtonTransition`. |
| Execution | MPI/halo/rotation and group failures have common ownership. Matrices remain per-system blocks, avoiding a packed matrix beyond `MAXNVAR=20`; native scalar capacity is guarded. `NewtonProduction`, `NewtonPeriodicOwner`, `NewtonPeriodicAxis`. |
| Correctness | 56 native cases pass on 1x1, 1x2, 2x2 and 2x4. All 91 supported ordinary cases pass. Dedicated cache/gradient, Krylov storage, writer, CLI/restart and Valgrind checks pass. |
| Convergence | Native/segregated/coupled cases converge for all named families; active auxiliary equations, full state and physical quantities are checked independently. See the performance report and endpoint results below. |

The complete suite includes accepted/damped steps, linear failure, CFL retry,
resolved groups that resume after dependency changes, terminal recovery and
exact rollback. Species and existing flamelet shared-code regressions remain
in the native suite; no new flamelet capability was added during this work.
Valgrind reports zero detected errors for the selected gradient/cache and
Newton/controller/periodic/flamelet checks. Leak certification was not requested
or claimed.

The final ordinary-suite count includes the comma-containing periodic cache test
through the prefix `Periodic LS cache preserves*`. Earlier quoted-name and tag
input-file attempts produced a filter warning; their logs are retained. The
same cache case also passed independently under `[Gradients]` and `[LSQCache]`.
The ordinary suite must run from its fixture directory.

## Final family and layout checks

Fresh runs use the unchanged numerical endpoint and the original **uncached**
physical reporter. All endpoint checks include every active equation component,
state and quantities; Newton true defects remain at most 0.1 and recorded
rollback errors are zero. Each listed update count is identical on 1x1 and 2x2.
Maxima cover both layouts and all three modes, relative to native on that layout.

| Case | Updates native / segregated / coupled | Max state relative difference | Max quantity difference |
|---|---:|---:|---:|
| Manufactured mean flow | 67 / 69 / 69 | 1.90e-11 | 1.38e-12 |
| Basic mixture species | 81 / 82 / 82 | 2.88e-8 | 6.72e-9 |
| SA with active BC transition | 3334 / 2933 / 2933 | 8.50e-7 | 3.35e-7 |
| Periodic SA | 5294 / 4838 / 4838 | 1.03e-7 | 7.18e-8 |
| Periodic SST with active LM | 1961 / 1960 / 7799 | 6.56e-10 | 7.89e-11 |

All pass the original state/quantity limits of 1e-6 (2e-8 for manufactured flow).
Dimensional manufactured flow retains its original raw RMS limit of 1e-6 and normalized limit 1e-9;
its largest measured raw RMS is 8.96e-7. Species/SA-BC retain raw 1e-10 and
normalized 1e-9. Periodic cases retain raw 1e-12 and normalized 1e-11. The full
component residuals, cross-layout residual differences, transition activity,
work counts, memory, commands and timings remain in `family_checks.json` and
the underlying records. No threshold was changed to obtain these results.

SA additionally converges in all three modes on 1x4, 2x1, 2x4 and 4x2, retaining
3327/2927/2927 updates. Together with the performance cohort this checks six
layouts. The maximum full-state difference from the same mode on serial is
2.54e-10; maximum quantity difference is 9.73e-11. Every continuous contention
check accepted these verification runs. Their single timings are not scaling
or speedup evidence.

The nonuniform 37249-point SA fixture repeats three times on each of 1x1, 1x4,
2x1, 2x4 and 4x2. Actual teams are verified and every thread owns at least 3072
points. All fixed-layout configuration/history/binary artifacts are bitwise
identical. The fixture covers one accepted update and forced linear/search
failures with exact restoration. Across layouts, relative L2 differences are
at most 2.90e-14 for residuals, 1.49e-10 for products and 1.15e-11 for accepted
states. This is useful larger-mesh correctness evidence, not a fully converged
large-case performance or race-detector certification.

## Native reference changes: isolated causes

The three periodic finite-iteration histories predated the completed periodic
matrix rows. A diagnostic driver links the unchanged final numerical libraries
and toggles only the existing partial/complete matrix representation after
construction. Its complete mode reproduces the production history exactly;
its partial mode reproduces **all three old reference vectors exactly**. The
change includes the associated residual/mass/passive-row paths selected by that
representation. No physics, mesh, other correctness fix or tolerance changes.

| Registered case | Legacy partial-row vector | Completed-row vector |
|---|---|---|
| `periodic2d` | -3.266248, -0.617784, -0.620252, 2.216476, -1.007761, -1035 | -2.749889, -0.078200, -0.052556, 2.731392, -0.001883, -2514.8 |
| `periodic3d_axis` | -3.353165, 0.157501, 2.051458, -9.5944, -9.5944 | -3.423367, -0.370731, 1.987021, -8.1134, -8.1134 |
| `inc_periodic_weak_heat` | -4.712559, -5.747243, -5.751727, 873.702714 | -6.125121, -5.651670, -7.181485, 75.768123 |

The native SST nozzle reference also predated exact prescribed scalar rows and
the zero-k wall F1 limit. It is **identical across uncached FCE, cache755, lazy66,
fusion a3 and the output fix**. A separate diagnostic compilation restores only
legacy prescribed-row clipping and omits the wall F1 assignment; every original
baseline history row then matches exactly. Omitting F1 alone does not restore
the baseline, and that failed attribution is preserved. The corrected vector is
`[-1.879310, 4.022717, 0.544361, 4.457020, 702.56, 702.56]`.

The references therefore follow demonstrated correctness changes. All existing
1e-5 tolerances remain. These short checkpoints are regression histories, not
claims of physical convergence. The three registered periodic cases now pass
with original result output, as do all six corrected-baseline runs and six
configuration guards. The streamwise case tests native compatibility; it does
not enable common Newton for streamwise models.

## Empty-rank collective output

Baseline reruns exposed an inherited ROMIO321 hang after the numerical checkpoint
when a rank owned no surface output. A minimal probe calls the actual shared
`CFileWriter` helper: balanced payloads pass, whereas first-only, last-only and
globally empty payloads each time out in `MPI_File_write_all`. The old helper
passed count one of a zero-size derived type.

The fix retains collective participation and the existing block/remainder
encoding for nonempty payloads, but uses count zero with `MPI_BYTE` on empty
ranks. All four probe modes now complete with exact byte checks. A permanent
`[FileWriter]` regression passes on one, two and four ranks; its nonempty payloads
cross the 1 MiB block boundary. Balanced before/after bytes agree exactly.
Actual baseline and periodic outputs also finish; all 18 volume/surface VTU
files have complete appended blocks and finite floating payloads. No writer
performance improvement is claimed, and no numerical acceptance path changed.

## Reproduction and preserved evidence

The local archive is
`newton_krylov_refactor_20261015/goal_completion_20261016/` beside this checkout's
parent feature directory. `NewtonKrylovQualification.json` binds the important
source, binary and evidence hashes. The performance report gives its separate
reproduction commands and contention policy. All commands below run sequentially;
use fresh output directories and the matching fixture data.

From the workspace root, define paths and the MPI environment:

```sh
nk_art="$PWD/newton_krylov_refactor_20261015"
nk_source="$PWD/scalar_limiter_sst_globalization_20261015/source"
nk_audit="$nk_art/goal_completion_20261016"
export TMPDIR="$nk_art/mpi_tmp" OMPI_MCA_orte_tmpdir_base="$nk_art/mpi_tmp"
export OMPI_MCA_io=romio321 OMP_WAIT_POLICY=PASSIVE OMP_NUM_THREADS=1
```

The native and ordinary tests require the existing fixture working directories:

```sh
cd "$nk_art/equation_set_verification_work/native_mpi"
OMP_NUM_THREADS=2 mpirun --bind-to none -n 2 "$nk_audit/final_io/newton_equation_tests"
cd "$nk_art/equation_set_verification_work/units"
"$nk_audit/final_io/test_driver" --input-file "$nk_audit/supported_units_final.txt"
mpirun --bind-to none -n 4 "$nk_audit/final_io/test_driver" '[FileWriter]'
```

Repeatability and the scoped corrected-baseline runner preserve fresh evidence:

```sh
python3 "$nk_art/run_newton_repeatability.py" --output "$nk_audit/repeatability_new" \
  --layouts 1x1,1x4,2x1,2x4,4x2 --repeats 3
python3 "$nk_audit/baseline_runner.py" \
  --binary "$nk_audit/final_io/SU2_CFD" --data /home/rausa/Software/su2-TestCases \
  --output "$nk_audit/baseline_new"
```

The standard comparison scripts accept `--binary`, `--reporter`, `--output`,
`--ranks` and `--threads`; `compare_periodic_flatplate.py` also accepts `--model`.
Use the original uncached reporter in `performance_20261016/original/` when
checking geometry-cache independence. Exact used decks, meshes/restarts,
commands and continuous contention records remain beside every run. Attribution
commands and diagnostic compile/link argument lists remain under
`native_reference/` and `nozzle_boundary_control/`; run their saved `run.cfg` in
a fresh directory with the saved input files. The diagnostic source never
replaces production files.

`baseline_final/`, `baseline_io_fixed/`, the initial and round-two periodic
timeouts, the F1-only counterfactual, the initial probe filename-harness error,
and ordinary-suite filter/working-directory mistakes are preserved. They are
excluded from passing cohorts. No failed evidence was relabeled or deleted.

## Capability limits and remaining costs

The explicit common modes cover steady primal single-zone CPU density-based
flow, `MGLEVEL=0`, and available implicit selected scalar equations. Periodic
FLOW/TURB/TRANS groups are supported; periodic species and streamwise models
are rejected. Unsupported unsteady, AD/adjoint, multizone, GPU, pressure-based
and NEMO selections are not advertised. `CNewtonLegacyIntegration` remains for
older execution modes outside this migration. Earlier forward-AD translation
unit compile checks do not constitute AD runtime qualification.

No NEMO integration, flamelet feature expansion, general cache framework,
snapshot pool, new source Jacobian or multilevel preconditioner was added.
The local-paper review and existing accurate-SA/adaptive-control experiments
are preserved in the performance archive. More Jacobian terms belong in a
separately validated preconditioning experiment, while the physical matrix-free
product stays complete. Source sinks/positivity and active branches matter;
adding every production derivative is not an established robustness improvement.

Full flow assembly during auxiliary probes remains necessary for the current
boundary/dependency API. Snapshot duplication and Krylov high-water storage also
remain. The existing exact-Arnoldi-breakdown FGMRES limitation observed by an
isolated storage benchmark is not repaired here; the common controller's
independent true-defect check prevents accepting that bad linear result and
reports bounded failure/recovery. No large-case convergence guarantee, complete
SU2 regression certification or MPI/OpenMP race-detector claim is made.
