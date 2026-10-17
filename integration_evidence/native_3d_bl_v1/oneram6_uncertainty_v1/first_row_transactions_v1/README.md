# First-row transaction batching — validated bounded stage

Base c65bb7b05a5442e485469ededab19ccc2b50bbb5, branch codex/native-3d-bl.
Full Goal3 ACTIVE/incomplete. One owned heavy job/nice19/build-j1/thread1/MPI<=4.

## Capability and safety contract

The first-row initializer previously submitted one original wall cell per worker
and round. Full vertex reservations serialize adjacent cells even though this
operator retains all original outer faces/vertices and writes only cell interiors.
Candidate admissions, complete metadata/fetch, original donor imports, and atomic
publication also repeated per cell. Saved M6 first-row work stopped after447 local
publications/1788 global work, with no accepted mesh. That failure does not prove
that batching solves the constructor's larger geometry/connectivity limitations.

Use the existing candidate_batch<=8 control, LookupBatch/FetchBatch and publication.
A dedicated ReserveInteriors route admits only one complete cell per candidate;
it arbitrates duplicate cell IDs while permitting shared read-only vertices. The
same collective phase must be used by all ranks. General movement/surface edits
retain the existing full-vertex reservation. Every first-row replacement must pass
ConstructFirstLayer's prescribed-interface/height/positivity checks AND SameSkin
before it can enter the common publication. Existing original vertices, complete
oriented outer faces, coordinates and markers remain unchanged inside this
operator. This is an interior transaction contract, not permanently fixed physical
boundaries; later coupled boundary adaptation remains available.

All candidates are validated privately before one owner-routed publication.
One failed candidate vetoes the complete batch. Versions/payloads, geometry,
original sensor-only P1/actual-query BL/finer/fade/noise0, first height and eventual
full q/J/composed-edge/transfer gates are unchanged. Initialization still chooses
among its existing background proposals; it does not certify final metric quality.
Global proposal/work admission never exceeds remaining work. Identity ranges use
(epoch,worker,slot), retain the existing5-node/128-cell reservation, and check overflow.

Known old-cell donor footprints share bounded imports/indexes. Only an import-cap
failure splits a group toward the existing one-cell bound; no larger donor cap,
reset ladder or arbitrary candidate retry. A failed one-cell import still rejects.
All retained candidates/payloads/results enter the expanded bounded scratch model.
Private groups run without per-candidate MPI barriers. Each worker checks its
deadline before/after each candidate, then the group elects failure/timeout before
any publication. Unfinished batches never publish. Nested metadata/donor/geometry/metric/publication timers and batch/import/
retry counters distinguish useful construction from control work. Accounting is
retired before finish returns, including refusal/deadline paths.

## Experiments and provenance

baseline_np4 is the old published binary with the same full frozen M6 field and
ADAP_NATIVE_MAX_WORK64. It reached the180s external timeout under host load~12
without a completed remeshing-stage report. Last stdout followed driver setup;
that alone cannot attribute time to initialization, weights or construction.
This is NOT a mesh rejection, successful baseline or comparative timing. Preserve
its original sources/binary/config/input hashes, signals and terminal receipt.
Do not rerun it unchanged or raise limits to obtain a timing.

The direct native replay bypasses CFD geometry/solver initialization, Hessian,
normalization and partition weighting. It retains ALL303359 original donor
tetrahedra, exact archived owner-UPPER6 node sensors, their native spectral core,
and the complete48954-facet reference/wall metric. It reads original mesh
connectivity and the same upper tensor components as ImportGeometry, including
its positive-orientation normalization. No wall-support reduction or adapted-node
refit is used. Source ownership is shifted independently of current-cell ownership.

working_cells.dat selects128 original wall tetrahedra and two real unchanged cells
that retain the complete mesh's global node/cell ID floors. Only64 initializations
are admitted; unresolved cells force a WorkBudget rejection before final meshing.
The small resulting subset is inspection/diagnostic evidence, not a whole adapted
wing or accepted CFD grid. Batch1 and8 must preserve the same geometric output,
outer skin, exact volume/positivity and marker bindings. Source/core/BL gates remain
fixed. MPI2/4 cover the full field within the existing2GiB native model; serial
controls cover reservation/height/atomicity. Do not raise memory admission merely
to force a serial full-source M6 replay.

No full M6 accepted mesh, volume uncertainty calibration, correction epoch, solution
transfer, converged viscous result, MPI scaling or complete-event speedup follows
from these bounded transactions. Those full-goal requirements remain open.

## Achieved evidence

Current9 serial controls/723 assertions and the selected MPI4 controls PASS:
shared-vertex read-only interfaces, unique/duplicate cell claims, malformed multi-
cell interior claims, asymmetric storage refusal/recovery, whole-batch geometry-
height veto, strict input preservation, work ceilings, existing general vertex
reservations/planning and ordinary batch operators. Primitive first-row comparison
uses16 disconnected boxes and proves batch1/8 exact geometry plus prescribed height.

The direct M6 replay retains all303359 original donors, all61330 original node
sensors and complete wall authority.64 parent cells publish PRIVATELY in each
case; unresolved input cells then cause the required WorkBudget rejection.
Independent rational arithmetic proves positive volumes, exact total volume and
unchanged complete outer skin relative to the original130-cell working subset.
Batch1/8 geometry and physical marker bindings match exactly on MPI2/4. No assertion
claims final full-target shape/edge or solution-transfer acceptance for these subsets.

| Ranks | First-row seconds, batch1 ->8 | Native seconds, batch1 ->8 | Wrapper tracked calls | Directory elections | Metadata batches |
|---|---:|---:|---:|---:|---:|
| 2 | 6.93309 ->6.04936 | 8.11895 ->7.39091 | 1326 ->234 | 935 ->123 | 32 ->4 |
| 4 | 4.66600 ->3.58462 | 5.54613 ->4.45663 | 670 ->124 | 471 ->65 | 16 ->2 |

Times/calls/elections/batches are maxima over ranks. First-row elapsed decreases
12.7%/23.2%, native kernel elapsed9.0%/19.6%, tracked calls82.4%/81.5%. These are
one-shot contended local measurements of a bounded stage, not scaling, statistical
performance qualification or complete CFD-ready adaptation. Native time includes
its own directory/source initialization, but excludes config/reference/field I/O,
CFD initialization, metric normalization, partition weighting/migration, return,
solution/BDF transfer and solver rebuild. Complete lifecycle affordability remains open.

Donor packing is not free:32 ->12 attempts with4 retries at MPI2;16 ->6 with2 retries
at MPI4. Retries only split a shared import at the unchanged donor cap. They do not
retry a failed geometric operator or increase a cap. Geometry and metric work stay
essentially unchanged. Summed metric evaluation is9.62 ->9.56 rank-seconds at MPI2
and9.88 ->9.98 at MPI4; avoid treating rank sums as elapsed time. Local/private-group
scheduling reduces synchronization loss; expensive composed-edge evaluation remains
the main constructor work. A shape-dominance screen before unnecessary length
measurement is a recorded next investigation, not implemented or claimed here.

## What did not work

The first grouped variant still voted after every private candidate. MPI2 wrapper
calls fell1326 ->258 but first-row time6.8685 ->6.8809s; no useful elapsed gain.
Its executed source patch/native hashes, exact output and raw receipts remain under
synchronous_* files/folders. It motivated group-local work with per-candidate LOCAL
deadline checks and a common vote before publication. Do not compare those earlier
absolute times as a controlled repeated timing of the final binary.

The old full-driver baseline timed out, as described above. A separate first direct
MPI2 attempt failed config parsing because the archived config still requested4
ranks. native_np2_b1_rejected_config preserves its exact config/provenance/log.
Only the replay rank setting became0/default; all field/geometry/height/fade controls
remain identical. The passing static controls were source/binary-verified and
reused after that fixture-only correction; final group-local code then reran its
affected controls. No failed native result is promoted to a pass.

## Complete scoped lifecycle and resources

Current affected curved return and initialized RANS/SA N4/M2 construction/removal
PASS the existing independent audit:24->6 accepted cells, exact volume1/h0 error0,
q minima.70846/.68723, J minima.45142/.40825, maximum full-target edge1.71991/
1.73205, transported sensor error0, flow+BDF defects<=2.75e-16, real SA restart,
and SU2+CGNS geometry/target reload. The cube uses its existing regional policy;
this does not turn the M6 first-row subset into an accepted wing mesh.44 composed-
gradation violations remain reported rather than corrected or treated as certified.

One owned heavy job/nice19/thread1/build-j1/local MPI<=4; host39–40GiB available
through the connected validation. Build5m44s plus test-diagnostic refresh34s and
final engine refresh66s; maximum process2.40GiB. Final native/audit campaign65.21s,
maximum process647MiB. These are NOT aggregate MPI memory. All GNUtime receipts
retain their exit codes/swaps; host swap has no established job attribution. All
owned handles are terminal. AdapNoExt, foreign jobs/ClusterResults and dependencies
were untouched; compile scratch stayed on4TB.

## Reproduction and inspectable grids

From repository root after building both binaries:

```sh
OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 MKL_NUM_THREADS=1 nice -n19 python3 integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/first_row_transactions_v1/prepare.py
OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 MKL_NUM_THREADS=1 nice -n19 python3 integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/first_row_transactions_v1/campaign.py
```

The campaign resumes only matching native source/build/binary/config/input evidence.
Changing native pins requires a new sibling case root. summary.json pins the final
acceptance helper. All links close within versioned repository inputs.

Open native_np4_b8/first_row_rejected.vtu (also batch1 and MPI2 variants) and use
NewCell=1/NewPoint=1 to hide unchanged ID-floor anchors. This is a small rejected
working subset, not the complete M6 domain, accepted grid or interpolated solution.
first_row_rejected.dat retains global IDs, coordinates and marker bindings;
comparison*.json/independent.json retain the exact certificates and scope.
