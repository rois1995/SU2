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
adjoints, and restarts the saved adjoint. It checks unchanged adjoint iterations,
finite nonzero sensitivities for all five targets, interior viscous-edge coverage,
SST source coverage, restart agreement, and an unchanged primal file. It retains
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
The current development sandbox blocks MPI launcher sockets, so two-rank runtime
validation must remain pending until these commands pass in a permitted local environment.

Passing this test verifies infrastructure. It does not establish nonlinear EPM
endpoint accuracy, resolve eigenvector degeneracy, or validate uncertainty bounds.
