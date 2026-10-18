# Newton–Krylov application qualification — 18 October 2026

Next milestone: matched NACA0012 SA/SST endpoints on1x1 and2x2 with lower residual and linear work.
Quiet-machine repetitions are deferred; time and RAM caps remain resource guards. A time cap is incomplete.
Broader T3A/nozzle/VKI sweeps are administratively stopped after active children finished; evidence is retained.
Default inner iterations remain0. Resolve rejection mechanisms before further control changes.
See [NACA rejection diagnosis](NewtonKrylovNacaRejections20261018.md) for the current work-based assessment.

The [bounded Krylov restart](NewtonKrylovRestartAssessment20261018.md) is not adopted: density-milestone work regresses
and its full trial was interrupted by the global-swap guard. The saved checkpoint passes physical/state checks;
the interrupted run remains unqualified. Production sources are restored. Existing fixed forcing0.05 is being
tested separately as a configuration change; no new recovery, forcing signal or inner default is adopted.

The [MUSCL boundary-gradient reuse](NewtonKrylovGradientReuse20261018.md) removes repeated native gradient work
for eligible first-order auxiliary probes. Six original-control pairs retain identical states/work; measured
SST communication counts decrease. Higher-order SA keeps its fallback. No timing speedup is inferred.

See the [existing fixed-forcing study](NewtonKrylovFixedForcing20261018.md) for nominal0.05 qualification
and separately labelled native/Newton accuracy comparisons. Implementation and configuration gains remain distinct.

Completed Newton assembly totals include the final full reporting refresh after the last logged attempt.
Earlier raw records call this offset an initial evaluation. This attribution is corrected without changing
counts: measured method entries confirm the offset in 20 archived application profiles. Interrupted-run
totals can omit unlogged work and retain an explicit estimate label. Native inferred counts stay labelled.

Work in progress. Baseline numerical implementation: `4712d82375e00ede15e5177ab38865eaaaa3e9f5`;
source checkout at study start: `8e61376ecd234414ce583c9b8f995e2bd62f4c12`.
Committed corrections: momentum probes `637177c3e4` and bounded verified-probe recovery `d27f4b37f6`.
See [momentum probes](NewtonKrylovProbeMetric20261018.md) and [recovery evidence](NewtonKrylovProbeRecovery20261018.md).

The [physical-progress forcing experiment](NewtonKrylovPhysicalForcing20261018.md) is
**not adopted**. Its six-update coupled prefix reduced Krylov work160->111 and assemblies
838/1004->689/805, but the full coupled trial retried107 times versus36 and had a larger
state error at the wall limit. Neither full mode qualified. All candidate regressions and
original-control state checks passed; that did not establish a practical improvement.
Production source is restored to d27. Candidate source, binaries and failures are preserved.
This was a policy experiment, not a proven correctness fix. No timing speedup is established.

The [larger verified-probe experiment](NewtonKrylovProbeRange20261018.md) is also **not adopted**.
It recovered an unavailable SA product without weakening verification, but full SA/SST
runs remained unqualified. Fewer retries did not establish a practical benefit.

The [application stopping and ILU-fill checks](NewtonKrylovApplicationChecks20261018.md)
retain the normal-stop SA state-agreement failure, the matched SST fill1 pilot and
native nozzle stability evidence. Native fill1 narrowly misses its pointwise state bound;
both Newton fill1 trials time out. No qualified Newton improvement is established.

The [native inner-preconditioning assessment](NewtonKrylovInnerPreconditioning20261018.md)
records the independently reproduced scaling bug fixed in `e47c7b14cd` and the separate
three-iteration strategy. That universal default is not adopted: parallel SA regresses
and SST/LM/periodic trials remain unqualified. The checkout retains zero inner iterations;
the broad archived-candidate assessment is deferred, with no general speedup claim.

This study uses existing application meshes, independently of the earlier generated fixtures.
All inputs, initial user edits, binaries, build options and source hashes are frozen under
`/media/rausa/4TB/SU2_Versions/SU2_v850_Develop_FixCGNSOutput/newton_krylov_refactor_20261015/application_qualification_20261018`. The two pre-existing dirty reports are preserved.

| Application | Mesh points | Initial condition |
|---|---:|---|
| NACA0012 SA | 14576 | Original supplied SA restart |
| NACA0012 SST | 14576 | Original supplied SST restart |
| Axisymmetric SST nozzle | 4980 | Original supplied restart |
| Physical T3A SST–LM flat plate | 68129 | Tutorial free stream |
| Periodic transonic SST stator (exploratory) | 17447 | Original supplied restart |
| Periodic VKI SA turbine | 15860 | Explicit cold start; supplied restart has incompatible point ordering/count |

Discovery preserves the original spatial discretization, physics and CFL controls.
It caps work explicitly and retains failed runs. Output is restricted to restart plus history,
with performance reporting enabled identically in all modes. Later long trials also save restart
checkpoints every100 updates; timeouts label the saved state as a checkpoint, never a converged endpoint.
T3A wall force monitoring and nozzle
inlet/outlet monitoring add measurements. Common Newton uses fixed CFL and fixed forcing
in the original-control discovery; recovery can still reduce CFL. Its coupled preconditioner
uses the lower block option. These choices will be tested separately.

The first three T3A setup attempts used an incorrect runner edit (`TIME_MARCHING=STEADY`).
The parser requires the original `NO` spelling; the edit is corrected and the failures are
retained as runner errors. The VKI deck uses the current `ITER` spelling for its old `EXT_ITER`.

Original case stopping may monitor density alone. Discovery completion is therefore **not**
qualification of every active equation. Fresh residuals, complete restart agreement and physical
quantities are required before any result can be described as qualified.

Frozen endpoint requirements retain each application's original density RMS target
(1e-12; T3A 1e-11), require normalized physical RMS <=1e-9 for **every active component**,
and bound scaled boundary projection by1e-12. Full-state component relative Linf <=1e-6,
pointwise scaled Linf <=1e-5 (scale floor 1e-3 of component maximum), and quantities
agree within relative1e-5 / absolute1e-8. Transition must be active. These checks are
independent of the solver's unchanged acceptance and derivative verification.

Definitive NACA comparisons also require stable native references: SA151 updates (history-13)
and SST545 updates (history-14). SST397 agrees with545 within3.80e-7 component error;
545 agrees with the1403-update stricter-history wall-limited endpoint within3.26e-8.
SST258 misses the1e-6 component bound against545 (3.73e-6), despite passing residual checks.
Remaining tuned SST decks use history-13 consistently in every mode; original-control
records remain unchanged. See `sst_reference_stability_final.json` and the pinned manifest.
The conservative audit preserves the original paired-check result, uses the same state/quantity
thresholds, and never changes numerical acceptance. The CSV retains `qualified` from the original
paired check and adds `comparison_qualified` plus complete conservative errors/reference hashes.
Coverage and eligible comparison counts below use `comparison_qualified`.

Initial endpoint checks: warm NACA SA native and segregated pass against the initial
11-update native reference. A conservative native151-update reference (density history stop
tightened from-12 to-13) exposes pointwise SA differences8.15e-5 for the original native
and8.35e-5 for segregated, above1e-5. Early agreement is therefore insufficient evidence
of a stable final state. Original records are retained; margin trials use the stricter reference.
Coupled passes physical residual checks but misses initial pointwise state agreement (SA field
relative scaled error1.66e-5 versus1e-5). It remains rejected. No target was relaxed.
SST and nozzle discovery endpoints miss the density target. The first SST coupled timeout
has no saved final restart; its endpoint is unverified. All capped and failed times describe
work to termination, not time to convergence.

The Giles stator is exploratory: `CTurboIteration` updates its boundary averages outside
the Newton residual wrapper. The tested capability guard now rejects this unsupported setup.
The established VKI turbine uses standard inlet/outlet boundaries and is the primary
substantial periodic candidate. Generated periodic fixtures remain supporting regressions.

The committed momentum finite-difference correction and bounded verified-probe recovery are
under application validation. The focused independent
derivative detector fails on the baseline and passes on the candidate; state/residual scales
and derivative/acceptance tolerances are unchanged. Its immutable source patch and binaries are
under `probe_metric/` and `recovery_release/`. Neither establishes an application speedup.
Quiet repetitions, full
physical qualification, cold-start comparisons, both layouts, and the CFL/forcing/preconditioner
study remain outstanding. Current discovery times are observations only.
The full native nozzle stops after548 updates but its fresh normalized transverse momentum
RMS7.65e-9 misses1e-9. Before bounded probe recovery, fixed-control Newton retains CFL0.0153.
The recovery prefix retains62.5 and advances density farther while spending more work;
see its separate before/after work table. Neither20-update endpoint qualifies.
SST native258-update endpoints pass residual checks but fail the stable-reference audit;
fixed-control Newton times out.
The pinned637 original-control cohort is complete (30 trials). Neither cold VKI nor T3A
qualified in any mode; all their Newton trials timed out. Original T3A native also timed out
(695 updates on1x1,1299 on2x2). Its coupled recovery retained CFL0.390625 after4 rejects.
The final-code d27 original-control cohort is also complete (30 trials). No Newton endpoint
qualifies against the full comparison requirements under these controls. Native SST passes
residual checks on both layouts but fails stable-state agreement. Early SA states, nozzle
all-component misses, cold periodic caps and LM
timeouts remain rejected. The separately labelled control study is now active.
All18 native151-update SA timing attempts qualified physically but crossed the quiet-machine
limit during execution. None is eligible. Final-code fixed-control Newton margin attempts
hit120-second limits on both layouts; these are failures, not time-to-convergence samples.
Forcing studies explicitly start the existing adaptive strategy at0.1 (true-defect limit stays0.1);
adaptive-CFL-only studies retain the original0.001 initial linear target. These are configuration changes.
The unstarted control plan was revised from108 to126 trials after the retained-CFL/work results:
matched fixed-CFL100 trials were added for warm SA/SST/nozzle on both layouts. Nozzle control
trials use density history stop-13 after the original stop missed the all-component target.
A further10 coupled trials reverse the existing lower-block order, with identical forcing
and adaptive-CFL settings. This includes turbulence feedback in the flow correction; benefit
is unproven. Three SST1x1 pilot trials add existing ILU fill1, with matched native settings
and the same adaptive forcing/CFL controls. Further fill-level expansion requires evidence.
The periodic native history reports relative linear residuals near1 with its original2-step
limit. Native adaptive CFL resets above0.99; a separate three-mode VKI1x1 pilot therefore
uses existing ILU and native assembled limit20, retaining its original linear residual target.
The142-trial plan records intended settings separately from actual results below.
That broad schedule is now deferred. The narrowed NACA reference/rejection assessment precedes
any further configuration expansion. Tuned SST history remains-13 in all modes;
all earlier139 trials remain. Qualified native reference candidates precede Newton comparisons.
All old plans/audits remain preserved. Preparation checks match108 Newton/native pairs;
these are setup checks, not measured convergence results. Both versions' early SA states
miss the conservative151-update state reference; tightened-stop controls remain necessary.

The first completed SA1x1 control group qualifies native adaptive CFL in149 updates, versus
151 at fixed CFL1000. Fixed CFL10000 qualifies native in34 updates (1360 linear iterations),
including the conservative reference audit. This is a configuration gain in work; its
single contended timing is ineligible. Forced-adaptive segregated/coupled stop at the300-second limit after
126/72 updates. Reverse-order coupled and diagonal coupled also reach that limit after59/74
updates. All four meet the fresh physical residual targets, but fail full-state agreement.
Their counts and failure times remain in the ledger; none is a convergence-time measurement.
The lower-block coupled run retries36 times, including33 failed attempts with zero outer
Krylov iterations. Its recurring CFL2000 failures recover at500, then accepted steps raise CFL
through1000 to2000. Reverse
ordering and diagonal preconditioning likewise retry29/36 times; no backtracking occurs.
These counters establish recovery work, without establishing the cause of each failed product.
A tighter native SA check (history stop-14) converges in298 updates and differs from the
pinned151-update reference by at most7.57e-6 pointwise, within the unchanged1e-5 limit.
All four Newton endpoints also fail the same state thresholds against this tighter state
(maximum point errors2.29e-5 to4.85e-5). The reference and raw qualifications are unchanged.
The native history stop-15 check reaches its300-second wall limit after2669 updates, with
fresh density RMS5.57e-15. Its saved state differs from the298-update state by5.87e-7
pointwise; it supports reference stability but remains a failed stopping test.
See `sa_first_control_assessment.json`, `reference_stability_commands.json` and
`sa_control_against_tighter_reference.json`. No eligible timing comparison is available.
All four largest SA turbulence point errors occur at(x1.34535,y0.00601870), downstream
of the unit-chord airfoil. `sa_control_state_error_locations.json` records complete field
names, coordinates, values and restart hashes. This locates the error without identifying
its cause. Local primary-paper notes and PDF hashes are in `paper_notes/ASSESSMENT.md`
and `paper_notes/manifest.json`; they motivate investigations, not unmeasured solver changes.

## Coverage

| Application | Layout | Actual trials | Qualified endpoints | Eligible timing repetitions |
|---|---|---:|---:|---:|
| naca_sa | 1x1 | 79 | 29 | 1 |
| naca_sa | 2x2 | 59 | 23 | 0 |
| naca_sst | 1x1 | 39 | 5 | 0 |
| naca_sst | 2x2 | 20 | 2 | 0 |
| nozzle_sst | 1x1 | 32 | 0 | 0 |
| nozzle_sst | 2x2 | 9 | 0 | 0 |
| flatplate_lm | 1x1 | 20 | 0 | 0 |
| flatplate_lm | 2x2 | 9 | 0 | 0 |
| periodic_stator | 1x1 | 3 | 0 | 0 |
| periodic_stator | 2x2 | 0 | 0 | 0 |
| periodic_vki | 1x1 | 17 | 0 | 0 |
| periodic_vki | 2x2 | 9 | 0 | 0 |

## Declared control matrix

[Control progress CSV](NewtonKrylovControlProgress20261018.csv) includes every intended setting,
including unstarted trials. `scheduling=deferred_naca_priority` marks the suspended broad plan.
`run_ended` records solver termination, not successful qualification.
Only actual runs have configuration/binary hashes. Supporting experiments and quiet repetitions
remain in the complete application ledger above.

| Application | Layout | Planned | Run ended | In progress | Pending | Comparison pass |
|---|---|---:|---:|---:|---:|---:|
| naca_sa | 1x1 | 16 | 16 | 0 | 0 | 3 |
| naca_sa | 2x2 | 16 | 10 | 0 | 6 | 2 |
| naca_sst | 1x1 | 16 | 6 | 0 | 10 | 1 |
| naca_sst | 2x2 | 13 | 2 | 0 | 11 | 0 |
| nozzle_sst | 1x1 | 14 | 4 | 0 | 10 | 0 |
| nozzle_sst | 2x2 | 14 | 0 | 0 | 14 | 0 |
| flatplate_lm | 1x1 | 15 | 3 | 0 | 12 | 0 |
| flatplate_lm | 2x2 | 15 | 0 | 0 | 15 | 0 |
| periodic_vki | 1x1 | 13 | 3 | 0 | 10 | 0 |
| periodic_vki | 2x2 | 10 | 0 | 0 | 10 | 0 |

## Selected quiet native baseline

Native SA CFL10000/history-13 was repeated outside the watched workspace.
All nine states/configurations/binaries match the earlier high-CFL results exactly.
Quiet eligibility still requires three preflight samples and every measured sample
within two external CPU cores, with no foreign heavy job or memory pressure.
Contended attempts remain excluded. Fewer than three eligible repetitions on either
layout means no median or solver speedup claim. The output-location change establishes
no I/O improvement. Exact commands, file hashes and all failed eligibility evidence:
`quiet_high_sa_scratch/`, `quiet_high_scratch_summary.json` and the archive/cleanup manifests.

| Layout | Attempts | Eligible | Median loop s | Median end-to-end s |
|---|---:|---:|---:|---:|
| 1x1 | 6 | 1 | — | — |
| 2x2 | 3 | 0 | — | — |

Eligible individual observation: 1x1, 34 updates, 4.282s solver loop / 4.808s end to end. This is one observation, not a repeated comparison.

## Qualified coupled SA endpoint

The history-14 diagonal coupled run qualifies against both the tighter native298 state
and pinned151 state, with unchanged physical/state/quantity requirements. It needs179
updates,7603 Krylov iterations and33415 measured assemblies per system; maximum accepted
true defect0.0889115 remains below0.1. All179 steps use alpha1;89 linear failures have
zero completed outer Krylov iterations. No backtracking occurs.
This is a convergence-control result on retained code, not an implementation speedup.
All four rows below are numerically qualified, but their timings are contended.
Native counts are inferred per-update assemblies; Newton counters are measured.
Native linear sums and coupled Krylov counts have different block widths.

| Mode / controls | Updates | Flow / turb assemblies | Linear work | Retries | Loop / elapsed s | Peak rank KiB |
|---|---:|---:|---:|---:|---:|---:|
| native fixed1000, history-13 | 151 | 151 / 151 | 6040 | — | 19.177 / 19.784 | 95304 |
| native fixed1000, history-14 | 298 | 298 / 298 | 10620 | — | 36.747 / 37.305 | 95264 |
| native fixed10000, history-13 | 34 | 34 / 34 | 1360 | — | 4.236 / 4.756 | 95540 |
| coupled adaptive1000, forcing, diagonal, history-14 | 179 | 33415 / 33415 | 7603 | 89 | 764.796 / 765.385 | 171400 |

The verified MUSCL products imply at least22809 assemblies per system
(three probes per completed product): a source-derived lower bound equal to68.3% of
the measured assembly count, not a measured phase count or wall-time fraction.
Additional refinement, failed products, defects, bases and trial evaluations add work.
Intervals ending in the89 failures contain3471 assemblies per system, measured between
attempt-log rows. They include intervening reporting, bases and rollback, so they are
not an exclusive failure-phase count or a claim that all this work is avoidable.
The corresponding count proof and numerical hashes are in
`sa_first_qualified_coupled_summary.json`. Quiet repeated comparisons remain incomplete.

## Current run ledger

[Machine-readable ledger](NewtonKrylovApplications20261018.csv). Every actual setting and failure
is retained; `prepared` and `running` entries have not completed.

| Case | Layout | Mode | Initial | Implementation | Control | History stop | Status | Comparison pass | Updates | Loop s | End-to-end s |
|---|---|---|---|---|---|---:|---|---|---:|---:|---:|
| naca_sa | 2x2 | coupled | original | bounded_restart_release | diagonal | -14.0 | incomplete_memory_guard | False | 207 | — | 1248.127 |
| naca_sa | 2x2 | coupled | original | bounded_restart_release | diagonal | -14.0 | iteration_cap | False | 20 | 55.847 | 56.349 |
| nozzle_sst | 1x1 | segregated | original | application_release | fd_step_1e-4 | -12 | diagnostic_complete | False | 1 | — | 1.989 |
| nozzle_sst | 1x1 | segregated | original | application_release | fd_step_1e-6 | -12 | diagnostic_complete | False | 1 | — | 2.426 |
| nozzle_sst | 1x1 | segregated | original | application_release | fd_step_1e-8 | -12 | diagnostic_complete | False | 1 | — | 4.579 |
| nozzle_sst | 1x1 | coupled | original | application_release | forcing | -12 | iteration_cap | False | 1 | 3.482 | 3.961 |
| nozzle_sst | 1x1 | segregated | original | application_release | forcing | -12 | iteration_cap | False | 1 | 3.301 | 3.765 |
| nozzle_sst | 1x1 | segregated | original | coarser_candidate | original | -12 | iteration_cap | False | 20 | 16.041 | 16.499 |
| nozzle_sst | 1x1 | segregated | original | recovery_release | original | -12 | iteration_cap | False | 20 | 16.326 | 16.784 |
| naca_sa | 1x1 | coupled | original | coarser_candidate | forcing | -13.0 | incomplete_time_cap | False | 34 | 300.353 | 300.929 |
| naca_sa | 1x1 | coupled | original | coarser_candidate | forcing | -13.0 | iteration_cap | False | 6 | 43.283 | 43.830 |
| naca_sa | 1x1 | native | original | coarser_candidate | original | -13.0 | qualified | True | 151 | 18.399 | 18.932 |
| naca_sa | 2x2 | native | original | coarser_candidate | original | -13.0 | qualified | True | 151 | 12.471 | 12.934 |
| naca_sst | 1x1 | coupled | original | coarser_candidate | original | -12 | iteration_cap | False | 20 | 85.907 | 86.466 |
| naca_sst | 1x1 | coupled | original | coarser_candidate | original | -12 | incomplete_time_cap | False | 227 | 300.218 | 300.727 |
| naca_sst | 1x1 | coupled | original | recovery_release | original | -12 | iteration_cap | False | 20 | 82.086 | 82.650 |
| naca_sst | 1x1 | native | original | recovery_release | original | -13.0 | qualified | True | 397 | 54.668 | 55.205 |
| flatplate_lm | 1x1 | native | original | recovery_release | adaptive | -11 | incomplete_time_cap | False | 1489 | 600.245 | 601.255 |
| flatplate_lm | 1x1 | coupled | original | recovery_release | forcing | -11 | incomplete_time_cap | False | 121 | 604.730 | 605.793 |
| flatplate_lm | 1x1 | segregated | original | recovery_release | forcing | -11 | incomplete_time_cap | False | 22 | 600.860 | 601.936 |
| naca_sa | 1x1 | coupled | original | recovery_release | adaptive | -13.0 | incomplete_time_cap | False | 73 | 300.287 | 300.874 |
| naca_sa | 1x1 | native | original | recovery_release | adaptive | -13.0 | qualified | True | 149 | 19.711 | 20.324 |
| naca_sa | 1x1 | segregated | original | recovery_release | adaptive | -13.0 | incomplete_time_cap | False | 125 | 301.036 | 301.548 |
| naca_sa | 1x1 | coupled | original | recovery_release | diagonal | -13.0 | incomplete_time_cap | False | 74 | 303.258 | 303.829 |
| naca_sa | 1x1 | coupled | original | recovery_release | fixed | -13.0 | incomplete_time_cap | False | 285 | 300.035 | 300.577 |
| naca_sa | 1x1 | native | original | recovery_release | fixed | -13.0 | qualified | True | 34 | 4.236 | 4.756 |
| naca_sa | 1x1 | segregated | original | recovery_release | fixed | -13.0 | incomplete_time_cap | False | 195 | 301.132 | 301.668 |
| naca_sa | 1x1 | coupled | original | recovery_release | fixed | -13.0 | incomplete_time_cap | False | 336 | 301.044 | 301.584 |
| naca_sa | 1x1 | native | original | recovery_release | fixed | -13.0 | reference_audit_failed | False | 991 | 121.317 | 121.838 |
| naca_sa | 1x1 | segregated | original | recovery_release | fixed | -13.0 | incomplete_time_cap | False | 256 | 301.433 | 301.964 |
| naca_sa | 1x1 | coupled | original | recovery_release | forcing | -13.0 | incomplete_time_cap | False | 72 | 303.705 | 304.270 |
| naca_sa | 1x1 | segregated | original | recovery_release | forcing | -13.0 | incomplete_time_cap | False | 126 | 300.092 | 300.614 |
| naca_sa | 1x1 | coupled | original | recovery_release | original | -13.0 | incomplete_time_cap | False | 72 | 302.609 | 303.169 |
| naca_sa | 1x1 | native | original | recovery_release | original | -13.0 | qualified | True | 151 | 19.177 | 19.784 |
| naca_sa | 1x1 | segregated | original | recovery_release | original | -13.0 | incomplete_time_cap | False | 165 | 301.390 | 302.146 |
| naca_sa | 1x1 | coupled | original | recovery_release | forcing | -13.0 | incomplete_time_cap | False | 59 | 301.273 | 301.783 |
| naca_sa | 2x2 | coupled | original | recovery_release | adaptive | -13.0 | incomplete_time_cap | False | 83 | 302.655 | 303.205 |
| naca_sa | 2x2 | native | original | recovery_release | adaptive | -13.0 | reference_audit_failed | False | 4928 | 211.659 | 212.152 |
| naca_sa | 2x2 | segregated | original | recovery_release | adaptive | -13.0 | incomplete_time_cap | False | 96 | 304.000 | 304.498 |
| naca_sa | 2x2 | coupled | original | recovery_release | diagonal | -13.0 | incomplete_time_cap | False | 87 | 300.747 | 301.282 |
| naca_sa | 2x2 | native | original | recovery_release | fixed | -13.0 | qualified | True | 43 | 4.024 | 4.587 |
| naca_sa | 2x2 | segregated | original | recovery_release | fixed | -13.0 | incomplete_time_cap | False | 157 | 301.345 | 301.827 |
| naca_sa | 2x2 | coupled | original | recovery_release | forcing | -13.0 | incomplete_time_cap | False | 76 | 302.717 | 303.241 |
| naca_sa | 2x2 | segregated | original | recovery_release | forcing | -13.0 | incomplete_time_cap | False | 142 | 301.181 | 301.672 |
| naca_sa | 2x2 | native | original | recovery_release | original | -13.0 | qualified | True | 151 | 13.044 | 13.519 |
| naca_sa | 2x2 | coupled | original | recovery_release | forcing | -13.0 | incomplete_time_cap | False | 71 | 301.489 | 302.024 |
| naca_sst | 1x1 | native | original | recovery_release | adaptive | -13.0 | qualified | True | 447 | 63.745 | 64.289 |
| naca_sst | 1x1 | native | original | recovery_release | adaptive | -13.0 | reference_audit_failed | False | 137 | 30.736 | 31.271 |
| naca_sst | 1x1 | coupled | original | recovery_release | forcing | -13.0 | incomplete_time_cap | False | 59 | 300.087 | 300.644 |
| naca_sst | 1x1 | segregated | original | recovery_release | forcing | -13.0 | incomplete_time_cap | False | 34 | 306.169 | 306.771 |
| naca_sst | 1x1 | coupled | original | recovery_release | forcing | -13.0 | incomplete_time_cap | False | 61 | 300.602 | 301.182 |
| naca_sst | 1x1 | segregated | original | recovery_release | forcing | -13.0 | incomplete_time_cap | False | 32 | 314.272 | 314.825 |
| naca_sst | 2x2 | native | original | recovery_release | adaptive | -13.0 | incomplete_time_cap | False | 3277 | 300.466 | 300.955 |
| naca_sst | 2x2 | segregated | original | recovery_release | forcing | -13.0 | incomplete_time_cap | False | 54 | 303.653 | 304.120 |
| nozzle_sst | 1x1 | native | original | recovery_release | adaptive | -13.0 | rejected | False | 6919 | 137.509 | 137.989 |
| nozzle_sst | 1x1 | coupled | original | recovery_release | forcing | -13.0 | incomplete_time_cap | False | 317 | 301.445 | 301.899 |
| nozzle_sst | 1x1 | segregated | original | recovery_release | forcing | -13.0 | incomplete_time_cap | False | 314 | 300.020 | 300.494 |
| nozzle_sst | 1x1 | native | original | recovery_release | original | -13.0 | rejected | False | 735 | 14.418 | 14.881 |
| periodic_vki | 1x1 | native | cold | recovery_release | adaptive | -12 | incomplete_time_cap | False | 2042 | 300.391 | 300.925 |
| periodic_vki | 1x1 | coupled | cold | recovery_release | forcing | -12 | incomplete_time_cap | False | 71 | 307.827 | 308.448 |
| periodic_vki | 1x1 | segregated | cold | recovery_release | forcing | -12 | incomplete_time_cap | False | 88 | 300.950 | 301.506 |
| nozzle_sst | 1x1 | segregated | original | baseline_4712d82375 | fd_step_1e-3_relative_0.01 | -12 | diagnostic_complete | False | 1 | — | — |
| nozzle_sst | 1x1 | segregated | original | baseline_4712d82375 | fd_step_1e-5_relative_0.01 | -12 | diagnostic_complete | False | 1 | — | — |
| nozzle_sst | 1x1 | segregated | original | baseline_4712d82375 | fd_step_1e-5 | -12 | diagnostic_complete | False | 1 | — | 2.727 |
| nozzle_sst | 1x1 | segregated | original | baseline_4712d82375 | fd_step_1e-6 | -12 | diagnostic_complete | False | 1 | — | 2.880 |
| nozzle_sst | 1x1 | segregated | original | baseline_4712d82375 | fd_step_1e-7 | -12 | diagnostic_complete | False | 1 | — | 1.765 |
| naca_sa | 2x2 | coupled | original | static_review_correctness_v2_release | diagonal | -14.0 | qualified | True | 356 | 1797.690 | 1798.629 |
| naca_sa | 1x1 | native | original | muscl_boundary_gradient_v2_release | original | -13.0 | qualified | True | 158 | 18.092 | 18.840 |
| naca_sa | 2x2 | coupled | original | muscl_boundary_gradient_v2_release | diagonal | -14.0 | running | False | — | — | — |
| naca_sa | 2x2 | native | original | muscl_boundary_gradient_v2_release | original | -13.0 | qualified | True | 151 | 51.890 | 52.558 |
| naca_sa | 1x1 | coupled | original | forcing_candidate | forcing | -13.0 | incomplete_time_cap | False | 209 | 300.938 | 301.445 |
| naca_sa | 1x1 | segregated | original | forcing_candidate | forcing | -13.0 | incomplete_time_cap | False | 157 | 300.054 | 300.632 |
| naca_sa | 1x1 | coupled | original | forcing_candidate | forcing | -13.0 | iteration_cap | False | 6 | 16.587 | 17.090 |
| naca_sa | 1x1 | native | original | forcing_candidate | fixed | -13.0 | qualified | True | 34 | 4.344 | 4.852 |
| naca_sa | 2x2 | native | original | forcing_candidate | fixed | -13.0 | qualified | True | 43 | 4.103 | 4.585 |
| naca_sa | 1x1 | coupled | original | forcing_candidate | original | -12 | reference_audit_failed | False | 11 | 42.537 | 43.081 |
| naca_sa | 1x1 | native | original | forcing_candidate | original | -12 | reference_audit_failed | False | 11 | 1.403 | 1.961 |
| naca_sa | 1x1 | segregated | original | forcing_candidate | original | -12 | reference_audit_failed | False | 11 | 9.216 | 9.786 |
| naca_sa | 2x2 | coupled | original | forcing_candidate | original | -12 | rejected | False | 11 | 18.367 | 18.862 |
| naca_sa | 2x2 | native | original | forcing_candidate | original | -12 | reference_audit_failed | False | 11 | 1.080 | 1.575 |
| naca_sa | 2x2 | segregated | original | forcing_candidate | original | -12 | reference_audit_failed | False | 11 | 5.590 | 6.062 |
| naca_sa | 1x1 | coupled | original | recovery_release | original | -18.0 | incomplete_time_cap | False | 1 | 3.068 | 3.656 |
| naca_sa | 1x1 | native | original | recovery_release | original | -18.0 | incomplete_time_cap | False | 13 | 1.650 | 2.167 |
| naca_sa | 1x1 | segregated | original | recovery_release | original | -18.0 | incomplete_time_cap | False | 1 | 3.038 | 3.635 |
| naca_sa | 2x2 | coupled | original | recovery_release | original | -18.0 | incomplete_time_cap | False | 1 | 1.768 | 2.289 |
| naca_sa | 2x2 | native | original | recovery_release | original | -18.0 | incomplete_time_cap | False | 19 | 1.699 | 2.216 |
| naca_sa | 2x2 | segregated | original | recovery_release | original | -18.0 | incomplete_time_cap | False | 1 | 2.619 | 3.085 |
| naca_sa | 1x1 | native | original | inner_candidate | original | -13.0 | qualified | True | 151 | 18.750 | 19.323 |
| naca_sa | 2x2 | native | original | inner_candidate | original | -13.0 | qualified | True | 151 | 14.177 | 14.780 |
| naca_sa | 1x1 | coupled | original | inner_candidate | diagonal | -14.0 | qualified | True | 166 | 556.675 | 557.225 |
| naca_sa | 2x2 | coupled | original | inner_candidate | diagonal | -14.0 | incomplete_time_cap | False | 574 | 1200.933 | 1201.498 |
| naca_sst | 1x1 | coupled | original | inner_candidate | original | -12 | iteration_cap | False | 20 | 55.908 | 56.456 |
| flatplate_lm | 1x1 | coupled | original | inner_candidate | forcing | -11 | incomplete_time_cap | False | 158 | 602.353 | 603.465 |
| flatplate_lm | 1x1 | segregated | original | inner_candidate | forcing | -11 | incomplete_time_cap | False | 28 | — | 661.039 |
| naca_sa | 1x1 | segregated | original | inner_candidate | forcing | -13.0 | incomplete_time_cap | False | 122 | 302.421 | 302.947 |
| naca_sa | 2x2 | segregated | original | inner_candidate | forcing | -13.0 | incomplete_time_cap | False | 258 | 300.516 | 301.010 |
| naca_sst | 1x1 | coupled | original | inner_candidate | forcing | -13.0 | incomplete_time_cap | False | 78 | 301.469 | 302.072 |
| naca_sst | 1x1 | segregated | original | inner_candidate | forcing | -13.0 | incomplete_time_cap | False | 46 | 304.674 | 305.175 |
| periodic_vki | 1x1 | coupled | cold | inner_candidate | forcing | -12 | incomplete_time_cap | False | 59 | 302.470 | 303.043 |
| periodic_vki | 1x1 | segregated | cold | inner_candidate | forcing | -12 | incomplete_time_cap | False | 84 | 301.573 | 302.156 |
| naca_sa | 1x1 | native | original | inner_scaling_release | original | -13.0 | qualified | True | 151 | 20.570 | 21.072 |
| naca_sa | 2x2 | native | original | inner_scaling_release | original | -13.0 | qualified | True | 151 | 12.626 | 13.135 |
| naca_sst | 1x1 | coupled | original | inner_scaling_release | original | -12 | iteration_cap | False | 20 | 84.426 | 84.961 |
| naca_sa | 1x1 | native | original | recovery_release | original | -13.0 | qualified | True | 151 | 18.913 | 19.465 |
| naca_sa | 1x1 | segregated | original | muscl_boundary_gradient_release | original | -13.0 | iteration_cap | False | 20 | 48.083 | 48.711 |
| naca_sa | 2x2 | segregated | original | muscl_boundary_gradient_release | original | -13.0 | iteration_cap | False | 20 | 49.640 | 50.180 |
| naca_sst | 1x1 | coupled | original | muscl_boundary_gradient_release | original | -13.0 | iteration_cap | False | 20 | 97.508 | 98.076 |
| naca_sst | 1x1 | segregated | original | muscl_boundary_gradient_release | original | -13.0 | iteration_cap | False | 20 | 111.275 | 111.892 |
| naca_sst | 2x2 | coupled | original | muscl_boundary_gradient_release | original | -13.0 | iteration_cap | False | 20 | 58.021 | 58.533 |
| naca_sst | 2x2 | segregated | original | muscl_boundary_gradient_release | original | -13.0 | iteration_cap | False | 20 | 62.004 | 62.619 |
| naca_sa | 1x1 | segregated | original | static_review_correctness_v2_release | original | -13.0 | iteration_cap | False | 20 | 47.514 | 48.085 |
| naca_sa | 2x2 | segregated | original | static_review_correctness_v2_release | original | -13.0 | iteration_cap | False | 20 | 48.956 | 49.491 |
| naca_sa | 1x1 | coupled | original | naca_rejection_flow3_release | diagonal | -14.0 | iteration_cap | False | 20 | 83.725 | 84.249 |
| naca_sa | 2x2 | coupled | original | naca_rejection_flow3_release | diagonal | -14.0 | iteration_cap | False | 20 | 51.990 | 52.542 |
| naca_sst | 1x1 | segregated | original | naca_rejection_flow3_release | original | -13.0 | iteration_cap | False | 20 | 63.654 | 64.198 |
| naca_sst | 2x2 | segregated | original | naca_rejection_flow3_release | original | -13.0 | iteration_cap | False | 20 | 33.477 | 33.961 |
| naca_sa | 1x1 | coupled | original | naca_rejection_inner3_release | diagonal | -14.0 | iteration_cap | False | 20 | 76.874 | 77.419 |
| naca_sa | 2x2 | coupled | original | naca_rejection_inner3_release | diagonal | -14.0 | iteration_cap | False | 20 | 48.234 | 48.738 |
| naca_sa | 1x1 | coupled | original | naca_rejection_v2_release | diagonal | -14.0 | iteration_cap | False | 20 | 85.491 | 85.998 |
| naca_sa | 1x1 | native | original | naca_rejection_v2_release | original | -13.0 | qualified | True | 151 | 21.418 | 21.960 |
| naca_sa | 2x2 | coupled | original | naca_rejection_v2_release | diagonal | -14.0 | iteration_cap | False | 20 | 74.231 | 74.759 |
| naca_sa | 2x2 | native | original | naca_rejection_v2_release | original | -13.0 | qualified | True | 151 | 16.050 | 16.558 |
| naca_sst | 1x1 | native | original | naca_rejection_v2_release | original | -13.0 | qualified | True | 397 | 60.211 | 60.786 |
| naca_sst | 2x2 | native | original | naca_rejection_v2_release | original | -13.0 | qualified | True | 397 | 43.669 | 44.102 |
| naca_sst | 1x1 | coupled | original | naca_rejection_v2_release | original | -13.0 | iteration_cap | False | 20 | 93.144 | 93.687 |
| naca_sst | 1x1 | segregated | original | naca_rejection_v2_release | original | -13.0 | iteration_cap | False | 20 | 102.571 | 103.116 |
| naca_sst | 2x2 | coupled | original | naca_rejection_v2_release | original | -13.0 | iteration_cap | False | 20 | 50.954 | 51.436 |
| naca_sst | 2x2 | segregated | original | naca_rejection_v2_release | original | -13.0 | iteration_cap | False | 20 | 59.187 | 59.676 |
| naca_sa | 2x2 | coupled | original | naca_rejection_v2_release | diagonal | -14.0 | qualified | True | 333 | 1090.736 | 1091.372 |
| naca_sst | 1x1 | segregated | original | naca_rejection_v2_release | original | -13.0 | incomplete_time_cap | False | 1254 | 3600.846 | 3601.375 |
| naca_sst | 1x1 | segregated | original | naca_rejection_flow3_release | original | -13.0 | incomplete_time_cap | False | 500 | 3601.092 | 3601.715 |
| flatplate_lm | 1x1 | coupled | original | baseline_4712d82375 | original | -11 | solver_failure | False | 0 | — | 1.432 |
| flatplate_lm | 1x1 | native | original | baseline_4712d82375 | original | -11 | solver_failure | False | 0 | — | 1.349 |
| flatplate_lm | 1x1 | segregated | original | baseline_4712d82375 | original | -11 | solver_failure | False | 0 | — | 1.356 |
| naca_sa | 1x1 | coupled | original | baseline_4712d82375 | original | -12 | rejected | False | 11 | 13.852 | 14.424 |
| naca_sa | 1x1 | native | original | baseline_4712d82375 | original | -12 | reference_audit_failed | False | 11 | 3.351 | 3.967 |
| naca_sa | 1x1 | segregated | original | baseline_4712d82375 | original | -12 | reference_audit_failed | False | 11 | 25.907 | 26.736 |
| naca_sst | 1x1 | coupled | original | baseline_4712d82375 | original | -12 | incomplete_time_cap | False | 16 | — | 121.018 |
| naca_sst | 1x1 | native | original | baseline_4712d82375 | original | -12 | iteration_cap | False | 200 | 73.236 | 73.902 |
| naca_sst | 1x1 | segregated | original | baseline_4712d82375 | original | -12 | iteration_cap | False | 20 | 72.321 | 72.900 |
| nozzle_sst | 1x1 | coupled | original | baseline_4712d82375 | original | -12 | iteration_cap | False | 20 | 5.296 | 5.773 |
| nozzle_sst | 1x1 | native | original | baseline_4712d82375 | original | -12 | iteration_cap | False | 200 | 4.396 | 4.807 |
| nozzle_sst | 1x1 | segregated | original | baseline_4712d82375 | original | -12 | iteration_cap | False | 20 | 5.328 | 5.779 |
| periodic_stator | 1x1 | coupled | original | baseline_4712d82375 | original | -12 | iteration_cap | False | 20 | 114.285 | 114.932 |
| periodic_stator | 1x1 | native | original | baseline_4712d82375 | original | -12 | iteration_cap | False | 200 | 19.509 | 20.140 |
| periodic_stator | 1x1 | segregated | original | baseline_4712d82375 | original | -12 | incomplete_time_cap | False | 18 | — | 121.040 |
| flatplate_lm | 1x1 | coupled | original | baseline_4712d82375 | original | -11 | iteration_cap | False | 20 | 129.516 | 130.661 |
| flatplate_lm | 1x1 | native | original | baseline_4712d82375 | original | -11 | iteration_cap | False | 200 | 82.402 | 83.438 |
| flatplate_lm | 1x1 | segregated | original | baseline_4712d82375 | original | -11 | incomplete_time_cap | False | 16 | — | 181.072 |
| periodic_vki | 1x1 | coupled | original | baseline_4712d82375 | original | -12 | solver_failure | False | 0 | — | 1.707 |
| periodic_vki | 1x1 | native | original | baseline_4712d82375 | original | -12 | solver_failure | False | 0 | — | 1.662 |
| periodic_vki | 1x1 | segregated | original | baseline_4712d82375 | original | -12 | solver_failure | False | 0 | — | 1.622 |
| nozzle_sst | 1x1 | segregated | original | probe_metric | original | -12 | iteration_cap | False | 20 | 7.975 | 8.448 |
| flatplate_lm | 1x1 | coupled | original | baseline_4712d82375 | original | -11 | iteration_cap | False | 1 | 43.213 | 44.499 |
| flatplate_lm | 1x1 | native | original | baseline_4712d82375 | original | -11 | iteration_cap | False | 1 | 0.426 | 1.813 |
| flatplate_lm | 1x1 | segregated | original | baseline_4712d82375 | original | -11 | iteration_cap | False | 1 | 12.930 | 14.264 |
| flatplate_lm | 2x2 | coupled | original | baseline_4712d82375 | original | -11 | iteration_cap | False | 1 | 24.777 | 25.717 |
| flatplate_lm | 2x2 | native | original | baseline_4712d82375 | original | -11 | iteration_cap | False | 1 | 0.252 | 1.207 |
| flatplate_lm | 2x2 | segregated | original | baseline_4712d82375 | original | -11 | iteration_cap | False | 1 | 7.193 | 8.197 |
| naca_sa | 1x1 | coupled | original | baseline_4712d82375 | original | -12 | iteration_cap | False | 1 | 2.500 | 3.051 |
| naca_sa | 1x1 | native | original | baseline_4712d82375 | original | -12 | iteration_cap | False | 1 | 0.125 | 0.689 |
| naca_sa | 1x1 | segregated | original | baseline_4712d82375 | original | -12 | iteration_cap | False | 1 | 2.562 | 3.124 |
| naca_sa | 2x2 | coupled | original | baseline_4712d82375 | original | -12 | iteration_cap | False | 1 | 0.765 | 1.446 |
| naca_sa | 2x2 | native | original | baseline_4712d82375 | original | -12 | iteration_cap | False | 1 | 0.080 | 0.687 |
| naca_sa | 2x2 | segregated | original | baseline_4712d82375 | original | -12 | iteration_cap | False | 1 | 1.220 | 1.718 |
| naca_sst | 1x1 | coupled | original | baseline_4712d82375 | original | -12 | iteration_cap | False | 8 | 31.357 | 31.971 |
| naca_sst | 1x1 | native | original | baseline_4712d82375 | original | -12 | iteration_cap | False | 8 | 1.089 | 1.651 |
| naca_sst | 1x1 | segregated | original | baseline_4712d82375 | original | -12 | iteration_cap | False | 8 | 18.969 | 19.599 |
| naca_sst | 2x2 | coupled | original | baseline_4712d82375 | original | -12 | iteration_cap | False | 8 | 11.792 | 12.336 |
| naca_sst | 2x2 | native | original | baseline_4712d82375 | original | -12 | iteration_cap | False | 8 | 0.690 | 1.217 |
| naca_sst | 2x2 | segregated | original | baseline_4712d82375 | original | -12 | iteration_cap | False | 8 | 6.359 | 6.862 |
| nozzle_sst | 1x1 | coupled | original | baseline_4712d82375 | original | -12 | iteration_cap | False | 1 | 1.668 | 2.131 |
| nozzle_sst | 1x1 | native | original | baseline_4712d82375 | original | -12 | iteration_cap | False | 1 | 0.017 | 0.507 |
| nozzle_sst | 1x1 | segregated | original | baseline_4712d82375 | original | -12 | iteration_cap | False | 1 | 1.449 | 1.855 |
| nozzle_sst | 2x2 | coupled | original | baseline_4712d82375 | original | -12 | iteration_cap | False | 1 | 0.782 | 1.255 |
| nozzle_sst | 2x2 | native | original | baseline_4712d82375 | original | -12 | iteration_cap | False | 1 | 0.008 | 0.472 |
| nozzle_sst | 2x2 | segregated | original | baseline_4712d82375 | original | -12 | iteration_cap | False | 1 | 0.737 | 1.154 |
| periodic_vki | 1x1 | coupled | cold | baseline_4712d82375 | original | -12 | iteration_cap | False | 1 | 3.856 | 4.817 |
| periodic_vki | 1x1 | native | cold | baseline_4712d82375 | original | -12 | iteration_cap | False | 1 | 0.040 | 0.586 |
| periodic_vki | 1x1 | segregated | cold | baseline_4712d82375 | original | -12 | iteration_cap | False | 1 | 6.793 | 7.368 |
| periodic_vki | 2x2 | coupled | cold | baseline_4712d82375 | original | -12 | iteration_cap | False | 1 | 2.068 | 2.587 |
| periodic_vki | 2x2 | native | cold | baseline_4712d82375 | original | -12 | iteration_cap | False | 1 | 0.033 | 0.584 |
| periodic_vki | 2x2 | segregated | cold | baseline_4712d82375 | original | -12 | iteration_cap | False | 1 | 1.999 | 2.499 |
| flatplate_lm | 1x1 | coupled | original | probe_metric | original | -11 | incomplete_time_cap | False | 50 | — | 301.067 |
| flatplate_lm | 1x1 | native | original | probe_metric | original | -11 | incomplete_time_cap | False | 695 | — | 301.077 |
| flatplate_lm | 1x1 | segregated | original | probe_metric | original | -11 | incomplete_time_cap | False | 23 | — | 301.059 |
| flatplate_lm | 2x2 | coupled | original | probe_metric | original | -11 | incomplete_time_cap | False | 103 | — | 301.039 |
| flatplate_lm | 2x2 | native | original | probe_metric | original | -11 | incomplete_time_cap | False | 1299 | — | 301.040 |
| flatplate_lm | 2x2 | segregated | original | probe_metric | original | -11 | incomplete_time_cap | False | 30 | — | 301.072 |
| naca_sa | 1x1 | coupled | original | probe_metric | original | -12 | rejected | False | 11 | 7.754 | 8.336 |
| naca_sa | 1x1 | native | original | probe_metric | original | -12 | reference_audit_failed | False | 11 | 1.376 | 1.905 |
| naca_sa | 1x1 | segregated | original | probe_metric | original | -12 | reference_audit_failed | False | 11 | 13.804 | 15.252 |
| naca_sa | 2x2 | coupled | original | probe_metric | original | -12 | rejected | False | 11 | 4.042 | 4.537 |
| naca_sa | 2x2 | native | original | probe_metric | original | -12 | reference_audit_failed | False | 11 | 1.052 | 1.606 |
| naca_sa | 2x2 | segregated | original | probe_metric | original | -12 | reference_audit_failed | False | 11 | 6.705 | 7.160 |
| naca_sst | 1x1 | coupled | original | probe_metric | original | -12 | incomplete_time_cap | False | 236 | — | 301.066 |
| naca_sst | 1x1 | native | original | probe_metric | original | -12 | reference_audit_failed | False | 258 | 40.705 | 41.323 |
| naca_sst | 1x1 | segregated | original | probe_metric | original | -12 | incomplete_time_cap | False | 202 | — | 301.034 |
| naca_sst | 2x2 | coupled | original | probe_metric | original | -12 | incomplete_time_cap | False | 425 | — | 301.044 |
| naca_sst | 2x2 | native | original | probe_metric | original | -12 | reference_audit_failed | False | 258 | 26.195 | 26.766 |
| naca_sst | 2x2 | segregated | original | probe_metric | original | -12 | incomplete_time_cap | False | 465 | — | 301.071 |
| nozzle_sst | 1x1 | coupled | original | probe_metric | original | -12 | incomplete_time_cap | False | 2139 | — | 301.040 |
| nozzle_sst | 1x1 | native | original | probe_metric | original | -12 | rejected | False | 548 | 15.533 | 15.969 |
| nozzle_sst | 1x1 | segregated | original | probe_metric | original | -12 | incomplete_time_cap | False | 1576 | — | 301.032 |
| nozzle_sst | 2x2 | coupled | original | probe_metric | original | -12 | iteration_cap | False | 3000 | 183.077 | 183.560 |
| nozzle_sst | 2x2 | native | original | probe_metric | original | -12 | rejected | False | 546 | 5.656 | 6.100 |
| nozzle_sst | 2x2 | segregated | original | probe_metric | original | -12 | iteration_cap | False | 3000 | 289.357 | 289.778 |
| periodic_vki | 1x1 | coupled | cold | probe_metric | original | -12 | incomplete_time_cap | False | 235 | — | 301.042 |
| periodic_vki | 1x1 | native | cold | probe_metric | original | -12 | iteration_cap | False | 3000 | 111.915 | 112.478 |
| periodic_vki | 1x1 | segregated | cold | probe_metric | original | -12 | incomplete_time_cap | False | 436 | — | 301.073 |
| periodic_vki | 2x2 | coupled | cold | probe_metric | original | -12 | incomplete_time_cap | False | 448 | — | 301.059 |
| periodic_vki | 2x2 | native | cold | probe_metric | original | -12 | iteration_cap | False | 3000 | 65.549 | 66.163 |
| periodic_vki | 2x2 | segregated | cold | probe_metric | original | -12 | incomplete_time_cap | False | 911 | — | 301.061 |
| flatplate_lm | 1x1 | coupled | original | recovery_release | original | -11 | incomplete_time_cap | False | 50 | — | 301.042 |
| flatplate_lm | 1x1 | native | original | recovery_release | original | -11 | incomplete_time_cap | False | 710 | — | 301.052 |
| flatplate_lm | 1x1 | segregated | original | recovery_release | original | -11 | incomplete_time_cap | False | 20 | — | 301.050 |
| flatplate_lm | 2x2 | coupled | original | recovery_release | original | -11 | incomplete_time_cap | False | 101 | — | 301.041 |
| flatplate_lm | 2x2 | native | original | recovery_release | original | -11 | incomplete_time_cap | False | 1419 | — | 301.064 |
| flatplate_lm | 2x2 | segregated | original | recovery_release | original | -11 | incomplete_time_cap | False | 26 | — | 301.025 |
| naca_sa | 1x1 | coupled | original | recovery_release | original | -12 | reference_audit_failed | False | 11 | 41.410 | 41.998 |
| naca_sa | 1x1 | native | original | recovery_release | original | -12 | reference_audit_failed | False | 11 | 1.378 | 1.907 |
| naca_sa | 1x1 | segregated | original | recovery_release | original | -12 | reference_audit_failed | False | 11 | 9.339 | 9.848 |
| naca_sa | 2x2 | coupled | original | recovery_release | original | -12 | rejected | False | 11 | 16.219 | 16.788 |
| naca_sa | 2x2 | native | original | recovery_release | original | -12 | reference_audit_failed | False | 11 | 0.929 | 1.423 |
| naca_sa | 2x2 | segregated | original | recovery_release | original | -12 | reference_audit_failed | False | 11 | 5.350 | 5.869 |
| naca_sst | 1x1 | coupled | original | recovery_release | original | -12 | incomplete_time_cap | False | 216 | — | 301.041 |
| naca_sst | 1x1 | native | original | recovery_release | original | -12 | reference_audit_failed | False | 258 | 36.407 | 36.936 |
| naca_sst | 1x1 | segregated | original | recovery_release | original | -12 | incomplete_time_cap | False | 52 | — | 301.053 |
| naca_sst | 2x2 | coupled | original | recovery_release | original | -12 | incomplete_time_cap | False | 445 | — | 301.036 |
| naca_sst | 2x2 | native | original | recovery_release | original | -12 | reference_audit_failed | False | 258 | 27.401 | 28.073 |
| naca_sst | 2x2 | segregated | original | recovery_release | original | -12 | incomplete_time_cap | False | 84 | — | 301.048 |
| nozzle_sst | 1x1 | coupled | original | recovery_release | original | -12 | incomplete_time_cap | False | 1183 | — | 301.035 |
| nozzle_sst | 1x1 | native | original | recovery_release | original | -12 | rejected | False | 548 | 11.236 | 11.734 |
| nozzle_sst | 1x1 | segregated | original | recovery_release | original | -12 | incomplete_time_cap | False | 537 | — | 301.088 |
| nozzle_sst | 2x2 | coupled | original | recovery_release | original | -12 | incomplete_time_cap | False | 2142 | — | 301.024 |
| nozzle_sst | 2x2 | native | original | recovery_release | original | -12 | rejected | False | 546 | 5.792 | 6.197 |
| nozzle_sst | 2x2 | segregated | original | recovery_release | original | -12 | incomplete_time_cap | False | 2438 | — | 301.039 |
| periodic_vki | 1x1 | coupled | cold | recovery_release | original | -12 | incomplete_time_cap | False | 235 | — | 301.074 |
| periodic_vki | 1x1 | native | cold | recovery_release | original | -12 | iteration_cap | False | 3000 | 121.860 | 122.402 |
| periodic_vki | 1x1 | segregated | cold | recovery_release | original | -12 | incomplete_time_cap | False | 452 | — | 301.023 |
| periodic_vki | 2x2 | coupled | cold | recovery_release | original | -12 | incomplete_time_cap | False | 479 | — | 301.031 |
| periodic_vki | 2x2 | native | cold | recovery_release | original | -12 | iteration_cap | False | 3000 | 67.211 | 67.698 |
| periodic_vki | 2x2 | segregated | cold | recovery_release | original | -12 | incomplete_time_cap | False | 1106 | — | 301.052 |
| naca_sa | 1x1 | native | original | recovery_release | fixed | -13.0 | qualified | True | 34 | 4.364 | 4.990 |
| naca_sa | 1x1 | native | original | recovery_release | fixed | -13.0 | qualified | True | 34 | 4.358 | 4.887 |
| naca_sa | 1x1 | native | original | recovery_release | fixed | -13.0 | qualified | True | 34 | 4.282 | 4.808 |
| naca_sa | 1x1 | native | original | recovery_release | fixed | -13.0 | qualified | True | 34 | 4.375 | 4.943 |
| naca_sa | 1x1 | native | original | recovery_release | fixed | -13.0 | qualified | True | 34 | 4.263 | 4.793 |
| naca_sa | 1x1 | native | original | recovery_release | fixed | -13.0 | qualified | True | 34 | 4.112 | 4.610 |
| naca_sa | 2x2 | native | original | recovery_release | fixed | -13.0 | qualified | True | 43 | 3.667 | 4.136 |
| naca_sa | 2x2 | native | original | recovery_release | fixed | -13.0 | qualified | True | 43 | 3.466 | 3.974 |
| naca_sa | 2x2 | native | original | recovery_release | fixed | -13.0 | qualified | True | 43 | 3.368 | 3.916 |
| naca_sa | 1x1 | coupled | original | recovery_release | original | -13.0 | incomplete_time_cap | False | 29 | — | 121.027 |
| naca_sa | 1x1 | native | original | recovery_release | original | -13.0 | qualified | True | 151 | 19.319 | 19.860 |
| naca_sa | 1x1 | segregated | original | recovery_release | original | -13.0 | incomplete_time_cap | False | 65 | — | 121.032 |
| naca_sa | 1x1 | native | original | recovery_release | original | -13.0 | qualified | True | 151 | 18.857 | 19.360 |
| naca_sa | 1x1 | native | original | recovery_release | original | -13.0 | qualified | True | 151 | 19.616 | 20.191 |
| naca_sa | 1x1 | native | original | recovery_release | original | -13.0 | qualified | True | 151 | 18.952 | 19.476 |
| naca_sa | 1x1 | native | original | recovery_release | original | -13.0 | qualified | True | 151 | 19.144 | 19.646 |
| naca_sa | 1x1 | native | original | recovery_release | original | -13.0 | qualified | True | 151 | 18.769 | 19.259 |
| naca_sa | 1x1 | native | original | recovery_release | original | -13.0 | qualified | True | 151 | 18.861 | 19.394 |
| naca_sa | 1x1 | native | original | recovery_release | original | -13.0 | qualified | True | 151 | 19.029 | 19.568 |
| naca_sa | 1x1 | native | original | recovery_release | original | -13.0 | qualified | True | 151 | 18.743 | 19.302 |
| naca_sa | 2x2 | coupled | original | recovery_release | original | -13.0 | incomplete_time_cap | False | 84 | — | 121.046 |
| naca_sa | 2x2 | native | original | recovery_release | original | -13.0 | qualified | True | 151 | 12.894 | 13.346 |
| naca_sa | 2x2 | segregated | original | recovery_release | original | -13.0 | incomplete_time_cap | False | 66 | — | 121.080 |
| naca_sa | 2x2 | native | original | recovery_release | original | -13.0 | qualified | True | 151 | 13.122 | 13.658 |
| naca_sa | 2x2 | native | original | recovery_release | original | -13.0 | qualified | True | 151 | 13.084 | 13.597 |
| naca_sa | 2x2 | native | original | recovery_release | original | -13.0 | qualified | True | 151 | 12.632 | 13.143 |
| naca_sa | 2x2 | native | original | recovery_release | original | -13.0 | qualified | True | 151 | 12.597 | 13.229 |
| naca_sa | 2x2 | native | original | recovery_release | original | -13.0 | qualified | True | 151 | 13.219 | 13.754 |
| naca_sa | 2x2 | native | original | recovery_release | original | -13.0 | qualified | True | 151 | 13.390 | 13.904 |
| naca_sa | 2x2 | native | original | recovery_release | original | -13.0 | qualified | True | 151 | 12.795 | 13.242 |
| naca_sa | 2x2 | native | original | recovery_release | original | -13.0 | qualified | True | 151 | 13.170 | 13.704 |
| nozzle_sst | 1x1 | segregated | original | probe_recovery | original | -12 | iteration_cap | False | 1 | 3.954 | 4.414 |
| nozzle_sst | 1x1 | segregated | original | probe_recovery_checked | fd_step_1e-8 | -12 | diagnostic_complete | False | 20 | — | 19.253 |
| nozzle_sst | 1x1 | segregated | original | probe_recovery_checked | fd_step_1e-8 | -12 | diagnostic_complete | False | 1 | — | 4.708 |
| naca_sa | 1x1 | native | original | recovery_release | original | -14.0 | qualified | True | 298 | 36.747 | 37.305 |
| naca_sa | 1x1 | native | original | recovery_release | original | -15.0 | incomplete_time_cap | False | 2669 | 300.112 | 300.743 |
| naca_sa | 1x1 | coupled | original | recovery_release | forcing | -13.0 | iteration_cap | False | 6 | 35.446 | 35.946 |
| naca_sa | 1x1 | coupled | original | recovery_release | forcing | -13.0 | iteration_cap | False | 6 | 42.225 | 42.784 |
| naca_sa | 1x1 | coupled | original | recovery_release | forcing | -13.0 | iteration_cap | False | 6 | 20.044 | 20.660 |
| naca_sa | 1x1 | coupled | original | recovery_release | diagonal | -13.0 | rejected | False | 78 | 324.573 | 325.099 |
| naca_sa | 1x1 | coupled | original | recovery_release | diagonal | -14.0 | qualified | True | 179 | 764.796 | 765.385 |
| naca_sst | 1x1 | native | original | recovery_release | original | -14.0 | qualified | True | 545 | 76.329 | 76.906 |
| naca_sst | 1x1 | native | original | recovery_release | original | -15.0 | incomplete_time_cap | False | 1403 | 180.083 | 180.631 |
| naca_sa | 1x1 | coupled | original | static_review_correctness_v2_release | diagonal | -14.0 | iteration_cap | False | 20 | 84.779 | 85.362 |
| naca_sa | 1x1 | native | original | static_review_correctness_v2_release | original | -13.0 | qualified | True | 151 | 18.889 | 19.440 |
| naca_sa | 2x2 | coupled | original | static_review_correctness_v2_release | diagonal | -14.0 | iteration_cap | False | 20 | 69.982 | 70.462 |
| naca_sa | 2x2 | native | original | static_review_correctness_v2_release | original | -13.0 | qualified | True | 151 | 13.307 | 13.824 |
| naca_sst | 1x1 | native | original | static_review_correctness_v2_release | original | -13.0 | qualified | True | 397 | 65.074 | 65.588 |
| naca_sst | 2x2 | native | original | static_review_correctness_v2_release | original | -13.0 | qualified | True | 397 | 47.730 | 48.270 |
| naca_sst | 1x1 | coupled | original | static_review_correctness_v2_release | original | -13.0 | iteration_cap | False | 20 | 92.129 | 92.672 |
| naca_sst | 1x1 | segregated | original | static_review_correctness_v2_release | original | -13.0 | iteration_cap | False | 20 | 103.107 | 103.654 |
| naca_sst | 2x2 | coupled | original | static_review_correctness_v2_release | original | -13.0 | iteration_cap | False | 20 | 46.646 | 47.136 |
| naca_sst | 2x2 | segregated | original | static_review_correctness_v2_release | original | -13.0 | iteration_cap | False | 20 | 51.690 | 52.223 |

## Original-control implementation comparison

Each row retains its convergence status. Assemblies are FLOW/TURB/TRANS;
Krylov work is outer/inner; R/B is retries/backtracks. Native assembly counts are
inferred from fine-grid updates and labelled in the CSV; Newton counts are measured.
Native linear work sums the reported iterations of every active system; it has no nested solves.
Interrupted counters omit unlogged work; their final reporting refresh is not verified. Timings are contended
observations, never eligible time-to-convergence measurements. A saved timeout state
is only its last checkpoint. State errors use the native reference recorded for that row.

| Case | Layout | Implementation | Mode | Status | Updates | Assemblies F/T/X | Krylov outer/inner | R/B | Loop/E2E s | Max normalized RMS | Max point error |
|---|---|---|---|---|---:|---:|---:|---:|---:|---:|---:|
| flatplate_lm | 1x1 | probe_metric | coupled | incomplete_time_cap | 50 | 1114/1241/1241 | 125/0 | 4/21 | —/301.067 | — | — |
| flatplate_lm | 1x1 | probe_metric | native | incomplete_time_cap | 695 | 695/695/695 | 10421/0 | —/— | —/301.077 | 4.520e-04 | — |
| flatplate_lm | 1x1 | probe_metric | segregated | incomplete_time_cap | 23 | 1581/251/257 | 477/0 | 2/17 | —/301.059 | — | — |
| flatplate_lm | 2x2 | probe_metric | coupled | incomplete_time_cap | 103 | 2096/2337/2337 | 239/0 | 4/21 | —/301.039 | 1.673e+00 | — |
| flatplate_lm | 2x2 | probe_metric | native | incomplete_time_cap | 1299 | 1299/1299/1299 | 19483/0 | —/— | —/301.040 | 8.428e-04 | — |
| flatplate_lm | 2x2 | probe_metric | segregated | incomplete_time_cap | 30 | 2961/322/356 | 773/0 | 2/17 | —/301.072 | — | — |
| naca_sa | 1x1 | probe_metric | coupled | rejected | 11 | 341/395/— | 51/0 | 3/0 | 7.754/8.336 | 1.196e-10 | 1.658e-05 |
| naca_sa | 1x1 | probe_metric | native | reference_audit_failed | 11 | 11/11/— | 440/0 | —/— | 1.376/1.905 | 1.963e-10 | 0.000e+00 |
| naca_sa | 1x1 | probe_metric | segregated | reference_audit_failed | 11 | 444/429/— | 429/0 | 4/0 | 13.804/15.252 | 1.310e-11 | 2.946e-06 |
| naca_sa | 2x2 | probe_metric | coupled | rejected | 11 | 364/421/— | 53/0 | 3/0 | 4.042/4.537 | 9.982e-11 | 1.776e-05 |
| naca_sa | 2x2 | probe_metric | native | reference_audit_failed | 11 | 11/11/— | 440/0 | —/— | 1.052/1.606 | 2.256e-10 | 0.000e+00 |
| naca_sa | 2x2 | probe_metric | segregated | reference_audit_failed | 11 | 465/372/— | 379/0 | 4/0 | 6.705/7.160 | 1.308e-11 | 3.215e-06 |
| naca_sst | 1x1 | probe_metric | coupled | incomplete_time_cap | 236 | 12482/13946/— | 1450/0 | 4/0 | —/301.066 | 1.625e-06 | — |
| naca_sst | 1x1 | probe_metric | native | reference_audit_failed | 258 | 258/258/— | 10173/0 | —/— | 40.705/41.323 | 8.922e-12 | 0.000e+00 |
| naca_sst | 1x1 | probe_metric | segregated | incomplete_time_cap | 202 | 10793/4311/— | 4116/0 | 2/0 | —/301.034 | 1.567e-07 | — |
| naca_sst | 2x2 | probe_metric | coupled | incomplete_time_cap | 425 | 23363/26043/— | 2633/0 | 4/0 | —/301.044 | 1.660e-06 | — |
| naca_sst | 2x2 | probe_metric | native | reference_audit_failed | 258 | 258/258/— | 10320/0 | —/— | 26.195/26.766 | 8.919e-12 | 0.000e+00 |
| naca_sst | 2x2 | probe_metric | segregated | incomplete_time_cap | 465 | 21517/12143/— | 8524/0 | 2/0 | —/301.071 | 2.970e-08 | — |
| nozzle_sst | 1x1 | probe_metric | coupled | incomplete_time_cap | 2139 | 31987/34739/— | 2746/0 | 8/0 | —/301.040 | — | — |
| nozzle_sst | 1x1 | probe_metric | native | rejected | 548 | 548/548/— | 6235/0 | —/— | 15.533/15.969 | 7.647e-09 | — |
| nozzle_sst | 1x1 | probe_metric | segregated | incomplete_time_cap | 1576 | 24309/28673/— | 19884/0 | 8/0 | —/301.032 | — | — |
| nozzle_sst | 2x2 | probe_metric | coupled | iteration_cap | 3000 | 43920/47505/— | 3578/0 | 8/0 | 183.077/183.560 | 7.859e-03 | — |
| nozzle_sst | 2x2 | probe_metric | native | rejected | 546 | 546/546/— | 6568/0 | —/— | 5.656/6.100 | 7.649e-09 | — |
| nozzle_sst | 2x2 | probe_metric | segregated | iteration_cap | 3000 | 44084/64886/— | 47505/0 | 8/0 | 289.357/289.778 | 7.863e-03 | — |
| periodic_vki | 1x1 | probe_metric | coupled | incomplete_time_cap | 235 | 10998/13792/— | 2794/0 | 0/0 | —/301.042 | 2.412e-05 | — |
| periodic_vki | 1x1 | probe_metric | native | iteration_cap | 3000 | 3000/3000/— | 12000/0 | —/— | 111.915/112.478 | 2.419e-05 | — |
| periodic_vki | 1x1 | probe_metric | segregated | incomplete_time_cap | 436 | 12036/5055/— | 4263/0 | 1/0 | —/301.073 | 3.417e-05 | — |
| periodic_vki | 2x2 | probe_metric | coupled | incomplete_time_cap | 448 | 20542/25734/— | 5192/0 | 0/0 | —/301.059 | 7.479e-06 | — |
| periodic_vki | 2x2 | probe_metric | native | iteration_cap | 3000 | 3000/3000/— | 12000/0 | —/— | 65.549/66.163 | 2.799e-05 | — |
| periodic_vki | 2x2 | probe_metric | segregated | incomplete_time_cap | 911 | 24411/11295/— | 9110/0 | 1/21 | —/301.061 | 4.110e-05 | — |
| flatplate_lm | 1x1 | recovery_release | coupled | incomplete_time_cap | 50 | 1123/1251/1251 | 128/0 | 4/2 | —/301.042 | — | — |
| flatplate_lm | 1x1 | recovery_release | native | incomplete_time_cap | 710 | 710/710/710 | 10646/0 | —/— | —/301.052 | 7.939e-04 | — |
| flatplate_lm | 1x1 | recovery_release | segregated | incomplete_time_cap | 20 | 1509/222/214 | 380/0 | 2/18 | —/301.050 | — | — |
| flatplate_lm | 2x2 | recovery_release | coupled | incomplete_time_cap | 101 | 2071/2309/2309 | 238/0 | 4/2 | —/301.041 | 1.673e+00 | — |
| flatplate_lm | 2x2 | recovery_release | native | incomplete_time_cap | 1419 | 1419/1419/1419 | 21283/0 | —/— | —/301.064 | 1.039e-03 | — |
| flatplate_lm | 2x2 | recovery_release | segregated | incomplete_time_cap | 26 | 3007/281/305 | 702/0 | 2/19 | —/301.025 | — | — |
| naca_sa | 1x1 | recovery_release | coupled | reference_audit_failed | 11 | 1629/2055/— | 423/0 | 0/0 | 41.410/41.998 | 1.334e-11 | 1.199e-06 |
| naca_sa | 1x1 | recovery_release | native | reference_audit_failed | 11 | 11/11/— | 440/0 | —/— | 1.378/1.907 | 1.963e-10 | 0.000e+00 |
| naca_sa | 1x1 | recovery_release | segregated | reference_audit_failed | 11 | 170/427/— | 388/0 | 0/0 | 9.339/9.848 | 1.257e-11 | 2.255e-06 |
| naca_sa | 2x2 | recovery_release | coupled | rejected | 11 | 1262/1564/— | 301/0 | 1/0 | 16.219/16.788 | 3.006e-11 | 1.352e-05 |
| naca_sa | 2x2 | recovery_release | native | reference_audit_failed | 11 | 11/11/— | 440/0 | —/— | 0.929/1.423 | 2.256e-10 | 0.000e+00 |
| naca_sa | 2x2 | recovery_release | segregated | reference_audit_failed | 11 | 260/372/— | 363/0 | 0/0 | 5.350/5.869 | 1.253e-11 | 2.789e-06 |
| naca_sst | 1x1 | recovery_release | coupled | incomplete_time_cap | 216 | 12074/13496/— | 1415/0 | 4/0 | —/301.041 | 1.515e-06 | — |
| naca_sst | 1x1 | recovery_release | native | reference_audit_failed | 258 | 258/258/— | 10173/0 | —/— | 36.407/36.936 | 8.922e-12 | 0.000e+00 |
| naca_sst | 1x1 | recovery_release | segregated | incomplete_time_cap | 52 | 11661/1698/— | 4050/0 | 0/0 | —/301.053 | — | — |
| naca_sst | 2x2 | recovery_release | coupled | incomplete_time_cap | 445 | 23585/26433/— | 2846/0 | 4/0 | —/301.036 | 1.398e-06 | — |
| naca_sst | 2x2 | recovery_release | native | reference_audit_failed | 258 | 258/258/— | 10320/0 | —/— | 27.401/28.073 | 8.919e-12 | 0.000e+00 |
| naca_sst | 2x2 | recovery_release | segregated | incomplete_time_cap | 84 | 20828/2309/— | 7305/0 | 0/0 | —/301.048 | — | — |
| nozzle_sst | 1x1 | recovery_release | coupled | incomplete_time_cap | 1183 | 30261/33665/— | 3402/0 | 5/0 | —/301.035 | 3.538e-03 | — |
| nozzle_sst | 1x1 | recovery_release | native | rejected | 548 | 548/548/— | 6235/0 | —/— | 11.236/11.734 | 7.647e-09 | — |
| nozzle_sst | 1x1 | recovery_release | segregated | incomplete_time_cap | 537 | 33622/11133/— | 13529/0 | 3/3 | —/301.088 | 4.508e-06 | — |
| nozzle_sst | 2x2 | recovery_release | coupled | incomplete_time_cap | 2142 | 70006/77519/— | 7502/0 | 6/0 | —/301.024 | 2.932e-04 | — |
| nozzle_sst | 2x2 | recovery_release | native | rejected | 546 | 546/546/— | 6568/0 | —/— | 5.792/6.197 | 7.649e-09 | — |
| nozzle_sst | 2x2 | recovery_release | segregated | incomplete_time_cap | 2438 | 48375/59744/— | 49612/0 | 5/0 | —/301.039 | 4.445e-02 | — |
| periodic_vki | 1x1 | recovery_release | coupled | incomplete_time_cap | 235 | 10998/13792/— | 2794/0 | 0/0 | —/301.074 | 2.412e-05 | — |
| periodic_vki | 1x1 | recovery_release | native | iteration_cap | 3000 | 3000/3000/— | 12000/0 | —/— | 121.860/122.402 | 2.419e-05 | — |
| periodic_vki | 1x1 | recovery_release | segregated | incomplete_time_cap | 452 | 12461/5263/— | 4442/0 | 1/0 | —/301.023 | 3.417e-05 | — |
| periodic_vki | 2x2 | recovery_release | coupled | incomplete_time_cap | 479 | 21720/27191/— | 5471/0 | 0/0 | —/301.031 | 7.479e-06 | — |
| periodic_vki | 2x2 | recovery_release | native | iteration_cap | 3000 | 3000/3000/— | 12000/0 | —/— | 67.211/67.698 | 2.799e-05 | — |
| periodic_vki | 2x2 | recovery_release | segregated | incomplete_time_cap | 1106 | 28888/13407/— | 10607/0 | 1/21 | —/301.052 | 4.884e-05 | — |

## Remaining work cost on final code

The first SA1x1 history-margin trial of each mode uses final numerical source d27f4b37f6,
the same supplied restart, CFL1000 and density history stop-13. The native endpoint qualifies;
both Newton trials hit120 seconds. All timings failed quiet eligibility.
The coupled prefix has zero retries/backtracking, yet assembles approximately160 flow and201
turbulence residuals per completed outer update. Its1161 Krylov iterations are authoritative
attempt telemetry; summing coupled per-system history fields would incorrectly double them.
This isolates a remaining product/preconditioning work cost even with every step accepted.
It does not establish an application time-to-convergence ratio. Krylov counts also operate
on different block widths; they are not equal-cost units between modes.

| Mode | Status | Updates | Flow assemblies | Turb assemblies | Krylov work | Retries | Backtracks |
|---|---|---:|---:|---:|---:|---:|---:|
| coupled | incomplete_time_cap | 29 | 4654 | 5836 | 1161 | 0 | 0 |
| native | qualified | 151 | 151 | 151 | 6040 | — | — |
| segregated | incomplete_time_cap | 65 | 4991 | 2586 | 3022 | 2 | 0 |

## Final-code CPU sampling

One10-second periodic VKI2x2 coupled window on final code produced3186 samples with no reported loss.
Self-cycle shares include live cached least-squares gradients15.26%, limiter calculation7.50%,
flow flux6.81%, turbulence edge flux4.16%, periodic projection4.56%, state restoration3.20%,
and ILU application2.89%. These are selected leaf symbols, not complete phase totals.
They describe CPU cycles, not wall-time fractions, blocked waiting, call counts or speedup.
The instrumented baseline phase tables remain separate. Sampling further disqualifies
the affected run from timing comparisons. Raw records, commands, PID identities and hashes
are under `release_sampling/1792294833149198230/`.
Optional inline debug lookup emitted DWARF section-size warnings; the separate no-inline
ELF-symbol report avoids those lookups. The initial report and raw data are preserved.
[Selected-symbol CSV](NewtonKrylovApplicationSampling20261018.csv).

The inspected point-wise OpenMP loops include implicit barriers, and vector dot products
start with a barrier. No missing barrier was identified at these inspected temporary-vector
boundaries; this limited audit does not establish the absence of every threading defect.

## Measured baseline work prefixes

These are instrumented, capped prefixes, not convergence comparisons or eligible timing results.
Counts are measured method scopes on rank zero; every rank agrees with the telemetry.
Full per-rank inclusive/exclusive phase times and communication/allocation hooks are in
[the profile CSV](NewtonKrylovApplicationProfiles20261018.csv).
Do not add inclusive times across nested scopes. Profiles retain their original source snapshot.

| Case | Layout | Mode | Updates | Flow | Turb | Trans | Wrapper calls | Verified products | Preconditioner builds |
|---|---|---|---:|---:|---:|---:|---:|---:|---:|
| flatplate_lm | 1x1 | coupled | 1 | 168 | 186 | 186 | 199 | 28 | 15 |
| flatplate_lm | 1x1 | native | 1 | 1 | 1 | 1 | 0 | 0 | 3 |
| flatplate_lm | 1x1 | segregated | 1 | 52 | 23 | 17 | 78 | 10 | 4 |
| flatplate_lm | 2x2 | coupled | 1 | 168 | 186 | 186 | 199 | 28 | 15 |
| flatplate_lm | 2x2 | native | 1 | 1 | 1 | 1 | 0 | 0 | 3 |
| flatplate_lm | 2x2 | segregated | 1 | 54 | 24 | 18 | 82 | 10 | 4 |
| naca_sa | 1x1 | coupled | 1 | 111 | 125 | 0 | 120 | 19 | 8 |
| naca_sa | 1x1 | native | 1 | 1 | 1 | 0 | 0 | 0 | 2 |
| naca_sa | 1x1 | segregated | 1 | 129 | 35 | 0 | 154 | 21 | 5 |
| naca_sa | 2x2 | coupled | 1 | 77 | 82 | 0 | 78 | 9 | 6 |
| naca_sa | 2x2 | native | 1 | 1 | 1 | 0 | 0 | 0 | 2 |
| naca_sa | 2x2 | segregated | 1 | 127 | 29 | 0 | 146 | 25 | 5 |
| naca_sst | 1x1 | coupled | 8 | 1253 | 1413 | 0 | 1403 | 177 | 18 |
| naca_sst | 1x1 | native | 8 | 8 | 8 | 0 | 0 | 0 | 16 |
| naca_sst | 1x1 | segregated | 8 | 769 | 105 | 0 | 845 | 199 | 17 |
| naca_sst | 2x2 | coupled | 8 | 931 | 1035 | 0 | 1023 | 123 | 22 |
| naca_sst | 2x2 | native | 8 | 8 | 8 | 0 | 0 | 0 | 16 |
| naca_sst | 2x2 | segregated | 8 | 535 | 128 | 0 | 633 | 115 | 18 |
| nozzle_sst | 1x1 | coupled | 1 | 189 | 200 | 0 | 190 | 21 | 18 |
| nozzle_sst | 1x1 | native | 1 | 1 | 1 | 0 | 0 | 0 | 2 |
| nozzle_sst | 1x1 | segregated | 1 | 211 | 17 | 0 | 212 | 23 | 11 |
| nozzle_sst | 2x2 | coupled | 1 | 189 | 200 | 0 | 190 | 21 | 18 |
| nozzle_sst | 2x2 | native | 1 | 1 | 1 | 0 | 0 | 0 | 2 |
| nozzle_sst | 2x2 | segregated | 1 | 211 | 19 | 0 | 214 | 23 | 11 |
| periodic_vki | 1x1 | coupled | 1 | 134 | 172 | 0 | 170 | 40 | 2 |
| periodic_vki | 1x1 | native | 1 | 1 | 1 | 0 | 0 | 0 | 2 |
| periodic_vki | 1x1 | segregated | 1 | 287 | 11 | 0 | 290 | 91 | 3 |
| periodic_vki | 2x2 | coupled | 1 | 130 | 168 | 0 | 166 | 40 | 2 |
| periodic_vki | 2x2 | native | 1 | 1 | 1 | 0 | 0 | 0 | 2 |
| periodic_vki | 2x2 | segregated | 1 | 183 | 12 | 0 | 187 | 56 | 3 |

## Reproduction

Input and build hashes: `manifest.json` in the evidence directory.
The isolated native output-location trial (`isolated_io/`) matches configuration, binary
and restart hashes of the workspace run, but also fails quiet eligibility. It proves no
storage or implementation timing gain. All files/links were verified before temporary cleanup.
Control plans/audits: `control_plan.json`, `control_setup_audit.json`; original108 versions
are retained with the `_original108` suffix. No physical or agreement target changed.
Final-code binary/source hashes: `recovery_release/manifest.json`; original-control commands:
`qualified_original_commands.json` / `qualified_recovery_commands.json`; controls: `control_commands.json`.
The control study uses `--graceful-limit`: at the nominal wall budget the harness signals
one owned solver rank through SU2's existing interruption handler, completing the current
outer update and saving restart, solver-loop time and rank RSS. A launcher timeout at
budget+60s remains the fallback. This changes failure capture; it changes no numerical target.
Six native/segregated/coupled checks on1x1/2x2 preserve final measurements and remain
unqualified despite exit0. A physically converged stored-state detector also rejects a
wall-limited exit. Deliberate two-second checks are labelled harness regressions, and
the synthetic detector is excluded from the trial ledger. Prior timeouts remain untouched.
Each trial preserves `run.cfg`, exact command, runner hash, exit status, log, history,
Newton attempt/trial diagnostics and per-rank process resources. Discovery commands:

```sh
python3 application_case.py --case naca_sa --mode native --updates 200 --timeout 120 --output NEW_DIRECTORY
```

Run from the evidence directory using the preserved runner and manifest. The runner
defers to external compiler/solver jobs. The enclosing `pilots_monitor` records host CPU/RAM
and swap throughout; correctness-only monitors never confer timing eligibility.
Only a physically qualified --quiet run whose every host observation passes may enter timing medians.

Solver-loop time is the native SU2 performance timer. End-to-end time includes MPI startup,
initialization and final output, excluding the contention gate and independent endpoint oracle.
Rank peak RSS comes from `/usr/bin/time`; the sum of rank peaks is an upper bound on their
simultaneous total. Process-tree RSS is sampled once per second. Profiling binaries will be
kept separate from release timing. Native per-system counts require measured profile confirmation;
Newton cumulative attempt counters exclude incomplete attempts on a timeout unless a complete
profile is available. Such failure counters are lower bounds. No timing assertions are used.
