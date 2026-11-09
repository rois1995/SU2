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

## CURRENT —291 full same-seed TWO-event M6 LIVE, sources FROZEN

Qualified production/checkpoint PUSHED2d558a1d4d. Active attempt
A291=`R/m6_runs/euler_build291_facewise_certificate_mach_only_real_cfd50k`.
Main tool83989 LIVE; wrapper2134033/start25213637, runner2134403/start25214361,
time2134869/start25214932, timeout2134870 and MPI2134871/start25214933;
six ranks2134925–2134929/start25214984,2134930/start25214985.
ActualHOST owned_startup.json verifies all11identities/pins. actual_runtime_startup.json
verifies all6runningexe inodes and84mappedELF files/rank against1520pins.
Same Mach-onlyL1/50k REALCFDseed. Initial61330coords and ALL22fields BIT-IDENTICAL
256/qualified254target; onlySENSOR_MACH,CL.269535/CD.013536. Initial original-grid
proof only, notadaptedCFD/acceptance. Native5400/outer14400 unchanged.
C++/binary/runtime/helpers/inputs FROZEN through outcome; no otherownedheavyjob.
Preserve run until terminal or preserved evidence meets authorized stopconditions.
ZEROacceptedmeshes. No transfer/adaptedCFD/diskrestart yet.

Current291 firstqualityentry round23306/native582.909946864s:690804cells, ALLknown
27052q-bad/27499J-bad/1204localgeometry-bad/0size-bad; completeflagsALLtrue.
ALL83matchingstages counts/flags/comparedwork EXACT289; oldentry578.990650232s.
No causal timingspeedup claim; the changed finalproof has notrun yet.
A291/initial_quality_entry_verified.json seals1520pins+all6sourceprefixes;
quality_entry_host_resources.json preservesactual11identities,CPU/RSS/hostpressure.
Existing analyze_m6291_frozen.py +analyze_m6291_quality_live.py reuse counters versus289;
no newbaseline/perf/GDB or repairdecisions. Keep compiledsources FROZEN.
Transfer/restart readiness8paths matchactual291pins, NOTexecutionproof.
Latestcompletequalitywindow23616/native1396.599569944s:714635cells,
13548q-bad/12235J-bad/1096localgeometry-bad/0size-bad,allflagscomplete.
Net13504q/15264J removed;16.5960q/s vs28916.7807q/s; ALL92matchingstage counts,
flags and comparedwork identical. Cumulative9676commits,92297qretired/78793introduced;
no missingfeatures/stuckshape/privatecollectives. Latestlocation2969surface/10579interior
q-bad. Churn/shortedge correlations do not establish cleanup causality.
A291/frozen_progress_checkpoint_v1.json sealsall1520pins,all11actualidentities live,
completeprefixes/counters/resource sample. SampledtreeRSS~8.9GB, amplehostheadroom.
These are pre-finalcertificate counters, not evidence of its speed/acceptance.

### Previous289 terminal evidence selects this connected certificate cycle

ZERO accepted meshes. Event1 exit25210/native5344.631751870:736323cells, ALLknown
0q/0J/0LOCALgeometry/0size, flagscomplete. Mandatory globalBoth certificate incomplete.
Wrapper reports timebudget; ORIGINALfailure is atlasSTORAGE admission requested
13482375068 vsceiling12884901888, caller5381336204/scratch8101038864,
operation QuerySurfaceUnionAtResolution. Neither hostOOM nor mesh inadmissibility
is proved. Original-grid postrefusal CFD is NOTadaptedCFD/transfer.

ALL30rawfiles FULL/metrics/noomissions/128MiBfilecaps; ALL1520pins unchanged.
ALL24nonmanifest payloads (owned/metrics/reference/rejections across6ranks) are
SHA-IDENTICAL287, not merely count-matched. No new VTU needed for this identicalmesh.
A289/capture_ready.json + terminal_certification_diagnosis.json seal evidence.
EXACTLYONEverifiedpidfdSIGTERM to MPI1564979/start23716465 AFTERexplicitrefusal,
fullcapture andpins. NEVERrepeat seal_stop_m6289.py or any historical stophelper.
Main61806 TERMINAL1/5576.256499s,pins_verified=true,changed=[],workflow_accepted=false.
ALL11actualHOST identities ABSENT in A289/terminal_handles.json. Source freeze was released after289; production candidate290 is now built as291
(test-only rebuild, CFD byte-identical290). Build291 TERMINAL0/47.096s; all actual
compiler/build handles absent. Production CFD SHA
6bb7708715643f9e4941ed85bcc51ccb422ca8b95f4c33a6f1d75f96ada7cc41.
Focused291 PASSED28cases/5616assertions/56.695s, pins unchanged/all5handles absent.
Captured-face291 TERMINAL0/111.747s: EXACTsame17776-face prefix/bounds/work as290,
work-exhausted partial diagnostic; pins unchanged/all5actualhandles absent.
Connectedserial291 PASSED91cases/67534assertions/203.088s; ONE MPI6 milestone
PASSED125cases EVERYrank/223.706s (assertions71215/72142/71418/71296/71243/71035
in logorder). Allpins unchanged/allactualownedhandles absent before nextstage.
R/certification291_qualification.json seals affected checks and explicitly
partial captured-face diagnostic, NOTwholemesh acceptance. C++/binary/helpers
remain FROZEN for active fullsame-seed TWO-event M6 above.
Qualification PUSHED2d558a1d4d; launch_m6291.py alreadyexecuted ONCE.
Do NOTrelaunch. No competing cleanup extension.

First face replay290 hit unchanged20M work cap after17776/69544faces,0overTol,
maxupper9.9999622526187977e-5,peak admitted1120212192B. Wholefield UNPROVED.
It failed one diagnostic expectation that allfaces fit; preserve evidence.
291 only classifies work-exhausted partial replay explicitly and adds a final
certificate work-exhaustion guard test; no production/budget/gate change.
ALL69544faces (17.8MB) remain prepared with allsealedrawhashes unchanged:
R/prepare_final290_face_queries.py; A289/final_face_queries_v1 metadata/selfcheck.
Targetpool401031168/actualprepared396836624.
Actual /proc/ps MUSTrequire_escalated; sandboxabsence previouslyfalse-terminal.

289source/qualification/startup/progress PUSHED31ff1c3bad/484f5678c8/736fe5a1ff/
f3ab201418/6b68cee93c/c349e03ccb. Fullterminal+newdiagnosis PUSHEDf0ae3aeb43.
SourcepinsSHA501927a52b6733c0ac7e54eed7d6cfc45a397a00eaa35a0f6ee6abcb2d063629.
CFD SHA ff961bf8e23d0f8a2422019cd7e34c5b13129e82191216d5c7e4a51ecbd77678.
R/certification289_qualification.json seals fivecases/allpins/terminalreceipts.
All6actualexe inodes+84mappedELF files/rank matchedpins atstartup.
Peak sampledtreeRSS25.777GB, not truephysicalpeak; host~24–25GB available/PSI~0.
Resourcefailure is internaladmission, not actualhostpressure or a residentcacheleak.

Finalwindow25203→25210: ~2460.107s/rank vs2872422.180;3outerfullcalls each.
Summed traversal3.713B vs1.020B (~3.64xwork/s over samewindow), scopebuild54.19Mvs14.75M,
facetbuild33.42Mvs4.08M,pieceSelection18.36Mvs4.03M. Preparedresident~391.7MB/rank
near396.84MBlimit, not larger finalresidentcache. Cleanranks0–4 eachaddEXACT37202
facetvisits versus287, sameas Wing support; additional reverse-target preparation
established with QuerySurfaceUnion's soleRefineSurfaceTarget throw in reverse,
resolution/=4 retry. Directionwallshare/exactretrycount not directly emitted.
Rank5 additionally90privateuppercalls/6commits; do notattribute them tofullqueries.

ONE connected290/291 candidate qualified for fullM6 attempt: final mesh→model direction can use fresh
rigorous per-face correspondence/scoped bounds already used by ParameterError,
while retaining global model→mesh and fullBoth fallback for uncertain bounds.
Do NOTuse HasAcceptedFaceCertificate association/orientation bool asdistanceproof,
or localG0 alone. Factored existing EntityGeometry branch into EntityFaceError with exactsameauthority,
body/sheet/marker support, currentbindings/coords, optionalaffinefloor and budgets.
Focused controls, bounded face replay, connectedserial and ONE MPI milestone
complete. Fullsame-seed TWOeventM6 is nowLIVE; allstrictgates unchanged.
No new scheduler/config/geometryrepresentation/largercavity/gate/budget. No competing
cleanup policy, localresolution experiment or proofcache variant. Unselectedbacklog.

SameREALCFDseed R/m6_inputs/euler_original_force_seed_v1,61330pts/303359tets,
M.8395/AOA3.06,CL.269535/CD.013536. Mach-onlyL1/50k, configSHA
5ad2dc1d9a1cd8bff87f633617b191b50defbc261ab3d0b2ed0987a34722b89e.
verify_m6289_initial.py PASSED all61330coords/22fields BIT-IDENTICAL256/qualified254
target,SENSOR_MACHonly. ORIGINAL-grid proof only.
Cachehostquota4GiB/rank715827882B,nativeworkingstorage12GiB/rank;
native5400/outer14400 unchanged. All acceptance/transfer/restart obligations remain.

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
paths now verified against actual289pins in A289/transfer_restart_readiness_reuse.json,
NOTexecutionproof. Driver cycles0..nCycles includes
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
