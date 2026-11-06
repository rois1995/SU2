# Experimental SA/SST prescribed-stress interface

This is a disabled-by-default research interface. It implements a saved physical
stress field and a same-parent SST EPM comparator. It has local operator/file
checks and tiny serial solver checks; it is not an uncertainty calibration.
Read the [assessment](SA_SST_APPROACH_ASSESSMENT.md),
[field format](STRESS_FIELD_FORMAT.md) and [verification report](CAMPAIGN_REPORT.md).

## Configuration

```ini
SOLVER= INC_RANS
KIND_TURB_MODEL= SST
SST_OPTIONS= V2003m
MGLEVEL= 0
STRESS_DISCREPANCY= PRESCRIBED
STRESS_DISCREPANCY_FILE= field.suf
STRESS_DISCREPANCY_SCALE= 1.0
STRESS_DISCREPANCY_PURPOSE= SCENARIO
STRESS_DISCREPANCY_RESTART= SAME_FIELD
VOLUME_OUTPUT= (COORDINATES, SOLUTION, PRIMITIVE, STRESS_DISCREPANCY)
```

These are feature settings, not a complete case. `prepare_example.py` below
writes a complete small mesh/configuration set. For linear SA use
`KIND_TURB_MODEL=SA`, `SA_OPTIONS=NONE`. The standard negative variant uses
`SA_OPTIONS=(NEGATIVE, WITHFT2)`. The parent parser still enforces its own rules.

For the comparator use `STRESS_DISCREPANCY=EPM_MATCHED`, no field file (or `NONE`),
`KIND_TURB_MODEL=SST`, `SST_OPTIONS=V2003m`, and explicit `UQ_COMPONENT`,
`UQ_PERMUTE`, `UQ_DELTA_B`, `UQ_URLX`. Do not enable `SST_OPTIONS=UQ` with it.
`UQ_URLX` is one algebraic blend, not iteration damping. `STRESS_DISCREPANCY_SCALE`
is an additional explicitly recorded amplitude in [0,1]. Negative scenarios use
a separately saved negative field. `NONE` preserves the existing solver route.

## Implemented support and restrictions

| Area | Current contract |
|---|---|
| Parent models | Linear SA/SA-neg; SST-2003m, without extra production modifiers |
| Flow | Steady, single-zone finite-volume RANS or density-based INC_RANS |
| Fluid | Ideal-gas compressible; constant-density incompressible; SI units |
| Grid | Stationary, MGLEVEL=0; no axisymmetry or periodic boundaries |
| Geometry | Analytic planar wall sheets or explicitly wall-free; box support |
| Field mesh | Positive triangles/tetrahedra, all five 3D coefficients; planar restrictions enforced |
| Boundaries | Prescribed stress must be exactly zero on every physical boundary vertex |
| Parallel code | Immutable replicated small field and rank identity check; MPI execution BLOCKED_ENVIRONMENT |
| Implicit solve | Parent approximation retained; complete new tangent not supplied |
| Adjoint/direct differentiation | Explicitly rejected |
| Curved geometry | Circle provider OFFLINE_ONLY; coupled circle/general CAD UNSUPPORTED |

The zero boundary requirement is checked at startup after reference scales are
set. This keeps wall force/heat and other boundary implementations consistent
with the first scope. It also applies to coupling-control files. Interior
nonzero field queries outside the saved mesh fail. Do not extend the support
claim to moving meshes, generic CAD, periodic ducts, multigrid, wall functions,
roughness, transition, QCR receiving models, hybrid RANS/LES, stochastic
backscatter, fixed-lift control, species, GPUs or pressure-based incompressible
flow. These configurations are rejected instead of silently omitting a term.

A QCR **donor** can use the existing QCR model with discrepancy mode NONE and
request the STRESS_DISCREPANCY output group. Its exported E is the QCR difference
from the linear parent, evaluated at nodes. It is diagnostic and is not applied
again. A prescribed QCR receiving model is unsupported; exact diagnostic replay
uses a linear parent and the separate mechanism below.

## What is inserted, and where

The kinematic correction is `E`; its momentum stress is `-rho E`. The scalar
viscous operator and compressible SIMD operator query it at the interior edge
midpoint, after forming their existing reconstructed velocity gradient. The same
stress tensor enters the native compressible energy-work flux. Incompressible
thermal flux is left alone. Boundary prescribed stress is already validated zero.

The residual always uses current density and velocity. The implicit matrix retains
the parent approximation, including native energy-work terms that already use
the stress. A complete derivative of the added momentum/SST terms and EPM state
feedback is not supplied. No exact-Newton or adjoint claim is made.

The SA source equation is unchanged. SST queries the field at each source node
and uses `deltaP=-rho E:grad(U)`. It adds this to the parent production **before**
its zero and upper limits. Both SST-2003 production equations use that limited
value. A positive-k branch avoids dividing a capped production by a tiny eddy
viscosity; exact wall sources retain the parent's skip. The zero correction uses
the original arithmetic path. A nonzero-work interior state with nonpositive
k, omega or eddy viscosity fails clearly.

Matched EPM forms `E=dev(R_effective)-A_parent`, including its blend once.
Removing the isotropic part preserves the 2003m pressure/stress convention in
all three comparison arms. It does not combine prescribed fields with legacy UQ.

| Location | Role |
|---|---|
| `Common/include/toolboxes/stress_field.hpp` | Validated physical field, exact P1 audit, point lookup and wall map |
| `Common/src/CConfig.cpp` | Options, unsupported combinations, unit conversion, rank/checkpoint identity |
| `CNumerics.hpp` | Shared signed work, matched EPM difference, QCR donor diagnostic |
| `flow_diffusion.cpp` | Scalar compressible/incompressible stress and native work flux |
| `numerics_simd/flow/diffusion/viscous_fluxes.hpp` | Compressible SIMD stress insertion |
| `turbulent/turb_sources.hpp`, `CTurbSSTSolver.cpp` | Signed SST source and source query coordinates |
| `CDriver.cpp` | Unit/restart and zero-boundary checks |
| `CFlowOutput.cpp`, `COutput.cpp` | Nodal diagnostics and restart sidecars |

`STRESS_E_XX/YY/ZZ/XY/XZ/YZ`, `STRESS_WORK`, `STRESS_VOLUME` are output field
identifiers. Their CSV labels are `Stress_E_XX`, etc. For SST the group also has
`STRESS_PARENT`, `RAW`, `LIMITED`, `OMEGA`, `LOWER_CAP`, `UPPER_CAP`, `COV_MIN`,
`CAP_GAP`, `K_FLOOR`, `OMEGA_FLOOR` (all prefixed `STRESS_`).
The minimum covariance eigenvalue is diagnostic; no hidden realizability limiter
changes the applied E. Values use SU2 internal units. Nodal E is not a substitute
for the face-midpoint E used in flux assembly. Sorted volume output excludes
halo duplicates; exported ghost volumes are zero. Use positive owned control
volumes for integration. CAP_GAP is raw minus limited production. Floor flags
use the actual solver lower limits. Query counts, failures, accumulated query
time and persistent bytes per rank are printed with result output.

## Restart and persistence

Every ASCII/CSV/binary checkpoint written with an active mode gets a matching
`checkpoint_stem.stress`. Copy this file with the checkpoint when changing its
name. SAME_FIELD checks mode, purpose, amplitude, model variant, physical field
hash (or EPM settings), solver/fluid kind and internal length/velocity/density
references. Missing or different identity aborts. NEW_SEGMENT explicitly permits
a changed field/amplitude or a baseline checkpoint with no sidecar. Record that
as a new experiment. This is an identity guard, not a checksum of the entire
checkpoint or proof that all boundary/numerical settings are unchanged.

## Offline preparation and analysis

The preparation and analysis tools below are in `SU2_PY/stress_field` and never
launch SU2. The separately described bounded solver-check runner launches only a
local executable, with no MPI launcher or scheduler.
Generate the tiny 81-node square and 13-case plan, then validate prepared inputs:

```bash
python3 SU2_PY/stress_field/prepare_example.py --output /tmp/stress-inputs
python3 SU2_PY/stress_field/campaign.py prepare --plan /tmp/stress-inputs/plan.json --output /tmp/stress-runs --config-probe /tmp/stress-checks/stress_config_probe --field-probe /tmp/stress-checks/stress_field_probe
```

The example has two configured iterations for future wiring checks. It is not a
convergence setup or a physical benchmark. `prepare` refuses existing output
directories, copies inputs, hashes them and writes `run.json` plus `run_matrix.csv`
with `PREPARED_NOT_RUN`. Without probes, validation is visibly NOT_CHECKED.

A plan has schema `SU2_STRESS_CAMPAIGN_1` and a `runs` list. Each run has a safe
unique `id`, `stage`, source `config`, `mesh`, optional `field`, optional `restart`,
and an `options` dictionary. Paths are relative to the plan. Options replace
existing entries, including multiline lists. Restart checkpoints are copied as
`initial.dat` or `initial.csv`, including a sidecar when present.

`campaign.py extract --run DIR --schema schema.json --output metrics.json`
requires an `execution.json` recorded by the runner, containing
`returncode:0`, the actual `command`, `inputs:{filename:sha256}`, and
`outputs:{filename:sha256}`. Changed inputs/outputs or absent evidence fail.
It does not infer convergence from exit code. The schema declares `files`, each
with `path`, `required` CSV columns, optional `metrics:{name:{column,units,window}}`
and optional `volume_column`/`integrate` lists. It reports final values, history
window ranges and volume integrals as EXTRACTED_NOT_QUALIFIED. Supply explicit
acceptance criteria before interpreting them. Set `stress_budget:true` on a file
entry and declare `covariance_tolerance` to obtain owned-volume stress/production
summaries. These are nodal quadratures, not an exact face-based energy balance.

`campaign.py compare --left a.csv --right b.csv --columns R1 R2 --scales 1 1
--key PointID --tolerance 1e-10 --output pair.json` aligns exact row IDs and
reports absolute/scaled RMS and maximum differences plus the worst ID. Different
meshes require explicit physical resampling. Scales are fixed positive values;
never normalize by a nearly zero converged residual. A failed comparison exits 1.

`export_target.py` takes `--csv`, `--mesh` (NPZ), `--definition`, `--probe`,
`--velocity-ref`, `--length-ref`, `--kind QCR|EPM`, `--donor-id`, `--output`.
The CSV needs `PointID`, `x,y[,z]` and the six `Stress_E_*` columns. Field nodes
must match donor output nodes exactly. It inverts the declared wall/support map
at those nodes, labels the artifact COUPLING_CONTROL, reports bounds separately,
and refuses incompatible wall values or stale output files. It does not silently
normalize a target into the scenario family. This is a nodal representation fit;
exact replay below checks insertion separately. Converged physical-field donor
recovery and representation-error studies remain unperformed.

## Same-state capture and replay

This diagnostic exports assembled residuals without a time update. It is limited
to one rank, one thread, ITER=1, implicit Euler for flow and turbulence, and at
most 10,000 mesh points. Use a separate new directory for each run.

For a donor checkpoint, keep its mesh and parent settings and add:

```ini
STRESS_DIAGNOSTIC= CAPTURE
STRESS_DIAGNOSTIC_FILE= control.sur
ITER= 1
```

Use SA QCR2000 with discrepancy NONE, or SST with EPM_MATCHED. PRESCRIBED capture
is also supported. This writes `control.sur`, `control.sur.flow.csv` and
`control.sur.turb.csv`. The tables contain global PointID and residual components
R0, R1, etc., in native equation order and internal units. RESIDUAL alone writes
the tables without creating a capture file.

Copy the same checkpoint, mesh and capture file to the receiving directory.
Use the linear parent (remove QCR2000 for SA) and:

```ini
STRESS_DISCREPANCY= REPLAY
STRESS_DISCREPANCY_PURPOSE= COUPLING_CONTROL
STRESS_DISCREPANCY_SCALE= 1
STRESS_DISCREPANCY_FILE= NONE
STRESS_DIAGNOSTIC= RESIDUAL
STRESS_DIAGNOSTIC_FILE= control.sur
ITER= 1
```

Replay uses the actual face/boundary/source correction, including the donor's
reconstructed gradients. The file binds the mesh bytes, initial flow/turbulence
state, units and selected parent/reconstruction settings. Each query key also
binds its kind, position, gradient, density, eddy viscosity and turbulence energy.
Checks reject changed queries, unused records, malformed/duplicate records,
checksum errors and incompatible identity. Capture files and residual tables
are not overwritten. Maximum size is 64 MiB and 300,000 unique queries.

The ordinary field sidecar check is replaced by the diagnostic identity check
for these controls, allowing the donor and receiver mode to differ. This does
not authorize ordinary nonlinear continuation with a different field. Exact
replay is not a nodal physical field, so it does not export a STRESS_DISCREPANCY
nodal group or work on a different state/grid. Compare residual tables instead.

For ideal-gas Roe/MSW diagnostic assembly, USE_VECTORIZATION=NO forces the scalar
route and YES uses SIMD. The log explicitly states the selected route. This
choice is deliberately confined to diagnostics; normal solver selection is
unchanged. Near-isotropic 1C EPM remains a documented parity qualification limit.

## Localized fields and study records

In a generator definition, the existing `seed` object may be replaced by:

```json
{"type":"LOCALIZED_PHYSICAL_V1","terms":[
  {"center":[0.3,0.2],"width":[0.15,0.05],"coefficients":[0,0,1,0,0]},
  {"center":[0.7,0.2],"width":[0.15,0.05],"coefficients":[0,0,-1,0,0]}
]}
```

Centers and positive widths use physical coordinates. The five coefficients use
the existing tensor basis; the final two must be zero in 2D. One to 64 terms
are combined, then passed through the existing smoothing and common scaling.
A directional seed width is not a directional smoothing operator.

`validation.py` accepts a study JSON such as:

```json
{"schema":"SU2_STRESS_STUDY_1","family":"declared flow family",
 "parameters":"parameters.json","cases":[
   {"id":"case_a","group":"geometry_a","split":"CALIBRATION","reference":"a.csv"},
   {"id":"case_b","group":"geometry_b","split":"VALIDATION","reference":"b.csv"}
 ]}
```

Reference CSV headers are `Output,Lower,Upper,Scale`. Intervals and positive
reference scales must be declared before evaluation. Related cases must share
a group; groups cannot cross the split. Select parameters using calibration
cases, then freeze the study before looking at validation results:

```bash
python3 SU2_PY/stress_field/validation.py freeze --study study.json --output study.lock.json
python3 SU2_PY/stress_field/validation.py assess --lock study.lock.json --results results.csv --output assessment.json
```

Result headers are
`Case,Output,Baseline,Minimum,Maximum,Requested,Completed,CostSeconds`.
Use empty Minimum/Maximum with Completed=0 for all-failed scenarios. Missing
cases stay visible. Partial scenario ranges are labelled incomplete. Reference
and parameter hashes are checked; outputs are not overwritten. These tools do
not fit parameters, choose reference uncertainty, prove convergence or compute
probabilistic coverage. Numerical-error studies remain separate evidence.

## Reproduce the small checks

```bash
python3 SU2_PY/stress_field/run_local_checks.py --build-dir /tmp/stress-checks --v07-root /path/to/SA_stress_uncertainty_v07
```

This builds a small serial Catch executable and two probes with C++17, and runs
operator, replay, parser, sidecar, field, preparation, synthetic-target and study
definition tests. Python
needs NumPy/SciPy. It never creates a flow solver. The normal Meson unit suite
also includes `stress_coupling_tests.cpp` and `stress_simd_tests.cpp`. The complete
serial solver link and direct scalar/SIMD viscous operator comparison passed.
The bounded runner below also checks assembled residuals and actual disk
continuation. MPI and physical studies remain unqualified; see the report.

The complete build used the pinned local Eigen, MEL and Meson sources and a
local Ninja executable. No dependencies were downloaded. With those available,
the commands were (set `NINJA` to your local executable):

```bash
export NINJA=/path/to/ninja
python3 preconfigure.py --no-codi --no-medi --no-opdi --no-mpp --no-coolprop --no-mel --no-fado --no-mlpcpp --no-eigen
python3 externals/meson/meson.py setup /tmp/su2-uq-build . --wrap-mode=nodownload --buildtype=debug -Ddebug=false -Dwith-mpi=disabled -Dwith-omp=false -Denable-tecio=false -Denable-cgns=false -Denable-autodiff=false -Denable-directdiff=false -Denable-tests=false -Denable-pywrapper=false
"$NINJA" -C /tmp/su2-uq-build -j 2 SU2_CFD/src/SU2_CFD
```

The additional SIMD test links those serial libraries. After the focused script
above has created `/tmp/stress-checks/test0.o`, run from the repository root:

```bash
g++ -std=c++17 -O0 -march=native -Iexternals/catch2 -Iexternals/mel -Iexternals/eigen -c UnitTests/SU2_CFD/numerics/stress_simd_tests.cpp -o /tmp/stress-checks/stress_simd.o
g++ /tmp/stress-checks/test0.o /tmp/stress-checks/stress_simd.o -Wl,--start-group /tmp/su2-uq-build/SU2_CFD/src/libSU2core.a /tmp/su2-uq-build/Common/src/libSU2Common.a -Wl,--end-group -o /tmp/stress-checks/test_stress_simd
/tmp/stress-checks/test_stress_simd
```

These commands compile the flow solver but execute only the small operator test.

To execute only the bounded serial integration suite:

```bash
python3 SU2_PY/stress_field/run_solver_checks.py --solver /tmp/su2-uq-build/SU2_CFD/src/SU2_CFD --output /tmp/stress-solver-checks
```

The output directory must be new. The runner generates 81-node and 27-node
fixtures, permits only one to four iterations per solve, uses a 45-second timeout,
and forces one thread. It saves configuration, mesh/field/restart hashes, logs,
execution records and a report. Four expected changed-input failures count as
successful rejection checks. Fixed internal-unit residual scales are one with
1e-10 tolerance; full saved-output restart tolerance is 1e-9. These are small
implementation checks, with no cluster access and no physical validation claim.
