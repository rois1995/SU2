# M6 selected-profile complete construction attempt

Published fixture WIP2794e4f744, production core unchanged from761067b031.
All owned jobs TERMINAL. Full Goal3 ACTIVE/incomplete. The complete saved-state
fixture ran once at MPI4 with the same300s native event ceiling, mesh constraints,
geometry authority, actual-query composition, selected sensor family and profile.
No CFD timestep/iteration was advanced. [Audited data](summary.json),
[design](DESIGN.md), [producer](run.py), [independent audit](audit.py).

## Result: initialization succeeds, construction rejects

Initial303359tetrahedra/61330points from suppliedCoarse.su2 and closed saved SA
restart. The selected lambda.875/version22,22profile trials/two evidence stages,
normalization295068.448184903 +/-5769.124263869 are IDENTICAL to the preceding
MPI4 result. This remains an integration_limited normalization, not an exact root.
Metric initialization took99.193s this time versus137.055s previously with the
same production implementation/target; different contention makes this no speedup
experiment.

Production remesh atomically REJECTS after300.434s, with no transfer or accepted
mesh. Four ranks' original flow/turbulence bytes, immutable reference and frozen
profile are checked; the fixture passes all assertions. Its successful execution
does not mean a successful adaptation. The initialization-stage saved_metric_budget
JSON correctly describes its earlier stage; selected_construction.json describes
the subsequent rejected event.

Private progress:38800 first-row publications and1707 later edits globally over
2200rounds;34 useful-progress windows/zero stagnant windows. Raw
selected_construction.json private_edits833 is RANK0 ONLY; summary.json explicitly
reduces all four logs (833/236/632/6). At stop,2351 remaining metric edges and6211
cached invalid cells remain. These counters are private search progress, not a
complete layer/mesh certificate or proof of infeasibility.

The elected final local reason is a cone obstructed by interface face
31334,31338,53318 at proposed apex8303350, feature edge starting25389. Its reported
cavity centroid is(1.77793877,1.55725498,.42845813). Rank2 also records a metric
shape rejection around(1.10303250,1.30047006,-.00288377450):q=.18017869 below.20,
J=.08390489 above.05. Other local rejected-feature centroids are retained in the
rank diagnostics. Do not infer an exact wing feature or global failure cause from
these last local samples. Native stop is TIME_BUDGET, not operator_stagnation.
No gate, profile, support, fade, thickness, wall height or budget was changed.

## Complete event cost (contended local observation)

| Scope | Maximum accumulated rank seconds |
|---|---:|
|Whole launcher including metric/setup/teardown|412.178|
|Metric initialization|99.193|
|Native event|300.434|
|Input import|2.082|
|Original source preparation|.856|
|Initial workload measurement|18.524|
|Repartition|1.605|
|Donor migration|.241|
|Communicator setup|.007|
|Refine scope|277.082|
|Coarsen/reassessment/normalization/transfer|not reached|

Engine totals are nested in refine and cannot be added to the parent rows:
first row23.116s, source/donor35.059s, maximum local reconstruction120.450s.
Rank2 has the largest reconstruction time; other ranks' engine residual times are
53.9--196.7s and include synchronization/unattributed control work. Do not label
all residual time pure MPI. There are395251 tracked calls/rank for the parent
and326764 nested kernel calls/rank,128805 directory elections,600 streamed private
query retries/rank,15977 donor batches/rank and2405 batch recoveries/rank.
No complete accepted-event/adaptation-to-CFD ratio or scaling claim follows.

Maximum predicted native work storage2089731500bytes/rank, below unchanged2GiB
ceiling but with little modelled headroom. Maximum SINGLE-process peak RSS769380352
bytes; sampled rank-RSS sum reaches2253.08MiB, not an aggregate peak/PSS. Host
available RAM37--40GiB. One job, nice19, threads1, affinity0-3; no cluster request.
The first build launcher used a nonexistent root ./ninja; preserved failure127.
The recorded prior Ninja executable then built the single affected target in
105.98s. No broad regression or extra CFD campaign was run for this fixture change;
the unchanged production core retains its preceding qualified control scope.

## Exact replay field

Each rank exported uniquely owned original-node sensor values BEFORE construction.
The audit verifies all61330global IDs exactly once, coordinates bit-for-bit against
the supplied mesh, finite/SPD tensors and original receipt hashes. The merged
[selected field manifest](m6_budget300000_np4/selected_field_manifest.json) records
core.003215747252591252,scale-4.442619538177675 and profile.875/version22.
selected_sensor.bin retains every row-major component and is compatible with the
existing frozen sensor record layout. Owned rank files preserve ownership proof.
All mesh/reference/restart symlinks close onto tracked inputs inside this repository.

This is an exact FROZEN sensor-only original-P1 replay field. It must still receive
actual-query geometric BL composition using the recorded profile/core. It cannot
replace unclipped frames for a later normalization correction, and must never be
rescaled as though clipping were homogeneous. The current full fixture correctly
retains the actual original family for any eventual explicit correction stage.
No failed private whole-mesh snapshot was exposed by the existing backend; there
is no _accepted or _rejected candidate grid to claim here. The original mesh,
saved solution, exact target and full final rejection diagnostics are inspectable.

## Consequence for the next implementation

The original hypothesis is only partly tested: complexity initialization no longer
prevents this field from entering remeshing; budget selection alone does not
complete construction, and no M6 candidate-domain reassessment was reached.
Current composed BL edits explicitly serialize because reservations do not yet
cover displaced/expanded joint stars. The measured next step is complete-proposal
dependency admission and safe concurrency, alongside retained source support;
not another local geometric fallback or another larger time/patch ceiling.
Whether that is sufficient for an accepted wing remains unproven. Preserve
nonmonotone/heuristic budget outcomes and all acceptance gates during that work.
