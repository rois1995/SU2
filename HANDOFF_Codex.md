# Native adaptation — current handoff

Updated2026-11-09 Europe/Rome. Goal ACTIVE. ONE native candidate passed strict
acceptance and conservative transfer in291; ZERO complete workflow events/checkpoints.
Completion remains TWO strictly accepted Euler50k CFD→mesh→CFD→mesh events,
conservative admissible transfer, finite continued ADAPTED CFD and ACTUAL isolated
ADAPTED disk restart; then dedicated BL/constraints/MPI and RANS500k. Do not count
repeat first-event candidates as two completed events. Actual points/cells differ
from metric complexity. Geometry1e-4,q>=.2,J>=.05,maxedge1.8,requested heights strict.
Validity-first construction, current nodal metric transport, CAD optional.

ONE active implementation/validation cycle; one ownedheavyjob; nice19/libs1/buildj3/
4TBscratch. Keep compiled/executing/pinned C++, helpers, binary, runtime and inputs
unchanged through build/check/run. Actual /proc/ps MUSTrequire_escalated; sandbox
absence is not authoritative. Fork pushes authorized; exact-whitelist staging,
preserve unrelated dirtylogs/.orig/untracked files. No new scheduler/config/larger
cavity, quality-aware construction, relaxed gate or raised budget.

R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
A291=`R/m6_runs/euler_build291_facewise_certificate_mach_only_real_cfd50k`.
A289=`R/m6_runs/euler_build289_prepared_borders_mach_only_real_cfd50k`.

## CURRENT —293 LIVE, frozen full same-seed TWO-event M6

Qualification PUSHED8f6e38a2ff. Full attempt launched ONCE with launch_m6293.py;
main tool90456 LIVE. A293=`R/m6_runs/euler_build293_bounded_reference_mach_only_real_cfd50k`.
Receipt R/m6_293_bounded_reference_launch.json; wrapper2629885/start26208916,
runner2630301/start26209645, time2630593/start26210179, timeout2630594 and
MPI2630595/start26210180; ranks2630614/20/24 start26210184,
2630629/32 start26210185,2630636 start26210186. All nice19.
A293/owned_startup.json records all11 actualHOST identities; all1520pins verified.
A293/loaded_runtime_startup.json verifies six executing inodes and84mappedELF/rank.
verify_m6293_initial.py verifies all61330positions/22fields BIT-IDENTICAL256 and
qualified254target; Mach-onlyL1/50k and same savedCFDseed. Initial host available
48.663GB, load2.00/3.32/3.01, diskfree121.809GB. One ownedheavyjob.
C++/binary/runtime/pinnedhelpers/inputs FROZEN. Do NOT launch another attempt or
edit them before verified terminal handles; preserve frozenrun unless outcome or
existing authorized stop evidence justifies action. Native5400/outer14400/native
12GiB/hostcache4GiB and ALL gates unchanged. No event outcome claimed yet.
Qualityentry23306/native578.709s:690804cells,27052q-bad/27499J-bad/1204geometry-bad/
0size-bad, ALLmeasurements known and completeflags. All83matchingstages against291
have EXACTcounts/flags/comparedwork. Latest sampled treeRSS8.737GB (notphysicalpeak),
hostavailable~35.7GiB, all11identitieslive. A293/construction_progress_assessment.json,
comparison291_matching_stages.json and stage_parser_reuse.json seal existingparser
reuse/prefixhashes. Read-only transfer_restart_readiness_reuse.json verifies seven
unchangedpins; driverdiff reviewed for outputbudget propagation only. Actualnewrun
transfer/continuedCFD/restart remainUNPROVED.
Latest completequality_exit24513/native2587.237s:733163cells,10q-bad/4J-bad/
945geometry-bad/0size-bad; allknown/complete. Intermediate phase exit is NOTshape
closure or acceptance. All111matchingstages EXACTcounts/flags/comparedwork versus291.
All11actualidentities stilllive, sampledtreeRSS9.167GB, hostavailable~37.0GiB. No stopcondition demonstrated. Continue frozen293; do not
start anotherjob. Existingparser remains first-event-only; followingevents require
separate event-specific acceptance/transfer/continuedCFD audit.

## Latest user steering — preserve adapted CFD, then boundary-query performance

Preserve frozen293. Once event1 is accepted/transferred/persisted and continued
adaptedCFD succeeds, retain mesh+reference+matchingrestart in an independent
checkpoint and prioritize ONE connected boundary-query performance cycle before
another adaptation. Frozen driver advances automatically: if already inside event2,
preserve its outcome before editing any pins. Do not interrupt an activeevent on
an assumption that the first checkpoint is complete. Same-seed first-adaptation
replay is the performance comparator: Mach-onlyL1/50k, unchanged constraints/gates/
budgets/construction. No source change yet; bounded static observations recorded in
[performance cycle](Handbook/ADAPTATION_BOUNDARY_QUERY_PERFORMANCE_20261109.md).
Return to preserved adaptedCFD for logical event2 if needed; actual diskrestart,
BL/MPI and RANS500k obligations unchanged. Currentrun has NOevent1output yet.

## Previous291 TERMINAL after native acceptance/transfer; reference write failed

Main83989 TERMINAL1/3716.395958s; pins_verified=true,changed=[],workflow_accepted=false.
ALL11actualHOST identities ABSENT, sealed A291/terminal_handles.json. NaturalMPIabort;
NOmanualsignal sent. Its sourcefreeze was released before the qualified persistence
cycle; CURRENT293 is frozen/live above. Do NOTrelaunch291 or historical stophelpers. All1520source/build/runtime/input/helperpins verified.

Native exit25210/3535.670512273s:736323tets, ALLknown0q/J/geometry/size, allflags
complete; stop=complete onEVERYrank, mandatorytwo-sided globalgeometry gate PASSED.
Actual accepted mesh149051points/736323tets. Target50k; transported-field complexity
estimate51431.85308138/uncertaintyindicator138.766 (NOTprovenbound); normalization
correction0/currentnodaltransport unchanged. Same initial61330coords/22fields exact.

Conservative transfer COMPLETED before memorypublication: trueCVmass residuals
Density7.722e-14,MomentumX2.649e-14,Y5.244e-14,Z3.140e-14,Energy7.627e-14;
0inadmissibleprojectedstates. Successful pinned path enforces scaledTARGETintegral
error<=1e-12 before publication; finalnumericaltargetdefects NOTprinted. Old/new
volume/integral changes are NOTthat measurement. Actual meshreplacement completed
on6ranks, transfer~10.24s. No adaptedCFDcycle1/restart followed.

Writing adapted_mesh_adap_00001.su2.native_ref FAILED in WriteReference:
"Native3D reference decoding exceeds storage budget." SolverMPI_ABORT1, not an
agent stop or hostOOM. Complete ASCII mesh is preserved at
A291/adapted_mesh_adap_00001.su2 (38361722B,SHA
960b6d56ef9e3c7e0a88b9b763b3d419b3edf9523a45662352898ad4a730c28d).
No .native_ref and no matching adapted restart: NOTa usable disk checkpoint.
A291/terminal_publication_diagnosis.json +R/analyze_m6291_terminal.py independently
validate mesh dimensions/indices, sixnativecomplete ranks, transfer residuals and
terminalprovenance. Sampled peak treeRSS13.615603712GB (NOTtruephysicalpeak).

## Active cycle — bounded reference persistence using existing native budget

Production292/test-only293 qualified for fullM6; sourcecode PUSHEDc63ea88490.
Build292263.284s; build29347.717s, productionCFD BYTE-IDENTICAL292:
89aa555d0a6a13679cc62f989fc30c5b4acec0c773bc3d972fe4cadf5629e75c.
Allactualbuild/testhandles terminal and pins unchanged before eachnextstage.
Focused29329cases/1583assertions/54.838s; expectednegative1exit/4.589s preservesprior
file/no.tmp; largeauthority1case/278215assertions/286.295s. 47,601,733B roundtrip,
2GiB refusal correctlycaught. AdmittedLoadPeak6,491,331,901B, sampledindividualRSS
1,005,264,896B: originalM6authority/entitypolicy, NOTlost291currentatlas replay.
Connectedserial97cases/67903assertions/205.072s; ONE MPI6 130cases EVERYrank/224.471s,
assertions72217/71302/71110/71487/71413/71342 inlogorder. MPIall10handles absent.
R/reference293_qualification.json seals5stages including explicitexpectednegative.
FullM6 launched ONCE as293, see CURRENT above; never relaunch the same attempt.
C++/binary/runtime/pinnedhelpers/inputs remainFROZEN for fullsame-seed TWOevents.

All3driver output modes propagate existingbudget plus priorreference/policy/reader/
marker envelopes; continuation capacities charged withshareddata once. Encoder
admits wire/text/atlas/self-decode, temporary cachecap0. Public2GiBdefault/native12GiB/
hostcache4GiB/work/time/gates unchanged. No operators or construction changes.
[Current design](Handbook/ADAPTATION_REFERENCE_PERSISTENCE_20261109.md).

Failed292largecase is PRESERVED:278214/278215passed, expectedexception mistakenly
insideCHECK. reference292_harness_failure.json records it. Test-only293 movedcall
outsideCHECK, chargedretainedstring, productionunchanged. Do notpromote failed292
ordinarycase or its default_refused=false harnessartifact into success.

Demonstrated source mismatch: CNativeReferenceIO3D.cpp EncodeReference calls
DecodeReference(bytes) with defaultReferenceStorageLimit3D=2GiB, while production
LoadReference callers already use configured NativeMemoryCeiling (12GiB here) and
callerreservation. WriteReference passes neither ceiling nor retainedstate caller.
Exacterror is DecodeBytes preflight>ceiling, BEFOREatlasdecode. Current wirebytes/
atlaswords/requested were not captured by failed writer; do not invent their values.

Read-only second-opinion audit confirms mismatch; notvalidation. Investigate ONE
connected propagation of existing configured ceiling and exact retained native
caller through writer/encoder self-check. Keep public defaults, resource refusals,
all canonical/topology/association validation, atomic sidecarpublication and MPI
alignment. No raising ReferenceStorageLimit/configbudget, no bypassing decoder.
Audit encode/write/load callers, actual memoryoverlap and diskroundtrip behavior.

DecodeBytes also estimates integerworkspace from aggregate atlaswords, including
large policy maps. Actual exit maximum_ratio_limbs=3, but arbitrary archives need
sound bounds. Do NOTsimplyremove workspace or reduce bitlimits. Tighter wire
preflight is a separate hypothesis onlyif configured-budget propagation remains
insufficient. Record unrelated alternatives in backlog.

Before substantial edit, state missing capability/evidence/acceptance. Qualify
focusedserial actual/representative archive roundtrip, malformed/truncated data,
resource-exhaustion/no partialpublication; then affectedserial +ONE MPI milestone
at established cadence. Immediately fullsame-seed TWO-event M6 afterqualification.
This fixes demonstrated nextblocker; preserve geometry/construction improvements.

## Qualified geometry integration to retain

Production290/291 (291testonly rebuild) PUSHED2d558a1d4d. CFD SHA
6bb7708715643f9e4941ed85bcc51ccb422ca8b95f4c33a6f1d75f96ada7cc41.
Fresh rigorous mesh→model bound percurrentface via shared EntityFaceError; retain
completeglobal model→mesh, originalBoth fallback for uncertain numericbounds.
Never use acceptedassociation/orientation bool or localG0 asdistanceproof.
ReferenceComponents retainsoriginalBoth. Exactbody/sheet/marker/support/coords/
bindings/budgets maintained. No CAD/config/scheduler/representation changes.

Focused28cases/5616assertions; connectedserial91/67534; ONE MPI6 125cases EVERYrank.
Pins/actualhandles verified betweenstages. Qualification291 explicitlypartial
captured-face diagnostic:17776/69544 atunchanged20Mwork,0overTol,wholefieldUNPROVED
there. Failed290whole-replay expectation remains preserved, not disguised aspass.
Full291 now supplies actual complete nativegeometry acceptance.

All163pre-final matchingstages versus289 counts/flags/comparedwork EXACT. Last
interval25203→25210:573.257515s versus2892460.103865s; new4fullcalls/rank complete,
fullModelToMesh~115s/rank,32618reverseupperqueries/~435–446s/rank (rank5also90private
queries). Finalnative3535.671s versus2895344.632sREFUSEDstorage. Newpeak13.616GB
includes transfer/write; old25.777GBsampled. No additive overlappingtimers claim.
Furtherfacecertificate reuse is backlog while persistence is activeblocker.

Same REALCFDseed R/m6_inputs/euler_original_force_seed_v1; Mach-onlyL1/50k,
M.8395/AOA3.06,CL.269535/CD.013536,original61330pts/303359tets. Native5400/outer14400,
12GiBnativeworking/rank,4GiBaggregatehostcache unchanged. Startup6exe inodes and
84mappedELF/rank matched1520pins. Geometry/shape/size closure achieved without273
cleanup; no wholefield leadingedge cleanup benefit established.

## Remaining obligations and records

Complete event1 persistence, adaptedCFD, event2mesh/transfer/adaptedCFD. Then copy
ACTUAL acceptedmesh+.native_ref+MATCHINGrestart to NEWisolateddirectory; ADAP_LOOP=NO,
RESTART_SOL=YES,READ_BINARY_RESTART=YES,inputSOLUTIONbasename DIFFERENT from output
RESTARTbasename,samequalifiedstandaloneCFD/runtime. Verifycoords/order<=1e-12,finite
fields,positive density/internalenergy and continuedCFD. Mesh00001 pairs FOLLOWING
Cycle1restart00001; mesh00002 pairs FOLLOWINGCycle2restart00002. Current291 supplies
none of these adaptedrestart files. Resume-cycle renumber/inputoverwrite and
rejected-event consumption bugs remainOPEN; isolatedsteadyrestart is separate.
Then dedicatedBL/curvedwalls/junctions/heights/MPI and RANS500k.

[Cleanup investigation](Handbook/ADAPTATION_POST_REFINEMENT_CLEANUP_20261108.md):
original cleanstar selection/zero-debt admission gap confirmed, fouractual262local
feasiblepointremovals, NOTcompletecause or globalLEbenefit. Conditional273cleanup
neverexecuted274/278/283/287/289/291. Do NOTwiden while persistenceactive. Missing
shocktarget allocation staysSEPARATE (sampled4–6%chord demanded vs3–4%existing).
Latest metric-bearing verified inspection remains274VTU; 291actualmesh above is
preservedSU2 but has no capturednodalmetric array. Do not fill fields fromanother
mesh without proving identity. No pertrialVTUs; export when meaningfullyuseful.

Detailed records: [certificate cycle](Handbook/ADAPTATION_PREPARED_SOURCE_CERTIFICATES_20261108.md),
[backlog](Handbook/ADAPTATION_BACKLOG.md), [next work](HowToProceed.md),
[pre-persistence handoff](Handbook/History/HANDOFF_before291_persistence_20261109.md).
