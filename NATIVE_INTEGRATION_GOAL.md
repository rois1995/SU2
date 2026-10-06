# Native integration, robustness and practical scaling goal

Activated 2026-10-06 by explicit user request, without a token budget.
Campaign completed against the unchanged deliverables below. Requirement proof:
[INTEGRATION_COMPLETION_AUDIT.md](INTEGRATION_COMPLETION_AUDIT.md).

Work only on `codex/native-integrated`, in this sibling worktree. Leave
`feat_adap_noExt`, its checkout, its existing .gitignore change and the other
workers' worktrees untouched. Pin branch and draft inputs before importing.

Deliverables:

1. Branch/worktree reconciliation ledger with ancestry, source pins and
   decisions; meaningful production work combined without silently enabling
   unsupported combinations; preserve standalone research for comparison.
2. Fresh integrated primal/MPI/MMG/CGNS build and focused native, output,
   transfer, sensor and two-pass regression checks at 1/2/4 ranks. Validate
   Stage G in an AD build separately, including fresh-recording counters,
   transfer/tape lifetime and interruption; report remaining gaps explicitly.
3. Reproducible robustness envelope: repeat refinement/coarsening, thin and
   opposing BLs, boundary adaptation, sharp geometry, anisotropy, target
   discontinuities, partition changes and explicit admission failures.
4. Controlled size/rank scaling measurements: remesh and transfer wall time,
   per-rank peak RSS, work/transaction counts, ownership imbalance and
   communication limitations. Separate adaptation from CFD, output and audits.
   Determine the first demonstrated failure or cost limit; do not infer broad
   scalability from tiny tests or rank-dependent mesh counts.
5. Updated HANDOFF_Codex.md, exact provenance/evidence and a reviewable branch.

Resource policy: one heavy job at a time, initially at most four MPI ranks,
-j2 builds, OMP_NUM_THREADS=1, OPENBLAS_NUM_THREADS=1. Defer to foreign jobs.
All builds and new artifacts live here, not in AdapNoExt or the low-space /tmp.

The previous completed native goal (57f0550db4) is prior evidence only.
Fresh integration results must be recorded against the new source revision.
Native remains experimental static single-zone primal-double 2D triangles;
3D, CAD, mixed volumes, moving meshes, time-domain and adjoint native paths
remain unsupported. MMG remains the default.
