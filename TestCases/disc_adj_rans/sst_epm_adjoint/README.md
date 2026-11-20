# Tiny SST–EPM infrastructure check

For the paper's physical case matrix and preparation-only configuration generator,
see [paper_cases/README.md](paper_cases/README.md). Those cases are separate from
this tiny infrastructure regression and have not been simulated.

This is a coarse numerical regression, not a physically resolved flat-plate study.
It generates a 117-point 2D rectangle or a 315-point 3D box with spanwise flow.
Each solver invocation is limited to 2,000 iterations and 300 seconds. No mesh
download, cluster access, or production calculation is needed.

From the repository root, with a reverse-AD executable built without OpenMP:

```bash
python3 TestCases/disc_adj_rans/sst_epm_adjoint/check_extraction.py \
  /path/to/SU2_CFD_AD build-epm-local/check-2d
python3 TestCases/disc_adj_rans/sst_epm_adjoint/check_extraction.py \
  /path/to/SU2_CFD_AD build-epm-local/check-3d --dimension 3
```

The output directory must be new. The script converges every flow and SST residual
below `1e-10`, saves the primal restart, runs ordinary and EPM-enabled coupled drag
adjoints, and restarts the saved adjoint. The `enabled` run uses the default
`SST_EPM_EXTRACTION= FIELD`. The `scalar` run uses `SCALAR_REPLAY`, the reference: its
CSV must match the field CSV row by row to `1e-10`. It also runs `SST_EPM_EXTRACT_ONLY= YES`
from the saved adjoint (one iteration measures the residual, its update is discarded),
which must reproduce the end-of-run sensitivities and field files to `1e-10`, and three option
rejections. It checks unchanged adjoint iterations,
finite nonzero sensitivities for all five targets, interior viscous-edge coverage,
SST source coverage, restart agreement, and unchanged primal and adjoint files. The final
adjoint restarts of the `disabled`, `enabled` and `scalar` runs must be bitwise equal (same
sha256): the runs differ only in the EPM options, and the restart is written after the
extraction. For every field run it runs `read_epm_field.py <json> --csv <csv>`, which
recomputes the five canonical gradients from the field files alone, checks the `.bin` sizes
against the JSON, and checks that the site uses add up to the CSV evaluation counts. It also
requires `byte_order` = `little` in the JSON, `uses` of 0 or 1 only (one evaluation per site
and recording), and the normal convention on every evaluated copy of every rank: an `EDGE`
normal points from `key_a` to `key_b` (positive along `coord_b - coord_a` on these
median-dual box meshes), a `BOUNDARY` normal points outward. It retains
configs, logs, histories, restarts, CSVs, field files and a `summary.json` with checksums.

For MPI validation, use an **MPI-enabled** AD executable and matching serial
reference directories from the commands above:

```bash
python3 TestCases/disc_adj_rans/sst_epm_adjoint/check_extraction.py \
  /path/to/MPI/SU2_CFD_AD build-epm-local/check-2d-mpi \
  --ranks 2 --reference build-epm-local/check-2d
python3 TestCases/disc_adj_rans/sst_epm_adjoint/check_extraction.py \
  /path/to/MPI/SU2_CFD_AD build-epm-local/check-3d-mpi --dimension 3 \
  --ranks 2 --reference build-epm-local/check-3d
```

The runner uses `mpirun --bind-to none -np 2`. It adds `--host localhost` only when
`PE_HOSTFILE` is not set (outside an SGE parallel environment). It shares the serial
primal restart and compares every sensitivity/probe with the serial result (`rtol=1e-5`,
`atol=1e-9`; the tighter checks are below). Evaluation counts include partition-local
work and need not be rank-independent. The merged field sites must equal serial.

On several ranks, a converged discrete adjoint has nonzero values at halo points. The
adjoint restart stores owned points only, so restarting a multi-rank adjoint is lossy
(native SU2: the 2-rank `restarted` run starts near a `1e-7` residual). On several ranks
the extract-only guard therefore measures the total adjoint: the owned value plus all its
halo copies, summed by the reverse halo exchange. It rejects a restart written by a
multi-rank run, and the runner records this as `extract_only_status`. Re-extraction from
a multi-rank adjoint is a short re-converge (`restarted`), whose fields must match the
end-of-run fields at the convergence level (below).

The adjoint is the fixed point of the recorded iteration. That iteration includes the
transposed linear solve of the implicit update (FGMRES with rank-local ILU). These solves
stop at `LINEAR_SOLVER_ERROR` (`1e-10` relative residual; the cluster runs reach it in
9-14 iterations, well below the `LINEAR_SOLVER_ITER` cap), not at the exact solution, and
the rank-local ILU makes the applied operator depend on the partition. So the adjoint
fixed point depends on the partition. dJ/da and the per-site `S_*` depend only on the
adjoint of the residual, and only when the loaded adjoint is the fixed point of the same
map that is replayed. On 2 ranks the runner therefore checks:

- field against scalar replay, and the reader against the CSV, to `1e-10`;
- dJ/da and the merged `S_*` against serial at the convergence level;
- the restarted extraction against the end of the run at the convergence level;
- the merged unit normals against serial to round-off (`1e-10`): the stored orientation does
  not depend on the partition.

Convergence level. Each run stops at an iterate, not at the fixed point. For an iteration
that contracts at rate `rho`, the error left is about `r_n rho/(1-rho)` (`r_n` the last
update), and an adjoint started from zero has size about `r_0/(1-rho)` (`r_0` the first
update). Its relative error is therefore about `rho r_n/r_0`, below `r_n/r_0`. The runner
takes `r_n/r_0` per adjoint component from the histories (final rms minus the first rms of
the run started from zero, in decades) and uses the worst component. dJ/da and `S_*` are
linear in the adjoint, so the tolerance between two runs is the sum of their two levels.
It is measured, not fixed, because `CONV_RESIDUAL_MINVAL= -10` bounds the absolute rms
only: the omega adjoint starts near `2e-11` in 3D, and the 3D 2-rank restart stops only
3.7 decades below that (level `1.8e-4`) while `rms[A_Rho]` is 6 decades below. On cluster
job 664789 the observed errors were 0.002 to 0.09 of this tolerance (2D: restarted
`8.4e-8` of `2.6e-6`, vs serial `2.1e-7` of `2.3e-6`; 3D: restarted `3.1e-6` of `1.9e-4`,
vs serial `2.5e-8` of `1.6e-5`). Real bugs are O(1) (a halo double count is a factor 2;
a missing site fails the site-set check). A tolerance above `1e-3` fails the run as not
converged enough to compare. `summary.json` records the tolerances
(`convergence_tolerances`).

Extract-only from the serial adjoint (`mpi_extract_field`, then `mpi_extract_scalar`) is
left to the guard. If the guard accepts, the runner enforces `1e-10` against serial. If it
rejects, the runner records `mpi_extract_from_serial_status` and the guard values. A guard
rejection (here or in `extract_only`) is accepted only if every guard residual is at most
`1e-5` (largest observed: `10^-5.46` for the lossy restart, `10^-6.09` for the serial
adjoint on 2 ranks). A zero or wrongly loaded
adjoint leaves the residual of the first iteration (`rms[A_Rho]` near `1e-3.5`) and fails.
The bound cannot see the omega adjoint, whose residual is always below `1e-9`. The
adjoint history now includes `LINSOL`, and `summary.json` reports the largest linear-solver
iteration count and residual. Halo copies of an edge site agree to round-off, not bitwise:
the ranks orient the edge by local numbering. The stored `EDGE` normal is oriented by the
key (from `key_a` to `key_b`), so the copies' normals agree too; the reader checks the
`dtau_*` and `normal_*` columns with the same test as the runner: `1e-10` relative, with a
floor of `1e-10` times the column's max norm. (Field files up to `bc43d43` stored the local
orientation: the 2-rank files of job 664770 have opposite normals on 27 of 28 (2D) and 169
of 174 (3D) shared edges, and the reader now rejects them.)

Cluster result. On CFDHub, job 664789 (binaries built from `d7fbb0c`, scripts at
`d1fc118`) passes the serial and the 2-rank runs in 2D and 3D. On 2 ranks, the
`extract_only` run from the 2-rank restart and the `mpi_extract_field` run from the serial
adjoint were both rejected by the guard, as expected above. So extract-only on 2 ranks has
not been positively verified. The recorded values are in
[smoke_results.json](smoke_results.json).

Passing this test verifies infrastructure. It does not establish nonlinear EPM
endpoint accuracy, resolve eigenvector degeneracy, or validate uncertainty bounds.
