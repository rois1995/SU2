# Worker-communicator spatial routing checkpoint

2026-10-10. Repository: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch: codex/native-3d-core; predecessor f13a08be3eefa2db8e547c10c52569d552f92ad2.
Goal 1 remains active. This checkpoint supplies one prerequisite for distributed
immutable original-sensor queries; it does not implement those queries yet.

## Change

CPassiveComm::AllgathervRounds and its typed wrapper accept an optional
communicator. Counts, rank, size and every native MPI collective use that same
communicator. CRankBoxTree::Build forwards it. Existing direct callers omit the
argument and retain SU2_MPI::GetComm(). Caller searches found no function-pointer
uses requiring migration; no config, export or meson registration changes are
needed. The new test uses the already registered CNativeDistributed3D test file.

A worker-only spatial index can now gather owner boxes without involving idle
CFD ranks. This reuses the house bounded-round transport and box hierarchy;
it adds no duplicate transport or index. Round chunking and box search algorithms
are unchanged. Empty send buffers avoid forming pointers from a null data()
value. Nonempty buffers use range assignment and keep the previous single-copy
cost. The first passing implementation zero-initialized then copied its buffer;
the final version removes that extra write. There is no timing claim for this.

## Validation

Actual final case folder:
/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_3d_core_v1/worker_field_controls_v2.

The existing run_distributed.py rebuilds actual HAVE_MPI house transport,
rank-box, ADT and native 3D objects with strict passive arithmetic flags.
Sequential low-priority MPI1/2/4 controls pass, with one numerical-library thread:

- MPI1: 46 Catch cases / 2752 assertions, including the 37 existing 3D cases.
- MPI2: nine cases per rank, 91 and 104 assertions.
- MPI4: nine cases per rank, 90, 91, 92 and 104 assertions.
- Worker sizes: N, max(1,N-1), max(1,N/2). Inactive ranks do not enter worker
  gathers. Parent-communicator reductions check the result after workers return.
- Variable typed counts include empty rank-zero sends and the all-empty M=1
  case. Box controls cover dimensions 2/3, an empty worker partition, touching
  corners, communicator-local owner ranks, and 127-byte bounded rounds.
- Default Build without an explicit communicator still sees all N CFD ranks.

All 15 mesh-record JSON and 15 inspectable SU2 files are byte-identical to
published distributed_controls_v3. Independent embedding/facet/marker/P1 audit
results are identical; only their authenticated input-log hashes change. The
refined mesh remains oversized private progress, as documented previously.

The final serial_transport child folder compiles the actual shared units
without HAVE_MPI, links a small assert control and passes empty/default/explicit
passive gathers and empty/populated 2D/3D rank boxes. Its receipt records actual
commands, source hashes before/after, log and binary hashes. No source changed
during either validation. Host snapshots document contention; these small
correctness runs are not performance or scaling measurements.

The earlier worker_field_controls_v1 passing receipt/logs/source snapshots and
serial control are retained. Its buffer-copy version differs from the final
source; its receipt is historical evidence, not a substitute for v2 validation.
Duplicate earlier manufactured meshes can be regenerated with the pinned runner;
the final v2 meshes are retained here.

## Replay and remaining scope

From the repository root, choose a new empty folder and run:

```
python3 integration_evidence/native_3d_core_v1/run_distributed.py /absolute/new/case/folder
```

The retained serial_transport.cpp and receipt supply the serial commands.
The shared default path has focused MPI and non-MPI coverage. A full SU2/AD build
and complete 2D CFD regression have not been run; changing these signatures
requires recompiling consumers, with no binary ABI compatibility promised.
3D runtime adaptation remains rejected. Distributed frozen donor ownership,
actual remote P1 queries/composition, a complete adaptation engine, ParMETIS,
CFD solution/history transfer, CGNS and the full performance envelope remain
Goal 1 work. The 2D/MMG operators and sensor/BL/fade/noise policy are unchanged.
