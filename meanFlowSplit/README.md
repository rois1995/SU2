# Test cases of the mean-flow PRs (split of su2code/SU2#2941)

Each PR is compared with the build before it:

| Folder | PR | Before | After |
|---|---|---|---|
| `pr1` | under-relaxation factor | develop | PR 1 |
| `pr2` | HLLC on moving grids | PR 1 | PR 2 |
| `pr4` | incompressible moving-grid Jacobian and smaller fixes | PR 3 | PR 4 |

The Roe PR (PR 3) is checked by its unit test `roe_contact_tests.cpp`.

```bash
BEFORE=/path/to/SU2_CFD AFTER=/path/to/SU2_CFD ./run_cases.sh pr1   # or pr2, pr4 (NP=2 MPI ranks by default)
python3 plot_split.py
```

All runs of the PR comments used 2 MPI ranks (the results of some cases depend a little on the number of ranks).
