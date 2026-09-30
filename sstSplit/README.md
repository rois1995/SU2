# Test cases of the SST PRs (split of su2code/SU2#2329)

`sstA/flatplate_1851`: the flat plate of issue #1851 (Mach 6.1, 800 K, Re 4.9e6 per m, wall at 300 K, turbulence intensity 5 %, SST V2003m, AUSM, 3000 iterations), before and after the bug-fix PR.

```bash
cd sstA/flatplate_1851
mkdir before after
(cd before && ln -s ../*.su2 . && mpirun -n 2 /path/to/SU2_CFD_before ../case.cfg)   # build of the PR below (#2945)
(cd after  && ln -s ../*.su2 . && mpirun -n 2 /path/to/SU2_CFD_after  ../case.cfg)   # build of the bug-fix PR
cd ../.. && python3 plot_sst.py
```

The inlet mean temperature and Mach number are averaged on the inlet marker (`MARKER_ANALYZE`).
