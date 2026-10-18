# Physical-progress forcing experiment — not adopted

The production source retains its existing forcing policy. This experiment
reduced prefix Krylov work, but did not qualify either full SA application.
The coupled endpoint had more retries and a larger state error. No speedup is
established. The candidate binaries, source patch, tests and failed runs remain
preserved.

## Hypothesis and scope

With pseudo-time merit enabled, the accepted Armijo residual includes the frozen
mass action of the step. Its origin resets at the next outer base. The experiment
passed the already computed physical merit to the existing Eisenstat–Walker
helper, to test whether physical progress avoids unnecessarily tight targets.
This was a policy hypothesis, not a proven correctness defect.

All forcing bounds, derivative verification, independent true-defect limits,
Armijo acceptance, recovery and rollback rules stayed fixed. No evaluation,
allocation, option or reduction was added to production code. Fixed forcing and
physical-only merit retained their previous policy result. The one-field change
and its proposed-policy test were subsequently removed from the working source.

For the original coupled SA accepted update3, stored physical merit changed from
4.3118271605760704e-18 to4.0427410976198762e-18, while augmented merit became
2.2643138812807092e-23. The helper produces target9.8729962781104864e-5 using
augmented progress and0.1 using physical progress. This counterfactual calculation
motivated a controlled experiment; it did not establish which policy converges
more efficiently. Evidence: `forcing_merit_diagnosis.json`.

## Tests and unchanged controls

The proposed-policy detector exercised FLOW, TURB and coupled groups with both
merit settings. Its three old-caller failures establish which signal the caller
uses; they do not independently prove either policy correct. The initial fixture
failure from missing auxiliary flow-time-step setup is retained separately.

The candidate passed69 detector assertions on each layout, all62 native tests
on1x1/2x2,91 ordinary tests, and species-boundary, residual-failure and turbo-support
CLI regressions on both layouts. Passing these checks did not establish an
application improvement. Commands/logs: `forcing_candidate_checks/`.

All six original-control native/segregated/coupled comparisons on1x1/2x2 had
byte-identical restart states and identical assembly counts to d27. Their early
11-update endpoints remain insufficient for definitive SA qualification. Native
CFL10000 qualified in34/43 updates on1x1/2x2; the serial state matched the earlier
native result byte-for-byte.

## Prefix and full-application results

The coupled six-update comparison has identical configuration SHA256
`82ac6942aeb4bc5b8e6f51bd26b8d31d9c993af6954a12e2872c9f72fa1ac9d6`.
Both endpoints remain unqualified.

| Measure | Existing policy | Proposed physical-progress policy |
|---|---:|---:|
| Accepted outer updates | 6 | 6 |
| Flow assemblies | 838 | 689 |
| Turbulence assemblies | 1004 | 805 |
| Outer Krylov iterations | 160 | 111 |
| CFL retries | 3 | 4 |
| Backtracks | 0 | 0 |
| Fresh density RMS | 4.05286e-13 | 4.06836e-13 |
| Maximum accepted true defect | 0.0881502 | 0.0923166 |
| Peak rank RSS, KiB | 139972 | 123768 |

The following complete SA1x1 trials use identical decks within each mode,
unchanged targets and a nominal300-second wall limit. Every run is contended,
wall-limited and unqualified. These describe work to failure, not convergence
cost or eligible throughput. The point errors use the same paired native state.

| Mode / policy | Updates | Flow / turb assemblies | Krylov | Retries | Fresh density RMS | Maximum point error |
|---|---:|---:|---:|---:|---:|---:|
| Segregated / existing | 126 | 12693 / 5225 | 5926 | 64 | 3.63471e-13 | 3.70357e-5 |
| Segregated / proposed | 157 | 13265 / 5560 | 6173 | 81 | 3.65610e-13 | 3.73317e-5 |
| Coupled / existing | 72 | 12210 / 14907 | 2605 | 36 | 1.91396e-13 | 3.15691e-5 |
| Coupled / proposed | 209 | 14317 / 15543 | 1071 | 107 | 3.22508e-13 | 6.32547e-5 |

All trials have zero backtracking and satisfy the independent true-defect limit
on accepted steps. More accepted updates and fewer Krylov iterations did not
produce a qualified endpoint. The proposed coupled trajectory recovered to much
smaller CFL. Further investigation must distinguish unavailable matrix-free
products, orthogonalization failure and insufficient linear convergence before
changing the forcing policy. It is not adopted on the evidence above.

## Reproduction and preserved source

Archive: `newton_krylov_refactor_20261015/application_qualification_20261018/`
beside the feature checkout. Exact commands and comparisons are in
`forcing_application_commands.json`, `forcing_original_control_agreement.json`,
`forcing_prefix_comparison.json`, `forcing_full_comparison.json` and
`forcing_candidate_not_adopted.json`.

`forcing_candidate/manifest.json` records source hashes, the precommit source
patch, build command and immutable binaries. Candidate CFD SHA256:
`0ceea9910ae12891555d77b49945c2969463a051a91d179397968d5f0e525c46`.
Retained d27 production CFD SHA256:
`7424b9a9003e0ba0a178c9b757347733a7c812a7254bf54c1cd576f8e2aa54c5`.
The source patch remains reproducible after restoring production files. Selected
qualified quiet repetitions and the remaining application matrix are outstanding.
