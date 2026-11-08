# Native adaptation — current handoff

Updated2026-11-08 Europe/Rome. Goal ACTIVE; **ZERO accepted meshes**.
TWO strictly accepted M6 Euler CFD→mesh→CFD→mesh at50k metric complexity,
conservative admissible transfer, finite continued ADAPTED CFD and ACTUAL isolated
ADAPTED disk restart. Then dedicated BL/constraints/MPI and RANS500k. Report actual
points/cells separately. Geometry1e-4,q>=.2,J>=.05,maxedge1.8,requested heights strict.
Validity-first construction unchanged; CAD optional. ONE active cycle/heavy job,
nice19/libs1/build-j3/4TBscratch. Freeze compiled/executing/pinned C++, binary,
helpers/config/runtime/inputs during build/check/run. Fork pushes authorized.

R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
A267=`R/m6_runs/euler_build267_coupled_geometry_repair_mach_only_real_cfd50k`.
A262=`R/m6_runs/euler_build262_compact_capture_mach_only_real_cfd50k`.

## CURRENT — 267 INTERRUPTED FOR PRESERVED DEADLINE OVERRUN; NO LIVE HEAVY JOB

Main tool68708 TERMINAL1/6225.642871s,all1520pins unchanged. Stoptool2384terminal0:
ONE pidfdSIGTERM to exactMPI3159077/start17805236, after sealing eight log/resource
files and verifying ALL1520pins. A267/owned_stop_once.json attempts1/signal_senttrue;
NEVER repeat this or run seal_stop_m6267.py. Source-backed native elapsed estimate
was>6103s against configured5400; no complete census for>2257s. No native event
refusal/acceptance was observed before stop. This is an interrupted overrun,
NOT a terminal refused-mesh census. Actual current mesh/state UNKNOWN/unexported.
All11 exact wrapper/runner/MPI/solver identities host-ABSENT in
A267/terminal_handles.json; sources may now be edited. No owned heavy job remains.
Whole owned-tree sampled peak16.045363200GB;individual3.139829760GB is NOTwhole peak.
Host had31.34GB available at job3999.9s;stop was NOTfor RAM pressure.

Evidence: A267/deadline_observation.json,deadline_overrun_sealed.json,
deadline_overrun_evidence/{rank0_stdout.log,...,run.log,resources.jsonl},
owned_stop_once.json,status.json,terminal_handles.json. No capture_ready.json or
current raw mesh exists. Latest COMPLETE saved inspection remains262 below.
R/stop_m6267_overrun.py is the executed once-only guarded helper; original
R/seal_stop_m6267.py was prepared for explicit refusal and NEVERexecuted.

Source audit (bounded read-only Luna/parent, no GDB/perf campaign): private trial
reductions/imports are local; no hidden MPI collective found. Worker join follows
all local slots. A single attempt can spend long time in geometry preparation,
certification/exact work, boundary replacement and metric checks without an
elapsed checkpoint. Optimizer gets remaining event time and checks between work,
but individual certification calls can delay yielding. Exact live stack unknown.

ONE connected next step268: enforce the EXISTING event deadline cooperatively
through those query/work paths and safe private-round rejection/publication.
Reuse AtlasBudget/work checks and existing checkpoint patterns; no new config,
scheduler, geometry representation, target policy, budget increase or gate change.
Assess all callers/catches before choosing implementation. Preserve a clean last
published mesh so ordinary refusal capture can run after deadline expiry. Validate
expired/unexpired/cached/recomputed behavior, geometry/feature/body/height gates,
resource exhaustion and MPI6 empty-rank/private-join alignment. Then immediately
return to full same-seed TWO-event M6. Primitive deadline checks are NOTcompletion.
268deadline changes are local/unvalidated: AtlasBudget rank-local callback and sticky expiry,
query/exact work checks, existing vote/join propagation and focused expiry tests. Build268 RUNNING: tool88079; helper3486849/start18637953, ninja3486900;
11changed compiled files, all other267source pins unchanged. About41.79GB host
MemAvailable/load2.57 at launch. Do not edit pinned code/helpers until terminal.

[Current coupled design/evidence](Handbook/ADAPTATION_COUPLED_REPAIR266_20261108.md).
[Deferred hypotheses](Handbook/ADAPTATION_BACKLOG.md): proposal old-connectivity
scores versus production transported-candidate scores require ACTUAL residual
cavity evidence. No immutable-original target change or competing implementation.

## Qualified267 and actual runtime progress (not completion)

Source checkpoint9011bab923; metadata checkpoint2cc9593ac5. Build267terminal0/
217.199605s;CFDSHA2ec933ebe76e432ff38f5bbe9d5920b965bda402560a1a8a0c20000c3543c7e5.
Serial65cases/56523assertions; ONE MPI6 99cases on EACHrank;allpins unchanged,
alltest/buildhandles terminal. R/coupled267_qualification.json seals it.
266serialfailure (4096work beforephase return) preserved;267 narrows new branch
failed-face retirement without increased geometry-bad cells. Existing operators,
complete <=64support before reservation,160work,nodeIDs,source/height/contact/
publication gates reused. Construction unchanged. No new config/scheduler.

Same real-CFDseed as262/256;61330points/303359tets,M.8395/AOA3.06,
userqualified stableCL.269535/CD.013536 (not resolved shocks/full residual convergence).
SeedSHA1eaee788bd62bd92a92ec539577dcbcfc0f8042dcbcec23adb04e825bb01a880.
Mach-only L1/50k;pressure/Cp remain physical outputs. Config byte-identical262,
SHA5ad2dc1d9a1cd8bff87f633617b191b50defbc261ab3d0b2ed0987a34722b89e.
Native5400/outer14400/12GiBworking perrank/hostcache4GiB unchanged.
All61330coordinates/22initialfields bit-identical256/qualifiedtarget. Initial83
completegroups throughqualityentry and112throughfirstqualityexit EXACT262counts/
flags/ALLnon-time/non-byte rankfields;1520pins unchanged. Proofs inA267.
Firstqualityexit24328/native2929.486600:732281cells/2q/0J/969geometry/0size,
allmeasured. Geometry construction introduces unmeasured shape; never claim its
cached zeros are global closure. Fresh entry24674:128q/153J/357geometry.

Firstactualshape-clean geometryentry24717/native3223.155418:733973cells/0q/0J/
344geometry/0size,allflagscomplete. Firstnew shape-clean g4 at25010/native3474.204508.
A267/geometry_phase_comparison262.json: g344→4 in251.049090s vs262456.202014s,
traversal82.963Mvs206.217M,upperwork85.775Mvs212.309M,commits398vs707;
BUT nativework31103vs15410,endcells734903vs734315. Different end meshes;
not isolated timing/per-operator causal proof. Complete-census q/J peaks30/23
vs64/61;shapeunknownframes6vs15,not continuous peak proof.
LASTCOMPLETEobserved state25011/native3480.721223:734908cells/6q/5J/2geometry/
0size,allflagscomplete. No latercensus beforeinterruption. These are NOTterminalcounts.

## Latest complete actual mesh and preserved target/construction improvements

A262/inspection/rejected_mesh.vtu and.su2:148608points/734315tets. PointData
captured_nodal_metric9rowmajor; correctly sized q/J/geometry CellData. Fourq/J
unknown values explicit;topology and nodalmetric COMPLETE. AlltensorsSPD,alltet
volumespositive,VTU/SU2coords/connectivity identical. GenericSU2tags inspectiononly.
262firstevent refused ontime;all30rawfiles sealed/all1520pinsverified beforeONE old
MPI2598241/start16555245stop. Allowned/inspection handles terminal; NEVERrepeatstop.
Current262 complete42cellcore155561218 containsallfourgeometrybad andthreeunknown
q/J;fourthunknownoutside42. Allfour actualWING3Dfaces share8184. Do notinfer absent
features fromunknownflags.263/265production-source diagnostic andsource transport
semantics are in[the detailedbook](Handbook/ADAPTATION_CLOSURE263_20261108.md).

Preserve219localfloor/lognormalization/flat handling,254basequadratic conditioning,
253witnesscutoff,256canonicalgeometrymerit,260paired-facepriority and267coupledrepair.
Current nodal metric transport/rebasing is production authority; immutable pertrial
snapshot does NOTmean globallyimmutable originalbackground.
[Targetdiagnosis](Handbook/ADAPTATION_TARGET_DELIVERY253_20261107.md):sampledshock
spacing4–7%chord vsoriginal3–4% explainslittlefirstrefinement THERE. Farfield40.398%
complexity physicallyunqualified. LEexactmetricedges.13–.19 and8–15xoriginaltriangles;
necessarygeometryresolution notproved. No norm/sensor/complexity/min-edgegate change.
CDT3D paper criteria differ;recoveryL2projection !=Lpobjective;iteratedFig20 notfirst50k.

## Transfer/restart and full completion still outstanding

TWO accepted meshes required. True CV-mass residual<=1e-10 EACHfield/timelevel,
scaled TARGET-integraldefect<=1e-12 andfulladmissibility BEFOREpublication. Source
guards unchanged;A256/transfer_restart_readiness.json is source audit,notruntimeproof.
Actualmatchingadaptedmesh/ref/restart order<=1e-12,finitefields,positive rho/internalE,
continuedadaptedCFD. Then NEW isolatedactualadapted diskrestart withADAP_LOOP=NO,
RESTART_SOL=YES,READ_BINARY_RESTART=YES,explicitcheckpointinput andDISTINCToutput.
Original-grid continuations/adap-named files afterrefusal areNOTadaptedproof.
Resume-cycle0overwrite/rejected-eventconsumption bugs OPEN;isolation isnotafix.
Then dedicatedBL/heights/curves/junctions/interfaces/laterops/resources/MPI andRANS500k.

Full priorstate/evidence/terminalstop history archived in
[pre268 handoff](Handbook/History/HANDOFF_before268_deadline_20261108.md).
