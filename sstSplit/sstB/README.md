# Test cases of the SST TMR PR

Each folder holds the configuration, the mesh and a plot script. Run every case in a sub-folder named as the run the
plot script reads, with the build of this PR (and, for the "before" runs, the build of the SST bug-fix PR below it).
The configurations already write the output the plot scripts need.

| Folder | Runs (sub-folder: options) | Plot |
|---|---|---|
| `tmr_flatplate` (NASA TMR flat plate, 137x97, TMR free stream) | `sstA_Vm`: before, `V1994m, VORTICITY`; `sstB_Vm`: same with this PR; `sstB_V1994_V`: `V1994, VORTICITY`; `sstB_V1994`: `V1994` | `python3 plot_tmr.py` (TMR data in `tmr/`) |
| `tmr_bump` (NASA TMR bump in channel, 353x161 from su2code/VandV) | `sstA_V1994m`: before, `V1994m`; `sstB_V1994m`; `sstB_V1994`: `V1994` | `python3 plot_tmr.py` |
| `flatplate_1851` (Mach 6.1, 800 K, turbulence intensity 5 %) | `sstB_V2003m`: `V2003m`; `sstB_V2003`: `V2003` (6000 iterations) | `python3 plot_versions.py` |

All runs used 2 MPI ranks.
