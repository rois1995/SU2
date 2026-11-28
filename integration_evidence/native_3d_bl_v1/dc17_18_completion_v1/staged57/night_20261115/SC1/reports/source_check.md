# SC1 whole-item source check

Base `862717ac8f` contains the accepted DEF1 report and legacy configuration rule.
The signed SC1 order was verified before and after copying. The DEF1 supervisors
and postprocessors were terminal. No SC1 production edits preceded this check.

## Certificate key and validity

- `Common/src/adaptation/CNativeBoundaryParameters3D.cpp:497-539` takes the raw
  requested facet, actual triangle, binding triplet, reservation, purpose and
  affine lower bound. The raw facet selects direct supports and the initial
  nearby search. `Entity(facet)` at lines 430–432 gives body, sheet or canonical
  facet, and marker. `Common/src/adaptation/CNativeAffineReference.cpp:2066`
  canonicalizes to the input facet outside component mode. Keep the actual
  facet used for the bound as well as the canonical anchor and entity.
- The immutable authority supplies support membership, realization, original
  facet vertices and orientation, affine authorities, error declarations,
  cubic nets, component mode and mapping version. The exact per-worker
  declaration is established by `Directory::AgreeParameters`
  (`Common/src/adaptation/CNativeDistributed3D.cpp:633,1463-1477`).
  `WriteSnapshot` at `CNativeBoundaryParameters3D.cpp:165-176` encodes those
  declarations, the exact mobility policy and entity tolerance. `Authority()`
  (`Common/include/adaptation/CNativeBoundaryParameters3D.hpp:130`) is the
  stable owning handle within the process. The wrapper state and current
  surface revision can change without changing the immutable object.
- Policy fields are version, bitwise angle, roles, protected edge chains,
  corners, bodies and sheets. `WithMobilityPolicy` at
  `CNativeBoundaryParameters3D.cpp:712-727` shows the exact comparison.
  The 64-bit identity alone cannot prove equality.
- `ParameterError` at `CNativeBoundaryParameters3D.cpp:1786-1792` resolves
  the three bindings via `Parameter(point)` at lines 1293–1299. The resolution
  order is private local, pending, then accepted. Pack each actual coordinate
  and its resolved binding as numeric bit patterns. Include feature, version,
  dimension, u and v (`Common/include/adaptation/CNativeAffineReference.hpp:83-88`).
  Sort vertex records by their coordinate bits. Neither `PointKey` nor
  `BindingKey` at `CNativeBoundaryParameters3D.cpp:30-34` is this certificate key.
- `EntityFaceError` reads the entity tolerance at lines 503–532. The census
  compares against its own tolerance at `EngineSelectionCensus.cpp:536-543`.
  A valid writer bound must be finite, nonnegative and at most its entity
  tolerance. An eligible read also needs the bound within the census tolerance.
  The affine maximum is calculated from the current raw facet and current
  points. `SurfaceDeviation` at lines 1584–1590 performs containment first.
  Preserve both deadline checks (lines 1787 and 500) on an actual shortcut.
  The query purpose and reservation affect the effort and numerical bound,
  while a completed eligible bound certifies the same directed distance.
  Leave the original legacy query vertex order and purpose in mode 6 and on
  fallback.

## Writers and readers

- Step C calls `WithinTolerance` for every child in
  `native_local/CNativeLocalCore3DCollapse.cpp:487-494`. The implementation at
  `native_local/CNativeLocalSurfaceGeometry3D.cpp:758-776` uses canonical
  facet and ordered corners. Only the successful `Commit` at collapse line
  511 can promote the direction's records. Contact verification, overlay
  exchange and state installation occur in `NativeRemeshRun.inc:1097-1163`.
  Retain the records for census use only after the actual close flag at
  lines 712–718. The existing report sink at collapse lines 517–525 is
  conditional, rotates the triangle, and cannot be the record writer.
- The final proof builds its owned-face list in
  `Common/src/adaptation/CNativeEngine3D.cpp:338-361`. It bounds the accepted
  ordered faces at `CNativeBoundaryParameters3D.cpp:541-566`. Preserve the
  entity short-circuit and component bypass. Write only from the actual
  per-face result of line 564. The per-entity maximum and the model-to-mesh
  proof stay unchanged.
- Engine commit candidates with existing whole-face geometry bounds include
  fresh guidance in `EngineTrial.cpp:291-325`, fresh geometry counts at
  524–548, 1493 and 1540, and optimizer fresh evaluations at
  `MeshOptimizer.cpp:1452-1455,1529-1535,2530,2544,2550-2555,2906,2921`.
  Whole-face guidance obtains the bound at
  `Common/src/adaptation/CNativeBoundary3D.cpp:3505-3518`. Candidate
  evaluations and the private optimizer alone do not publish mesh faces.
  Associate only a valid existing bound of the exact fresh face with a
  successful `Directory::Publish` at `EngineCommit.cpp:530,1157`, before
  the census update. Retained memo values, partial guidance without an
  admitted bound, old faces and filtering-only queries do not supply fresh
  records. The speculative rollback at `EngineControl.cpp:773-839` applies
  to standalone coarsening, which the bidirectional eligible engine
  invocation does not enter (`EngineAdapt.cpp:43-45`,
  `EngineControl.cpp:1152,1175-1194`).
- `EngineSelectionCensus.cpp:323-340,507-543` implements the one face
  reader. All paths reach it through `EngineCensus.cpp:248-253`. Construction
  seed (lines 412–453), repair batch and slot updates
  (`EngineCommit.cpp:671,1379`), and absent rows on construction re-entry
  (`EngineControl.cpp:585-619,683`) are in scope. The seed function can run
  in improvement as well. Gate the reader on the actual Construction phase
  so the improvement-entry and improvement update paths stay legacy.
  Count-mode hits include qualified records whose affine guard refuses the
  shortcut. Only eligible would-skip hits contribute elapsed query seconds.

## Ownership, publication and accounting

- `CSinglezoneDriver.cpp:529-539,614-638` constructs a single remesher before
  the loop and completes replacement before the next event. It calls
  `onAccepted` at line 2041 after mesh/solver installation. The reference
  callback at `NativeRemeshRun.inc:1581-1591` is the publication point.
  The certificate table is separate from reference I/O. Consume the old
  snapshot into event-private state and install the prepared snapshot only
  on acceptance, leaving the remesher empty after rejection.
- `NativeRemeshRun.inc:567-597,600-671` selects at most two stages but frees
  each worker communicator at its join. Retain only the needed communicator
  until selection on eligible SC1 stages, release discarded stages, and
  gather only the selected proof records before the pack at lines 1552–1558.
  Nonworkers get no snapshot. Preserve legacy split/free cadence for OFF
  paths. `CNativeImport3D.cpp:2032-2034,2052-2076` and
  `CNativeBoundaryParameters3D.cpp:849-866` preserve the authority through
  worker return and rebind. Current vertex numbering and revision do not
  identify a certificate.
- Existing admission points include `NativeRemeshRun.inc:108-126,188-190,
  688-690,981-1032,1539-1557`, `EngineExchange.cpp:91-143`, and the owned
  proof reservation at `CNativeEngine3D.cpp:329-356`. Charge old, stage and
  proposed tables and temporary allocations to the corresponding live
  working budgets. The generic `CPassiveComm::AllgathervRounds`
  (`Common/src/parallelization/CPassiveComm.cpp:226-269`) allocates between
  collectives. The new S-only gather needs admitted and allocated storage
  plus elected local preparation before entering payload MPI.
- Use the existing Census, Certification, Topology, Transfer/Import and
  Output operations. `CNativeTimerTree3D.hpp:175-183,220-242,278-285,328-371`
  supplies the existing clock boundaries and ledger definition. Store
  timestamps and per-phase top-three spans separately. Reuse the same
  root and scope samples. Keep the ledger rows and `fixedstorage=4120`.
  Account for scope wrappers at `CNativeEngine3D.cpp:261-272` and the
  lane timer at `EngineAdapt.cpp:379-382`. The timer root is created in
  `CNativeRemesher3D.cpp:211-215` with initial phase Preparation.

## Contradictions and readings used

1. **Raw anchor.** The actual EntityFaceError facet argument controls support
   selection even when the canonical entity matches. Include it as an
   additional exact input. Compare both effective tolerance uses rather
   than assuming their numerical equivalence.
2. **Quality entry.** A seed call can also occur in Improvement. Gate by
   engine phase at the shared measurement call. Construction re-entry
   may add missing rows and remains a construction update.
3. **Timer gap label.** The order describes an unattributed frame as
   phase-only. The existing ledger also includes the default legacy
   `Timer` operation in the timed lane. Use the operation and ownership
   of the innermost frame so the submitted spans reconcile with coverage.
4. **Stage communicator.** The selected communicator is freed before
   normalization selection in the base. Keep an eligible handle alive
   across selection rather than gathering prematurely or gathering the
   discarded stage. Explicitly agree allocations and mode before payload MPI.
5. **Idle ranks.** A parent nonworker has zero reader invocations and
   zero stage records. Emit its zero-count row when the event had an
   eligible selected worker stage and timings are on.
