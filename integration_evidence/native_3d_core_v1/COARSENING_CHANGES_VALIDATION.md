# Complete-star collapse retries and conservative orientation filtering

The prior selector ignored changes outside a rejected candidate's edge cells,
even though collapse imports the complete removed-vertex star. The new directory
stages notifications to every old/new star owner before its final vote. Selection
keeps bounded local vertex/cell and blocked-direction indices and reactivates only
affected directions after publication. This includes remote changes with unchanged
local candidate cells. An exact local nonpositive surviving-cell determinant also
removes provably invalid directions before expensive import/reconstruction; cells
containing both endpoints are excluded because collapse deletes them. Invalid
fields retain the private repair path. The frozen original-P1 sensor, geometric
composition, quality/sliver/length bounds and512-round budget are unchanged.

The6production/test files, plus the lifecycle counter audit and reusable cluster
case-count/self-check updates, are based on b49f089b43 on
codex/native-3d-transport-accounting. Actual build/source/object/archive/binary pins
are in coarsen_changes_build_v1/validation.json; all68 configured unit objects and
774 thin archive members are retained in the build provenance. Source snapshots
allow review of the exact compiled change. No modified2D or MMG implementation.

## Actual validation

| Folder under this report | Result and scope |
|---|---|
| coarsen_changes_build_v1 | PASS complete configured MPI/MMG/CGNS test driver |
| coarsen_changes_mpi1_v1 | PASS71cases;12independent frames/6state pairs/6actual output round trips; serialMMG48assertions/2cases |
| coarsen_changes_mpi2_v1 | PASS71cases/rank;24frames/12state pairs/12output round trips |
| coarsen_changes_mpi4_v1 | PASS71cases/rank;48frames/24state pairs/24output round trips |
| coarsen_changes_lifecycle_mpi1_v1 | PASS six actual metric/Hessian OFF/ON controls, replacement/BDF/output/phase accounting |
| coarsen_changes_n4_n4_m2_v1 | PASS three n4 rotated/spatial wedge events with independent exact audits |
| coarsen_changes_restart_su2_n2_n1_m1_v1 | PASS persistent SU2 reference/current+BDF restart and later adaptation |
| coarsen_changes_restart_cgns_n2_n4_m2_v1 | PASS persistent CGNS restart with M<N and later adaptation |
| coarsen_changes_package_checks_v1 | PASS31lightweight orchestration/parser tests and recorded new counter/notification closure; two collector failures retained |
| coarsen_changes_n8_n1_m1_v1 | FAIL full campaign at second adaptation; first now accepted, independent first-cycle audit pending |

All numerical runs were sequential, nice19, one library thread, MPI<=4 on one CPU,
after host/load/memory/affinity inspection. These are contended correctness runs.
No local elapsed-time/scaling/overhead/affordability conclusion follows. New timer
fields are nested in selection and publication, not additional exclusive phases.
The audit rejects incomplete/negative/fractional counts and escaped parent timers;
sent/received record counts close globally over each checked log. Requested-byte memory estimates are charged; vendor allocator-inclusive
bounds still need calibration. There are no extra notification exchanges during
refinement; the mode-consistency check adds two startup worker reductions to3D.

## Frozen larger-case result

All five baseline parameter/environment/config/input-mesh/initial-sensor files are
byte-identical (baseline_comparison.json). The target remains64/16/64, aspect1.5,
spatial/directional variation.125, q>=.20,J>=.05,L<=1.8,noise0,512rounds.

First event:457accepted refinements to3552private cells, then428collapse attempts,
272accepted/156rejected,295early orientation exclusions and120full-star
reactivations. All2489final cached cells pass, with no pending directions. The
accepted590point/2489tetrahedron actual SU2 output is in envelope_mesh_00001.su2.
trajectory_comparison.json verifies exact coordinate/connectivity/physical-marker
equality with the earlier rejected private mesh, ignoring numbering. This is a
fixed accepted-edit geometry comparison; the prior call rejected and skipped
return/rebuild/transfer, so total adaptation work differs. No speedup claim.

Second event:512collapse attempts,429accepted/83rejected,545private cells,
323pending directions, zero cached invalid cells, no pending repair proposals;
1883orientation exclusions and988full-star reactivations. The last rejection was
q=.16126001018226588/J=.042957520912039047 on cell67901 for edge390,391. It is one
witness, not the classification of all rejections. The accepted first CFD mesh,
current/BDF state and original reference remain protected by rejection. The
545cell private result is not published or independently certified/exported yet.

The running audit_first_cycle.py separately checks exact embedding, original-P1
q/J/integrated edges, transported metrics/current+BDF states and actual SU2 output
for cycle0. Its first_cycle_audit.json, when terminal, is scoped only to cycle0;
the campaign validation.json remains FAIL. Do not conflate partial acceptance
with three-event success. No independent_audit.json is fabricated for this case.

## Remaining work

Classify the second coarsening event's rejection/dependency/queue behavior before
changing operators or budget. Establish whether remaining directions are
admissible, geometrically/metric-blocked or simply not examined within512rounds.
Keep exact checks, complete dependencies, original-P1/private query rules,
rejection state preservation and frozen contracts. Extend practical size/M=N/M<N
limits after the next fix. Prepare fresh cluster bundles for repeated n4 timing,
instrumentation OFF/ON, worker subsets and optional profiler campaigns. Measure
complete cost, not reconstruction alone. Practical larger completion and repeated
cluster performance/calibration remain Goal1 gates; Goal2 has not started.
