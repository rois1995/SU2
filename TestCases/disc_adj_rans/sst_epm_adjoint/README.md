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
adjoints, and restarts the saved adjoint. It also runs `SST_EPM_EXTRACT_ONLY= YES`
from the saved adjoint (one iteration measures the residual, its update is discarded),
which must reproduce the end-of-run sensitivities to `1e-10`, and three option
rejections. It checks unchanged adjoint iterations,
finite nonzero sensitivities for all five targets, interior viscous-edge coverage,
SST source coverage, restart agreement, and unchanged primal and adjoint files. It retains
configs, logs, histories, restarts, CSVs and a `summary.json` with checksums.

For local MPI validation, use an **MPI-enabled** AD executable and matching serial
reference directories from the commands above:

```bash
python3 TestCases/disc_adj_rans/sst_epm_adjoint/check_extraction.py \
  /path/to/MPI/SU2_CFD_AD build-epm-local/check-2d-mpi \
  --ranks 2 --reference build-epm-local/check-2d
python3 TestCases/disc_adj_rans/sst_epm_adjoint/check_extraction.py \
  /path/to/MPI/SU2_CFD_AD build-epm-local/check-3d-mpi --dimension 3 \
  --ranks 2 --reference build-epm-local/check-3d
```

The runner uses `mpirun --host localhost`, shares the serial primal restart, and
compares every sensitivity/probe with the serial result (`rtol=1e-5`, `atol=1e-9`).
Evaluation counts include partition-local work and need not be rank-independent.

On several ranks, a converged discrete adjoint has nonzero values at halo points. The
adjoint restart stores owned points only, so restarting a multi-rank adjoint is lossy
(native SU2: the 2-rank `restarted` run starts near a `1e-7` residual). On several ranks
the extract-only guard therefore measures the total adjoint: the owned value plus all its
halo copies, summed by the reverse halo exchange. It rejects a restart written by a
multi-rank run, and the runner records this as `extract_only_status`. The 2-rank
extract-only checks (`mpi_extract_field`, `mpi_extract_scalar`) load the serial adjoint
and compare with serial to `1e-10`. Their history residuals are those of the owned part
only, so the runner checks the guard line in the log instead. Halo copies of an edge
site agree to round-off, not bitwise: the ranks orient the edge by local numbering. The
reader and the runner use the same test: `1e-10` relative, with a floor of `1e-10` times
the column's max norm.
The current development sandbox blocks MPI launcher sockets, so two-rank runtime
validation must remain pending until these commands pass in a permitted local environment.

Passing this test verifies infrastructure. It does not establish nonlinear EPM
endpoint accuracy, resolve eigenvector degeneracy, or validate uncertainty bounds.
