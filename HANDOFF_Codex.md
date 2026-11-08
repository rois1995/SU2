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

## CURRENT —260 TERMINAL REFUSED; diagnose the FOUR actual residuals

[Currentdesign/evidence](Handbook/ADAPTATION_GEOMETRY_PRIORITY260_20261108.md).
Source checkpoint c04a9b6491c863f2fa6218df3dca20314df9aadc unchanged.
Tool89696 TERMINAL1/5566.772822s;NOownedheavyjob now.
Firstevent explicitly REFUSED beforestop. Finalexit37121/native5340.295245:
734313cells,ALLmeasured,0q/0J/4geometry/0size,allnativecensusflags complete.
TWOaccepted events impossible,so after ALL30rawfilessealed andALL1520pinsverified,
ONEpidfdSIGTERM sent toownedMPI2214416/start15601692. A260/owned_stop_once.json
attempts1/signal_sent. All11exactownedhandles ABSENT(A260/terminal_handles.json).
NEVERrepeat this or any historicalstop. Runtime/source maynowbe edited only for
ONE evidenced connected nextcorrection after bounded actualresidual diagnosis.

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
R/capture261_plan.json. Compact9-fieldJSON implemented,NOnewbuildyet. All584311
actualmetadata rows roundtrip EXACTLY throughprototypeANDproductiondecoder.
Rank2metrics134MB→66.0MB,rank5→63.7MB;missingrow extrapolation NOTfullcaptureproof.
Reader14testsPASS;legacyLE scientificfieldsidentical. C++producer/tightcaptest
UNBUILT/UNVALIDATED. Nextfreeze/build261 withlaunch_build261.py (CFDbinarymust
staybyte-identical),focusedfailure_capture+actualCPPofflineinspection,thenfullM6.
Qualifyaffectedlocalformat/caps/associations,thenimmediatefullsame-seedM6retry
torecoveractualmesh/support;no timingbaseline/newproductionpolicy.

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
