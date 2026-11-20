# SU2_CFD memory study scripts

Scripts used for the RAM measurements in the SU2 issue on memory use.

- `memgrid.py`: writes a bump-in-channel `.su2` grid. 2D quad/tri/mixed, 3D hexa/tet/mixed, same points for all types.
- `rsswatch.py`: runs a command and samples the RSS of all `SU2_CFD` processes from `/proc`. Writes the peak summed over ranks.
- `run_set.py`: main study. One dimension and size, all element types, physics, time schemes and rank counts.
- `conv_set.py`: linear solver and preconditioner study (use `--ram_only`).
- `tables.py`: makes the markdown tables from the CSV results.
- `heap_attrib.py`: groups a heaptrack peak flamegraph by the innermost SU2 function.
- `mpi_hello.c`: MPI "hello world" to measure the RAM of the MPI library alone.

Setup: put the scripts in `$STUDY/jobs`, set `STUDY` and `SU2_CFD` (path of the binary), then for example:

    python3 run_set.py --dim 3 --size M --ranks 1:4:16

Results go to `$STUDY/results`, runs to `$STUDY/cases`.
