# My assessment of the SA/SST stress-field approach

I think this is worth implementing as a research tool. It gives us a clear way
to ask how much a result changes when the modeled turbulent stress is wrong in
a specified place and direction. It is not yet a way to put a trustworthy error
bar on a prediction.

## What makes sense

Keeping the added field fixed during a solve makes the experiment repeatable.
The flow and turbulence variables still respond. Keeping the original SA
equation is the right choice: SA does not provide a turbulent kinetic energy
that could justify a replacement stress-production equation.

For SST, adding the signed work of the extra stress before limiting production
is consistent with the proposed equations. Both SST-2003 source equations must
use that same limited production. This agrees with the corrected relationship
in the [SST reference](https://tmbwg.github.io/turbmodels/sst.html).

Using the same ordinary SST model for the baseline, the EPM comparison and the
fixed-field comparison is essential. Otherwise a difference in the stress
convention can look like a difference between the two research methods.

Saving the actual field, rather than only a random seed, is also sound. It lets
us change the CFD mesh without changing the question being asked.

## What I would be careful about

The biggest open question is which fields to choose and how strong they should
be. Geometry, smoothing and a size limit do not tell us the actual model error.
A collection of well-behaved fields can miss an important direction entirely.
Its smallest and largest answers are therefore a range of tested scenarios,
not a guaranteed range and not a confidence interval.

A field frozen from a known QCR or EPM solution is a useful wiring test. If it
reproduces that solution, it shows that the stress was inserted correctly. It
does not show that independently chosen fields would have predicted the answer.
Freezing at the original baseline and freezing at the corrected solution answer
different questions and must remain separate experiments.

The wall rule deserves as much attention as the stress formula. We must
interpolate the raw field first, then apply the wall rule at each flux or source
location. Averaging stresses already damped at mesh nodes changes the near-wall
behavior. A circular example cannot establish support for an aircraft surface,
a thin gap, or a mesh with several element types.

Source limiting does not limit the applied momentum stress. That is deliberate,
but it means we cannot claim exact transfer of energy between the mean flow and
the modeled turbulence wherever a production limit is active. The stated energy
estimate also does not guarantee convergence of a steady compressible solve.

Very weak turbulence and repeated EPM eigenvalues need explicit treatment.
There may be no unique anisotropic EPM direction at an isotropic state. The
existing `feature_UQ` tests demonstrate this limitation. A deterministic choice
does not turn it into a smooth physical response.

## Decision and scope

Proceed with the disabled-by-default prescribed-field interface and a clearly
named matched SST comparator. Keep the field generator outside SU2, preserve
the parent transport equations, and reject combinations whose coupling has not
been implemented. Retain all failed and untested cases in the evidence.

The EPM fixes are reused from `feature_UQ`: commits `671a0f4` and `36ffbeeb`,
both based on upstream `f4b67d23`. The separate adjoint estimator is excluded,
as requested. No cluster access or production runs are authorized. Small local
algebra, file, source and flux checks can establish implementation details;
they cannot establish physical uncertainty coverage.

## What is ready now

The saved-field generator and reader, SA/SST flow coupling, matched SST EPM
comparison, restart identity checks and nodal diagnostics are implemented.
There are tools to prepare small cases, inspect saved results, compare tables
and fit a nodal field from a QCR or EPM donor. All new solver behavior is off by
default. Unsupported combinations stop with a clear error.

The local checks cover the signs in momentum and energy, the SST production
limits, the unchanged SA equation, very small positive turbulence energy, field
units and file integrity. The reader agrees with all 4,000 saved points in the
circular example, and with 200 further points checked using your MeshAdaptation
barycentric routine. Thirteen tiny configurations have been prepared and checked
by the parser. The complete serial solver builds and links. A further small
check confirms that its scalar and vectorized viscous operators agree for the
tested stresses, including matched EPM in 3D. No flow calculation has been run.

The initial coupled scope is a fixed mesh with planar walls or no walls. The
added field must vanish at every physical boundary. Circular geometry is checked
only as a field reader; curved-wall flow, periodic cases and several other model
combinations are rejected. The linear solve uses an incomplete derivative of the new terms. That can affect
convergence and must be tested before larger work.

The remaining work is real and separate: actual parallel
and disk-restart checks, whole-flow scalar/vectorized agreement, exact face-by-face
donor replay, and coupled target recovery. The nodal donor fit does not replace
those checks. The small local tests do not make the method physically qualified.
The [runbook](STRESS_DISCREPANCY.md) and [verification report](CAMPAIGN_REPORT.md)
state the implemented scope and untested items explicitly.

## My recommendation

Use this as the starting infrastructure for controlled experiments. First prove
that a saved donor correction reproduces the same discrete residuals and then
the donor solution. After that, test fields chosen without looking at the donor
answer, with both signs and more than one mesh. Only independent reference data
can tell us whether the chosen fields capture useful model error.

I would not call the resulting range a reliable error bar yet, or use successful
wiring tests to argue that this improves SA or outperforms EPM. The approach is
reasonable; its predictive value remains an open research question.
