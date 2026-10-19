# Reuse FLOW fields during segregated SA advances

19 October 2026. Both final full 71-case suites, eight affected CLI checks and six
byte-identical application prefixes pass. The SA-only recovery/resolved-owner tests
also pass on both layouts. Full application endpoint qualification remains in progress.

One captured auxiliary SA advance, including its retries, retains unchanged FLOW
primary values. Its local scope reuses synchronized FLOW halos, gradients, limiters,
vorticity and strain. Properties, eddy viscosity, scalar gradients, boundary closure,
row masks and snapshot/history restoration remain live. Every return ends the scope;
the next base prepares fresh fields. MPI ranks agree on boundary eligibility.

Eligibility covers supported static compressible SA boundaries. SST/LM and standalone
operators retain their refresh paths. Derivative verification, true defect, Armijo,
stopping requirements, CFL/forcing and the common inner default remain unchanged.
No solver option or dependency is added.

## Measured work

Actual CNSSolver gradient call sites and MPI/GOMP calls are counted on the main
thread of each rank within StartSolver. Initialization is excluded. Counting boundaries
match before/after; every observed replay retains restart/history/diagnostic bytes.
These are supporting 20-update regression controls: history −13, nominal Newton
linear tolerance 0.001 and fixed CFL 1000. Full qualification retains history −14.
These prefixes are not qualified convergence endpoints.

| Case / layout | FLOW gradients before → after | MPI Allreduce before → after | MPI Isend / Irecv, each before → after | GOMP barriers before → after |
|---|---:|---:|---:|---:|
| SA 1×1 | 10288 → 7312 | 40903 → 39435 | 0 → 0 | 389781 → 380189 |
| SA 2×2, each rank | 17860 → 15316 | 69023 → 67771 | 53807 → 48719 | 984128 → 945412 |
| SST 2×2, each rank | 24150 → 24150 | 87998 → 87998 | 68520 → 68520 | 1253357 → 1253357 |

SA gradient calls decrease 28.9% in serial and 14.2% in parallel; parallel sends and
receives each decrease 9.46%. SST control counts remain unchanged. Aligned-allocation
counts remain unchanged. No memory reduction or timing speedup is established.
Uninstrumented scope categories are not measured zeros.

## Supporting application comparisons

Updates, assemblies, linear work, retries/backtracking and all numerical outputs
are identical before/after. These single-run observed times are contended resource
costs, ineligible for speedup claims. N/A means this legacy prefix had no native
state comparison; byte-identical implementation-pair agreement still passes.

| Case / layout / mode | Updates | F / T assemblies | Krylov | R / B | Loop s before / after | E2E s before / after | Peak rank KiB before / after | Native max point error |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| naca_sa / 1x1 / segregated | 20 | 1808 / 786 | 907 | 2 / 0 | 89.008 / 75.762 | 89.741 / 76.425 | 130664 / 131264 | 7.354e-05 |
| naca_sa / 2x2 / segregated | 20 | 3809 / 678 | 1606 | 1 / 0 | 116.878 / 97.052 | 117.516 / 97.596 | 97872 / 98084 | 7.317e-05 |
| naca_sst / 1x1 / segregated | 20 | 3952 / 336 | 1226 | 0 / 0 | 226.085 / 217.443 | 226.716 / 218.183 | 157392 / 158248 | N/A |
| naca_sst / 2x2 / segregated | 20 | 3946 / 339 | 1234 | 0 / 0 | 144.941 / 150.127 | 145.587 / 150.710 | 92052 / 92360 | N/A |
| naca_sst / 1x1 / coupled | 20 | 3379 / 3815 | 430 | 3 / 0 | 172.430 / 168.170 | 173.211 / 168.847 | 175872 / 176192 | N/A |
| naca_sst / 2x2 / coupled | 20 | 3518 / 4019 | 500 | 3 / 0 | 146.039 / 136.406 | 146.675 / 137.019 | 105164 / 105284 | N/A |

## Evidence and remaining checks

Evidence root: `newton_krylov_refactor_20261015/application_qualification_20261018/`.
`fixed_flow_reuse_20261019/{validation,focused_checks,measured_counts}.json`,
`{measured_counts,prefix_comparisons}.csv`, `recovery_retry1/validation.json` and
`gradient_profiles/state.json` retain
commands, checks, source/configuration/binary hashes and resource observations.
Before/after observer manifests and raw counters are preserved. All original source
bytes were restored; rebuilding recovered the identical clean CFD SHA256:
`8f28ca5005ff4e790cc5870750d5098d2c84c7eab1612746c1815d76373d066f`.

An extension to the existing failure test exercises SA-only linear/search failures,
CFL retries, exact dependent-state rollback and resolved-owner exit before a changed
FLOW state. Its forced unit-test conditions do not change application controls.
The extension and final full suites pass on both layouts. Its first fixture version
omitted the required carrier FLOW timestep and stopped at PSEUDO_TIME; the failed
build/test artifacts remain preserved. Preparing the carrier timestep makes the test
reach its intended failures. Production CFD bytes remain identical.

Reproduce the serial prefix from the evidence directory with a fresh output path:

```sh
nice -n 19 python3 application_case.py --case naca_sa --mode segregated \
  --phase qualification --work-only --graceful-limit --control original --cfl 1000 \
  --history-margin 1 --updates 20 --timeout 900 --ranks 1 --threads 1 \
  --release fixed_flow_reuse_release --output FRESH_SA_1X1 \
  --reference static_review_correctness_v2_applications/naca_sa/1x1/native
```

Exact commands for the other checks are in validation.json and gradient_profiles/state.json.
The [complete application ledger](NewtonKrylovApplications20261018.csv) retains every
attempt and qualification status. Residual assemblies and Krylov iterations do not
fall in this implementation comparison. FLOW probes and coupled groups still refresh
live fields. Full matched −14 endpoints, separately labelled CFL/forcing/preconditioning
studies, and selected quiet-machine repetitions remain necessary for performance conclusions.
