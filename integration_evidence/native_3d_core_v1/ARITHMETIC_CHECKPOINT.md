# Native 3D exact-arithmetic cost checkpoint — 2026-10-10

Working repository: `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`.
Branch: `codex/native-3d-core`; predecessor `f5e94f46624c100520dcf0da59561059d1f7457a`.
Goal 1 remains ACTIVE with the unchanged full contract in
`NATIVE_3D_DEVELOPMENT_GOALS.md`. This is an isolated kernel/performance checkpoint,
not an enabled MPI remesher or CFD-affordability result.

## Retained implementation

The exact integer product and signed difference skip only leading/trailing regions
where both the mathematical operation and its carry/borrow are zero. Arrays, bit
representation, signs (including signed zero), overflow rejection and all exact
comparison/ratio/clipping semantics are unchanged. This removes redundant serial
carry/borrow work without replacing exact predicates by approximate decisions.
No public API, tensor construction, target, quality/length gate or 2D source changed.
The production diff is confined to CNativePredicates3D.cpp.

The initial product-only attempt was inconclusive: with detail off, the median
2.582561 s became2.575202 s, a0.3% difference within its spread. That experiment is
preserved in arithmetic_controls_v1, arithmetic_optimized_v1 and
arithmetic_comparison_v1. Extending trimming to differences gave the retained
result below; the product-only measurement is not evidence for a speed-up claim.

A private standalone arithmetic oracle compares every limb and sign against the
original implementations. It covers zero, signed, sparse, dense, extreme-position
operands, full carry/borrow chains and matching overflow rejection. The public API
has no arithmetic testing/debug hooks. The existing strict unit runner also builds
and runs this oracle. The mesh probe now exposes an ON/OFF detailed-field timer
control and reports exact-orientation work counts; its adaptation decisions do not
use that control.

## Correctness evidence

- arithmetic_controls_v2: PASS37Catch cases /2651assertions,290 exact Fraction
  orientation references and6272 bitwise arithmetic controls (3136 products and
  3136 differences). Source snapshots, stage logs and hashes retained.
- arithmetic_optimized_v2: PASS five full refinement/coarsening cycles and the
  independent Fraction geometry, embedding/facet/marker/P1/metric audit. All15SU2
  grids and15mesh JSON exports are byte-identical to both arithmetic_baseline_v1
  and the pre-optimization coupled_meshes_v2 checkpoint.
- arithmetic_edge_oracle_v2: PASS104edges/37source groups:71covered,33rejected,
  147intervals. Thin strips, rotated anisotropy, extreme scales and coincident
  rounded cuts remain represented. Maximum relative length error4.116228e-17,
  absolute weight error4.886722e-20, relative width error7.119334e-20; unchanged
  from the pre-optimization independent oracle.
- arithmetic_comparison_v2: PASS12sequential repetitions, three per binary and
  detail mode. Every repetition matches all15audited mesh JSON hashes and every
  non-timing work/residual field:182accepted edits,8390edge evaluations,1556reuses,
  unchanged exact-orientation counts and mesh/patch sizes. Exclusive case phases
  close; OFF records have zero detailed field times. Binary and producer-source
  hashes are verified before/after; only the predicate source differs between
  the two producer receipts.

Historical v1 source snapshots preserve the product-only implementation and its
original oracle/runner. Do not combine those version-specific pins with current
sources. No source was edited while a validation process was live.

## Short repeated cost comparison

Same five tiny manufactured cases, one low-priority process, library threads1,
g++9.4.0, alternating baseline/candidate order over three repetitions. Process
wall time, user/system CPU, peak RSS, load and process observations are retained.
Host load was about3.9–4.4 on8CPUs; other workers were not stopped. Results below
are summed sequential case-parent seconds, including field setup, selection,
geometry, metric validation, serial commit, final gate and output. Compilation,
external auditing and Python collection are separately recorded, not included.
No MPI/partition/solver-transfer costs exist in this probe.

| Detail | Version | Median s | Min–max s | Useful accepted edits/s at median |
| --- | --- | ---: | --- | ---: |
| OFF | baseline | 2.577178 | 2.559608–2.594829 | 70.62 |
| OFF | candidate | 2.427019 | 2.402638–2.449179 | 74.99 |
| ON | baseline | 2.692444 | 2.603416–2.872252 | 67.60 |
| ON | candidate | 2.441999 | 2.406729–2.807788 | 74.53 |

OFF median parent time decreases5.83% (speed ratio1.06187); all three candidate
OFF observations are below all three baseline OFF observations. Median process
CPU also decreases2.57->2.42s. This supports a modest improvement on this declared
serial fixed-work envelope, not a universal gain. Maximum probe RSS across the
final matrix is4420KiB; small allocator variation is not a memory-improvement
claim and does not cover SU2/MPI/import resident memory.

Detailed-timer ON/OFF median ratios are1.04473baseline and1.00617candidate, but
ON has wider overlapping ranges and a candidate outlier. This is an explicit
instrumentation-overhead control, not a precise universal overhead estimate.
Primary cost claims therefore use OFF. Exclusive selection/validation continue
to dominate; the new arithmetic does not solve repeated whole-mesh selection or
source indexing/import scaling. All per-repetition phase data are in comparison.json;
phase medians must not be summed and called elapsed time.

## Replay and compact publication

Check host contention and announce new absolute case folders before running.
The existing run_geometry.py and run_coupled.py rebuild validated objects and
independently audited producers in NEW folders as described in
BOUNDARY_METRIC_CHECKPOINT.md. The optional third run_coupled argument is ON/OFF;
defaultON preserves the earlier invocation. All runtime scripts work without Git
on compute nodes; geometry provenance falls back to listed per-file snapshots.

For this exact comparison, rebuild the baseline from f5e94f4662 predicate source
with the same current probe and unchanged remaining production sources, preserving
its producer receipt, then rebuild the candidate with current sources. The helper
requires identical probe/auditor sources and exactly one differing production
source; it deliberately refuses an arbitrary baseline executable:

```bash
PYTHONDONTWRITEBYTECODE=1 OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 MKL_NUM_THREADS=1 \
 nice -n 19 python3 integration_evidence/native_3d_core_v1/run_arithmetic_comparison.py \
 integration_evidence/native_3d_core_v1/arithmetic_baseline_NEW \
 integration_evidence/native_3d_core_v1/arithmetic_optimized_NEW \
 integration_evidence/native_3d_core_v1/arithmetic_comparison_NEW
```

Inputs are validated producer folders containing their locally rebuilt probe
executables. Compiled objects/executables are not published. Unique producer grids,
source snapshots, oracles, commands, logs, receipts, resource/phase/work observations
are published; byte-identical mesh JSON copies from the twelve repeated runs are
omitted from Git. Their hashes remain in validation.json/comparison.json and refer
to the published producer frames. Those redundant copies remain local. This keeps
review/replay evidence without replicating all grids for each timing repetition.

## Remaining goal work

Full Goal1 is unchanged: globally complete distributed dependencies, version/
conflict/atomic failure handling, tetrahedral cost-weighted M<=N workers, return
and mesh/state/history reconstruction, runtime/CGNS and MPI1/2/4 correctness and
scaling qualification remain incomplete. Reuse the existing passive scalar record
exchange/worker failure-election model for the next distributed dependency step.
Production candidate selection needs incremental maintenance when that engine is
introduced; the bounded128cell/96sweep toy controller is not a scaling solution.
General planar coarsening/rounded-point association, composed geometric integration
and full 3D BL generation remain as previously documented. Native support still
rejects3D. 2D/MMG/fade/noise paths are unchanged. Existing local papers remain
sufficient; no additional paper requested for this arithmetic step.
