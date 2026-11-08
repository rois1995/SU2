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
A271=`R/m6_runs/euler_build271_cooperative_deadline_mach_only_real_cfd50k`.
A262=`R/m6_runs/euler_build262_compact_capture_mach_only_real_cfd50k`.

## CURRENT — connected serial274 LIVE; production cleanup273 frozen

Latest pushed edd8ce29a9. Terminal271 and bounded272 evidence/fixture are pushed.
Production correction273 is local/uncommitted; ONE connected cycle, no competing
implementation. Read the correction273section of the linked cleanup record.

Build273 TERMINAL0/516.728824s, all1520pins unchanged. New CFD SHA
369a994f75a2a7cce76973096189f641f3d696aa045bc6bdd0db049499448512.
Buildtool66136/helper3983839/start19647189+ninja3983955 absent/terminal receipt.
Focused273 tool29794 TERMINAL1/112.746s,43684of43685assertions. Actual four M6
contexts/sharedcleanupgate and alignedphasecontrols PASSED; only synthetic unit
check failed because it modified a shared cached length without changing exact
endpoint identity/coordinates. Corrected that TESTONLY to move an endpoint too.
No production correction. Allowned4014048/4014397/4014398/4014399/4014400 terminal,
sealed in casecurrent_production_20261108T154314_4014048/terminal_handles.json.
R/cleanup273_focused_diagnosis.json preserves failure and three raw useful removals.

Build274 TERMINAL0/27.107709s, allpinsunchanged, CFD BYTE-IDENTICAL273 SHAabove.
Tool52770/helper4031843/start19734794+ninja4031888 verifiedabsent; receipt
R/build274_terminal_handles.json. Only compiled change was testcheck above.
Focused274 PASSED43685assertions/3cases97.369868s, allpinsunchanged; tool8761
andhelper4034511/process4034736/4034737/4034738/4034739 verifiedterminal.
Casecurrent_production_20261108T154915_4034511 seals gate, actual4M6contexts,
alignedphasecontrols. ProductionCFD remains SHAabove.
Connectedserial274 NOWLIVE tool72355/helper4044704/process4044908,
casecurrent_production_20261108T155213_4044704;
use actual status/owned handles before starting MPI. Allcompiledsources/helpers/
runtime/inputs FROZEN; nice19/libs1. ONEownedheavyjob, noM6/MPIalongside.
After serialterminalpass: cleanup274_validate.py mpi6 ONCE at connected milestone.
qualify_cleanup274.py andlaunch_m6274.py prepared (not run); then immediately
fullsame-seed TWOeventM6 using old sameMachL1/50k/gates/budgets. No higher budget.

Correction: ONE finite improvement-only short-edge opportunity after exhausted
q/J repair with complete closed geometry/size/heights. Existing coarsening queue,
fullstars/claims/versionchecks/geometry/height/clearance/publication/accounting;
strictfreshq/J + ordinaryCoarsen sizeadmission + strictly decreased complete
short-edgedeficit. Raw contractions ONLY; no compoundnewtrials/config/scheduler.
Construction remains validity-first. Return to existingquality; actualchanged
supports reopen blocked features. One-shot marker prevents another cleanup in
that Adapt invocation. Alreadystrict meshes acceptwithoutoptionalcleanup.
Read-only source review found no concrete defect. Successful actual cleanup
publication+strict returned polish remains an integration-validation obligation.
NoactualM6afterproduction273yet. ZEROacceptedmeshes; fixturesnotcompletion.

LatestUSERtask: investigate missing post-refinement metric cleanup and leading-edge
crowding. [Current design/evidence](Handbook/ADAPTATION_POST_REFINEMENT_CLEANUP_20261108.md).
Source audit confirms general coarsening exists, but geometry-only returns start
splitting; fully clean acceptance does not require another short-edge pass.
Localoptimizer selects bad cells/longedges, and zero shape-debt cannot strictly
decrease. Its interiorCollapse also excludes physicalskin vertices; fixing merit
alone cannot provide curved boundary cleanup. Generic CollapseVertex remains the
existing production primitive. Source eligibility concerns do NOTprove desired
collapses admissible or that cleanup solves the terminal root-cell blocker.

Saved262fourACTUALclean full contexts43/20/46/35cells selected; all cachedq/J known,
geometry/size clean, no unknown neighbor. Selected exact production edge lengths
.153/.130/.122/.310. Independent exact-coordinate screen7/8positive/no duplicates;
192234610→16709 hasone nonpositivecell. This is NOTproduction admission evidence.
Inputsready inA262/post_refinement_cleanup_inputs_v1; exactq/J/edgeoracles,current
boundary/reference preserved;734315pairedrows verified. Allpinned original
current_witnesses_v1 files untouched. Test272 extends existing current-source
fixture with one hidden clean-cleanup mode: originaloptimizer control, both
selected collapse directions, complete-context freshgeometry/current-nodal metric
transport and existing160work/30s optimizer. Reports q/J/geometry/size failures,
point removal, exact short-edge/directional distributions and refusal reasons.
Four unique local point-removal replacements now demonstrated:3raw and1collapse+
ONEoptimizertrial. Eachreducesonepoint and short-edge deficit, preservesq/J0,
geometry0,size/interface and currentnodaltransport. OrdinaryRepairmerit refuses
all forzero-debt/oversizeprogress; unchangedoptimizer controls have0trials.
Counterexamples:one positivity obstruction,one entity-support refusal,andother
directions produce2geometryfailedcells (physicalvsconservative notseparated).
Spanwise extent medians improve1.45→1.88/2.11 and1.53→3.32; smoothcontrol1.11→1.28.
Nonlocalcontact/distributedpublication/heights notyetqualified; noM6acceptance.
This proves a LOCALcleanup/admission gap, not that it cures terminal rootq1.
Preserve old fixture behavior; implement ONE justified connected correction next.

Band growth is independently reproduced:262has3.1–17.6%more LEbandtriangles than247,
median triangle extent ratios1.51–1.72, production metric span-edge medians.134–.165
andnose.169–.198. Nodal requested-spacing ratios are diagnostic restrictions;
geometry necessity/collapse feasibility and pointwise target drift remain separate.
Fourunknownq/J in262 remain explicit. See A262/post_refinement_cleanup_selection_v1.json,
leading_edge_delivery_reproduction_v1.json,leading_edge_delivery_feature_caveat.json,
post_refinement_cleanup_positivity_v1.json. No per-trial VTU was created.

## Latest complete native outcome271 — REFUSED on one q cell

First nativeevent exited at round25173/native3378.395280s:
735624cells,1q-bad,0J-bad,0geometry-bad,0size-bad; ALLmeasurement/censusflagscomplete.
Stop=operator_stagnation, not elapsed/work/RAM exhaustion. Firstevent rejection
prevents TWOaccepted events. Main18445 TERMINAL1/3781.842572s, all1520pinsunchanged.
No actualtransfer/adaptedCFD/restart occurred; later original-grid CFD is NOTproof.

All30raw files sealed within unchanged128MiB/file caps. BUT rank4owned stream hit
cap:644221of735624rows saved,91403missing; ONLYraw rank4partial. Nativeglobalcensus
is complete; savedconnectivity is NOTcomplete. No full271inspection mesh can be
claimed. Do NOTfillmissingcells from262 or raise caps. A271/capture_ready.json,
pin_check_before_owned_stop.json and owned_stop_once.json preserve this explicitly.
The first stop-helper invocation refused before any signal because full-capture
assumption failed. Revised bounded partial seal then sent EXACTLYONEpidfdSIGTERM
toMPI3560612/start18806492. NEVERrepeatstop. Stoptool53222terminal0; all11exact
wrapper/runner/MPI/solver identities ABSENT inA271/terminal_handles.json. This
receipt released271source freeze before272test edits. No liveM6job remains.

Soleq-badcell152833287 onrank5: q=.198870799615863,J=.060471062672186514,
geometryexcess0, physicalfaces0; two boundary-associated vertices withmobility2.
Nodes201448499,192569399,201303160,201400503; boundaryedge201303160–201400503
metriclength.1842519914. Rootregion y≈.035. All14repairfeaturesblocked; no feature
absence inference from incomplete metadata. Complete current surrounding supports
must be proven before source-faithful candidate/publication claims; cap loss is a
real obstacle if needed dependencies were omitted. Latestfullinspection remains262.

Production271 qualificationR/deadline271_qualification.json: focused7/107,
serial81/62880, ONE MPI6 115cases EACHrank; allpins/handles verified. Callbackdeadline
is cooperative, not external-sort/index preemption. Actual271refusal is stagnation,
so this run does NOTdemonstrate runtime deadline expiry or causal speedup.
[Deadline integration/history](Handbook/ADAPTATION_DEADLINE268_20261108.md).
[Coupled repair design](Handbook/ADAPTATION_COUPLED_REPAIR266_20261108.md).

## Source, seed and reusable evidence

Same savedREALseedR/m6_inputs/euler_original_force_seed_v1:61330points/303359tets,
M.8395/AOA3.06,userqualified stableCL.269535/CD.013536 (not resolvedshock proof).
SeedSHA1eaee788bd62bd92a92ec539577dcbcfc0f8042dcbcec23adb04e825bb01a880.
Mach-onlyL1/50k;pressure/Cp remain physicaloutputs. Configbyte-identical262,
SHA5ad2dc1d9a1cd8bff87f633617b191b50defbc261ab3d0b2ed0987a34722b89e.
Native5400/outer14400/12GiBworking perrank/hostcache4GiB unchanged. All61330initial
coordinates/22fields bit-identical256/qualified254target. First83complete stages
throughqualityentry EXACT267non-time/non-byte rankfields;1520pins verified. Later
paths differ; no isolated timing or same-mesh causal improvement claim.

Preserve219localfloor/lognormalization/flat,254basequadratic conditioning,
253witnesscutoff,256canonicalgeometrymerit,260pairedfacepriority,267coupledsplitrepair
and271cooperativedeadline. Currentnodaltransport/rebasing is production authority,
not globallyimmutable originalbackground. [Targetdiagnosis](Handbook/ADAPTATION_TARGET_DELIVERY253_20261107.md):
sampledshockdemand4–6%chord vsoriginal3–4%; cleanup does NOTsolve targetallocation.
Farfield40.398%complexity physicallyunqualified. No norm/sensor/complexity change.
CDT3D requestedsections read; itsgeometrycriteria differ, L2recovery !=Lpobjective,
iteratedFig20 not first50k. [Backlog](Handbook/ADAPTATION_BACKLOG.md) retains unrelated
recovery/norm/gradation/geometrymetric/metricproposal hypotheses; no competingcycle.

A262/inspection/rejected_mesh.vtu and.su2:148608points/734315tets; nodalmetric9,
correctly sizedCellData, fourq/Junknownexplicit; full topology/tensorfieldcomplete.
AlltensorsSPD/allpositivevolumes, VTU/SU2coords+connectivitymatch. GenericSU2tags
inspectiononly. Noacceptedmesh. NEVERrepeatold262MPI2598241/start16555245stop,
nor267MPI3159077/start17805236stop. Actual267interruptedstate unknown/unexported.

## Completion still outstanding

Two accepted meshes. TrueCVmassresidual<=1e-10 EACHfield/timelevel, scaledTARGET-
integraldefect<=1e-12 andfulladmissibility BEFOREpublication. A271/transfer_restart_readiness_reuse.json
verifies all9audited256source/buildpathsunchanged, NOT runtimeproof. Need actual
adaptedmesh/ref/restart matchingcoordinates/order<=1e-12, finitefields,positive
rho/internalE, continuedadaptedCFD. Then NEWisolatedactualadapted diskrestart:
ADAP_LOOP=NO,RESTART_SOL=YES,READ_BINARY_RESTART=YES, explicitcheckpointinput and
DISTINCToutput withsamebuild/runtime. Original-grid/adap-named outputs afterrefusal
areNOTadaptedproof. Resume-cycle0overwrite/rejectedeventconsumption remainOPEN;
isolation not a fix. Then dedicatedBL/heights/curves/junctions/interfaces/laterops/
resources/MPI andRANS500k. Do not markgoalcomplete fromdiagnostics or onegateclosing.

Prior detailed state archived in
[pre272 handoff](Handbook/History/HANDOFF_before272_cleanup_20261108.md).
