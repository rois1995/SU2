# Paper cases: classical EPM versus the adjoint estimate

This directory prepares the results campaign; it does not run it. The user has
selected CFDHub for execution; follow the [cluster workflow and storage contract](../../../../Docs/CFDHUB_SST_EPM.md).
SU2/postprocessing outputs stay inside the modified checkout under global scratch;
MPI and scheduler runtime files keep their default locations. The
scientific question is the accuracy and cost of approximating the **corrected,
unchanged classical EPM endpoints**, including cases where the approximation fails.
The five-case range is an epistemic response envelope, not a confidence interval
or a guaranteed bound.

## Selected matrix

| Case | Conditions | Purpose | Adjoint quantities | Nonlinear diagnostics |
|---|---|---|---|---|
| NACA 0012 | 2D, M=0.15, Re_c=6e6, alpha=2 degrees | Attached-flow derivative verification and endpoint error | CD, CL | Cp, Cf, wall y+ |
| NASA wall-mounted hump, no plenum, no control | 2D, M=0.1, Re_c=936000 | Separation, reattachment and strong model sensitivity | Lower-wall CD, CL | Cp, signed Cf, separation/reattachment, velocity and shear-stress profiles |
| ONERA M6, test 2308 | 3D half wing, M=0.8395, Re_MAC=11.72e6, alpha=3.06 degrees | Three-dimensional and shock/boundary-layer challenge | Wing CD, CL | Sectional Cp, shock position, Cf, wall y+ |

NACA at 2 degrees is a controlled verification operating point with nonzero lift,
not an exact match to the published 0/10/15-degree Cp comparisons. A subsequent
10-degree airfoil comparison is useful if the paper needs matched experimental Cp.
Do not use 15 degrees as the sole separation benchmark: steady convergence and
separation must be demonstrated, and near-stall experiments have 3D effects.

The hump is the principal separated-flow case. Its two force objectives integrate
the **entire lower wall**, including the upstream development and recovery sections,
normalized by chord times unit span. They compare the estimator with nonlinear EPM;
they are not experimentally measured isolated-hump forces. Keep the wall extent,
reference pressure and area identical across runs and grids. Reattachment position,
Cp and Cf are diagnostics of the nonlinear solutions: the current drag/lift adjoints
do **not** predict their local uncertainty. New scalar objectives would need new
adjoint work. NASA's posted SSTm solutions are useful context, not an exact reference
for this branch's SST-2003m variant.

ONERA M6 is a challenge case, not an assumed success. We use JST with fixed CFL=1,
without a slope limiter, because the extraction implementation excludes limiters.
All its baseline, adjoint and EPM configurations use that same discretization.
Demonstrate shock resolution and converged coupled adjoints before interpreting it.

## Preparation

From the SU2 repository root:

```bash
PAPER_CASES=TestCases/disc_adj_rans/sst_epm_adjoint/paper_cases
python3 "$PAPER_CASES/prepare.py" naca0012 build-epm-local/paper/naca0012
python3 "$PAPER_CASES/prepare.py" nasa_hump build-epm-local/paper/nasa_hump
python3 "$PAPER_CASES/prepare.py" onera_m6 build-epm-local/paper/onera_m6
```

These commands create **24 fully expanded configurations**, with each option defined
once, and three manifests marked `mesh_required`. They leave the missing mesh
explicit; they do not fabricate a ready-to-run campaign. The checked-in `.cfg`
files are generator overlays, not files to pass directly to SU2.

When meshes are available, prepare a **new** directory for each case and grid:

```bash
python3 "$PAPER_CASES/prepare.py" naca0012 /path/to/new/naca-medium \
  --mesh /path/to/naca-medium.su2
python3 "$PAPER_CASES/prepare.py" nasa_hump /path/to/new/hump-medium \
  --mesh /path/to/hump-medium.su2
python3 "$PAPER_CASES/prepare.py" onera_m6 /path/to/new/m6-medium \
  --mesh /path/to/m6-medium.su2
```

The preparer checks the ASCII SU2 dimension, nonzero point/element counts and known
boundary-marker sets, copies the mesh, and records its source path and SHA256 plus
configuration/template hashes. It refuses to overwrite a campaign. This checks
the input contract, **not** connectivity, geometry correctness or grid independence.
Archive the upstream mesh URL/version, conversion command and hash with the manifest.

Every prepared case contains:

| Directory | Input | Depends on |
|---|---|---|
| `baseline` | Ordinary SST-2003m direct solve | Checked mesh and physical setup |
| `adjoint_drag`, `adjoint_lift` | Fresh, fully coupled adjoints; `SST_EPM_POSTPROCESS=YES` | Same converged baseline restart |
| `nonlinear/1C_KEEP`, `1C_SWAP`, `2C_KEEP`, `2C_SWAP`, `3C` | Ordinary direct classical EPM, `(V2003m,UQ)`, both strength controls=1 | Baseline restart as initial guess |

Each directory has `run.cfg`; paths are relative to that job's working directory.
All output restarts are local, so endpoint and adjoint runs cannot overwrite the
baseline. Fresh adjoints use `RESTART_SOL=NO` while still reading the primal;
nonlinear endpoints use `RESTART_SOL=YES` with the baseline as an initial guess.
Each adjoint writes its own `sst_epm_sensitivities.csv`.

Future CFDHub execution uses the cluster-built reverse-AD binary without
OpenMP in each job directory: `SU2_CFD_AD run.cfg > run.log 2>&1`. Solve and qualify
the baseline first. Each endpoint supplies both CD and CL; do not duplicate primal
solves per objective. MPI campaigns require the still-pending runtime verification.
There is intentionally no automatic solver/cluster launcher in this directory.

## Meshes and physical setup

**NACA 0012:** chord=1, unit-span reference area=1, markers `airfoil`, `farfield`.
Use a consistent family matching the NASA sharp-trailing-edge geometry and verify
farfield sensitivity. The existing [pinned SU2 225x65 mesh](https://raw.githubusercontent.com/su2code/TestCases/4d2b3728e93f657199c9eec1919c4a752e1159cb/rans/naca0012/n0012_225-65.su2)
is suitable for verification setup, not proof of mesh-independent results.
Its Git blob is `a16d4dafde7d62c7fec3e4a54717cb30d79ccf81`.
Use the [NASA mesh/numerical-analysis resources](https://tmbwg.github.io/turbmodels/naca0012_val.html)
for the refinement study. Temperature=300 K, turbulence intensity=0.001 (fraction),
viscosity ratio=10 are declared study inputs. Compare fully turbulent force results
with suitably tripped experiments, keeping Reynolds number and operating point matched.

**NASA hump:** use the [no-plenum, contoured-upper-wall grid family](https://tmbwg.github.io/turbmodels/nasahump_grids.html).
The [NASA archive](https://www.nasa.gov/wp-content/uploads/2026/02/nasahump-grids.zip)
contains the formatted 2D PLOT3D grids. The converter accepts only that one-block
2D format, with or without gzip, and preserves double precision and chord units:

```bash
python3 "$PAPER_CASES/convert_hump.py" \
  /path/to/hump2newtop_noplenumZ409x109.p2dfmt.gz /path/to/new/hump-409.su2
```

It rejects invalid cell orientation/nonconvex cells and records the source-file
SHA256 in the mesh header. Boundary mapping: lower no-slip adiabatic `wall`,
contoured slip `upper`, total-condition `inlet`, static-pressure `outlet`.
Use 409x109, 817x217 and 1633x433 as the initial refinement plan; accept or refine
further based on observed changes and y+, not their names. The 103x28 and 205x55
grids are useful for setup checks, not substitutes for resolution evidence.

The chord-normalized grid is represented with c=1 m and Re_c=936000. The preparer
computes the reference density/pressure using SU2's Reynolds initialization and
explicit Sutherland constants, then applies NASA's ratios
`Tt/Tref=1.002`, `Pt/Pref=1.007`, `Pout/Pref=0.99962` from the
[FUN3D reference input](https://github.com/TMBWG/turbmodels/blob/main/Nasahump_validation/fun3d.nml_typical_hump_sst).
Tref=537 degrees Rankine=298.333333 K. This is dynamically scaled geometry,
not a claim of the experiment's dimensional ambient pressure.
Use intensity=0.00077 and viscosity ratio=0.009 at the inlet. Check the developed
boundary-layer profile at x/c=-2.14, target thickness about 0.08c, and achieved
reference velocity/Mach before accepting the setup. If back-pressure adjustment
is needed, document and freeze it across the full comparison. Do not substitute
SU2's existing bump-in-channel case: it is a different geometry and experiment.

**ONERA M6:** use metre coordinates, MAC=0.64607 m, adiabatic `WING`, `FARFIELD`,
and `SYMMETRY`. `REF_AREA=0` asks SU2 to derive the projected monitored half-wing
area; record the actual resolved value and keep coefficient conventions consistent
across grids. Temperature=288.15 K, declared intensity=0.001 and viscosity ratio=10.
The [SU2 tutorial mesh](https://su2code.github.io/tutorials/Turbulent_ONERAM6/)
has only 43,008 elements and is expressly a setup mesh. Obtain/build a consistent
three-level, wall-resolved family before drawing paper conclusions. No refined M6
mesh is bundled or claimed to have been checked here.

External benchmark meshes/data are **not downloaded by these scripts or bundled**.
The NASA PLOT3D converter has been checked with synthetic fixtures; conversion of
the actual NASA grids and geometric inspection remain pending.

### Existing mesh families inspected on 2026-11-08

The user supplied local verification mesh folders. These assets need not be
downloaded again. Inspection was read-only; no CFD or local SU2 build was run.

| Family | Files and point counts | Setup consequence |
|---|---|---|
| NACA 0012 quadrilaterals | `Mesh_1` through `Mesh_5`: 3,704; 14,576; 57,824; 230,336; 919,424 | Unit chord and approximately 500-chord farfield checked on the coarsest grid; candidate refinement family |
| Bump in channel | `Mesh_0` through `Mesh_4`: 3,649; 14,337; 56,833; 226,305; 903,169 | Different from NASA hump; do not use with `nasa_hump.cfg` |
| ONERA M6 mixed viscous candidates | `Coarse_NS.cgns`: 912,869; `Medium_NS.cgns`: 2,397,578; `Fine_NS.cgns`: 8,866,049; `VeryFine_NS.cgns`: 35,448,464 | CGNS metadata contains prism/pyramid/tetrahedral sections; qualify wall resolution and cluster AD memory requirements before selecting levels |

NACA's supplied markers are `Wall` and `Farfield`; the preparer now maps the
template's `airfoil`/`farfield` to these names in all boundary, plotting and
monitoring settings. ONERA ASCII files use `WING3D` instead of `WING`, which is
also supported. Original mesh bytes are preserved, and the mapping is recorded
in the manifest. Unknown or mixed marker sets still fail rather than being guessed.

The separately supplied M6 `Coarse.su2` has 61,330 points and only tetrahedra;
it is not the same mesh as `Coarse_NS.cgns`. Do not choose by the word "Coarse"
alone. The current preparer accepts ASCII SU2 only: the NS CGNS family needs a
verified conversion on the cluster or explicit CGNS preparation support before
use. Geometry/scale, trailing-edge definition and near-wall spacing still require
qualification; the inspected ASCII half-span is about 1.21848 m. Metadata alone
does not establish agreement with test 2308 or suitability for SST.

The supplied channel bump occupies x=0..1.5 and rises 0.05 above a flat lower
boundary; its domain extends x=-25..26.5 with a flat upper boundary. It cannot
replace the no-plenum NASA hump's geometry, incoming boundary layer and contoured
upper boundary. It can be an additional controlled verification case, but it
does not fill the separated NASA-hump slot in this matrix.

## Results and acceptance plan

1. Establish steady baselines and adequate grids: all flow/SST residuals below
   1e-10, force drift over the final 200 iterations, tighter-solve noise estimates,
   y+ approximately <=1 on relevant walls, and three-grid changes smaller than the
   estimator errors being interpreted. Retain failed/oscillatory cases as failures;
   the iteration limit of 20,000 is a stop cap, not convergence evidence.
2. Validate coupled-adjoint derivatives before claiming they are correct. The
   planned h=0.005, 0.01, 0.02 one-sided CFD Taylor checks require a nonlinear
   continuation interface that is **not implemented**. No such configs are emitted.
   Neither lowering `UQ_DELTA_B` nor relabeling the endpoint as a derivative replaces
   that check. Record ambiguity/probe spread; retain `frozen_forcing_unverified`
   until the relevant mathematical/numerical qualifications are established.
3. Compare each canonical prediction `J0 + dJ/da` with its corresponding converged
   full-strength EPM value. Report signed/absolute errors in coefficient units,
   error relative to the nonlinear response when noise-resolved, and error in the
   envelope endpoints/width. Define the displayed envelope over the baseline plus
   the five targets consistently. Include all five individual points so a good
   envelope cannot conceal incorrect individual predictions.
4. Show baseline/EPM Cp and Cf, hump separation/reattachment and velocity profiles,
   and spanwise M6 Cp sections. These explain physical changes and comparison with
   experiment; they are not local adjoint uncertainty bands. Report any pressure
   reference shift explicitly, particularly for the hump.
5. Measure wall time, core-hours, memory, iterations, rank/thread counts, hardware,
   executable/source hashes and restart hashes. Separate baseline, each adjoint,
   tape/replay extraction, five nonlinear solves and validation overhead. Report
   one-QoI and two-QoI costs independently. No speed-up is assumed.

At one grid per case this setup is **18 primal solves + 6 adjoints = 24 jobs**,
before retries: 3 baselines and 15 endpoints, plus 30 canonical replays within the
six adjoint jobs (extra ambiguity probes may occur). Three grids per case would
make 72 jobs. The planned 15 small-amplitude states per case are additional work,
not part of this runnable setup. The tiny existing serial smoke tests establish
infrastructure only, and the two-rank runtime gate is still pending.

Use three main paper figures: (a) derivative/Taylor verification once available;
(b) five predicted versus nonlinear responses and envelopes for each QoI/case;
(c) cost versus prediction error. Supplement with grid/noise tables and selected
flow diagnostics. Poor agreement in a challenging case is a result to report,
not a reason to weaken classical EPM or silently remove the case.

## Lightweight setup checks

```bash
python3 "$PAPER_CASES/test_prepare.py"
python3 "$PAPER_CASES/test_prepare.py" --binary /path/to/SU2_CFD_AD
```

The second command only invokes `--dryrun` on synthetic metadata fixtures: three
baselines finish dummy-geometry preprocessing, and 21 dependent configs parse and
reach the expected missing-primal-restart error. SU2 dry-run mode still loads
restarts. This is a configuration check, not a solver or real-mesh validation.
Both tests pass locally; no CFD iterations or MPI/cluster commands are launched.

## Primary benchmark references

- [NASA NACA 0012 definition and experimental sources](https://tmbwg.github.io/turbmodels/naca0012_val.html).
- [NASA hump definition and experimental data](https://tmbwg.github.io/turbmodels/nasahump_val.html),
  [SSTm reference results and inflow settings](https://tmbwg.github.io/turbmodels/nasahump_val_sst.html).
- [NASA ONERA M6 test 2308 and pressure data](https://www.grc.nasa.gov/www/wind/valid/m6wing/m6wing.html),
  [SU2 setup and coarse-mesh warning](https://su2code.github.io/tutorials/Turbulent_ONERAM6/).
