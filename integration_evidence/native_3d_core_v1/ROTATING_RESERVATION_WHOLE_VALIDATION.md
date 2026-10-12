# Isolated rotating reservation: stronger whole-case validation

2026-10-12. Repository `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`;
branch `codex/native-3d-transport-accounting`; starting tip
`64fa4386409a6e8bef66697cfb3539ff19043b85`. Goal 1 remains active.

**All three predeclared whole cases and six existing actual lifecycle controls
pass. The rotating scheduler remains isolated from production.** This extends
its earlier complete 97-case MPI1/2/4 and targeted protocol qualifications;
it does not establish repeated performance or close Goal 1.

## Reproduction and provenance

`ROTATING_RESERVATION_WHOLE_MATRIX.json` pins the three whole-case recipes before
numerical launch: turning aspect-ratio-2 size4 wedge, covariant H20 thin size4
wedge, and actual CGNS persisted restart on a size2 wedge, all N4/M2 with the
unchanged 512-round production budget. The separate lifecycle `matrix.json`
pins six actual metric/Hessian/CFD/BDF controls at MPI1 (three fresh fixtures
per detail OFF/ON setting). The frozen acceptance gates remain q>=0.20,
J>=0.05 and full original-P1 edge length L<=1.8; ADAP_HESSIAN_NOISE=0.

`rotating_reservation_whole_build_v1/run.py` compiles only the exact previously
audited isolated directory variant and links the actual configured68test objects
and unchanged production archives. The link map excludes the normal directory
archive member. Rotation carries one extra checked phase word per claimed vertex
and uses the existing votes, with no extra collective. No new public config,
completion policy, metric, quality, operator, solver or production scheduler change.
The current standard binary SHA256 is `9811747b59cbb64dc91a95a53d8f4bb06d281249d95e2f608705a5681b8076ec`;
isolated binary SHA256 `86274904578f8e2be7ccb8600e3f5e0f1bda7c02de2cc88b9984c40119765e2f`.
Both identities and all source/object/archive/auditor pins are in the receipts.

Each runner reuses the existing complete exact auditor on the actual new
trajectory: positive orientation, tetrahedral embedding/no overlaps, incidence,
complete skin and links, immutable planar domain and finite marker coverage,
original-P1 tensors, q/J/full-chord lengths, actual SU2/CGNS output/readback and
current plus both BDF history transfers. Subsequent meshes and frozen P1 source
triangulations change; only the initial config/input/frozen-source controls must
match exactly. Those initial controls match in all three cases.

## Actual accepted results

| Case folder | Accepted tetrahedra, three events | Minimum q | Minimum J | Maximum L | Maximum state relative error |
|---|---|---|---|---|---|
| rotating_reservation_a2_n4_m2_v1 | 404/56/433 | 0.38176800 | 0.15651186 | 1.799025803 | 3.899e-16 |
| rotating_reservation_h20_n4_m2_v1 | 289/51/287 | 0.24666457 | 0.08654324 | 1.795778117 | 1.128e-14 |
| rotating_reservation_restart_cgns_n2_n4_m2_v1 | 46/7/36 | 0.52852259 | 0.24505011 | 1.793062585 | 3.844e-16 |

Each whole case passes eight independently audited geometry/metric frames and
three actual state transfers. Transported sensor errors are exactly zero,
including physical tensor components in H20. The collector explicitly requires
L<=1.8, without relying on the general auditor's small comparison tolerance.
H20 has exact physical volume1/2097152; all eight saved normalized geometry/donor
frames equal those of the same previously audited unscaled rotating variant.
This establishes this affine-covariance check, not boundary-layer generation.

The CGNS case tears down and reconstructs the real driver using the adapted
CGNS mesh and immutable `.native_ref`, writes/reloads both binary BDF histories,
and resumes the next adaptation from the preserved accepted geometry/state.
Reload state error is <=3.844148161277339e-16; both binary restart files match
the retained checkpoint values exactly. No physical PDE evolution or persistence
of a future frozen tensor source is claimed.

The six MPI1 lifecycle controls actually compute the metric/Hessian, remesh,
repartition/rebuild the CFD geometry/solver, transfer BDF histories, and write
mesh output. All independent geometry records are byte-identical to their
standard serial controls, as expected when rotating priority has only one rank.
Accounting and the existing fixed-work OFF/ON checks pass. Their fixture timings
are not used for a local speed or profiling claim.

## Reservation observations and performance limit

Second coarsening only; rank pairs refer to the two active workers:

| Case | Standard attempts | Rotating attempts | Standard vetoes | Rotating vetoes | Standard/rotating rounds |
|---|---|---|---|---|---|
| Turning aspect2 | 130/64 | 82/80 | 0/92 | 37/38 | 156/119 |
| H20 | 106/54 | 78/78 | 0/87 | 51/50 | 141/129 |
| CGNS restart size2 | 9/7 | 11/9 | 0/9 | 9/8 | 16/20 |

Rotation removes the lowest-rank proposal preference in these observed cases,
but changes the accepted mesh trajectory and can require more rounds. The
aspect2 coarsening accepts104rather than101proposals and finishes56rather
than71tetrahedra. The small restart case uses more rounds; fairness alone is
not proof of lower cost. Raw per-rank attempts/accepts/rejects/vetoes, donor
work and wrapper traffic are retained separately for all six operation events.
No elapsed-time ratio is computed. Repeated matched-target cost/scaling,
M=N/M<N selection, instrumentation overhead, wait/traffic distinction, weight
calibration and practical memory/default512limits remain required before any
production promotion or full Goal1 completion. The pre-existing fixed-work
cluster donor comparison remains unchanged and uses the standard scheduler.

## Evidence, failures and compact storage

`check_rotating_whole.py` verifies every immutable case/build/audit pin,
normal configured objects/archives/executable, exact initial controls, strict
metric bounds, zero transported tensors, changed-trajectory work accounting,
eight H20 covariance records and six serial lifecycle control records.
`ROTATING_RESERVATION_WHOLE_VALIDATION.json` contains the resulting verified
records. Raw accepted meshes, donor/current/BDF states, initial/reference files,
actual CGNS/SU2 outputs and VTUs are retained within each named case folder.
For example, inspect `rotating_reservation_a2_n4_m2_v1/envelope_mesh_00003.su2`
and the adjacent `envelope_s2_states.json`; VTUs and their export receipt are retained alongside those files.

The first collector failed because the historical CGNS control predates the
`donor_box_filtered` counter. `rotating_reservation_whole_build_v1/collector_failure_v1`
retains its exact checker source and cause. The correction reports that legacy
counter as missing rather than inventing zero; all current cases must expose it.
All numerical runs and independent audits passed before this collection error;
none was rerun and no numerical guard was changed.

Actual host contention/memory/process observations and one-CPU/nice19/thread1
limits are retained. All numerical and audit jobs are terminal; no local
performance campaign or cluster submission was launched. Only the terminal
shared isolated executable/object were removed after all receipts passed,
freeing806641624bytes; their hashes, recipes and link map remain. Build/case
receipts were kept immutable and the matrix checker also passes after cleanup.
Production binary/objects, AdapNoExt and other workers remain untouched.
