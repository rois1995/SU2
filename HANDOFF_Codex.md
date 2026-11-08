# Native adaptation — current handoff

Updated2026-11-08 Europe/Rome. Goal ACTIVE; **ZERO accepted meshes**.
TWO strictly accepted ONERA M6 Euler CFD→mesh→CFD→mesh at50k metric complexity,
conservative admissible transfer, finite continued ADAPTED CFD and ACTUAL isolated
ADAPTED disk restart. Then dedicated BL/constraints/MPI and RANS500k. Actual mesh
points/cells are separate from target complexity. Geometry1e-4,q>=.2,J>=.05,
maxedge1.8,requested heights strict. Validity-first construction; CAD optional.
ONE active cycle/heavy job; nice19/libs1/build-j3/4TBscratch. Freeze compiled/
executing/pinned C++, binary, helpers/config/runtime/inputs through build/check/run.
Fork pushes authorized; exact-whitelist staging, preserve unrelated dirty files.

R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
A278=`R/m6_runs/euler_build278_source_chord_certificate_mach_only_real_cfd50k`.
A274=`R/m6_runs/euler_build274_metric_cleanup_mach_only_real_cfd50k`.

## CURRENT — build278 TERMINAL REFUSED; 283 QUALIFIED; fullsame-seedM6 launch NEXT

ZERO accepted meshes. A278 first native event REFUSED on elapsed-time budget.
Exit24869/native5336.347948825s:734914cells,0q/0J/0LOCALgeometry/0size, ALLmeasured/
censusflagscomplete. Final WHOLE-SURFACE TWO-SIDED geometry proof INCOMPLETE.
One extra geometry round (+3work,+2tets,rank3accepted+1,upper88/.0564s) closed two
local failures. Quiettail was globalcertification, NOT continued geometryrepair.
Cleanup NEVERexecuted. No transfer/adaptedCFD/diskrestart proof.

A278/capture_ready.json seals ALL30 COMPLETE raw files/734914rows, no omissions,
allcachedmeasurements known, within unchanged128MiB/filecaps. ALL1520source/build/
binary/runtime/config/inputpins verified beforestop. Exactly ONE pidfdSIGTERM sent
only to MPI241348/start20630721; owned_stop_once.json records it. NEVER repeat.
Main35468 TERMINAL1/5562.945725s,pins_verified=true,changed=[]; ALL11 exactowned
identities ABSENT inA278/terminal_handles.json. Oldwrapper240820/runner241097/
time241346/timeout241347/MPI241348/ranks241371–76 areTERMINAL. No original-grid
CFD continuation/adap-namedrestart afterrefusal is accepted/adapted proof.

Exitcounterdiagnosis inA278/terminal_certification_diagnosis.json: EACHrank3full
queries/~1318.7s,~1074–1090s traversal/~157–158Mtraversalwork,~204–212s scopebuild,
17–32s facetprep. Referencequery order has4entities; failure during THIRDentity
(0,10154,2),37202sourcefacets. Earlier2queries returned, but this alone doesNOTprove
they were within tolerance. Need bounded exactinput reproduction to distinguish
source→mesh versus mesh→source before selecting another correction. Existing
chordbound advanced beyond the firstquery but didNOTcomplete final certification.
Do not claim full capability or isolated downstreamspeedup. No budgets/gates raised.

LatestPUSHEDbf8e47ad38 preserves282 strict optional-witness candidate and bounded
failures/diagnosis. 279/280/281/282builds+probes TERMINAL/allpinsunchanged/handlesabsent.
281metadata candidate REPLACED, not layered. Its matched20M coverage gain70.35→71.72%
was insufficient;282returned to qualified277source plus one-shot graph retry.

282 full-only trace tool28308 TERMINAL0/533.852s,allpinsunchanged/5ownedhandlesabsent:
casegeometry_current_queries_20261108T211658_779719. Query stillEXHAUSTED20M before
reverse, NOTfullcertificate. Exactdisjoint equal-facet dyadic parameter-area coverage
93.23098231% versus28070.34779086%, NOTphysicalarea. Remainingqueue~148k versus295k,
worstupper.0004222 versus.0006970. Physicaltarget6accepted/1invalidgraph(3trimmed,
33omittedwitnessfaces); reference1511accepted/5invalid(29trimmed,906omitted). Full
originalsupport remains. direction_trace_assessment.json seals allcounters/results.
This is substantial boundedcoverage progress, NOTclosure or speedup. Timecostrose:
query518.812s,traversal97.881s,preparation~420.93s. CPUreceipt325.24user/335.91elapsed
atmidquery proves little descheduling, not absence of memory/hostcontention.

Build283 TERMINAL0/49.216s,pinsunchanged; helper812520/ninja812636 absent.
CFDSHAb885585c5229d67bb01929c5737d175d39d7949c7a098a0789ef960758a1204c.
Probe283 TERMINAL0/249.056s,allpinsunchanged/5ownedhandlesabsent.
casegeometry_current_queries_20261108T212914_818035; strictcontrols passed.
assess_final283.py VERIFIED allnon-timequeryfields/domaincounts/graphresults EXACT282,
93.23098231% coverage; stillEXHAUSTED20M BEFOREreverse. Query518.812→182.754s,
preparation420.931→85.541s,traversal97.881→97.212s. No fullcertificate or mesh yet.
R/convexity283_bounded_comparison.json seals comparison.
Serial TERMINAL0/212.587s,86cases/63018assertions,allpinsunchanged; helper832315/
time832784/timeout832785/test832786/orted832787 ABSENT.
MPI6 TERMINAL0/241.121s,120cases perrank,allpinsunchanged/all10ownedhandlesabsent.
Qualification R/certification283_qualification.json SEALED: implementation behavior
and boundedactualprogress ONLY; actual_saved_entity_certificate_complete=false.
No heavyjoblive atthisreceipt; fullsame-seedM6 launchimmediatelyafterqualifiedpush.
C++/binary/helpers
FROZEN through serial/MPI and subsequentfullM6. ONLY Common/src/adaptation/
CNativeBoundary3D.cpp changes from282: break convexity classification loops atfirst
exactcounterexample. Existing code continued ALL boundary-edge/vertex pairs after
proving nonconvex, costly for large recoveredfootprints. All subsequent crossing,
topology and coverage validation remains; classification identical.282graphretry
unchanged, no prepared-domain metadata. C++/binary/helpers FROZEN throughbuild/check.
Verifyactualterminalhandles before edits. No otheractiveagent/heavyworkload.

Current probe_final283_entity2.py combines existing boundedtrace WITHalready-required
pinch/truehole/storage,curved/cache/work controls to avoid a separate repeatquery.
Samefull-onlyactualthirdentity37202reference/53406physicalfaces,20Mwork/8GiB/600s/
128MiBfilecap. No budget/gate raise or baseline/perf/GDB. Check non-time query
coverage/decisions against282 and prepCPU/wall. Full20Mquery is EXPECTEDincomplete
because283onlyskips uncharged, classification-irrelevant predicates; do NOT invent
fullproof. Strong282boundedcoverage plus affectedserial+ONE MPI milestone justifies
return to fullsame-seed TWOeventM6 underUNCHANGED nativebudgets/finalBoth gate.
Requiring complete20Mpreflight indefinitely would replace the actual target loop.
Qualifier/launcher must explicitly preserve incompleteactualquery result and certify
only changed implementation behavior, NEVER wholemesh acceptance. No fullM6 launch
before serial+MPI checks and actualhandles terminal. Prepared283qualification explicitly records this bounded scope; check
assess_final283.py against actual282counts/graphs/allnon-timequeryfields (ignore
controlrecords before actual607712MODELmarker). Then certification283_validate.py
serial and mpi6, qualify_certification283.py(probe,serial,mpi dirs),launch_m6283.py.
Qualifier pins actualstandaloneCFDSHA against buildreceipt, never marks incomplete
query complete. FullM6launchername euler_build283_pinched_graph_witness_mach_only_real_cfd50k.

Read [design/evidence](Handbook/ADAPTATION_PREPARED_SOURCE_CERTIFICATES_20261108.md).
No configuration/scheduler/CAD/geometryrepresentation/metric/constructionpolicy change.
All strictgates/budgets/transfer/continuedCFD/actualadaptedrestart obligations remain.
ZEROacceptedmeshes; no transfer/adaptedCFD/restartproof. Current283earlyexit LOCAL/
UNQUALIFIED;282source checkpointbf8e47ad38 pushed. BuildCFD282 SHA
6de0b795c7ea6707ac07b71cd038d7c7bae22469ab88eb6d90c69c295e456775.

Same REAL savedCFDseed, Mach-onlyL1/50k; initial61330coords/22fields BIT-IDENTICAL
256/qualified254target, SENSOR_MACHonly. Sourcefreeze preserved through terminal.
First83complete groups through qualityentry EXACT274 all non-time/non-bytefields.
First CONTROLdivergence vs274 atownership24384:274gain318.325>2cost156.787 migrates
287947cells;278gain389.179<=2cost198.030 skips. ALLsixrankmath/priorcommondecisions/
sourcehash/prefixes inownership24384_sensitivity274.json. Later geometry-return
meshes differ, so no causal timing/cleanup claim. Firstgeometryreturn278exit24445:
732471cells,G555,q/Jincomplete thenfresh215q/275J. Laterexit24768hadG21/60UNKNOWNq/J;
fresh24868measured0q/J/G2; actualterminal24869nowmeasures0q/J/localG allknown.

LatestPUSHED checkpoint e3b879bb24 (shapeclosure/fork); terminal/stop/diagnosis now
LOCAL and needcheckpoint. SampledtreeRSSpeak13.937GB, NOTphysicalpeak. No actual
memorypressure; lastheadroom~33GB. Oneownedheavyjob/nice19/libs1/4TBscratch.
A278/transfer_restart_readiness_reuse.json verifies8auditedsource/build/helperpaths
against currentpins/binary. ReadinessONLY; NOactualtransfer/restartproof. Actual
CFDworkflow uses test_driver solverclasses, NOTstandaloneSU2_CFD ranks. Isolated
adapted diskrestart must later use same standaloneCFDbinary/runtime. Noactiveagent.

Use R/analyze_m6278_frozen.py,analyze_m6278_quality.py andverify_m6278_initial.py.
These reuse complete first-event census/counter facilities; no newbaseline/perf/GDB.
Compare whole-field q/J/g counts and work at matching stages/times, net removal/s,
rank busy/wait/debt/churn, not commits or debt percentage alone. Wall time is host/
cache/ownership sensitive; rank CPU includes MPI spinning. Do not add overlapping
timers. All observation timeouts require checking actual owned handles, not restart.

## Current correction and qualification

[Final certification design/evidence](Handbook/ADAPTATION_FINAL_CERTIFICATION_20261108.md).
274 reached complete734806cells with0q/0J/0LOCALgeometry/0size, then REFUSED when
mandatory final WHOLE-SURFACE TWO-SIDED geometry proof exhausted elapsed time.
Each rank added one query/~1903.6s/~651–659M traversalwork; preparation<.4s. ONE
extra repair round closed the last q/J cell. Local geometry zero is NOTglobalproof.

Actual first entity(0,0,0),2060referencefacets/2827physicalfaces:old276production
source-only query exhausted1Mwork/2.747s. Connected277 extracts existing rigorous
chord/deviation envelope unchanged and uses N=3 finite-footprint bound +outward
Bernstein-deviation margin in MODEL-source traversal. Original N=10 fallback,
both final directions, exact support/body/feature/height/resource gates retained.
Ten added coefficientworkunits charged; no new cache/config/scheduler/CAD dependence.
Import source-tube caller audited. Physical-source-only construction path unchanged.

277boundedsource query COMPLETE194957work/.659s, bound9.5858962087e-5. Test-only278
CFD BYTE-IDENTICAL277, Both actualquery now CERTIFIED within247449additionalwork
(total442406 including source query)/1.543s; reversebound2.2833012834e-8.
This is bounded proof-path improvement, NOTfull-run speedup or complete mesh proof.
Focused27810578assertions/2cases12.649s; serial62994/84cases154.265s; ONE MPI6
118cases EACHrank169.654s. Six assertion totals67602/66675/66703/66878/66495/66756
in INTERLEAVEDlogorder, NOT mappedrankIDs. Allpins unchanged, allqualificationhandles
terminal. R/certification278_qualification.json seals source/binary/runtime/input/
query evidence. Read-only model review found no concrete defect; not proof itself.
274evidencecheckpoint4b40e63f5c and qualifiedimplementation2cf112b7ab PUSHED.
No further fixturecampaign before frozen M6 outcome.

## Leading-edge cleanup investigation retained

[Cleanup design/evidence](Handbook/ADAPTATION_POST_REFINEMENT_CLEANUP_20261108.md).
Actual262clean full43/20/46/35cell contexts:three raw strict collapses removeonepoint
and reduce exact short-edge deficit while retaining q/J/localgeometry/size clean;
one collapse+existingoptimizer witness also works. Otherdirections fail positivity/
support or geometry (physical vs conservative cause unresolved). Original clean
optimizer selects0trials; zero-debt progress and skin eligibility are real limits.
No blanket minimum-edge gate. These local replacements do NOTprove global LEcause.

Integrated273cleanup: ONE finite improvement-only short-edge opportunity after
EXHAUSTEDq/J choices with closed geometry/size/heights. Existing coarsening selection,
complete stars/full-vertex claims, version checks, freshstrictq/J/Coarsen-size/geometry/
height/clearance/publication/accounting and strict short-deficit decrease. Raw
contractions only; return to existing quality with changed-support invalidation.
Construction unchanged. Alreadystrict meshes do not wait for optionalcleanup.
274NEVERexecutedcleanup; no globalbenefit/publication claim. Do not widen its policy
or introduce competing construction/target/scheduler work during278. Unproven
alternatives live in[the backlog](Handbook/ADAPTATION_BACKLOG.md).
Shock target remains separate: sampled demand4–6%chord vs existing3–4%;cleanup
cannot establish targetallocationcorrectness. Preserve localfloor/lognormalize,
conditioned recovery/cutoff, corrected geometrymerit, coupledshape-reclosure and
current nodalmetrictransport authority. Pressure is not a sensor; physicalCp output
remains available. CDT3Dreferences are hypotheses, not equivalent gates or a first50k
benchmark; L2recoveryprojection differs from Lpobjective.

## Saved meaningful mesh and terminal safety

Latest COMPLETE rejected inspection: A274/inspection/rejected_mesh.vtu (+.su2),
148781points/734806tets; Tensors=captured_nodal_metric (9components). q/J/localgeometry
are correctly sized CellData; allknownclean. Independent VTU/SU2exactcoordinates/
connectivity, positivevolumes, SPDmetric and ALL734806rawcell-ID q/J/g/edge-extrema
joins pass. Final globaltwo-sided geometrycertificate INCOMPLETE; meshREFUSED.
Emptybad/cavity files are valid zero-cell binaryVTK. No per-trial mesh exports.
GenericSU2markers are inspectiononly, not a CFDusable adapted restart.

274all30rawfiles complete/all1520pins verified; exactly ONEsignal to MPI4074848/
start19809786 after sealing capture. Main80377 TERMINAL1/5953.058s/pinsunchanged,
all11handles ABSENT. Exporter157001/157290/157291 alsoABSENT, both readers terminal.
NEVER repeat274stop or historical2713560612/2673159077/2622598241 stops.271capture
PARTIAL644221/735624cells; do not fill from262.262full148608/734315 has4UNKNOWNq/J
and4localgeometryfailures; not shape-certified. Actual267interruptedstateunknown.

## Completion still outstanding

Two accepted meshes. TrueCVmassresidual<=1e-10 EACHfield/timelevel, scaledTARGET-
integraldefect<=1e-12 andfulladmissibility BEFOREpublication. Actual matchingadapted
mesh/ref/restartcoords/order<=1e-12, finitefields,positive rho/internalE and continued
ADAPTEDCFD. Then NEWisolatedACTUALadapted diskrestart: ADAP_LOOP=NO,RESTART_SOL=YES,
READ_BINARY_RESTART=YES, explicitcheckpointINPUT distinctfromOUTPUT, sameCFDbinary/
runtime. Original-grid/adap-namedoutputs afterrefusal are NOTproof. Resume-cycle0
inputoverwrite/rejectedeventconsumption bugsOPEN; isolation is not a productionfix.
Then dedicated BL/heights/curves/junctions/interfaces/laterops/resources/MPI and
RANS500k. Supporting checks and one certifiedentity never complete this goal.

[Prior detailed handoff/history](Handbook/History/HANDOFF_before278_full_M6_20261108.md),
[full integrated specification](Handbook/ADAPTATION_INTEGRATED_NEXT_TASK.md).
