# Bounded global evidence before BL-profile freezing

Qualified connected source: WIP checkpoint d66fe6f1e6 plus collective allocation
handling and measured-quota fixture in the qualification commit. Full Goal3 is
ACTIVE/incomplete. All owned jobs are terminal. Reproduce report only with
`python3 integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/adaptive_budget_evidence_v1/report.py`.
It verifies current source/build/binary/config/input/helper pins, executed capsules,
raw log hashes, independent audits, bounds and MPI comparison; it runs no SU2.
See [machine-readable summary](summary.json) and [design](DESIGN.md).

## What changed

For opt-in BL-budget classification whose uncertainty is dominated by the sampled
BL correction, retain original donor geometry/indexes and the complete proposal.
Allow at most two stream doublings, 8192 -> 16384 -> 32768, under the existing
80M preparation/PDF-work and 2GiB/rank admission ceilings. No cap was raised.
Every extension restarts the complete ordered profile scan at zero. The first
estimated admissible sampled parameter is selected without assuming monotonicity;
this is not minimization over all real-valued parameters.

Keep uniquely owned original-P1 sensor stencils and compose geometry at actual
query points. Outside-domain zeros remain in the full sample denominator. The
indicator takes the larger pooled or between-batch empirical 3SE, plus background
interval/numerical indicators. These are HEURISTICS, not proven bounds or calibrated
sequential confidence statements. No support/thickness/fade/tangential change.
Resource refusal during an extra stage retains the prior complete estimate;
geometry/protocol failures remain fatal. Profiles freeze before local operations.
Strict/default configurations retain original sampling behavior and release
preparation. Controlled planar strict integration remains the preferred path.

## M6: selection now resolved; normalization still provisional

Same closed saved M6 restart, zero CFD advances. First height unchanged, protected
distance 2e-6 and explicitly permitted outer growth maximum 2. These are declared
experimental physical limits, not automatically justified simulation accuracy.

| Samples | lambda .75 estimate | Heuristic indicator | Budget300000 classification |
|---|---:|---:|---|
|8192|308423.477503|12078.099539|uncertain|
|16384|308138.063615|8476.734390|uncertain|
|32768|308122.760018|5971.558562|estimated incompatible|

At32768, lambda .875 gives281779.016752 +/-5603.507878: estimated attainable.
The complete nonmonotone ordered rescans take22 evaluations/version22, two
successful evidence stages. The finer original flow-sensor family is then
normalized and frozen at295068.448185 +/-5769.124264: discrepancy -1.64385%,
indicator1.92304% of300000. This final normalization remains integration_limited;
it is not an exact root or an accepted mesh. Sensor background now contributes
about10701, so it cannot be replaced with the intentionally coarsest background2.

MPI2/4 select the identical lambda/version/stages. Maximum trial-relative drift
8.998e-14; final estimate/indicator drift1.072e-11 after full sensor gradation.
Strict comparisons retain the finer sensors. Tiny near-zero correction counts can
differ with arithmetic; geometry/support sample count21331 and query work agree.
Sensor gradation reaches its fixed point (245 final sweeps, zero directed residual).
The composed nodal audit still has large residuals (maximum ratio132746 and
172185 directed violations on MPI4); sensor gradation is no composed certificate.

Two independent saved8192-point holdouts support the observed profile distinction:
lambda .75 means314146 +/-12153 and312539 +/-11852, both estimated above300k;
lambda .875 means287470 +/-11440 and285662 +/-11101, both estimated below300k.
They are noisy estimates with their own uncertainty, not exact references. Do not
claim the new narrower indicator covers every holdout mean or proves no missed mass.

Budget100000 still rejects collectively after the original nine8192-point profiles,
with no extension: minimum sampled endpoint260488.18 +/-10780.29. Mandatory lower
estimate17173.58 +/-6184.98 does not establish that mandatory requirements alone
exceed100000. This is an estimated conflict across permitted sampled profiles,
not proof of continuum infeasibility. Original restart/reference stay unchanged.

## Accuracy and complete workflows

Serial primitives28 cases/10625 assertions PASS; MPI4 uncertainty/prefix/rollback/
nonmonotone-rescan4 cases/rank PASS. Scalar independent integral21.2065463356722,
Gaussian64/128 difference <1e-10. Observed errors for128/256/512 points are
1.43892/.318112/.418184; indicators3.87652/5.27110/2.64409 cover each. Neither error
nor indicator is monotonically decreasing. Serial/MPI4 trial values are identical.
This limited reference does not calibrate M6 tails or adaptive selection coverage.

Actual opt-in and strict/default N4/M2 RANS/SA current+BDF workflows both PASS with
independent geometry/height/q/J/full-composed edges/original-P1 transport,
conservation/positivity, atomic rejection, SU2+CGNS reload and actual restart.
Each contains two independently complete cases. These controlled accepted grids
are in [enabled workflow](qualification_v2/enabled_mpi4) and
[strict workflow](qualification_v2/disabled_runtime); no M6 mesh was constructed.

## Work, memory and communication

| Measurement | MPI4 | MPI2 |
|---|---:|---:|
|Launcher seconds|152.556|184.984|
|Complete fixture metric seconds|137.055|169.793|
|Extra stencil stages seconds|5.069 +10.305|4.938 +10.459|
|Preparation accumulated maximum seconds|28.728|31.229|
|Background maximum seconds|1.218|2.020|
|Correction maximum seconds|.377|.470|
|Assessment reductions maximum seconds|.151|.014|
|Nested sensor gradation maximum seconds|102.455|132.241|
|Retained integration bytes maximum/rank|228838384|336316000|
|Retained preparation bytes maximum/rank|213421792|303562488|
|Maximum single-process peak RSS bytes|612536320|1023090688|

Times are nested/contended, not additive clean scaling measurements. Process RSS
maxima are not aggregate job peaks; the late process monitor recorded no live
ranks and cannot establish an aggregate peak. Host available RAM remained about
38--40GiB; one owned job at a time, nice19, threads1, local ranks<=4.

Both ranks use798785 fixed proposal-preparation work and59326787 PDF candidates,
389065 fresh correction evaluations,277084 profile geometry queries,21331 initial/
appended geometry queries and4096 donor preparation batches. Exactly one donor
index build/rank survives both extensions; no donor search during density
assessments. Background40043388 determinant evaluations, six builds/rank with
21 reuses/rank; new original-family sensor trials require recomputation.
27 complexity trials/937 gradation sweeps/27347867 owned visits dominate metric
normalization; gradation reductions are nested within the102/132s, not additive.
No extra whole-mesh gather was introduced by extension; existing support setup
still has its recorded root proposal. Reused prep is larger than the earlier
single-cloud configuration; this is a deliberate bounded accuracy/work tradeoff.

The earlier7.515s metric receipt used unresolved lambda .75 at a much coarser
sensor endpoint. It is a DIFFERENT final field and cannot establish a speedup or
regression for this137s result. No remesh/transfer/return lifecycle timing here.

## Preserved failures and limits

The first MPI4 tight fixture failed during initial admission because its arbitrary
2MiB quota did not admit rank-dependent initial preparation. qualification_v1 and
failed_tight_fixture snapshots are preserved. The corrected fixture measures the
initial global retained maximum before creating an explicitly insufficient extra
stage; refusal is collective and old estimates remain exact. Two interrupted
fixture diagnostic builds and raw logs are retained. No native ceiling change.
The MPI2 test-only watchdog is explicitly360s versus180s MPI4 because the observed
complete normalization needs~185s launcher; every native bound remains unchanged.

No new constructor fallback, accepted M6, physical accuracy, globally calibrated
uncertainty or complete-event affordability claim. Global work control currently
chooses background versus correction; it is not a distributed per-cell absolute-
error allocator. Zero-hit/rare-tail cases can remain integration_limited. No2D
opt-in selection replacement; its strict integration behavior remains qualified.
