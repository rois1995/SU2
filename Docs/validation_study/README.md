# First physical study: attached flat-plate boundary layers

The study design and nominal reference tables are prepared. **No physical case
has been run. This package is not yet a set of runnable solver cases.**
The remaining preparation inputs are stated below instead of being filled with
assumptions that could hide model error.

## Cases and the question being tested

Test whether a small, fixed collection of smooth stress fields gives a useful
range of wall friction for a smooth, attached boundary layer with almost no
streamwise pressure change. This is a narrow first claim. It says nothing about
separation, curved surfaces, three-dimensional flow or probability of error.

| Data set | Use | Selected observations |
|---|---|---|
| Wieghardt experiment, NASA-hosted table | Calibration | Cf at Re_theta 1079, 1998, 3409, 6150, 9878 and 13047 |
| Österlund/KTH SW981005A | Validation | Cf at Re_theta 12633.1; Ue 21.4208 m/s |
| Österlund/KTH SW981008A | Validation | Cf at Re_theta 26612.1; Ue 52.182 m/s |

All Wieghardt stations belong to one calibration group. All Österlund runs
belong to the validation group. Do not move stations between the groups or use
the validation results to select amplitudes. The faster Österlund case deliberately
tests a Reynolds number beyond the calibration range. These are held out from
fitting; the publicly available reference values are not claimed to be blinded.

The NASA archive supplies measured Cf values and describes the source experiment.
Its computational Mach number of 0.2 was chosen to accelerate a compressible
calculation; it must not be mistaken for the measured inlet condition.
[NASA case description](https://www.grc.nasa.gov/www/wind/valid/fpturb/fpturb.html),
[reference table](https://www.grc.nasa.gov/www/wind/valid/fpturb/WIECF.data).

The KTH files provide the velocity, density, viscosity, station and Cf used here.
Both selected measurements are at x=5.5 m. The experiment report is Österlund,
*Experimental Studies of Zero Pressure-Gradient Turbulent Boundary-Layer Flow*,
KTH, 1999. [Data description](https://www.mech.kth.se/~jens/zpg/index.html),
[SW981005A](https://www.mech.kth.se/~jens/zpg/data/zpg_v1/SW981005A.txt),
[SW981008A](https://www.mech.kth.se/~jens/zpg/data/zpg_v1/SW981008A.txt).

TMR's flat-plate correlations are useful secondary checks, but are not additional
independent experimental cases. The TMR page also cautions that its plotted 5%
error bars are not established error levels. This package does not adopt them as
measurement uncertainty. [TMR flat-plate description](https://tmbwg.github.io/turbmodels/flatplate_val.html).

## What is in this directory

- `study_preparation.json`: case groups, verified operating values, status and prerequisites.
- `references/*.csv`: eight nominal Cf values in the existing analysis-table format.
- `sources.json`: exact source URLs, selected source lines, citations and hashes of
  the transcribed scalar tables. It does not claim hashes of original remote bytes.
- `parameter_candidates.json`: three candidate strengths and four signed fields.
  These are proposed choices, not fitted parameters.
- `acceptance_gates.json`: declared output scales, convergence, mesh and field checks.
- `run_matrix.csv`: 58 unexecuted run definitions. It contains 18 baseline mesh
  runs, 24 calibration scenarios and 16 validation scenarios after parameter selection.
  This is the initial matrix, not a total cost estimate: numerical gates also
  require field refinement, sensitivity checks and second-start repeats.
- `results_template.csv`: headers only. There are no invented solution values.
- `check_preparation.py`: checks the package without running any solver.

The design uses linear SA and SST-2003m, fixed planar geometry and zero added
stress on every physical boundary. The scenario family starts with a broad
shear correction and two patches of opposite shear, each with both signs.
The candidates are epsilon=0.002, 0.005 and 0.01, with fixed smoothing and wall
length ratios. They are deliberately modest in number; they are not exhaustive
stress bounds. Use each declared case's physical reference length and freestream
speed when generating fields. Save those choices and the generated fields.

## Output and comparison rules

The primary quantity is signed Cf, divided by a fixed scale of 0.005 for reporting
errors and widths. Cf means wall traction along the downstream tangent divided
by half the freestream density times speed squared. Compute it for each actual
solution before forming a scenario range.

For this first canonical-flow study, compare Cf at predeclared Re_theta values.
Determine momentum thickness by integrating the dimensional velocity profile with
a documented edge velocity and enough wall-normal extent. Interpolate each
solution's Cf curve only within its covered Re_theta range; never extrapolate.
Report the physical streamwise station of each comparison separately, so the
change of coordinate cannot hide a shift in boundary-layer development.

Keep dimensional velocity profiles and U/Ue versus physical y as secondary
outputs. Wall-scaled profiles alone can conceal changes in wall friction.
The KTH full profiles are linked but have not been imported here; profile scoring
must wait for the original arrays and an agreed measurement-error treatment.

Each reference table has equal Lower and Upper values because only the nominal
mean has been retrieved. **This does not mean zero experimental uncertainty.**
Do not turn those point references into uncertainty-coverage claims. Retrieve the
source uncertainty before constructing physical reference intervals. Do not
invent a common percentage or fit reference intervals to make a band succeed.

## Before these cases become runnable

1. Retrieve and hash the original data files and experimental reports. Resolve
   the inlet, trip, pressure history, wall conditions and measurement uncertainty.
   Shell downloads were unavailable here; the small factual tables were checked
   through the web reader and transcribed with source-line provenance.
2. Match physical operating conditions and domains. Fully turbulent SA/SST has no
   transition model in this interface. Establish an evaluation region where trip
   and inlet treatment no longer control the result, using a separate sensitivity
   check. If that cannot be established, mark the case unsuitable for this scope.
3. Generate three nested meshes with adequate near-wall and outer-boundary
   resolution. Build the simplex field mesh separately. Verify that all comparison
   stations and field support lie within the intended domains. Do not rescale the
   field when changing the CFD mesh.
4. Run the baseline checks only after runs are authorized. Meet all gates in
   `acceptance_gates.json`; retain failures and stop affected branches.
5. Use calibration cases to choose the smallest candidate amplitude that meets a
   declared miss/width criterion for each model: zero interval misses at all six
   calibration outputs and mean range width divided by the fixed Cf scale no
   greater than 0.2. Source-supported reference intervals and numerical gates
   must be ready first. These are proposed study choices, not measured errors. If none does, record that the
   proposed family is inadequate. Do not inspect validation responses and retune.
6. Write a selected parameter file and a `SU2_STRESS_STUDY_1` study file, then use
   `validation.py freeze`. The preparation schema here is deliberately different,
   so it cannot be mistaken for an already calibrated, frozen study.
7. Evaluate the held-out cases under the same settings only after separate
   authorization. Report misses, widths, costs, failed cases and numerical errors.
   A descriptive range is still not a confidence interval.

The proposed numerical gates require six decades of residual reduction, small
Cf drift over 200 iterations, three CFD meshes, a first point at y+<=1, and
independent field refinement. In addition to fixed tolerances, each numerical
error indicator must be smaller than 10% of the smallest scenario response being
interpreted. A response below that noise level stays unresolved. These are chosen
study criteria, not results or claims that all cases can attain them.

Run only this offline integrity check now:

```bash
python3 Docs/validation_study/check_preparation.py
```

No scheduler files, cluster commands or automatic CFD launcher are included.
Preparation is complete as a concrete study design with real nominal references;
physical inputs, uncertainty intervals, meshes and calibrated parameters remain
explicit prerequisites for a runnable validation campaign.
