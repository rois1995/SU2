# DEF1 normal lab setup correction

The first bound normal lab launch ended before the Catch test selection. All 13 subprocesses exited 53 at MPI_Win_create with MPI_ERR_WIN and emitted no fixture assertions. The standalone lab wrapper lacked the established Open MPI pt2pt OSC setting. The failure is retained under lab/build681/normal_setup1. The R3 correction adds that setting and the existing compile-tmp path to the receipted environment. The solver and test driver used for the retry are the original build681 binaries.
