# Focused assessment-2 correctness checkpoint

Working repository: `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`.
[Summary](summary.json) and [finding reconciliation](../../../Handbook/STATIC_ASSESSMENT_2.md).

All three final stages PASS against identical source, build, binary and thin-library
object pins. Serial:22 cases/186116 assertions, including initialized RANS/SA
BL58 cells → unconstrained6 cells, current+BDF conservative transfer, both mesh
formats and restart. MPI2 and MPI4:7 cases per rank, including worker subsets,
disjoint first-row owners, exhausted proposal budget, reference admission, one-rank
refusal with bitwise retained current/BDF fields, and subsequent successful replacement.
These are correctness checks, not performance or CFD accuracy qualification.

Run sequentially from the working repository with the configured local build:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 integration_evidence/native_3d_bl_v1/static_assessment2_v1/run.py serial
PYTHONDONTWRITEBYTECODE=1 python3 integration_evidence/native_3d_bl_v1/static_assessment2_v1/run.py mpi2
PYTHONDONTWRITEBYTECODE=1 python3 integration_evidence/native_3d_bl_v1/static_assessment2_v1/run.py mpi4
```

The local collector expects `integration_evidence/build-integrated-v2` and the
existing `/tmp/native-build-tools/ninja`; this is not a cluster submission bundle.
Passing stages are reused only when their pins and artifact hashes still match.
Changed/failed stages are archived. Each stage has a300-second execution ceiling;
run one at a time. Sources, commands, binaries/libraries hashes and all input/output
artifacts are retained. No binaries or whole build trees are copied into evidence.

Inspectable accepted smoke grids and states are in `serial/`:
`policy_m1_s0_written_SU2.su2`, `policy_m1_s1_written_SU2.su2` and the corresponding
CGNS/reference/restart artifacts. See actual filenames in that folder. These are
manufactured cube controls, not RAE2822 or physically converged RANS simulations.
Prior failures remain in the `*_previous_*` folders with their receipts.
Private replay is separately recorded in `../connectivity_replay_v1`; its output
still fails the full shape/length gates and is not an accepted adapted mesh.
