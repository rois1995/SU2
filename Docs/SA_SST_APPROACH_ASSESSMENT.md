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

The saved-field generator and reader, SA/SST coupling, matched SST EPM comparator,
restart guards and diagnostics are implemented. The generator supports localized
fields and combinations. Study tools freeze parameters and reference files and
report missing cases, range misses, width and cost. All new behavior is off by default.

Exact capture now records the correction at actual face and source queries.
Replaying it at the same saved state gives identical flow and turbulence residuals
in tiny 2D and 3D QCR/EPM tests. Actual disk restarts pass for SA and SST with the
incompressible energy equation enabled and disabled. The bounded final suite has
37 checks and 20 comparisons, on 81-node and 27-node meshes with at most four
iterations. These are wiring tests, not converged physical cases.

Actual scalar/vectorized assembly agrees for prescribed stress, nonuniform 1C EPM
and uniform 3C EPM. A nearly uniform 1C EPM comparison missed the fixed tolerance
and remains recorded as a limitation. The deliberately strong SST test field also
violates physical covariance constraints; the diagnostics flag that clearly.

The initial coupled scope remains fixed planar walls or no walls, with zero added
stress on physical boundaries. Circular geometry is tested only as a provider.
The implicit derivative is incomplete. Parallel execution could not be tested
because the local launcher is blocked by socket restrictions. No cluster was used.

Converged donor recovery, physical-field representation error, mesh studies and
independent validation remain separate work. Exact replay does not replace them.
The [runbook](STRESS_DISCREPANCY.md) and [verification report](CAMPAIGN_REPORT.md)
state the supported scope and actual evidence.

## My recommendation

Use this as the starting infrastructure for controlled experiments. Same-state donor residual recovery now passes the tiny checks. Next test
representation error and recovery of the converged donor solution. After that, test fields chosen without looking at the donor
answer, with both signs and more than one mesh. Only independent reference data
can tell us whether the chosen fields capture useful model error.

I would not call the resulting range a reliable error bar yet, or use successful
wiring tests to argue that this improves SA or outperforms EPM. The approach is
reasonable; its predictive value remains an open research question.
