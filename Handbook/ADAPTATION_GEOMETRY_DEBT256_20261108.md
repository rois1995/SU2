# Canonical geometry-debt integration256

Current implementation/validation cycle,2026-11-08. ZERO accepted meshes.
R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
A254=`R/m6_runs/euler_build254_conditioned_mach_only_real_cfd50k`.
Read HANDOFF_Codex.md first for live handles. All final Euler/transfer/adapted CFD/
adapted disk-restart/BL-MPI/RANS500k obligations and strict gates remain open.

## Confirmed residual and target delivery

254terminal complete741236cells/251q/205J/95geometry/0size;actual saved mesh
150080points. No accepted event. Current wing-face reconciliation of all owned
shards and SU2 faces:117bad cores own wing faces,143interior bad cores share
CURRENT wing vertices,NONE is free interior. Provisional reference-node-based
classification missed119new-node adjacent cells;it is preserved explicitly and
superseded by bounded_residual_selection.json v2. Six complete contexts use
25/50/52/53/60/63cells without enlarging the existing64old-cell cavity limit.
Note MaximumReplacementCells is128,not64;selection deliberately bounds the OLD
four-star context at64. These are complete GLOBAL incidences,not local-rank stars.

Exact LE production-cache reproduction:55545wing faces,95geometry-bad wing
cells,64in explicit swept LE band(vs795old247). At five span stations the latest
mesh has665/647/545/566/575triangles,8.2–15.1xoriginal,median exact edge length
span.134–.163,nose.157–.192,>99%edges<.5. Over-density persists. This does not
establish the necessary geometry spacing or isolate one construction operation.
A254uses corrected Mach-onlyL1/50k target,not247's older tensor. Flow/target
chain and paper limitations remain in ADAPTATION_TARGET_DELIVERY253_20261107.md.

## Preserved actual-cavity evidence

Existing serializer67421TERMINAL0/33.062778s:741236owned/metric rows paired,
35066boundary points,70128physical faces;all hashes before/after identical.
A254/current_witnesses_v1/witness_preparation.json pins every capture/input/helper.
Existing compound254case=current_compound_20261108T004404_1638191:
41955assertions/102.915297s,all pins unchanged,all5host handles absent.
It reproduced captured q/J AND strict geometry-bad counts in all6contexts.

Logging-only build255TERMINAL0/231.209741s,1303pins unchanged,both targets up
to date,all recorded build handles gone. SU2_CFD binary byte-identical frozen254.
Case=current_compound_20261108T005405_1672862,tool52151TERMINAL0/103.936634s,
41955assertions,pins unchanged,all5host handles gone. Added no decision change:
physical-face failure counts,canonical squared certificate excess,maximum face
error,vertex counts,and existing GeometryRepair METRIC validator result.
A254/bounded_cavity255_assessment.json records actual results. All6canonical
geometry sums match complete captured cell excess-squared sums to rounding.

Representative raw contraction in adjacent-volume context256755094:
q28→18,J25→17,failed physical faces8→7,certified sum1.2603913593e-6→
1.0659596395e-6,maxfaceerror.00062440856UNCHANGED,vertices29→28.
Area-weighted guidance nevertheless rises6%. Surface context254960898 has raw
q18→16,J17→15,failedfaces13→12,certifiedsum7.6122864588e-7→
6.8830112542e-7,maxerror.000437397044UNCHANGED,vertices42→41,weightedguidance
rises.76%. Replacement/interface and GeometryRepair metric admission pass.
These are actual certified-face improvements,not merely failed-cell regrouping.
Overlapping contexts/results must NOT be added as disjoint global improvements.

## ONE connected correction and validation

R/geometry_debt256_plan.json freezes the rationale/scope/acceptance. Shared
GeometryGuidanceExcess now sums squared absolute-tolerance excess per face,
with the same extended subtraction as the complete census. It previously
multiplied each contribution by twice triangle area. Removing that weight
prevents area redistribution vetoing a lower canonical certificate debt.
All coarsening/refinement/repair/optimizer callers share this helper.
Nonnegative ordered partial sums retain conservative early rejection;incomplete
results never prove progress. Exact authority/association,private context refresh,
body/feature/seam/height checks and final strict certification are unchanged.
No operator,scheduler,support bound,metric/geometry representation,config or
time/work budget change. Construction remains validity-first.

Existing bounded-guidance test now covers zero and1e-4tolerance with new score
semantics and full/early equivalence. Actual adjacent-volume compound witness
requires a useful complete-context contraction(previous area-weighted rule fails).
Focused serial and ONE MPI6 milestone via geometry_debt256_validate.py;then
immediate full same-seed two-eventM6. No new baseline/perf/GDB campaign.

Limits:the diagnostic current-nodal donor reconstruction reproduces cached q/J
at unchanged cells,but does not prove original frozen-field equality at trial
positions. Nonlocal contact/publication,global queue selection and original-field
evaluation remain production obligations. Existing one-vertex fallback can
construct pure-star raw CollapseVertex candidates;the diagnostic retained-star
union is not required for an unchanged-retained raw contraction. However there
is NO generic collapse→retained-star optimization orchestration. The isolated
tip case required that combination. Conditional two-face shape+geometry expansion
can also change the imported context and prevent a pure-star CollapseVertex
input. Do NOT claim the scalar correction guarantees all diagnostic candidates
are reached/admitted. Observe actual useful production progress and strict whole-
field counts. Record orchestration/support alternatives in backlog;do not add
them as a competing implementation without measured production evidence.


## Qualification and full return

Build256passed480.354111s,all1303pins unchanged,both targets up to date,
all owned build handles terminal. Focused serial58cases/46938assertions passed
165.939102s;ONE MPI6 milestone99cases on EACHrank passed183.821737s. All
source/runtime/input pins unchanged;all5serial andall10MPIhandles terminal.
geometry_debt256_qualification.json seals these independent completed stages
and3actual raw contraction witnesses with no increased maxerror. Do not rerun
them absent changed scope/failure evidence. These do not establish M6 acceptance.

Immediately returned to full same-seed two-event256M6,tool10984 RUNNING FROZEN.
Actual original-grid22flow/metric fields and coordinates match254and qualified
conditioned target BIT-IDENTICALLY. ConfigSHA and host4GiB cachequota match254.
Current actual owned/source/runtime/input handles/pins are in A256/owned_startup.json;
read HANDOFF for live state. No new production implementation while frozen.


First31matched actual construction censuses have identical254counts,flags and
replicated global work. At length_split_exit round20572:670142cells,3766geometry
bad,3117sizebad,q/JUNMEASURED. Native451.071s versus254509.246s is not isolated
code-speedup evidence under differing host contention. Current and baseline
INITIAL22flow/metric fields,configuration and host cache quota match exactly.
Use construction_comparison254.json and guarded source-prefix receipts;compare
net whole-field q/J and geometry progress only once complete shape measurement
starts. No accepted mesh or adaptation/transfer/restart claim follows.


First complete quality entry23314 is EXACT254:690715cells/27016q/27453J/1205g/
0size;all selected per-rank geometry/support/cache/traversal/construction counters
match,including245091394upper-work. Entry native705.664vs755.523 is timing
confounded;initial states and counted work are controlled. At window23424,256has
20201q/19573J/1144g atquality380.265s versus25420157q/19485J/1148g at388.130s.
New geometry upper-work223125688vs236680834,netq6815vs6859,commits3507vs3525.
Early net global progress remains similar;no closure benefit established.
Use explicit measured-time gaps and work-normalized rates in
early_quality_comparison254_assessment.json;do not extrapolate a terminal result.
All11owned identities/argv/cwd and1520pins still match in window2receipt.
TreeRSS8.87GB,available~30.9GB,diskfree10.809GB;no actual resource pressure shown.
Run stays frozen. All strict acceptance,transfer/adaptedCFD/disk/BL/RANS gates open.


Frozen256 now demonstrates actual full shape/size closure and entry to existing
dedicated geometry repair:24512/native3113.844530 has0q/0J/775g/0size;26112/
native4406.013439 has0q/0J/192g/0size on733808cells,all completeness flags true.
Guarded construction between these frames introduces shape failures and existing
repair recloses them. No accepted event yet;geometry remains open. This is stronger
than254shape closure,not proof that all geometric gates will close within budget.
Missing retained-star orchestration is no longer an established prerequisite for
shape closure. Latest live handoff/counters take precedence over this snapshot.


## Terminal256 outcome

First native event REFUSED on unchanged time budget. Final31454/native5340.832826
has734280cells/78geometry/0size,with68q/J-unmeasured;final whole-field shape counts
UNKNOWN. Last fully measured31442/native5338.745886 has734246/0q/0J/73g/0size.
Late complete endpoints repeatedly alternate construction734280/78g/debt~1.466891e-5
and repair734246/73g/~1.977194e-6. Matching counts/excess do not establish mesh
identity. Existing routes prove shape closure;residual geometry/construction
repair power/admission now drives the next bounded actual-cavity diagnosis.

All30capture files sealed and1520pins unchanged BEFORE ONE pidfdSIGTERM to owned
MPI1753220/start14514129. Tool10984TERMINAL1/5612.727731s;all11handles absent.
Original-grid CFD after refusal is not adapted continuation. Capture is PARTIAL:
584336/734280owned;rank2/rank5 hit file caps and rank4/rank5 shape incomplete.
No complete256mesh export/global-star proof from these files. Latest complete
saved inspection remains254. No extra baseline to fill capture. Preserve all
partial/unknown evidence and verify support before actual-cavity tests. All strict
acceptance/transfer/adaptedCFD/disk/BL/RANS obligations remain open.


### Bounded terminal source survey

No edits/builds/tests:existing geometry-only service uses MoveVertex and
CollapseVertex/Reconnect;the metric optimizer is skipped in geometry mode
(CNativeEngine3D.cpp4896–4978). This does not prove that enabling it would help:its
metric/shape merit is not automatically a geometry-progress objective. Geometry
guidance must strictly reduce complete canonical excess in dedicated repair
(3662–3699);shape/size clean entry is2967–2974. Failed feature service blocks the
feature;accepted service can requeue it while users remain (832–854). Construction
return3002–3011 and phase reset2779–2814 rebuild the frontier. These mechanisms
are consistent with the measured cycle,but do not prove repeated cavity identity
or justify a scheduling/operator policy change.

Existing capture_feature+transaction_observer provides the ACTUALLY selected
feature,dependencies,old/fresh imported patch and rejection/certificate path
(CNativeEngine3D.hpp117–118;cpp3596–3600,3644–3648,4086–4092,5402–5404). It is
not an arbitrary forced-feature replay. Before using it,obtain one proven complete
actual residual star/dependency closure from existing evidence/facilities;the
truncated terminal capture alone cannot assert global completeness. Compare full
physical-face debt/failed sets,associations/body/coverage,q/J/edges and neighbors
to distinguish candidate limitation from debt reintroduced at construction.


## Face-level residual257 diagnostic

Build257passed207.187881s,all1303pins unchanged,allrecordedhosthandles terminal.
ProductionCFDbinary byte-identical256;onlytestfixture changed. Successful host
serialgeometry_faces257_20261108T031129_2038858passed1case/1674assertions/57.644050s,
allsource/runtime/input pins unchanged,all5handles terminal. Earlier sandbox
launch failed MPI_Init socketEPERM before test and is separately preserved.

All78actualWING3Dfaces pass containment and reproduce captured excess EXACTLY
using originalauthority+PrivateParameters with reconstructedlocalcoverage. All
are near the swept leading edge (face-centers y.1012–.8228,x-y/sqrt3=.000071–.012747),
none marked near genuinefeatures. Three changed construction faces account for
94.8785%of terminal squared excess1.466890995618264e-5. This rules out stale reporting
as the explanation for those values,but not conservative upper-bound looseness.

24face-onlyprojectedmidpoint probes on8faces:allchildcontainment valid;4faces have
a direction closingbothchildren below1e-4. Another split raises maximum bound
.000517→.002151. Longestedge is not uniformlybest. Independent singleton-facet
projections must NOT be combined as one shared-edge/cavity proposal. The next
bounded test uses existing complete254currentcontexts and SplitEdge+wholepatch
assessor,including otherincidentfaces andvolume/neighbors. No current256global
starproof from truncatedshards. No production correction selected yet.

Do NOT blindly restore the old entity-construction geometry-merit veto:its removal
7e24ddebb06 was required by demonstrated valid curvedconstruction (runtimehistory
20261028). Geometry-directed candidate selection is a hypothesis to test,not a
blanket quality-aware construction or newgate. All finalstrict gates remain.


## Complete-patch follow-through258 and selected259hypothesis

Build258passed207.196763s,all1303pins unchanged,both targets up-to-date,allowned
recordedhandles terminal. ProductionCFDbinary byte-identical256. Onefocusedserial
current_compound_20261108T032410_2072069passed43563assertions/102.522122s,allpins
unchanged/all5handles terminal. No repeatedMPI/baseline campaign. Existingcomplete
254contexts avoid inferring current256support frompartialcapture.

20corephysicaledgeSplitEdge trials:14constructed,6inversionrefusals. Onlyone
constructedchoice strictlyreducesgeometry withoutq/Jchurn:complete50cell174743936
edge18795-202090700physicalbadfaces7→6,g7→6,debt7.6299890926e-7→7.6153322748e-7,
ZEROq/Jretirement/introduction(q6/J4remain),maxedge/maxerrorunchanged,points33→34.
Longest18795-201969696insteadadds1q/1Jwithnogeometryprogress. Otherchoicesraise
gdebt8–9%. Wholepatchreplacement/interface/GeometryRefine pass for theuseful
choice;globalcontact/publication/originalfrozenfield trialpositions are notproved.

Selected ONEconnected259hypothesis,NOTIMPLEMENTEDyet:use bothincidentface
geometryprediction toprioritize existinggeometryqueue ONLYon geometry-directed
returns. Preservevalidity-firstconstruction,initiallegacychoices,repairfairness,
fullvertex/dependency/publicationgates andallbudgets. No blanketmeritveto.
R/geometry_priority259_plan.json specifieshelperprojectionequivalence,fresh
parameters,allincidentfaces,transient/mapmemoryadmission,stickywork exhaustion,
phase-localinvalidations andpredicted-vs-ACTUALcompleteSplitEdge delta checks.
Useactivephysicalcomposition/heightstate:actualM6compositionempty;full_target1
isnot proof ofnonnullcomposition. Skip predictor whereactualproductioninput
usesBalancedSplitPoint/heightcomposition. Qualifyaffectedserial+ONEMPI6milestone,
thenIMMEDIATEfullsame-seedM6. No acceptedmeshes/transfer/adaptedCFD/disk/BL/RANSyet.
