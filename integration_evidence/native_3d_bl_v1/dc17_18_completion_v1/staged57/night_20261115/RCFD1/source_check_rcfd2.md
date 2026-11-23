# RCFD2 source check

The pinned order is `inputs/REAL_CFD_2.md`, SHA256 `1aeb15e9490031caa4d8438fcc1630c2835b20b60bb0609b51ef9d38aed71a88`. R-S has the original verified configuration digest `d5d38759da215f8abb8b6c292519725d8ec6da16212553033759ceed7b516fc1`. The R-SM config changes one complete HMIN line and adds the scalar norm. R-SG changes one complete HGRAD line of R-SM. The copied runner verifies these exact bytes and all sidecars.

`Common/src/CConfig.cpp` registers ADAP_NORM, ADAP_HMIN and ADAP_HGRAD as scalars and checks norm ≥1, positive minimum size below maximum, and finite gradation >1. The steady loop expands scalar values in the absence of the corresponding level lists. The unchanged ADAP_SIZES sets the level complexity to 50,000. `SU2_CFD/src/drivers/CSinglezoneDriver.cpp` prints effective metric settings before cycles that will remesh, and `SU2_CFD/src/solvers/CSolver.cpp` uses the effective norm and gradation and supplies the requested metric diagnostics.

The copied RCFD1 runner binds SM and SG as surface-on workflows. The one-sided refinement guard remains report-only for these modes through the inherited S binding. The same full controller and restart helpers check the standing gates and record the six payload digests. Event-1 differences among the metric variants are report findings. The source paths pinned by RCFD1 remain intact.

The scoped C1 supervisor that was active at the new order completed its remaining checks before RCFD2. Its evidence was committed and pushed as `142f3e8751`. The C1 queue resumes at §1.2(b) after the two real-CFD runs.
