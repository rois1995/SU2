# Periodic review cleanup — 2026-10-07

All 12 assessed CodeQL comments received commit-linked replies and were resolved. The seven active periodic PRs have zero unresolved threads at the recorded verification time. #2967 remains closed.

| PR | Cleanup commit | Addressed threads |
|---|---|---:|
| [#2963](https://github.com/su2code/SU2/pull/2963) | [6481f6949d](https://github.com/rois1995/SU2/commit/6481f6949defeceae9026af0eb38787f5f3e0f76) | 2 |
| [#2964](https://github.com/su2code/SU2/pull/2964) | [afb78422e8](https://github.com/rois1995/SU2/commit/afb78422e81e7492141bf5d0996aa5301ba46142) | 9 |
| [#2965](https://github.com/su2code/SU2/pull/2965) | [95c84b7adb](https://github.com/rois1995/SU2/commit/95c84b7adbcb3b2cb134bb19fcb85c7078d5f365) | 1 |

The changes rename local loop indices and promote nDim before multiplication. Scoped token/diff checks establish unchanged expressions and values for supported dimensions. Repository pre-commit ran, but its configuration excludes SU2_CFD, so those hooks skipped these files. No new CFD runs or reference changes were made.

The original branch-heads.json retains the heads used for numerical validation. The combined checkpoint includes the closed implicit experiment and is unchanged.

The CI snapshot records new B/C/D runs pending. G/H builds failed during GitLab/Eigen checkout; H sanitizer regression stopped after an early-EOF checkout and missing Tutorials/E387 data. These setup failures do not establish a source-code defect or passing CI. No retry or workflow change was made.

Exact replies, verification timestamps, CI states and failure excerpts are in [review-cleanup-20261007.json](review-cleanup-20261007.json).
