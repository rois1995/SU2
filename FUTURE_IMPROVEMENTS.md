# Improving the reliability of the SA/SST stress-field approach

The central idea is worth keeping: apply a saved stress correction, let SA or
SST respond, and measure what changes. The biggest scientific improvement is to
give the choice of corrections a stronger physical and experimental basis.
Smooth, bounded fields alone do not establish likely model errors.

This document describes future work. It does not claim that the changes or
validation below have been completed. The current evidence is recorded in
[CAMPAIGN_REPORT.md](Docs/CAMPAIGN_REPORT.md). Production runs and cluster access
remain excluded. The separate adjoint estimator remains outside the agreed scope.

## Recommended changes

### 1. Calibrate field strength and spatial scales against independent data

Amplitude, smoothing length and wall damping largely determine the resulting
range. Start with a small number of these parameters and estimate them from a
defined family of flows. Freeze those choices before testing on entire cases
excluded from calibration, including different operating conditions.

Measure both how often the range misses the reference and how wide it is. A
range that contains everything because it is enormous is not useful. Keep
inflow, measurement and numerical uncertainties separate, so the stress field
does not simply compensate for incorrect boundary conditions.

Calibration across multiple flow cases has precedent, but success within one
family does not establish general reliability. See
[Edeling, Cinnella and Dwight](https://sam.ensam.eu/bitstream/handle/10985/10035/Dynfluid-JCP-Cinella-2014.pdf?isAllowed=y&sequence=1).

### 2. Make the fields reflect local flow structure

Allow different variation lengths along and across a shear layer, and different
correction strengths in attached boundary layers, separated regions and wakes.
One global smoothing length and freestream-based amplitude may distribute the
correction poorly. Derive these choices from geometry and the baseline solution,
then freeze and save the field before the corrected solve.

Test the restrictions too: does the wall damping exclude known stress errors?
Does forcing zero correction at an outlet make the answer depend on the outlet
location? More flexible fields should earn their complexity through these tests.

### 3. Separate a mathematical correction from a physically possible stress

For SST, monitor whether the reconstructed total turbulence covariance implies
negative fluctuation energy in any direction. The correction alone need not be
positive. The existing minimum-eigenvalue diagnostic is a starting point; add
clear summaries of where and how strongly the condition is violated.

SA has no transported turbulence energy with which to complete the same check.
Describe its field as a momentum-stress discrepancy unless additional information
is introduced and justified. Do not invent an SA energy variable just to reuse
an SST test.

Flag violations rather than silently clipping the field during the solve:
clipping changes the supposedly fixed experiment. Record source-limiter activity
and the difference between the applied stress work and limited SST production.
Physical stress constraints do not by themselves guarantee plausible dynamics;
see [Matha and Morsbach](https://arxiv.org/abs/2311.01355).

### 4. Explore combinations of fields

Five simple directions are useful checks. Important responses may require a
correction concentrated near separation, opposite signs in different regions,
or a rotated three-dimensional stress. Build a modest collection of smooth,
localized fields, test both signs, and examine combinations that strongly affect
the selected output. Small finite-difference tests can guide selection without
an adjoint.

The computed extremes remain observed scenario extremes, not proven bounds over
every allowed field. Calculate lift or drag for each actual solution before
taking its range; a pointwise envelope is not a single realizable solution.

### 5. Finish exact donor replay

This is the first software priority. Save the donor correction at the actual
flux and source evaluation locations, then verify that replay reproduces the
donor's discrete equations at the same state. Keep face and source values
separate, including their reconstruction and units.

Next fit the same correction into the mesh-independent field representation and
measure the additional error separately. This distinguishes an insertion bug,
an inadequate field representation and a different nonlinear solution. The
current nodal fit cannot establish that distinction by itself. Recovering QCR or
EPM verifies the machinery; it does not validate predictions against reality.

### 6. Treat numerical robustness as part of the evidence

Improve the incomplete implicit derivative where testing shows it matters.
Check gradual introduction of the correction and different starting states,
always reaching the same declared final field. Retain every failed scenario.

Measure mesh error, incomplete convergence and field interpolation error
separately. They must be small compared with the reported response. Explicit
stress prescriptions can be highly sensitive to stress errors in some
formulations; that motivates testing ours, rather than proving it has the same
problem. See [Wu and colleagues](https://arxiv.org/abs/1803.05581).

### 7. Keep the claim narrow and testable

Initially claim usefulness only for the tested flow family. Report reference
cases missed, range width, computational cost and failed solves. Probability
claims require an additional justified statistical model and independent
validation. A random generator or a successful donor recovery does not supply
that justification.

## How to proceed in practice

There is still implementation work. The core field interface is implemented;
the full verification infrastructure and scientific validation are not complete.
The order below avoids adding complexity before knowing which part needs it.

### Step 1: implement exact replay and equation comparison

Add a clearly labelled same-grid diagnostic format for donor face/source values.
Bind it to the mesh, saved state, parent-model settings, units and reconstruction
settings. Reject missing, duplicate or mismatched records. It must not become a
replacement for the physical field format used across different meshes.

Add a way to assemble and export the equation imbalances at a saved state
without taking a flow iteration. Compare momentum, energy where applicable,
and SST k/omega blocks separately. Use the actual correction insertion paths.
Start with serial, fixed-mesh cases and fail clearly for unsupported paths.

Completion evidence: small tests for corrupt input and exact replay, followed by
a small same-state donor/receiver comparison. Use the handoff's normalized
per-block tolerance of 1e-10 with declared reference scales; investigate failures
before changing the tolerance. Passing here establishes insertion correctness,
not convergence from arbitrary starting states or physical accuracy.

### Step 2: complete small end-to-end checks

Use tiny local fixtures to compare baseline against zero correction, then test a
nonzero correction. Compare assembled scalar and vectorized results, rather than
only individual flux operators. Perform a real disk restart and compare it with
uninterrupted continuation. Then check small local parallel cases if the local
build supports them; otherwise retain an explicit untested status.

This needs test harness work and may expose solver fixes. Existing field readers
and sidecar checks do not need to be rewritten merely because these tests remain.
No cluster or production cases are needed. Keep any local runs strictly small.

### Step 3: close diagnostic gaps and make failures visible

Add owned-volume summaries of physical-stress violations, source limiting and
stress-work differences. Complete field-query failure, timing and memory reports.
Preserve input/output hashes, final amplitude and failed-run records in the
campaign tools. Distinguish exact replay error from physical-field fitting error.

Use failures from the preceding checks to decide whether to improve the implicit
derivative. A complete derivative can help convergence but cannot make an
unphysical stress field credible. Do not add adjoint support as part of this step.

### Step 4: extend the offline generator only where justified

Add localized fields and a small set of combinations. If reference discrepancies
show that global scaling or smoothing is inadequate, add spatially varying
strength and directional smoothing. Keep these choices fixed during each solve.
Test saved-field reproducibility, units, rotations, wall behavior and mesh
independence. Change the file schema only when the saved representation needs it.

### Step 5: prepare calibration and independent validation

Choose a narrow initial flow family and a few outputs before fitting parameters.
Prepare versioned reference-data records, separate calibration/validation case
lists, and scripts reporting misses, widths, numerical errors, costs and failures.
Include measurement and boundary-condition uncertainty where data allow it.

Preparing those tools is implementation work. Running a meaningful calibration
and validation study is research work and may exceed the present small-local-test
scope. Keep such runs prepared and unexecuted under the current restriction.
Do not choose new parameters after looking at the held-out results and still
describe those results as independent validation.

## Priority and stopping point

Do steps 1 and 2 first, then fill the diagnostic gaps exposed by them. Establish
that the equations are inserted and continued correctly before enlarging the
field family. After that, use independent data to justify additional flexibility.

Infrastructure is ready for controlled research when the replay, comparison,
restart and reporting tools exist, the small supported cases pass, and remaining
unsupported cases are stated plainly. Predictive credibility is a later result
that must be earned on independent data. Even a well-tested field family can
exclude the real model error.
