# Historical build199 qualification and live-monitoring notes

Superseded by [the current handoff](../../HANDOFF_Codex.md). The run is now terminal,
with complete EXIT802117cells/4368q/2537J/3098geometry/0size. All10owned handles
are absent/all1401pins unchanged. Historical LIVE labels below are not current.
Build39029/1296pins refers to compilation; full M6 exec47583 used1401pins.
Original root-relative links are preserved verbatim in this historical section.

Preserved section SHA256: 1624ef0be3a38b71106deaab02b8e2a90a5de6d2e856c139d0a2b6bd9f959669

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
Latestcompletequalitywindow23/round23040: 801065cells/5464q/3327J/
3115geometry/0size. Sincefirstqualityentry3987.734514s,
net48617q/50065J/655geometry through42147publications;
netq12.191634/s. Constructioneditssinceentry0;wholefieldrate
includesanyconstruction,notnecessarilypurepublishedrepairyield.
Remainingqsurface/interior868/4596;J873/2454.
Queue82056/blockedfeatures544/stuckshape28;
Counters alone do not establish repair power; interior cells may have constrained vertices.
Actual198retry1same-roundnetq11.530341/s;selectedwholefieldexact=False,
rankdebtsexact=False,commitsexact=False,churnexact=False.
Traversal3071363852→1439580134(-53.1290%).
12complete lateownershipmigrations audited forglobalcounts/debt/completeflagsand20selected
cachecounters/entries/bytes;notproofeverykey/history. Currentjoinshare0.40159
includesprotocol+wait,notpureidle. PredicatecountsarenotuniformCPUcost;timersoverlap/
hostconditionsdiffer. Noisolatedspeedupclaim. FullruncontinuesFROZEN. Actual
quality_window23_comparison.json/service_summary.json,quality_migration_assessment.json and
all_quality_census_assessment.json andquality_matching_window_assessment.json retaincounters.
First measured selected-state difference is window21;later states/work are not assumed identical.
Confirmed ownership decision difference at round22848: current migrate=0,
198retry1 migrate=1, confirmed on all6ranks. Do not assume future state identity.
Confirmed ownership decision difference at round22912: current migrate=0,
198retry1 migrate=1, confirmed on all6ranks. Do not assume future state identity.
Latestwindow23HOSTaudit1793531655.1068807:all10LIVE/1401pinsunchanged/
82mappedELFcovered. Actualquality_window23_live_provenance.json/resources.json.
Afterwindow4hostfollowup1793528607:load23.98/RAM30.81GBavailable/CPUpsiavg6021.81%,
memorypsiavg60some10.44/full.79%;quality_window4_followup_resources.json. Snapshotnotcausality.
ZEROaccepted adaptedmeshes/transfer/adaptedCFD/adaptedrestart. FullgoalACTIVE.

End of preserved build199 monitoring section. Terminal authority is the current handoff.
