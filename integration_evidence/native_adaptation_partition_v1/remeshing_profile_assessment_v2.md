# Current-source remeshing profile — 2026-10-08

Branch codex/native-unsteady-performance; timing build native_exclusive_profile_build_v1.
App SHA256 77fbefa77179acd6d8e5c66da3eed61974f8dede4f4d2ce5937544cf9185c6f6;762 source pins.
83 core cases/rank pass MPI1/2/4; timing mean accounting closes in93/109/126 observed backend profiles.
Current-source BDF2 vortex and coarse straight-wall SA RANS each pass two actual N4/M3 adaptation cycles,
including independent sensor/BL metric, current and previous history conservation, positivity and mesh gates.

## Real frozen RAE Euler-to-BL profile

Folder: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_adaptation_partition_v1/frozen_rae_euler_to_bl_n4_m3_profile_v1/runtime_np4
Four CFD ranks, three remeshing workers; no CFD solve/solution transfer in this frozen fixture.
99 Hz user CPU-clock sampling with16KiB DWARF stacks;33.489s whole process includes profiler overhead.
Candidate byte-identical to prior uninstrumented M3:12246points,50739commits,2665 across owners.
Independent final metric/geometry/height/topology/shape/length audits PASS. Do not claim timing speedup from this profile.

| Exclusive transaction scope | Per-worker minimum / mean / maximum seconds |
|---|---|
| Protocol |0.102643 /0.109364 /0.114851|
| Dependencies/import/reservations |3.16921 /3.17123 /3.17232|
| Original-sensor donor import and IDs |3.14826 /3.16130 /3.16827|
| Reconstruction and private metric evaluation |3.83193 /8.28538 /14.3431|
| Certificates, collision/participant validation and decisions |5.50699 /11.5722 /16.0203|
| Staging, directory preparation/election and publication |1.02704 /1.03933 /1.04562|
| Candidate selection (separate from round scopes) |1.6916 /2.64026 /4.08256|
| Adaptation unclassified residual |1.48143 /2.91229 /3.86089|
| Entire Engine::adapt (inclusive) |32.8913 /32.8913 /32.8913|

Mean exclusive scopes plus selection and explicit unclassified mean reproduce the inclusive mean to printed-log rounding.
Unclassified mean is8.85%: inter-round checks/control, temporary destruction and miscellaneous work are not silently dropped.
All stage wall timers include MPI waiting. Their maxima may occur on different workers and must not be added.
Tracked collective totals remain partial diagnostics, excluding validation elections, and must not be added to stages.

Native import/reference0.00547s,target/policy0.00050s,whole working partition0.04410s max rank.
Its inner weight/graph/ParMETIS/migration maxima are0.02437/0.00148/0.01577/0.00193s.
Engine initialization0.03368s,incoming residual check0.00019s,final contract scan0.00050s max rank.
N-rank reader/reference binding return0.02174s. Inactive rank consumed0.22024s CPU while waiting32.9298s.
Remesh outer interval contains these scopes plus communicator/statistics/logging/final synchronization overhead.

## CPU attribution

Authoritative sampling_summary_v3.json:9745 native samples,5339 with MPI API frames (54.79%).
4771 sampled MPI frames are PMPI_Allreduce,421 Alltoall,72 Alltoallv;47 Wtime frames identify timer overhead
but do not measure its incremental cost against prior instrumentation.
Nearest visible native function: World::Fail3533,World::exchange1196,FieldPatch::evaluate1185 (12.16%),
Engine::phase867,boundary583,CellChoice460,chooseCached268,Engine::round206.
These are statistical nearest-function CPU attribution, not exclusive wall times or proof that every MPI sample is waiting.
Reconstruction varies strongly among workers; complementary validation waits and election samples require distinguishing
synchronization frequency from straggler-induced waiting before changing the protocol. Earlier failed preflight consolidation
must remain retained and must not be repeated blindly. Metric evaluation is a material CPU contributor even after donor indexing.

The initial classifier wrongly labeled template return/argument types as sites; its raw report remains saved.
An attempted call-only correction discarded perf's +offset symbols and MPI parents: sampling_summary_v2.json is rejected.
Corrected classifier recognizes direct +offset functions and generic-handler native lambda owners, with runnable regressions;
stack_classifier_initial.py,stack_classifier_rejected_call_only.py and classifier_reconciliation.json preserve the diagnosis.
No SU2 source or field acceptance was changed by classifier corrections.

## CFD return/lifecycle coverage

Actual N4/M3 vortex and SA logs now separately report CFD geometry partition/migration/preprocessing; this includes initial
geometry too, so only subsequent rows belong to remeshing. The shared geometry timings are nested in replacement geometry-build.
Replacement reports exclusive geometry-build,marker checks/wall-distance,solver initialization,direct-original solution/history
transfer and finalization. Transfer is already included in replacement. Adapted mesh/restart output and metric construction remain
separate lifecycle scopes. Include both working and CFD return partitions in adaptation cost, never sum nested scopes twice.

Remaining: real unsteady RAE performance/restart and reverse workload, same-source unprofiled M3/M4 repeat comparisons,
practical memory/scaling limits and post-restart remeshing. SST/CGNS and accepted-mesh SU2/CGNS/rank-change restart controls subsequently PASS; see subset_unsteady_lifecycle_checkpoint_v1.json. Goal ACTIVE; no affordability,converged-flow or native3D claim.
