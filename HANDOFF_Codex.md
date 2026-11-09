# Native adaptation — current handoff

Updated2026-11-09 Europe/Rome. Goal ACTIVE, ZERO accepted meshes.
TWO strictly accepted ONERA M6 Euler CFD→mesh→CFD→mesh at50k metric complexity,
conservative admissible transfer, finite continued ADAPTED CFD and ACTUAL isolated
ADAPTED disk restart. Then dedicated BL/constraints/MPI and RANS500k. Actual point/
cell counts differ from complexity. Strict geometry1e-4,q>=.2,J>=.05,maxedge1.8,
requested heights. Validity-first construction, current nodal metric transport,
CAD optional/no-CAD authority sufficient. ONE active cycle/owned heavy workload.
Nice19/libs1/buildj3/4TBscratch. Freeze compiled/executing/pinned C++, binary,
helpers/config/runtime/inputs during build/check/run. Fork pushes authorized;
exact-whitelist staging, preserve unrelated dirty files. No new scheduler/config,
larger cavities, quality-aware construction, raised budgets or relaxed gates.

R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
A287=`R/m6_runs/euler_build287_exact_face_coverage_mach_only_real_cfd50k`.
A283=`R/m6_runs/euler_build283_pinched_graph_witness_mach_only_real_cfd50k`.
A274=`R/m6_runs/euler_build274_metric_cleanup_mach_only_real_cfd50k`.

## CURRENT —287 full same-seed TWO-event M6 LIVE/FROZEN

Qualification SEALED/PUSHED6dfc9e73f7, sourcecbaba0b675. Full287 ACTIVE tool65435;
ONE owned heavy workload. Exact actualHOSTstartup tree inA287/owned_startup.json,
all1520source/build/runtime/config/input pins verifiedunchanged atstartup.
Wrapper1242721/start22850740; runner1243010/start22851483;
time1243216/timeout1243217/MPI1243218 allstart22852005;
ranks1243221–1243225/start22852008,rank1243226/start22852009.
ConfigSHA5ad2dc1d9a1cd8bff87f633617b191b50defbc261ab3d0b2ed0987a34722b89e,
EXACTsame283config. Mach-onlyL1/50k savedREALCFDseed, TWOevents,native5400/outer14400.
Allcompiled/executing/pinned code/helpers/binary/runtime/config/inputs FROZEN.
Do not introduce another implementation/build/test while it runs. Let terminal
outcome or preserved existing authorizedstop evidence drive next work. Current
run/status and actualPID+startticks govern; never repeat historicalstophelpers.
Atlaunch load1.13,MemAvailable48.610GB,diskfree122.610GB. FirstCFDCL.269535/CD.013536,
metric recovery0.361s, all61330fits withoutWLSfallback. Initialrestart verification
PASSED:61330coords/22fields BIT-IDENTICAL256/qualified254target,SENSOR_MACHonly.
A287/initial_cfd_target_equivalence.json +initial_verifier_reuse.json. ORIGINAL-grid
proof ONLY, not adaptedCFD/transfer. ALL6actual executableinodes and84mappedELFfiles
match pinned hashes inA287/actual_runtime_startup.json. Cachehostquota4GiB,rank
715827882bytes, nativeworkingstorage12GiB/rank unchanged. Startup/live evidence
PUSHED startupcheckpointdaf3e29d10.
Qualification explicitly full_mesh_accepted=false/actualcertificate_complete=false.

LatestactualHOST1794185457:ALL11ownedidentities LIVE,load7.30/8CPUs,MemAvailable
39.781GB,sampledtreeRSS8.921GB (notphysicalpeak). ALL1520pins reverifiedunchanged
inA287/pin_check_quality_window3.json. No observedresourcepressure/stopcondition.
Qualityentry23306/native573.762890:690804cells,27052q/27499J/localG1204/size0,
ALLknown/flagscomplete, EXACT283entrycounts (oldnative783.249625).
THIRDcomplete qualitywindow23488/native1097.106411:708230cells,17888q/16900J,
localG1144/size0, ALLknown/flagscomplete; ownershipcommitted1107.335627 preserves
allflags/counts. Counts/5543cumulativecommits EXACT283samewindow.
Cumulativenetq9164/J10599/G60 removed in523.343521s (old851.539423s),
q17.5105/s vs10.7617/s; qremoved/commit1.6533, debtmax/mean1.18294,
joincallwallshare.458825 (NOTCPUidle). Traversal318.391Mvs324.591M(-1.91%);
walltimedifference cannotbe solelycode. Secondwindow qretired19572/introduced16914
net2658; nostuck/missingfeatures. Mostremainingqinterior; shortedgecorrelation NOT
constructioncausality. Currentthirdwindowlocation/ladder/churn ledger preserved
inquality_counter_assessment.json and complete rawprefixes; no newinstrumentation.
Completeprefix/churn/location/ladder assertions PASS in existing analyzers;
firstevent/first uninterruptedqualitypass ONLY. Eight unchangedtransfer/restart
readinesspaths match actual287pins, NOTactual transfer/adaptedCFD/restartproof.
CurrentprogressdataLOCAL; push checkpoint. KeepM6frozen; no otherjob/implementation.

Connected production change ONLY CNativeBoundary3D.cpp's private planar boundary-
segment/open-triangle intersection proof: exact triangle-edge and segment-line
separating axes replace general tetrahedron clipping; reuse existing nonzero face
orientation. ALL original early vertex winding checks retained. Exact orientation,
finite coverage, holes, seams, bodies, features, heights and finalBoth gate unchanged.
No cache identity/ownership/publication/config/archive change. Independent exact
rational interval oracle140352checks/0mismatches and read-only review support logic;
execution evidence below, not a mathematical-review acceptance claim.

285/286vertex-winding removal candidates REJECTED: reverse common-domain work
+28.42%/+21.87% despite fasterwalltime and identicalbounds. Patches/diagnoses inR;
NO MPI/M6ofthose variants. Current287RESTORES all old earlyguards. Do not reintroduce
failed removal or infer performance from walltime alone.

287 qualification: build33.108s, focused12cases/7281assertions52.692s; actualsource
probe81.718s, reverse188.780s; serial90cases/67446assertions198.989s; ONE MPI6
milestone124cases EVERYrank217.203s, assertions72054/71127/71330/70947/71155/71208.
ALLpinsunchanged; all5handles absent eachserial/probe, all10absentMPI. Casepaths:
- focused geometry_current_queries_20261109T000910_1197741
- source geometry_current_queries_20261109T001022_1200684
- reverse geometry_current_queries_20261109T001246_1206798
- serial current_production_20261109T001710_1218462
- MPI placement_mpi6_20261109T002042_1226839

Actual283thirdentity37202reference/53488physicalfaces, complete736019rawcells,
prepared by prepare_final283_entity2.py with exactcoordinate/orientation/hashchecks.
Current287compares SAMEinputs against284production-equivalent283diagnostic:
source163356common domains EXACTbounds/decisions/order,work19,648,840→19,533,020
(-0.5894%); reverse58737common EXACTbounds/order,work15,307,265→15,162,399(-0.9464%).
Savings SMALL, do not claim substantial closure. At20Mcap source164858vs163356domains,
reverse60892vs58737; BOTH still incomplete/no outsideproof. Equal-facet source
parametercoverage81.9035%vs81.8703%, NOTphysicalarea/partialacceptance. Nested graph
work/timers overlap upper/traversal; never add. Scope notpreparation dominates only
somequeries; reverse lifting remains substantial. No perf/GDB/newbaseline campaign.
Full M6 outcome must decide whether speed remains blocker or transfer/CFD is reached.

CFD SHA b6dae65a3ac3e0f74b4c8886872f90c4581d75dfc824cd1a21bbe9a1d3ce97dc.
Test_driver SHA e270586fc48f6a0d312d93651215fcd1b0bed4d2a7761a130cc90b255dec1313.
Build sourcepins SHA e29893bbddaab041dc43ca022e6169fe8e2bb1a06082df8ca9ee24dc3aa52e4e.
Launch uses same savedREALCFDseed R/m6_inputs/euler_original_force_seed_v1:
61330points/303359tets,M.8395/AOA3.06,CL.269535/CD.013536; Mach-onlyL1/50k.
SeedSHA1eaee788bd62bd92a92ec539577dcbcfc0f8042dcbcec23adb04e825bb01a880.
Native5400/outer14400 and all gates unchanged. Use verify_m6287_initial.py to verify
61330coords/22fields BIT-IDENTICAL256/qualified254target after original CFD stage.
Original-grid verification is not adapted CFD/transfer. Starting solution is reused.

Actual /proc/ps MUSTrequire_escalated: sandboxabsence previously caused false
terminal inference, corrected beforeaction. Before any restart/stop inspect exact
owned PID+startticks. Never signal historicalhandles. launch/runstatus alone is
not proof of terminal state; tools and actual host identities govern. Preserve
startup tree, all1520pins, runtime maps/resources and complete-prefix counters.

## Preserved terminal283 blocker and inspection

First event REFUSED mandatory final globalTWO-SIDED geometry proof on elapsedtime.
Exit25175/native5344.368047s:736019cells,0q/0J/0LOCALgeometry/0size, ALLknown/census
complete. Local closure is NOTacceptedmesh/globalproof. No transfer/adaptedCFD/
restart. Lastprequiet25169/native4621.108681 had0q/J,G16. Sixrounds closed16G,+33tets.
Final~720s/rank spent3fullqueries, traversal121–128Mwork/622–640s,scope52–59s,facet24–40s.
Earlier2queries returning does NOTestablish their bounds within tolerance.

All30rawfiles COMPLETE736019rows/metrics/noomissions,128MiB/filecaps retained;
all1520pins sealed before EXACTLYONEpidfdSIGTERM to MPI860479/start21882642 AFTER
explicit refusal/capture. ALL11actualHOSThandles terminal; main63535terminal1.
NEVER repeat seal_stop_m6283.py or historicalstops. Terminal/capture/diagnosis
receipts inA283; fd9147dced PUSHED. Directional284diagnostics4bf88fb28d PUSHED.

Hostcontention mattered to walltime, not established as solecause. Matchinground
23872:274commits17201 vs28317029,netq/commit1.137vs1.142,traversal700.8Mvs682.8M,
qualitytime1462.65vs2219.91s. Hostload~15/8CPUs,ranks~78–79%CPU/elapsed includesMPIspin.
Schedstats flag0 limits queue interpretation. SampledtreeRSSpeak14.0215GB is NOT
physicalpeak; no actualRAMpressure. Compare countedwork and wholefield q/J-bad
removal/s at matching complete stages, geometrycounts, churn,busy/wait/debt spread.

Read-only analyze_m6287_frozen.py and analyze_m6287_quality_live.py compare287vs283
using existing complete six-rank prefix counters. Firstevent only; cumulativechurn
only first uninterrupted qualitypass, latergeometryreturns altercounts elsewhere.
No interpolation/causalclaim from nearest quality times. Parserreuse/sourcehashes
recorded. No need newbaseline. Helpers need actual complete stages before analysis.

Latest meaningful independently validated mesh remainsA274/inspection/rejected_mesh.vtu
(+su2):148781points/734806tets,captured_nodal_metric9 SPD, q/J/localgeometry known in
CellData. FullglobalBoth incomplete, REFUSED.262full148608/734315 has4UNKNOWNq/J and
4localG, NOTshape-certified.271partial644221/735624 must not be filled fromothermesh.
No per-trial VTU required; complete283rawcapture remains available ifbeneficial.

## Cleanup, transfer, restart and later qualification

[Post-refinement cleanup](Handbook/ADAPTATION_POST_REFINEMENT_CLEANUP_20261108.md):
original clean-star selection/zero-debt admission gap confirmed. Four actual262
clean contexts prove three rawcollapses+one privatecollapse/optimizer removal,
strict localchecks and better short-edge/directional measures. Opposite directions
fail positivity/support/geometry; genuine regression vs conservative refusal not
fully separated. NOTwholefield leadingedge cure. Integrated273 finite one-shot
shape-preserving cleanup is qualified274, reuses claims/versions/publication/
Coarsen size checks and strict short-deficit decrease. Conditional at stalledshape
withgeometry/size/heightsclosed, NOTunconditional postrefinement or minimumedgegate.
Alreadystrict meshes can acceptwithoutcleanup.274/278/283NEVERexecutedcleanup.
Preserve it; no globalbenefit claim/policywidening while certificatecycle active.

Do not equate workflow_accepted alone with numerical transferproof. Require trueCV
massresidual<=1e-10 EACHfield/timelevel, scaled TARGETintegraldefect<=1e-12 and full
admissibility beforepublication. Hardtargetdefect/admissibility sourcegates exist;
old/newdomainintegral difference is NOTtargetdefect. Source/readinessaudit ONLY,
not runtimeproof. Driver cycles0..nCycles includes final adaptedCFD afterevent2.
Original-grid continuation after refused event proves none ofthis.

After TWOaccepted: isolate ACTUALacceptedmesh+.native_ref+matchingrestart in newdir;
ADAP_LOOP=NO,RESTART_SOL=YES,READ_BINARY_RESTART=YES, explicit checkpointINPUTbasename
DIFFERENTfromRESTARTOUTPUT, samequalifiedstandaloneCFDbinary/runtime. Checkcoords/
order<=1e-12,finitefields,positive density/internalenergy and continuedCFD. Cycle0
resume-overwrite/rejectedevent consumption bugs OPEN; do not pretend originalgrid
outputs areadapted. Then dedicatedBL/curvedwalls/junctions/constraints/MPI, RANS500k.
Missing shockallocation remainsSEPARATE: sampledtarget4–6%chordvs existing3–4%; no
norm/sensor/complexity/geometrymetric change. Cleanup/certificate speedup does not
retarget shocks. Other hypotheses in[backlog](Handbook/ADAPTATION_BACKLOG.md).

Current design/evidence:
[certificate cycle](Handbook/ADAPTATION_PREPARED_SOURCE_CERTIFICATES_20261108.md),
[earlier final proof](Handbook/ADAPTATION_FINAL_CERTIFICATION_20261108.md),
[pre287 detailed handoff](Handbook/History/HANDOFF_before287_full_M6_20261109.md),
[pre283 history](Handbook/History/HANDOFF_before283_full_M6_20261108.md).
