# Automatic bounded tetrahedral repair checkpoint

2026-10-11. Working repository:
`/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`.
Branch `codex/native-3d-core`, predecessor
`47071504dd6212828176a8e3f6627aa1d5e87a06`.
Goal 1 remains active with its full scope. Production 3D remains guarded.

## Behavior and scope

`Repair` now selects failing cells' vertex, edge and face features automatically.
It shares the existing incremental engine with `Refine` and `Coarsen`: complete
owner/version dependencies, vertex-conflict reservations, immutable original-P1
sensor imports, metric validation, owner routing, atomic publication, selection
updates and strict whole-event completion. The accepted input remains unchanged
on success and rejection. Different workers may select different trajectories.

Vertex proposals move toward the neighbor barycenter, using the physical surface
ring for wall points and the complete star for interior points. Three displacement
fractions are tested. Face/edge proposals reconnect the admitted cavity to up to
16 existing vertices. Each proposal uses the immutable finite-facet geometry
operator and the actual frozen target; new cells must pass q>=0.20 and J>=0.05,
and reduce changed-edge excess or improve quality without increasing excess.
Final completion also requires every edge length<=1.8. There is no target
relaxation, original-donor resampling or coarse spreading of wall tensors.

Failing-feature queues update only after published edits. Blocked features are
retried when adjacent topology changes. Repair-only vector/queue storage is
separate from ordinary cell selection state; memory accounting uses an
incremental counter, avoiding a new whole-cell scan in each admission check.
Reconstruction and metric timing remain exclusive even when proposals alternate
between geometry and metric checks. The extra history counters report moves,
reconnections and attempted private proposals; no split/collapse history is
invented for a repair.

This is an automatic **repair event**, not yet a fallback inside a rejected
refinement/coarsening operation. Placement is deliberately bounded and does not
establish a robust solution for arbitrary cavities or highly variable metrics.
Original finite coplanar triangle seams remain constraints; coplanar geometry
patch authority is still needed before claiming general original-wall freedom.
The production native remesher still rejects non-2D input.

## Validation

Final `automatic_repairs_v2/validation.json` and
`automatic_repairs_v2/actual_nonmpi/validation.json` pass with stable source pins.
MPI1:64 cases/3969 assertions; MPI2:20 cases/rank,1372/1813 assertions;
MPI4:20 cases/rank,2521/1717/2119/2572 assertions. The actual non-MPI control
checks automatic reconnection, existing private movement/reconnection,
6->48->6->48 refinement/coarsening, strict final metrics and rejection.

The automatic matrix covers four actual failure mechanisms on ordinary, thin
(h=1e-8) and sheared (h=1e-4) faceted domains, with all N=M and admitted M<N
subsets. There are 72 accepted repair events (36 moves,36 reconnections),
108 independently audited meshes including36 initial meshes, and204 closed
parent profiles. Round/storage controls preserve accepted input bytes. Nine
independent missing/tampered-data controls reject across the three MPI audits.

| Case, ordinary domain | Initial q / J / maximum length | Final q / J / maximum length |
|---|---|---|
| Three-cell face reconnection | 0.50724 / 0.30619 / 2.04939 | 0.98118 / 0.79550 / 1.19719 |
| Four-cell sliver reconnection | 0.15696 / 0.04816 / 1.50000 | 0.38668 / 0.09547 / 0.77621 |
| Wall vertex movement | 0.12619 / 0.03122 / 1.44900 | 0.43611 / 0.21320 / 1.44900 |
| Interior vertex movement | 0.12747 / 0.03222 / 1.44900 | 0.37209 / 0.21320 / 1.44900 |

The independent auditor checks exact positive volumes, no positive-volume
intersection, conformity and sphere/disk links, convex domain containment,
oriented finite-facet coverage and marker preservation, and original-P1 target
shape/integrated-edge values. Deliberately bad initial shapes are measured and
geometrically audited; final gates are never relaxed. Existing auditors retain
shape enforcement by default. Reported native final q/J/length values match the
independent calculation. These are small manufactured domains, not CFD results.

All702 earlier JSON/SU2 grids from the prior movement/reconnection checkpoint
are byte-identical, covering incremental refinement, anisotropic refinement,
repeated general coarsening and prescribed repairs. Their new audit reports and
raw logs are retained; duplicate grids are omitted from Git. The earlier actual
transported original-P1 query controls also rerun through the shared field core.
There is no new per-candidate donor observer in the automatic engine: do not
present this checkpoint as a separate private-query residual measurement.

V1 native MPI1 passed. Its runner had an accidental extra audit-script argument;
the prescribed parser also initially included new automatic facet records.
Both were corrected, and saved-log re-audits pass. The final V2 rebuild includes
incremental repair storage accounting and all corrected parsers. Retain the
failed receipt, source snapshots, raw logs and successful re-audits.

## Performance evidence and limits

The final matrix observed at most19 rounds and2,134,136 requested work bytes
under the engine's conservative storage model. It attempted2433 private repair
proposals across the matrix. Tiny selected-worker event times were about5–29ms.
Geometric proposals rejected before progress dominate several cases; ranking and
screening those proposals is a useful next measured target. These values are
an envelope for the fixtures only, not general size/rank limits or RSS.

Actual non-MPI alternating instrumentation OFF/ON medians, three pairs each:

| Event | OFF | ON |
|---|---:|---:|
| Refine | 0.051964s | 0.052384s |
| Coarsen | 0.209952s | 0.211742s |
| Automatic reconnection repair | 0.014612s | 0.014918s |

Each comparison asserts identical grids, accepted work and query work.
`instrumentation_summary.json` retains each sample/range. Three workstation
pairs do not resolve general overhead, speedup, scaling or CFD affordability.

Engine phases include initialization, seed/index/import/measurement, selection,
dependency/reservation/donor exchange, reconstruction, metric validation,
routing/publication, incremental updates, completion and remaining time.
Nested query/cache/search/interpolation/composition/trace/integral subcosts,
private CPU, accepted/rejected/conflict work and bounded storage are exported.
Parent profiles close migration, worker-communicator creation, worker event,
return, independent test audit and record export. Manufactured metric fixture
preparation happens once outside repeated subset-event accounting; there is no
real ParMETIS, CFD rebuild/solution/history transfer, complete RSS or collective
wait qualification here. Those remain explicit Goal1 deliverables.

## Inspect and replay

New inspectable grids and original fields:
`automatic_repairs_v2/mpi{1,2,4}_automatic_repairs/auto_*_m*.su2` and `.json`.
Initial bad grids are named `auto_*_initial.su2`; their JSON contains the source
sensor field. Raw logs, source/binary SHA pins, reports and failed receipts remain.

```sh
python3 integration_evidence/native_3d_core_v1/run_distributed.py /absolute/new/case/folder
python3 integration_evidence/native_3d_core_v1/run_serial_engine.py /absolute/new/case/folder/actual_nonmpi
```

Use a new output folder; the runners announce it and reject overwriting evidence.
Run sequentially at low priority after an actual host-contention check.
No cluster launch is requested overnight.

## Next work

Integrate repair with blocked refinement/coarsening, exercise multiple/partial
cavities and larger source imports, and improve measured proposal/tracing costs.
Add coplanar geometry-patch authority with real feature/marker preservation.
Then complete weighted tetrahedral partitioning, runtime mesh/solution/history
return to N CFD ranks, SU2/CGNS, full lifecycle profiling and cluster qualification.
No full Goal1, curved-CAD, robust-BL or unsteady-affordability claim is made.
