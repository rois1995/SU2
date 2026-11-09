# Native adaptation — current handoff

Updated2026-11-09 Europe/Rome. Goal ACTIVE, ZERO accepted meshes.
TWO strictly accepted ONERA M6 Euler CFD→mesh→CFD→mesh at50k metric complexity,
conservative admissible transfer, finite continued ADAPTED CFD and ACTUAL isolated
ADAPTED disk restart. Then dedicated BL/constraints/MPI and RANS500k. Actual point/
cell counts differ from complexity. Strict geometry1e-4,q>=.2,J>=.05,maxedge1.8,
requested heights. Validity-first construction, current nodal metric transport,
CAD optional/no-CAD sufficient. ONE active cycle/owned heavy workload.
Nice19/libs1/buildj3/4TBscratch. Freeze compiled/executing/pinned C++, binary,
helpers/config/runtime/inputs during build/check/run. Fork pushes authorized;
exact-whitelist staging, preserve unrelated dirt. No new scheduler/config/larger
cavity, quality-aware construction, raised budget or relaxed gate.

R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
A289=`R/m6_runs/euler_build289_prepared_borders_mach_only_real_cfd50k`.
A287=`R/m6_runs/euler_build287_exact_face_coverage_mach_only_real_cfd50k`.
A274=`R/m6_runs/euler_build274_metric_cleanup_mach_only_real_cfd50k`.

## CURRENT — full289 LIVE and FROZEN; no C++/binary/helper/input edits

Fullsame-seed TWOeventM6 LIVE tool61806. Wrapper1564408/start23715184;
runner1564745/start23715930; time1564977/timeout1564978 start23716464;
MPI1564979/start23716465; ranks1564982–1564985 start23716467,
1564986–1564987 start23716468. ALL11actualHOST handles recorded A289/owned_startup.json.
ALL1520source/build/runtime/input pins verified; all6actual executable inodes match,
84mappedELF files EACHrank matchpins: A289/actual_runtime_startup.json.
Actual /proc/ps MUSTrequire_escalated; sandboxabsence previouslyfalse-terminal.
Never infer termination fromelapsed/stalelogs. StopONLY verifiedownedrun after
preserved authorizedfailure/bottleneck/resourcepressure. NEVER repeat historical stops.

Implementation31ff1c3bad, qualification484f5678c8 PUSHED. SourcepinsSHA
501927a52b6733c0ac7e54eed7d6cfc45a397a00eaa35a0f6ee6abcb2d063629.
CFD SHA ff961bf8e23d0f8a2422019cd7e34c5b13129e82191216d5c7e4a51ecbd77678.
R/certification289_qualification.json seals fivecases/allpins/terminalreceipts.
Current actualexe is test_driver onall6ranks, standaloneCFD also built/qualified.
No acceptedmesh/transfer/adaptedCFD/restart observed. Do not declare completion from
fixture success, primitive speed, localgeometry0 or workflow_accepted alone.

SameREALCFDseed R/m6_inputs/euler_original_force_seed_v1,61330pts/303359tets,
M.8395/AOA3.06,CL.269535/CD.013536. Mach-onlyL1/50k, configSHA
5ad2dc1d9a1cd8bff87f633617b191b50defbc261ab3d0b2ed0987a34722b89e.
verify_m6289_initial.py PASSED all61330coords/22fields BIT-IDENTICAL256/qualified254
target,SENSOR_MACHonly. A289/initial_verifier_reuse.json; ORIGINAL-grid proof only.
Cachehostquota4GiB/rank715827882B,nativeworkingstorage12GiB/rank;
native5400/outer14400 unchanged. Runnerresource samples are not truephysicalpeak.

## Active connected correction and qualification

289prepares immutable projected nonconvex border endpoints once in PlanarReference,
reuses sameordered exactcoordinates in Contains/ContainsSegment/ContainsFace.
All predicate operands/order, winding/crossing/earlyguards, axis/root/support/body/
feature/height proofs and acceptance unchanged. ConstructionBytes/ResidentBytes
include map/vector storage. No mutablecache/index/newrepresentation. Segment
corner loop unchanged. Parameter views share immutable definition lifetime.
Read-only lifecycle audit found no issue; actual tests below establish affected scope.
Preserve prior287exact finite-segment/open-triangle proof and ALLearlyvertexguards;
285/286earlyguard removal was rejected for reverseworkregressions, notlayered.

Actualcurrent287 source query37202reference/53512physicalfaces: ALL82448domains,
bounds/order/PER-DOMAIN chargedwork EXACT. Query83.042→27.921s, traversal81.156→
25.990s, serialCPU89.79→35.01s. Savedqualified283reverse53488physicalfaces:
ALL60892domains/bounds/order/work EXACT. Query177.592→99.755s, traversal110.111→
33.958s, serialCPU184.71→106.92s. SAME20M/8GiB/600s, bothcertificates incomplete.
These are primitive savings, not wholeM6 speed or acceptance. Current287 physical
input differs from283; never describe old283query ascurrentmesh.

Build289379.306s; focused12cases7281assertions52.757s; serial90cases67446assertions
200.069s; ONE MPI6 124cases EVERYrank220.460s, assertions72048/70941/71161/71223/
71127/71321 (logorder, notrankmapping). Allpins unchanged/allactualhandles terminal
before nextstage. Case suffixes: focused023659_1524711/source023809_1528202/
reverse023937_1532219/current_production024223_1539645/placement_mpi6024619_1550272,
all20261109. Qualification explicitly full_mesh_accepted=false.

288residual-tolerance searchhint REJECTED: only+.511percentage-point equal-facet
parametercoverage at20M,commonwork+.4023%,bothincomplete. Patch/comparison/rejection
saved; discarded,notlayered. Further branchclosure NOTselected: currentphysical
9graph witnesses accepted/0invalid afterexistingtrim. Otherideas backlog.

## Frozen-run outcome must drive next action

287terminalREFUSED finalmandatory globalBoth certificate on elapsedtime at
native5346.697s/736323tets, ALLknown0q/J/LOCALgeometry/size. Lastqualityentry had2q/2J;
~2s privateclosure then ~2420s/rank/3fullqueries,168–172Mtraversalwork/~2342–2357s.
Finalcertificate speed demonstrated blocker, not continuedpolishing or coarsening.
ALL30rawfiles complete/metrics/noomissions,1520pinssealed before exactlyONEownedMPI
stop; ALL11oldhandles absent. Terminaldiagnosis/capture/stop receipts inA287,
PUSHEDa7a8c5baca. Do NOTreuse historical stophelpers. No transfer/adaptedCFD/restart.

Read-only analyze_m6289_frozen.py + analyze_m6289_quality_live.py reuse existing
six-rank complete-prefix counters versus287. Firstevent only; cumulativechurn only
first uninterruptedqualitypass, since geometryreturns altercounts outsidecounter.
Compare netwholefield q/J-bad removal/s,geometrycounts,work,CPU,busy/wait,churn and
rankdebtspread at actual matching stages. Nearest times are not causal interpolation.
No new baseline/perf/GDB campaign. Preserve sourcefreeze until actualhandles terminal.

## Cleanup diagnosis, inspection and outstanding obligations

[Cleanup record](Handbook/ADAPTATION_POST_REFINEMENT_CLEANUP_20261108.md): original
clean-star selection/zero-debt admission gap confirmed. Fouractual262cleancontexts
prove3rawcollapses+1privatecollapse/optimizer removal,strictlocalchecks and improved
short-edge/directional measures. Otherdirections failpositivity/support/geometry;
genuine regression versus conservative refusal notfully separated. NOTwholefield
leadingedge cure. Integrated273cleanup qualified274 is finite and conditional on
stalledshape withgeometry/size/heightsclosed, NOTautomaticpostrefinement or minimum
edgegate.274/278/283/287 NEVERexecutedcleanup. Preserve; no policywidening while
certificatecycle active. Missing shockallocation remainsSEPARATE: sampledtarget
4–6%chord versus existing3–4%; no new sensor/norm/complexity/geometrymetric decision.

Latest meaningful verified mesh A274/inspection/rejected_mesh.vtu (+su2):148781pts/
734806tets,captured_nodal_metric9 SPD and q/J/localgeometry known in CellData.
GlobalBoth incomplete,REFUSED.262has4UNKNOWNq/J,notshape-certified.271partial
644221/735624 mustnotfillfromothermesh. Complete287raw available; no pertrialVTU.
Export another finalmesh onlywhen meaningfully beneficial, alongside actualmetrics.

Require trueCVmassresidual<=1e-10 EACHfield/timelevel, scaledTARGETintegraldefect
<=1e-12 and fulladmissibility beforepublication. Hardtarget/admissibility gatesexist;
old/newdomainintegral change is NOTtargetdefect. Eightunchangedsource/helper readiness
paths auditedagainst287pins,NOTexecutionproof. Driver cycles0..nCycles includes
finaladaptedCFD afterevent2. Original-grid continuation after refusal proves none.

After TWOaccepted: isolate ACTUALacceptedmesh+.native_ref+MATCHINGrestart in newdir;
ADAP_LOOP=NO,RESTART_SOL=YES,READ_BINARY_RESTART=YES, checkpointINPUTbasename DIFFERENT
fromRESTARTOUTPUT,samequalifiedstandaloneCFD/runtime. Verifycoords/order<=1e-12,
finitefields,positive density/internalenergy and continuedCFD. Event1mesh00001 pairs
with FOLLOWINGCycle1restart00001; event2mesh00002/followingCycle2restart00002; prove
coords beforepairing. Cycle0resume-overwrite/rejectedevent consumption bugsOPEN.
Then dedicatedBL/curvedwalls/junctions/constraints/MPI and RANS500k.

Current detailed records: [certificate cycle](Handbook/ADAPTATION_PREPARED_SOURCE_CERTIFICATES_20261108.md),
[earlier final proof](Handbook/ADAPTATION_FINAL_CERTIFICATION_20261108.md),
[backlog](Handbook/ADAPTATION_BACKLOG.md), [next work](HowToProceed.md),
[pre289 handoff](Handbook/History/HANDOFF_before289_full_M6_20261109.md),
[pre287 handoff](Handbook/History/HANDOFF_before287_full_M6_20261109.md).
