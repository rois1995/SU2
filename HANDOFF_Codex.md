# Codex handoff — current state, 2026-10-28

Continue DC-17/DC-18 under [AGENTS.md](AGENTS.md). Keep tracked progress, local
TODO and transfer findings current. Full goal includes requested BL/RANS, movable
boundaries on fixed geometry, genuine features/bodies, absolute tolerance independent
of h0 and all final gates. Numeric baseline settings and agent algorithms are
separate from user requirements; see the linked decision.

No complete curved CFD/M6 adaptation, transfer and restart workflow is accepted.
DC-17/DC-18 remain open. The selected foundation is SCOREC/MeshAdapt classified
cavities and transported nodal size fields inside SU2. Requirements, source-code
comparison, decisions and acceptance: [connected design](Handbook/ADAPTATION_ESTABLISHED_DESIGN.md).

Current checkpoint159a5fe538 is pushed to rois1995/SU2, codex/native-3d-bl (WIP).
Build94 PASS1297.353s; build95 PASS246.340s; unchanged1295 source pins and both
binaries up to date. Audit95 verifies25 selections:309 serial/286 MPI/73 atlas,
121 focused nodal cases. Focused95 executes all121:115 pass/6 fail; new normalization,
reader and reload checks pass. Remaining failures expose stale new-target bootstrap,
missing source-cell ID compatibility, initialized output metadata and redundant
endpoint composition. Original input rollback and behavioral thresholds stay fixed.

M6 build94 rejects371.127s after12 global private edits, before quality/transfer.
Build95 rejects288.467s, same actual blocker: QuerySurfaceUnion requests14,034,944
scratch bytes with7,892,552 left in its internal reservation (721130288 total vs
714987896), despite configured12GB/rank; no work/time exhaustion. Native total
308.10→223.47s, import203.69→173.91s; host contention qualifies timing comparisons.
Sampled whole process tree about3.44GB, not allocation peak. The old metric prism
failure is gone. All M6 ranks and handles are terminal; source pins unchanged.

Mapped94 FAIL159.373s: flat Euler/RANS transfer/reload pass. Curved variants reach
quality:295 splits/238 collapses,609 private edits,180 cells/33 invalid at the same
67207168 geometry work cap. First collapse107576←103688 fresh replay builds valid
cells with35649 work and1.88e-16 surface error; size rejects it. Neither exhaustion
nor conservative rejection establishes geometric impossibility.

**Build96 RUNNING, session91876. Do not edit pinned C++/tests/helper sources.**
It connects fresh authoritative target bootstrap, current-source snapshots between
construction/normalization/adaptation, sensor-only endpoint sampling, exact reused
accepted-face associations and geometry arrays admitted against the unchanged
configured memory pool. Integer workspace/work bounds stay fixed; diagnostic byte
bounds now include geometry peaks. Tests retain geometry/version/facet/input
rollback assertions and verify initialized target attributes. Runtime qualification
pending; no fixture or private progress closes the workflow milestone.

Next: focused checks and accepted-face checks, actual curved workflow and M6 early;
then repeated neighboring edits/MPI, transfer/history and restart. BL regional
projection and FIRST_ROW source handling remain unqualified. The 3D independent
P1 history audit reuses existing readers; self-check only passes, no M6 data or
restart claim yet. TODO stays local. Historical details/receipts:
[runtime archive](Handbook/ADAPTATION_RUNTIME_HISTORY_20261028.md),
[transfer findings](Handbook/TRANSFER_OPERATORS_FINDINGS.md).

## Operations

Check owned handles before new jobs; one heavy job, nice19, build-j1, libraries
single-threaded, at most6 local ranks. Check host load/RAM and save process-tree
receipts; sampled RSS is not allocation peak. Compile temp on4TB. Reuse evidence
only with source/binary/runtime/config/input/helper provenance. Push labelled
checkpoints with explicit paths; TODO stays local. Leave unrelated files alone.
[Archived inputs](Handbook/ADAPTATION_REFERENCE_INPUTS.md).
