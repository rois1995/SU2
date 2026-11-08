# Native adaptation — current handoff

Updated2026-11-08 Europe/Rome. Goal ACTIVE;ZERO accepted meshes.
TWO strictlyaccepted ONERA M6 Euler CFD→mesh→CFD→mesh at50k METRICCOMPLEXITY,
conservativeadmissible transfer,finite continuedADAPTED CFD andACTUAL isolated
ADAPTED diskrestart. Then dedicatedBL/constraints/MPI qualification andRANS500k.
Actualpoints/cells areseparate. Passingfixtures/balancedweights/lowerwork are notcompletion.
Strictgeometry1e-4,q>=.2,J>=.05,maxedge1.8,actualrequestedheights. Validity-first
construction:NOunnecessaryfreshq/J/ranking/veto. Exactpositive/nondegeneratecells,
topology/body/features/seams/heights andfinalcertification preserved. CADoptional.
ONEactivecycle/ONEownedheavyjob,nice19/libs1/build-j3/4TBscratch. Nevereditpinned
compiled/executing C++,binary,helpers/config/runtime/inputs duringbuild/check/run.
Git snapshots anddocumentation allowed. Forkpushes authorized.

R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
A260=`R/m6_runs/euler_build260_paired_geometry_priority_mach_only_real_cfd50k`.
A256=`R/m6_runs/euler_build256_canonical_geometry_debt_mach_only_real_cfd50k`.
A254=`R/m6_runs/euler_build254_conditioned_mach_only_real_cfd50k`.

## CURRENT — COUPLED REPAIR267 FULL M6 RUNNING/FROZEN

Build267tool62297 TERMINAL0/217.199605s,allsource/helperpinsunchanged.
CFDSHA2ec933ebe76e432ff38f5bbe9d5920b965bda402560a1a8a0c20000c3543c7e5.
All18ownedbuildhandlesabsent. Serial51722terminal0/158.825857s:65cases/56523assertions,
allpinsunchanged/allfivehandlesabsent. Case current_production_20261108T101712_3134567.
Includesusefulactual42-cell g4→3/sourceqJ0 andprevious4096-work phase-return.
ONE MPI6tool81849terminal0/180.424182s:99cases onEACHrank,allpinsunchanged/allten
ownedhandlesabsent. Case placement_mpi6_20261108T102227_3148251. Assertions
60231/60372/60027/60398/61113/60283(printorder,notrankmapping).
Provenance-onlyfirstMPIpreflight78262launchedNOtests;oldreaderexactbytesarchived
028b7184ff fromb240f86b35;unchangedpayloads/57sourceinputsverified innewimmutable
A262/placement267_reuse_v1. Originalreceipt andfailedpreflight preserved.
R/coupled267_qualification.json SEALEDallrequiredpass/ONEactualcount-retiringcandidate;
allsource/runtime/input/helperpinsverified. ONE owned fullM6 tool68708 RUNNING/FROZEN.
A267=R/m6_runs/euler_build267_coupled_geometry_repair_mach_only_real_cfd50k.
Sourcecheckpoint9011bab923,qualifiedCFDSHAabove. Wrapper3158528/start17803942,
runner3158802/start17804580,MPI3159077/start17805236;time3159075/timeout3159076,
SIXtestdrivers3159101–3159106. A267/owned_startup.json records ALL11exactidentities.
SamequalifiedREALseed SHA1eaee788...01a880;Mach-onlyL1/50k,TWOevents,native5400/
outer14400/12GiBrankworkingstorage/hostcache4GiB unchanged. Initial original-grid
CFDrestart alreadywritten;NOTadaptedproof. Initialverifier267checksall22fields.
Initialverifier COMPLETE:all61330coordinates/all22fields bit-identical256/qualifiedtarget;
configbyte-identical262/SHA5ad2dc1d...22b89e. Firsttwo completeconstructioncheckpoints
(round1627 length_collapse_exit/ownership_staged):293202cells/2495collapses,
2972geometrybad/73682sizebad;shapeUNMEASURED. Counts/flags/listedgeometrywork
EXACT262,all1520source/runtime/inputpinsunchanged. A267/first_construction_equivalence262.json.
Initialqualityentry23314/native691.831276:690715cells,ALLmeasured,27016q/27453J/
1205geometry/0size,allcensusflagscomplete. EXACT262counts/flags andALLnon-time/
non-byte rankfields across83completegroups;all1520pinsunchanged. A267/
quality_entry_equivalence262.json sealsproof/prefixhashes. No timing speedup or
mesh-bitidentity claim. Continue frozen through shape/geometryclosure.
All C++/binary/runtime/helpers/config/inputs FROZEN. No other ownedheavyjob.
Letthisattemptfinishunlesspreservedevidence meetsauthorizedstopconditions.
Compare firstconstruction/qualityentry counts+work againstcomplete262;newrepair
activatesonlygeometryrepairmode. Judge wholefieldq/J/geometry and usefulcounted
progress,notcommits/canonicaldebt alone. No extra baseline/perf/GDBcampaign.
267restrictsnewgeometryrepair topresentfailed-faceedges;raw/finalcandidate must
retirefailedphysicalfaces withoutincreasinggeometrybadcells,plusstrictcanonical
excessdescent,closedshape,size,heights,contact,publication. Existingoperators,
64supportbound/160work/nodeIDs/reservations/memory/timebudgets;constructionunchanged.
266serialfailure preserved:64of65passed,4096workexhaustedbeforephasereturn.
No266MPI/M6wasrun. 267fix qualifiedthatfailurewithoutraisingbudgets.
[Currentdesign](Handbook/ADAPTATION_COUPLED_REPAIR266_20261108.md).
Sourcebase988090d1e4 plus connected count-directed repair correction and fixture parity
checks. Construction remains unchanged. Read
[design/qualification](Handbook/ADAPTATION_COUPLED_REPAIR266_20261108.md).
Production extends existing complete-core support import before reservation into
geometry repair; same64 bound/version checks. Geometry-repair-only existing edge
split plus bounded optimizer, then unchanged source/shape/geometry/size/height/
contact/publication gates. Shared160 work and existing node range; additional
concurrent frozen field admitted explicitly. No new config/operator/scheduler.
Only compiled files: Common/src/adaptation/CNativeEngine3D.cpp and
UnitTests/SU2_CFD/adaptation/fixtures/native_current219_witness.inc.
Qualification passed; immediately run full same-seed two-event M6. No accepted mesh yet.

265 build terminal0/209.303411s, allpins unchanged, old production CFD byte-identical;
allten build handles absent. Focused serial tool98076 terminal0/98.567218s,
45549 assertions, allpins unchanged/allfive ownedhandles absent. Actual complete
42-cell context reproduces39finite q/J and252edgeoracles;threeunknown q/J explicit.
Six edge controls yield THREE source-gated split/reclosure candidates, allq/J0.
8184–205050218 plus ONE boundarymove189402863→newID reduces physicalbadfaces/
geometrybadcells4→3, canonicalexcess4.533680586e-6→4.533675765e-6, maxedgeunchanged.
8184–201067564 plus78optimizertrials reducesexcess36.18% butretains4badcells;
8184–294622922 plus79trials also reducesexcess with4badcells. Counts remain primary.
R/closure265_assessment.json preserves fullrows. Localcandidates areNOT nonlocal
publication/contact/acceptedmesh proof. 266 compares existing SplitSurfaceStrip
(empty moves, complete context) against ring split+unchangedneighbors before
source-gated optimization. Legacy fixture behavior retained.
263 serialpassed40107assertions;264 compilefailure preserved (nonexistentbad_cells
member), corrected in265 after allhandles terminal. No264casewasrun.
[Prior diagnostic/source semantics](Handbook/ADAPTATION_CLOSURE263_20261108.md).
No immutable-original-target policy introduced: current nodal transport/rebasing
is production authority. CADoptional/allgates unchanged/fullgoalunchanged.

## Build262 — terminal refused; complete saved mesh, q/J partial

A262=`R/m6_runs/euler_build262_compact_capture_mach_only_real_cfd50k`.
Tool46776 TERMINAL1/5597.155218s. Firstevent EXPLICITLYREFUSED on native metric-update
elapsed-time budget at round36312. Actual734315cells;4geometry/0size; terminal
q/Jwhole-fieldcounts UNKNOWN because shape_complete=false onrank5. Earliercomplete
0q/Jframes areNEVERsubstitutesforterminalcounts. ZEROacceptedmeshes.
ALL30rawfiles sealed/stable/withinUNCHANGED128MiB caps;ALL1520pins verified BEFORE
ONEpidfdSIGTERM toMPI2598241/start16555245. A262/owned_stop_once.json attempts1/
signal_senttrue;NEVERrepeatstop. ALL11exactownedidentities absent in
A262/terminal_handles.json. Source/runtime/inputs remainedunchanged. NoMPIjobnow.
Capture262FIXPROVED actualownedcoverage734315/734315,ALLsixowned_complete and
complete_shardTRUE,includingemptyrank3. Metricscompletefalse iscachedshapeunknown,
NOTlostrows. DoNOTclaimcompleteq/Jcensus. Fullactualexport/readback COMPLETE;allpriorheavyhandles terminal BEFORE263buildabove. Tool76274 TERMINAL0/
338.682921s,ALL1557pinsunchanged,all3hosthandlesabsent. Independentreader43849
TERMINAL0/4.882237s;exacthostscriptabsent. Seeinspection_export_terminal_handles.json,
inspection_reader_status.json andinspection_reader_terminal_handles.json.
LATEST COMPLETEsavedmesh A262/inspection/rejected_mesh.vtu and.su2:
148608points/734315tets;captured_nodal_metric9rowmajor inPointData; q/J/geometry in
CellData withcorrectlengths. ALLtensorsfinite/SPD,alltetvolumespositive,VTU/SU2coords/
connectivityidentical. FOURcellsq/JUNKNOWN;knownq/Jbad0 isNOTglobalq/Jclosure.
4geometryfailures. ReaderpartialTRUE solelyrank5metricsincomplete+4unknowncells;
fulltopology/tensorfield COMPLETE. GenericSU2tagsMarker_0/1/2 areinspectionfallbacks,
NOTapprovedsolverinput. Rawlargeartifacts local;code/hashes/metadata arepushed.
All4knowngeometrycore globalvertexstars EXACTLYmatch122savedcavitycells. Per-core
four-starunions107/42/98/97;vertexstarmax86. Existing64boundexcludes3whole-core
contexts. COMPLETE42cellcore155561218coversALL4geometrybad and3of4unknownq/J;
unknown223403263liesoutside42. All4geometrybadshare8184 andownactualWING3Dfaces
(ref10821/16404,body0/sheet10154,y≈.313–.319). TwofeatureflagsKNOWNfalse;twoUNKNOWN,
so doNOTinfer absenceofgenuinefeatures fornewcells. Seegeometry_residual_location.json.
No largerboundneededforfirst42celltest;preserveunchangedoutsideinterfaces/neighbors.
Nextsingleactivecycle:classifyactualfaces/referencebindings/featureunknowns,choose
completeexisting<=64context,reproduceproductioncurrent-source transport/rebasing atchangedtrialpositions,
then test ONEexistingcoupledcandidate hypothesis. Unknowncachedq/J MUSTNOTsilently
passNaN-basedexpectedmetric comparisons. No productioncorrectionselectedyet.


Build262passed81.191946s;1303sourcepinsunchanged;productionCFDbyte-identical
0563a1b5a75ee8b6ae0de9a4464e275412457ec42ba1a764b7775e724c89b7c8.
Focusedserial690assertions;MPI2 690/140with4actualemptyrankgenerations;
R/capture262_qualification.json verifies32CPPexports/8legacycapcontrols. Reader14tests
andall584311old260metadata rows roundtrip exactly;legacyLEscientificfieldsidentical.
Build261failedONLYnewtestcapassumption;correctedtestcontrolpreservedandqualified262.
Captureencoding/header checkpointc88bf6402a;correctedcoherentcheckpoint5484de33d2.
InitialCFDall61330coords/22fields bit-identical256/qualified254,Mach-onlyL1/50k.
ALL112initialthroughfirstqualityexit per-rankfields excepttime/bytes EXACT260;
A262/first_quality_exit_equivalence260.json. Firstexit24328/native2985.719094:
732281/2q/0J/969g/0size (allmeasured). Actualshape-clean geometryentry24717/native
3284.905332:733973/0q/0J/344g/0size. Subsequentlygeometryrefinement reintroducedshape,
thenreclosed. ALL357matchedstages through4gregimehadidenticalcounts/flags/comparedwork
vs260;NOTmeshbitidentity/timingspeedupproof. All1520pins checkedatfirstqualityexit.
Sourcecheckpointf8bd84aa8d pushedfirstqualityproof;01f2f0e488 pushedcurrent4gdiagnosis.

Terminal1135complete4gframes:485commits=242splits+243quality-progress,244211upperqueries
ALLexactmemo hits/0misses/0upperwork/0traversal. Two scalar734313/734315states andtwo
canonicaldebts;NOTmeshidentityproof. A262/late_four_geometry_cycle_assessment.json.
Terminalexitnative5340.187351s has4q/JUNMEASURED;lastcomplete36311/native5338.503479
has734313/0q/0J/4g/0size. DoNOTsubstitutethisforfinalshape. ALL2034matching measured
stages equal260counts/flags/comparedwork;terminal_comparison260_summary.json.
SampledWHOLEownedtreepeak13.984GB;3.054GBindividualpeak isNOTwholejobpeak.
A262/terminal_assessment.json setsnexttargetrepairpower/construction,NOTfreshqueryspeed.
Blockerconfirmed repair/constructioncycling,notfreshcertification. Fullterminalcounteranalysis completedONCE aftermesh export;no repeatneededabsentnewfailure.
[Capture/nextvalidationguards](Handbook/ADAPTATION_CAPTURE261_20261108.md).
Existingfixtureusesold-connectivity CURRENTnodaltensors;productiongatesrebasetensors
ontocandidateconnectivity. Changedtrialmetric/edgeequivalence mustbeproved. Geometrymode skipsordinaryoptimize_repair;
rawcollapse+retainedstar optimization isONLYhypothesis untildemonstratedonactual
COMPLETEcurrentstars+neighbors withstrictgeometry/shape/size/height gates.
NOproductionrepaircorrectionselected;no competingimplementation. KeepCADoptional.
BoundedREADONLY Luna survey /root/current262_field_facilities COMPLETE;primaryreview
confirmed CavityMetricSource::ImportMetricSamplesLocal andPublishMetrics usecurrent
nodalsensors andcandidateconnectivity,NOTstaticoriginalglobalbackground. M6composition/
edge_breakpoints EMPTY byGetnAdap_BL0+factory. No target-transportpolicychange.
ACTIVE [source-faithful diagnostic263](Handbook/ADAPTATION_CLOSURE263_20261108.md):
hiddenfixturevariant+actual42context;explicitunknownoracles/candidate-sourcegate,
thenmeasuredsplit/reclosurefollowup264.
No productioncorrectionselected. Sourceeditscomplete;263buildaboveisnowFROZEN.

Currentgeometrypriority260 productiondesign/evidence:
[Paired-face geometry priority](Handbook/ADAPTATION_GEOMETRY_PRIORITY260_20261108.md).
Frozenbuild/run obligations apply tothenextsingleownedexport/check;noC++editsuntil
requiredsource/build/validation handles terminal. Forkpushes authorized.


Build260 tool89696 TERMINAL1/5566.772822s; all11 owned handles absent.
Firstevent explicitly REFUSED beforestop. Finalexit37121/native5340.295245:
734313cells,ALLmeasured,0q/0J/4geometry/0size,allnativecensusflags complete.
TWOaccepted events impossible,so after ALL30rawfilessealed andALL1520pinsverified,
ONEpidfdSIGTERM sent toownedMPI2214416/start15601692. A260/owned_stop_once.json
attempts1/signal_sent. All11exactownedhandles ABSENT(A260/terminal_handles.json).
NEVERrepeat this or any historicalstop. Source was unfrozen after260 for the capture correction; freeze all pinned sources and helpers during the next M6. No competingproductioncorrection.

A260/capture_ready.json:PARTIAL584311/734313owned rows;rank2=207520/290036,
rank5=201238/268724 duepaired128MiBmetricstreamcaps. Rank4 andothers complete.
Native finalshape IScomplete;rawpartialcapture doesNOTprove independentfullmesh
recomputation/globalcurrentstars/full260VTU. Checkedlocalizer findsall4seeds/59
availableneighbors;ALL8involvedvertexstars failnecessaryfaceclosure. Thisblocks
completeactualcavityprobes;seeavailable_star_closure.json. No baseline/perf/GDB.
Latest COMPLETE fullmesh withmetric remains254inspection;doNOTpresent partial260
as a complete mesh. GoalstillZEROaccepted;transfer/adaptedCFD/restart unproved.

Firstqualityentry23314 matched256exactly:690715cells/27016q/27453J/1205g/0size,
upperwork245091394. ALL112pre-priority non-time/non-byte rankcounters matched256.
Firstgeometryscoring2285probes/13662faces took98.552601s transition(maxrank5
97.072809guidance). Freshquality24492:732861/208q/257J/595g,vs256813g/187q/202J.
374vs156g removed,but68.397496Mvs3.177143Mupperwork;notanisolatedspeedupclaim.
Firstactualshape-clean24717/native3216.817235:733973/0q/0J/344g/0size.
Complete55g snapshot pushedinb240f86b35;subsequently reachedthe4g regime.

Late4g completeframes alternate734313↔734315cells andcanonicaldebt
5.2032901988e-7↔4.5336805862e-6. Scalarstates doNOTprove meshidentity.
Preserved3659.872694→4043.537094 interval:121commits=60splits+61quality-progress,
60819upperqueries ALLexactcachehits,ZEROadditionalupper/traversalwork.
A260/late_four_geometry_cycle_assessment.json. Thisdemonstrates residualrepair/
construction cycling,notfreshcertification traversal,asnextblocker. Inspectactual
faces/referenceassociations andcompleteadjacentvolume candidates using existing
operators/support/certificates;doNOTrestoreblanketconstructionmeritveto or start
anunrelatedcache/scheduler/cavitysize/metric-policy campaign.

ACTIVE follow-through: [bounded capture prerequisite](Handbook/ADAPTATION_CAPTURE261_20261108.md),
R/capture261_plan.json. Compact9-fieldJSON is qualified in coherent build262.
All584311 actual metadata rows roundtrip exactly; rank2 metrics134MB→66MB,
rank5→63.7MB. Extrapolation is NOT proof of future full capture. Immediately run
full same-seed M6; verify actual coverage before exporting a complete mesh or
probing current residual stars. No additional production repair change selected.

Build260tool12364TERMINAL0/72.144637s,all1303pins unchanged,both targetsuptodate,
all5recordedhosthandles terminal(R/build260_terminal_handles.json).
Focusedserialtool78487TERMINAL0:current_compound_20261108T040424_2174583,
PASSED58cases/48598assertions/160.800347s,allpinsunchanged/all5hosthandles terminal.
ONE MPI6tool47079TERMINAL0:placement_mpi6_20261108T040816_2184910,
PASSED99cases onEACHrank/177.042167s,allpinsunchanged/all10hosthandles terminal.
Assertions60284/60406/60023/60231/60203/61109(printorder,notrankmapping).
ANSIcoloredsummary parsing fixed duringterminalaudit;NOtest rerun/signal.
R/geometry_priority260_qualification.json seals bothstages,all20resolvedpredictions,
14actualcompletecavitypredictionDELTAs(maxrelativeerror6.45e-17),andpreservedcanonical
rawcontractioncontrols. Qualifiertool43397TERMINAL0. Runtimeuse wassubsequently
measured in260above;strictacceptance remainsUNPROVED. DoNOTrepeat successfulchecks
absentchangedscope/failure.

## What changed and invariants

Only explicitrefine_geometry returns rank EXISTINGgeometry_queue bypredictedNET
canonicalexcessreduction over BOTHactualphysicalfaces in COMPLETEruntime accepted
BoundaryParameterState. FreshPrivateParameters,sameBindNewPoint(midpoint,allfacets),
ContainsFace beforeparent/fourchildvalidatedbounds. SmoothsameEntity/Mobility2/
noacceptedfeaturetrace only. Composition/actualheights bypass predictor because
actualsplitpointmaydiffer. ActualM6compositionEMPTY(GetnAdap_BL()==0);full_target1
isNOTproof ofnonnullcomposition. NoEuler/RANS-labeldispatch/CADdependence.

Initialconstruction EXACTlegacy. SerializedCellStategeometryscores/census unchanged;
fair/worstrepairqueues,rejectedhistory,alloperators/gates/budgets unchanged.
Phase-localEdge→scoremap admittedBEFOREallocation/inSelection.Bytes,notmigrated;
allqueueinsert/eraseuseeffectivepriority. Successfulrank(1,2],ordinaryfallback
raw/(1+raw)∈[0,1),NEVERveto. AtlasStorage/cumulativework/cachequotas reused;
work/byte/bitexhaustion propagates. Native deadline checkedbetweenoptionalprobes.
Fullvertexclaims retirechangedpaired-supportentries withinclaimed surfacepass.
Surfacequeueexhaustion CLEARSmap/mode BEFOREinteriorhandoff;reopened/resumedentries
uselegacyrawkeys. ChangePhase/Reset clear,SwapState swaps both. Build259review
caughtthislifetimegap BEFOREqualification/M6;259terminal0butNEVERrun/qualified.
DoNOTrestore blanketconstructionmeritveto(7e24ddebb06 wasintentionalimprovement).

## Evidence driving260 and comparison obligations

256 reached actualshape/sizeclosure butstalledgeometrycycles:repair734246/0q/0J/
73g/0size versusconstruction734280/78g. Final31454/native5340.832826 has68q/J
UNMEASURED;finalwholefieldshape UNKNOWN. Lastfull31442/native5338.745886 has
734246/0/0/73/0,allcompletenessflags true. Neverpresent earliercleanframe asfinal.
A256/terminal_assessment.json andlate_geometry_cycle_assessment.json preserveit.

257all78actualWING3Dfacebounds reproduceEXACTLY,alloneface/none near genuinefeature;
threechangedconstructionfaceshold94.8785%of squaredexcess1.466890995618264e-5.
256capturePARTIAL584336/734280owned,49301/68962physicalfaces;23badseeds lackq/J.
NOcomplete256VTU/globalstarproof. DoNOTinfer globalcurrent support from partialshards.

258 usedALREADYcomplete254contexts:20rawboundaryedges/14constructed/6inversions.
Complete50cell174743936 SECONDlongestedge18795-202090700 removesphysicalbadface7→6,
g7→6,debt7.6299890926e-7→7.6153322748e-7,NOq/Jchurn(q6/J4),maxedge/errorunchanged,
points33→34,replacement/interface/GeometryRefinepass. Longestedgeadds1q/1Jwithno
geometrygain. Thisjustifiespriorityhypothesis,NOT allcavityfeasibility orclosure.
Fullnonlocalcontact/publication andoriginalfrozenfield atchangedtrialpoints remain
productionobligations. Detailed257/258stageproofs andhistorylinkedincurrentbook.

NewfullrunmustuseSAMEsavedREAL original-gridCFDseed euler_original_force_seed_v1:
61330points/303359tets,M.8395/AOA3.06,userqualifiedstableCL.269535/CD.013536,
NOTfullshock/residualconvergence. ONLYSENSOR_MACH;pressure/Cp remainphysicaloutputs.
L1/50k/configSHA5ad2dc1d9a1cd8bff87f633617b191b50defbc261ab3d0b2ed0987a34722b89e,
Native5400/outer14400,hostcache4GiB/rank715827882 unchanged. Verifyactualnewinput/
config/initial22fields andconstructioncounts/work against256,notwalltime alone.
256firstquality690715cells/27016q/27453J/1205g/0size,upperwork245091394.
Predictiononlylategeometryreturn shouldleaveinitialconstructionunchanged;CONFIRM.
Comparewholefieldq/J/geometry andusefulcountedwork atmatchingstages. No forecast or
isolatedspeedupclaim from walltimes. No baseline/perf/GDB campaign.

[Targetdiagnosis](Handbook/ADAPTATION_TARGET_DELIVERY253_20261107.md):sampledcompression
spacing4–7%chord versusoriginal3–4%explainslittlefirstrefinementTHERE;notresolved
lambda-shockproof. CorrectedHessianconditioning254fixes71rawHessians,flowunchanged;
farfield40.398%complexity remainsunqualified. KeepMach-onlyL1/50k target. LE254
exactmetricedges.13–.19 and8–15xoriginaltriangles;necessarygeometryspacingnotproved.
Preserve219localfloor/lognormalization and253witnesscutoff/254conditioning/256canonical
guidance improvements. No mandatorycurvaturemetric/minedgegate/normswitch.

## Historical owned stops and latest COMPLETE saved mesh

256:all30partialrawfiles sealed/1520pinsverified BEFOREONEpidfdSIGTERM toowned
MPI1753220/start14514129 AFTERfirsteventrefusal. A256/owned_stop_once.json attempts1,
all11handles absent terminal_handles.json,tool10984TERMINAL1/5612.727731s.
254:all30completefiles sealed/pinsverified BEFOREONEoldMPI1274730/start13602153stop;
all11absent/tool70856terminal1. NEVERrepeat stops. 253alsoallterminal afterONEold
MPI871976/start12808268stop;partialterminalq/Jnotwholefieldcounts. No oldsolverlive.
Original-gridCFD/adap-namedfiles afterrefusal areNOTaccepted/adaptedproof.

Latest COMPLETEinspection A254/inspection/rejected_mesh.vtu and.su2:
150080points/741236tets,PointData captured_nodal_metric9rowmajor;
CellData q/J/geometry_excess_l2/productionedgebounds. All260bad+2400neighborcells
andreference saved. Export55016terminal0/329.411249s andindependentreader22015
terminal0/2.681038s,allpins/inputsunchanged/allhandles terminal. VTU/SU2coords/tets
identical,arrays/associationslengthscorrect,metricsSPD,alltetvolumespositive,
counts251q/205J/95gexact. NOTacceptance. Userwantsonlymeaningfulprogressedmeshes,
NOTpertrialexports. Rawlargeartifacts local;hashes/metadata/codepushed.
Fulloldhandoffarchived Handbook/History/HANDOFF_before260_full_start_20261108.md;
historicalnotes neveroverrideCURRENTlivehandles.

## Acceptance,transfer,restart and remaining full goal

TWOstrictacceptedEuler events required. DoNOTtrustworkflow_accepted alone.
A256/transfer_restart_readiness.json sourceaudit(runtimeverified=false):actual
CV-masstrueresidual<=1e-10 EACHfield/timelevel,fullstateadmissibility andscaled
TARGET-integraldefect<=1e-12 BEFOREpublication. Sourceguards unchanged. Summary
prints trueresiduals,NOTnumericaltargetdefects;distinguish executedhardgateproof
from anactuallyobservednumber. Old/newdomainintegrals/postCFDevolutioninsufficient.

Requireactualacceptedmesh1/2+.native_ref andmatchingrestart1/2:exactpoint/header/
coordinateorder<=1e-12,finiteconservedfields,rho/internalenergy>0. Reuseexisting
run_m6_seed.validate_state throughisolatedmatching-gridaliases. AfterFULLowned
workflowTERMINAL,pin/copyACTUALadaptedmesh/ref/restart intoNEWisolateddirectory;
ADAP_LOOP=NO,RESTART_SOL=YES,READ_BINARY_RESTART=YES,explicitactualcheckpointbasename
andDISTINCTSOLUTION_FILENAMEinput/RESTART_FILENAMEoutput. Runactualsamebinary/
runtimeCFDcontinuation,verifydiskload+matchingorder+finite/admissibleoutput.
Resumecycle0overwrite/rejected-eventconsumption bugs OPEN;isolationisNOTafix.
Preserve/reuse independentlyqualifedsuccessfulstages withfullprovenance.

Then dedicatedBL/MPI qualification:curvedwalls,junctions,interfaces,subsequentops,
ownership/memory/bounds/exhaustion/requestedheights;generic99cases NOTdedicatedBL
proof. ThenRANS500k. Geometry/metric/heights determineoperators,no scenariolabels.
Goal ACTIVE;doNOTclaimcomplete from diagnostics or one accepted event.
