# Application stopping and ILU-fill checks

These checks change convergence or preconditioning controls, not the numerical implementation.
The retained CFD executable is unchanged. All timings below are contended and ineligible.
They describe work to a stopping condition or wall limit, not a qualified timing comparison.

## SST ILU fill1 pilot

Each pair differs only in `LINEAR_SOLVER_ILU_FILL_IN` (baseline default0). Initial states,
CFL/forcing policy, history stop-13, discretization and frozen physical/state/quantity targets match.
Native linear work sums both active systems; Newton work is outer Krylov iterations.
These counts operate on different block widths. Native assemblies are inferred from fine-grid
updates; Newton assembly counters are measured.

| Mode | ILU fill | Updates | Flow / turb assemblies | Linear work | Retries | Loop / elapsed s | Peak rank KiB | Full comparison |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| native | 0 | 447 | 447 / 447 | 15936 | — | 63.745 / 64.289 | 103580 | Pass |
| native | 1 | 137 | 137 / 137 | 4598 | — | 30.736 / 31.271 | 120960 | Fail |
| segregated | 0 | 32 | 12359 / 1880 | 4508 | 16 | 314.272 / 314.825 | 186756 | Fail |
| segregated | 1 | 34 | 12184 / 1324 | 4105 | 17 | 306.169 / 306.771 | 197956 | Fail |
| coupled | 0 | 61 | 12160 / 13641 | 1445 | 33 | 300.602 / 301.182 | 164712 | Fail |
| coupled | 1 | 59 | 11968 / 13524 | 1545 | 32 | 300.087 / 300.644 | 177516 | Fail |

Native fill1 stops after137 updates with lower work than fill0’s447, but its stable-reference
point error1.00168104e-5 exceeds1e-5. It remains unqualified. No threshold was relaxed.
Both Newton modes reach the300-second wall limit and miss the density target. Stronger fill
has not demonstrated a qualified Newton benefit; the different unfinished trajectories
do not establish a throughput ratio. Native fill1 also uses more memory.

## Longer SA run with unchanged controls

The diagonal coupled SA run used the same deck, executable and original initial state as
the300-second trial; only the available wall budget increased to900 seconds. It stopped
normally after78 updates (324.573s loop, 325.099s total),
with fresh density RMS9.75869e-14. Its conservative point error
1.3079037e-05 exceeds1e-5. This is a state-agreement stopping failure,
rather than a wall-time failure. A separately labelled history-14 trial retains the same
frozen physical/state/quantity requirements. It now qualifies after179 updates:33415 assemblies
per system,7603 Krylov iterations,89 linear retries and no backtracking. Both native-reference
comparisons pass (maximum conservative point error7.85129e-6). Its764.796-second solver loop
and765.385-second elapsed time were contended and remain ineligible. This is a stopping-control
result on unchanged numerical code. The main ledger retains the full hashes and comparison table.

## Native nozzle stability

The548-update original and735-update tighter-history endpoints differ by at most1.47016e-8
component-wise and2.07463e-7 pointwise. Comparing735 with6919 adaptive-CFL updates gives
7.67599e-10 and9.71838e-9. Quantities agree much more closely than1e-5. Nevertheless,
all three fail normalized transverse momentum: about7.647e-9 versus the1e-9 requirement.
This persistent behavior is not yet proof of a roundoff floor or boundary defect.
No nozzle reference is promoted. Same-state native/reporter replay checks residual-path
consistency with the retained reporter and the earlier physical report; its commands and
current outcome are preserved separately under `nozzle_residual_replay/`.

## Evidence

Archive: `newton_krylov_refactor_20261015/application_qualification_20261018/` beside the checkout.
Exact decks, commands, work, physical checks and hashes are in `sst_fill1_comparison.json`,
`fill1_pilot_commands.json`, `sa_long_diagonal_command.json`, `sa_long_diagonal_ready.json`,
`nozzle_native_stability.json` and the per-run directories. The full
[application CSV](NewtonKrylovApplications20261018.csv) retains every observation and failure,
including raw-versus-conservative qualification flags. The
[control-progress CSV](NewtonKrylovControlProgress20261018.csv) distinguishes intended
unstarted settings from measured runs. Broad qualification and three eligible repetitions
remain unfinished.

## Same-state nozzle replay and preconditioning prototype

The retained and earlier reporters give identical flow residuals and normalized residuals
at both735- and6919-update nozzle states. One native flow evaluation at the same saved
state gives similar raw RMS values: transverse momentum7.83205e-12 versus7.83486e-12
at735 updates, and2.15801e-12 versus2.16456e-12 at6919. This excludes a stale reporter
as the explanation for the normalized failure. It does not yet prove a numerical floor.
Commands, input hashes and outputs: `nozzle_replay_commands.json`, `nozzle_residual_replay/`.

An isolated debugger prototype changed only the existing block preconditioner's inner
iteration member from0to3. All residual/derivative/true-defect/acceptance controls remain
unchanged. The untouched baseline reproduces every original20-update SST attempt row.

| Inner iterations | Outer Krylov | Native inner Krylov | Flow assemblies | Turb assemblies | Retries | Final CFL |
|---|---:|---:|---:|---:|---:|---:|
| 0 | 430 | 0 | 3382 | 3818 | 3 | 15.625 |
| 3 | 251 | 864 | 2231 | 2487 | 3 | 15.625 |

Both prefixes have20 accepted updates and remain unconverged. The lower residual work
justifies testing this existing mechanism in a compiled candidate; it is not a demonstrated
application speedup. Debugger timings are ineligible. Peak owned-tree RSS includes GDB.
Exact member-layout validation, untouched-field checks, commands and hashes are preserved
in `inner_preconditioner_probe/{commands,interpretation}.json`.
