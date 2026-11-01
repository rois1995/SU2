# Native adaptation — current handoff

Updated2026-11-01(Europe/Rome). Goal ACTIVE: TWO strictly accepted ONERA M6 Euler CFD → mesh →
CFD → mesh events at50k metric complexity; actual conservative admissible transfer,
finite continued CFD and isolated usable adapted disk restart. Then dedicated BL/
constraint/MPI qualification and corresponding RANS500k workflow. Actual point/cell
counts are separate from target complexity. Geometry1e-4,q>=.2,J>=.05,maxedge1.8 and
requested heights stay strict. Construction preserves exact positive/nondegenerate/
topology/body/feature/height validity and deliberately omits q/J. CAD optional;
no-CAD geometry authority sufficient. ZERO accepted adapted M6 meshes so far.

## Current —199fulltwoeventM6 LIVE/FROZEN; winding qualification PASS

R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
ACTIVE=`R/m6_runs/euler_build199_real_cfd50k_triangle_coverage`.
Actual directory inherited an old triangle_coverage label from launcher substitution;
this is build199 WINDING screening, not198coverage code. No rename/restart/config
change. R/geometry_winding199_M6_preflight.json records planned/actual paths.
Exec47583LIVE. Source86d292e517 PUSHED;CFDSHA
ea0067da5f39f2cf09b75bbc737bfda14d57fd4decc15b1f5d1454000a6408da.
TestdriverSHAba37efed110714d5b89420991015a01440da4ae454a7bd2398ddde4f83b7ada4.
Only sharedwinding screening changed: PlanarReference::Contains/ContainsFace and
mappedcomponent face winding omit predicates only when ray contribution or full
height separation is proven. Boundary/hole intersection/orientation/support/
association/body/feature/height/final gates remain. No cache/config/scheduler/
budget/CAD requirement change; preserve198construction and197geometrycontinuation.
Design:[winding screening](Handbook/ADAPTATION_GEOMETRY_WINDING_20261101.md).

Build199exec39029TERMINAL0/90.228679s,1296pinsunchanged/all2HOSThandlesABSENT.
Focusedexec25002PASS3306assertions/10cases18.279344s;all5HOSThandlesABSENT.
Serialexec41273PASS23048assertions/72cases241.540099s;all5HOSThandlesABSENT.
SingleMPI6exec41384PASS216cases EACH6ranks333.504807s; all10HOSThandlesABSENT/
pinsunchanged1793526137.041104 BEFOREfullM6. NoCpp/header/test/helper/binary edits
sincequalification. R/geometry_winding199_qualification.json retains exactreceipts.
Same12older168savedquerypayload/bounds EXACT198,cached/recomputed checksPASS;
traversal198551→101600(-48.8293%). Coverage141128→44177(-96951),same1164domains/
790graphcalls/distance32289/lift11489. All12improve;hard48.84–50.50%lesswork.
Selectedpreparation/piececounts unchanged. OlderqueriesarenotlateM6population;
countedwork isnotCPUcost/fullM6speedup/acceptance. R/geometry_winding199_*assessment/
trace_attribution/source_review JSON preserveproof/limits. Read-onlyreviewfoundno
concreteproof-contract defect; not a substitute forruntimeacceptance.

Actual HOSTstartup1793526273.877595: helper1607128/start94120510;
time1607401/start94121590;timeout1607402+launcher1607403/start94121591;
ranks1607407/start94121602,1607408/start94121603,1607409/start94121604,
1607410+1607413/start94121605,1607416/start94121607. All10LIVE/all1401pinsunchanged/
82mappedELFcovered. ACTIVE/owned_startup.json. All6CFDranksnice19/libs1 and actual
TMPDIR+OMPI_MCA_orte_tmpdir_base onowned4TBcompile-tmp/native_winding199_runtime/m6;
ACTIVE/runtime_environment.json. Defaultsharedmemorytransport unchanged. Root/tmp
~15MBfree;4TB~61GBfree. No unrelatedfiles/jobs modified. Preflight40.15GBavailableRAM.
Same savedrealCFDseed/config/originalmesh/native_ref andACTUALcontinued00000
BYTEIDENTICAL latest198retry1. ACTIVE/early_runtime_provenance.json;00000 isORIGINAL
not adaptedrestartproof. CL.269535/CD.013536,rms-5.735442;61330points/303359cells.
Target50kmetriccomplexity isseparate fromactualpoint/cellcounts. Same2events/
native5400/outer14400/strictgeometry1e-4,q.2,J.05,maxedge1.8/heights.

Cpp/headers/tests/helpers/binary/config/inputs FROZEN untilactualterminal orpreserved
failure. No competingimplementation/profiler/newoptions/raisedlimits/gaterelaxation.
Firstcompletecoarsening:292709cells/2627collapses;ALL17selectedper-rankfieldsEXACT
latest198retry1,uppercalls14870same. Traversal158224112→93487444(-40.91454%);
scope/facet/preparation/discoveryworkunchanged. Engine363.104827vs268.138009s;
noisolatedwalltimegain. All18initialsplitgenerations retain17selectedper-rankfieldsEXACT198retry1.
Constructionends727818cells/91910splits,0sizebad/3770geometrybad;q/JUNMEASUREDthere.
Traversal532351350→250860529(-52.87689%),same33088uppercalls/preparation/discovery.
Engine1133.899305vs730.857434s;notisolatedwalltimegain. CompletequalityENTRY
1185.729006s:54081q/53392J/3770geometry/0size,debt25413.45557726506;wholefield/
rankdebts/gaugesEXACT198retry1(entry781.278426s). Initialmigrationglobalcounts/debt/
completeflags/20selectedcachecounters conserved;debtmax/min15.776024→1.538801.
Notproofeverycachekey/history orimmutableoriginaltarget. ACTIVE/construction_comparison.json,
quality_entry_comparison.json,quality_initial_ownership_assessment.json retainactualrows.
All10HOSTidentitiesLIVE/1401pinsunchanged/82mappedELFcovered1793527603.102109.
Entrysnapshot30.52GBavailableRAM,CPUpsiavg6010.53%,memorypsiavg60some.51/full.43%;
sampledsummedRSSmax12.938GBnotphysicalRAM/truepeak. Actualqualityrepairnowrunning.
ACTIVE/quality_entry_live_provenance.json andquality_entry_resources.json preserveevidence.
Actualcomplete
censuses,geometrywork,netwholefieldq/Jbad/s andgeometrycounts chooseutility.
ComparetoLATEST198retry1 atactualmatchingstages/times,notassumedfuturestateidentity.
ZEROaccepted adaptedmeshes/transfer/adaptedCFD/adaptedrestart. FullgoalACTIVE.

## Previous198 terminal evidence — never resignal

Full preserved handoff and exact historical handles/receipts:
[build198 terminal archive](Handbook/History/HANDOFF_Codex_build198_terminal_20261101.md).
Latest198retry1 TERMINAL69b502b359:all6natural5403.711369s rejection;all10HOSTidentities
ABSENT/all1401pinsunchanged1793524505.578266 before199edits. ActualEXIT803395cells,
all6cache/censusflags0:terminalwholefieldq/J/geometry/size UNKNOWN. LastCOMPLETE
window803251cells/2985q/1636J/3098geometry/0size,quality4537.669830s. Final18.564879s/
291publications haveunknowngatechanges. Net51096q/51756J/672geometry tolastcomplete,
46164publications,11.260405netq/s. All7groups audited.37stuck/4162badunion,47453queue.
Constructionreturns0:197continuationunexercised,notdisproved. Noacceptedmesh.

Parent198firstattempt COMPLETEEXIT802821cells/3529q/1903J/3107geometry/0size;
69b/parentterminalcheckpointa604d1c54f. Same-source retrymatchedparentselectedstates/
workthroughwindow21,extra migrationafter22912,firstsnapshotdifference atwindow22.
No causal timing/migrationbenefit claim. Retryterminal_comparison.json showsactual
parentbracketingtimes,notinterpolation. Geometrycostskew/unprovenownershipalternative
andnegative motionbound evidence remain in[certification analysis](Handbook/ADAPTATION_GEOMETRY_CERTIFICATION_20261101.md)
andbacklog. No furtherunchanged-source retry selected. All144xxxx/129xxxx historical
handles mustNEVERberesignaled. Currentownedhandles are199listedabove.

## Historical evidence — all terminal,never resignal

Full build191–197 handoff detail is preserved in the
[historical archive](Handbook/History/HANDOFF_Codex_build191_to197_20261101.md),
including exact handles, source/runtime/input receipts, terminal failures and all
prior counter/geometry analyses. Historical ACTIVE notes never override CURRENT.

| Run | Preserved measured outcome | Limit |
|---|---|---|
|197|Lastcomplete2299q/1062J/3079geometry|TerminalEXITincomplete;returnbranchunexercised|
|196|Firstshape-pass47q/26J/3051geometry|Thenbroadcoarsening;terminalEXITincomplete|
|194|CompleteEXIT4369q/2352J/3111geometry|Noacceptedmesh|
|193|CompleteEXIT8628q/5768J/3151geometry|Noacceptedmesh|
|191|CompleteEXIT16191q/12965J/3509geometry|Noacceptedmesh|

197/196 true terminalwholefield q/J/geometry/size areUNKNOWN; do not substitute
cached subsets or earlier complete states. All natural failures were preserved
BEFORE verified owned-launcher stops; all exact identities/pins independently
verified terminal before subsequent source edits. Full receipts remain in R/m6_runs.

## Next monitoring and acceptance

199fulltwoeventM6 isLIVE/FROZEN afterconnectedqualification. Keepitfrozen until
actualterminal orpreserved authorizedfailure. Comparecompletecensuses andcounted
work versus198retry1 withoutassumingidenticalfuturestates. No newimplementation
whilehealthy. Firststrictacceptedmeshmustlead toactualconservativeadmissibletransfer,
finitecontinuedCFD andsecondstrictacceptedmesh; preserveeachverifiedstage. Isolated
adapteddiskrestart onlyafterallownedrunhandles terminal. ThenBL/MPI andRANS500k.

Analysis caution: historical matching-window summaries assume the first quality
pass. The retry1 stored command now distinguishes published repair churn from
whole-field construction changes if the197return crosses construction. Adapt analysis assertions to
actual phase/census coverage; an analysis assertion alone is not a solver failure.

Read-only downstreamreadinessreview:
R/m6_runs/euler_build196_real_cfd50k_witness_first/workflow_acceptance_readiness.json.
Existing175transfer_restart_acceptance_source_review.md and audit_native_unsteady.py
were located/reused; P1integralaudit needs IMMEDIATEposttransfer states,notlaterCFD.
Existing workflowreceipts alone are insufficient transfer/state/restartproof.
Actual pinned distributedtransfer unconditionally gates finite trueresidual<=1e-10,
fulladmissibility and1e-12scaledtargetintegrals before publication; inspect ACTUAL
per-eventprojectiontable/source provenance. Donor-vs-newintegralchange is a different
quantity (CLOSEDsliverrule/open-boundary domaincorrection). Existing validate_state
can be reused on an isolatedcopy of ADAPTEDmesh namedm6.su2 and actualmatchingflow;
functionuses parsedpointcount,not61330hardcode. Thenactual RESTART_SOLYES/ADAP_LOOPNO
probe with distinctinput/outputprefixes onlyafterownedrunterminal. No newhelper/
source/configchange now; no adaptedstate/restartclaim.

Existing closure plan: shape/height → residual size → geometry-only repair via
EnableGeometryRepairs/strict complete-patch guidance,or valid construction return,
then shape remeasurement/reclosure.Neither191nor193reached geometry-only entry. Same
cumulative5400s/work/memory limits. Require TWOstrict accepted mesh/native_ref pairs,
actual conservative admissible transfer(double residual1e-10,targetintegral1e-12),
finite continued CFD,rho/internalenergy>0. Then isolated COPYfinal adapted mesh/
native_ref+matchingflow,actual RESTART_SOLYES/ADAP_LOOPNO/correct SOLUTION_FILENAME/
distinct outputs,coordinate/order/state audit. Original seed00000 is NOT adapted
restart evidence. Then dedicated BL curved/junction/interface/subsequent-operation/
redistribution/requested-height MPI qualification and RANS500k. Goal stays ACTIVE.

Readonly seed R/m6_inputs/euler_original_force_seed_v1:61330points/303359cells,
Mach.8395,AOA3.06,CLmean.2695353537415/CDmean.0135358565364,last200ranges1.114e-7/
3.121e-8,finite/rho/IEpositive/exact coordinate/order. Userforce-policy qualified,
not a residual-converged aero reference. SeedSHA1eaee788bd62bd92a92ec539577dcbcfc0f8042dcbcec23adb04e825bb01a880.
Reuse per first remesh; distinct SOLUTION_FILENAMEbaseline_euler_seed/RESTART_FILENAMErestart_flow.

Open limits: historical191transported nodal tensors drifted~8.3%at16fixed samples;
193/196/197samples are recorded with their actual windows in the archived evidence.
Neither runtime tags nor these samples prove immutable original continuous-CFD-target
fidelity. General
adaptation resume cycle0/path collision and rejected-cycle consumption remain open;
isolated solver restart does not prove general adaptation resume fixed. Broad182MPI
timed out; serial_full170 NOT passing. BL/RANS qualification not started.

## Operating links and discipline

[AGENTS](AGENTS.md) · [complete task](Handbook/ADAPTATION_INTEGRATED_NEXT_TASK.md) ·
[winding screening](Handbook/ADAPTATION_GEOMETRY_WINDING_20261101.md) ·
[certification analysis](Handbook/ADAPTATION_GEOMETRY_CERTIFICATION_20261101.md) ·
[envelope](Handbook/ADAPTATION_GEOMETRY_ENVELOPE_20261031.md) ·
[memo](Handbook/ADAPTATION_NUMERIC_MEMO_20261031.md) ·
[ownership](Handbook/ADAPTATION_CENSUS_OWNERSHIP_20261031.md) ·
[geometry/smoothing](Handbook/ADAPTATION_GEOMETRY_QUERY_REUSE_20261031.md) ·
[feature](Handbook/ADAPTATION_FEATURE_PUBLICATION_20261031.md) ·
[saved CFD](Handbook/ADAPTATION_M6_CFD_SEED_20261031.md) ·
[cache/ownership](Handbook/ADAPTATION_CACHE_PHASE_20261030.md) ·
[backlog](Handbook/ADAPTATION_BACKLOG.md).

One owned heavy job,nice19,libs1,build2–4workers(helper3),MPI6bounded; TMPDIR4TB
compile-tmp,not nearly full root/tmp. Check host RAM/load/disk; sampled summedRSS
is not physical memory/job peak. Preserve resource/pin/handle receipts,leave other
jobs alone. Fork pushes authorized:originhttps://github.com/rois1995/SU2.git branch
codex/native-3d-bl. Git metadata outside writable root:escalated git -c gc.auto=0
-c submodule.recurse=false; diff/status --ignore-submodules=all. Whitelist adds; no
binaries/meshes/restarts/rawperf. Leave unrelated155logs/.orig/submodules/untracked
steering/history files untouched. Primary owns integrated design/source/acceptance;
bounded read-only routine agents done, no independent jobs.
