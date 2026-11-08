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
