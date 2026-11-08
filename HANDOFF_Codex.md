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

## CURRENT — full TWO-event M6 build278 LIVE/FROZEN

Main tool35468, launched from PUSHED2cf112b7ab (qualified source implementation).
Exact owned identities:wrapper240820/start20629566,runner241097/start20630193,
time241346+timeout241347+MPI241348/start20630721,
sixACTUALCFDworkflowtest_driver ranks241371–76/start20630766.
A278/owned_startup.json verifies ALL1520source/build/binary/runtime/config/inputpins
and83ACTUALmapped libraries. ActualCFD solver workflow runs inside test_driver
classes; NOT standaloneSU2_CFD ranks. Both binary hashes pinned. ProductionCFD SHA
c8ac77f8acc1c1d389af440989bd3ba5e5616b62613ccce7a3e09a878d24492a.
Initial sampledwhole-treeRSS2.455GB is NOTpeak/physicalusage. HostMemAvailable47.411GB
atlaunch. Native5400/outer14400,12GiBworkingperrank,hostgeometrycache4GiB unchanged.
Same REALsavedCFDseed, Mach-onlyL1/50k. verify_m6278_initial.py PASSED:all61330coords/
22fields BIT-IDENTICAL256 and qualified254target, SENSOR_MACHonly. This is initial
original-grid proof, NOT adapted CFD or transfer. Source/build/input/runtimeFROZEN.
NOstop marker, demonstratedfailure/resourcepressure or otherheavyjob. Let runfinish
unless preserved evidence satisfies existing authorized stop/fix/retry conditions.
No active sub-agent. Do not start another build/fixture/MPI campaign alongside it.
Initial six-rank partition EXACT274 outside time fields (receipt initial_partition_equivalence274.json).
First83complete groups through qualityentry23314 MATCH274 EVERY non-time/non-byte
rankfield, count, flag and workcounter (quality_entry_equivalence274.json). Quality
entrynative896.835554s vs274691.828611s:690715cells,27016q/27453J/1205localG/0size,
ALLknown/flagscomplete. Timinghigher; no construction speedup claim. Await frozen outcome; cleanup not invoked. No source/binary/config/ownership policy change during this frozen attempt.
Latest geometry_repair_entry24868/native4017.111492s:734912cells,
0q/0J/2localG/0size, ALLmeasurements/flagscomplete. WHOLE-FIELD SHAPEclosure proven,
NOTgeometry/fullmeshacceptance. Last reported geometryrepair entry; freshq/J reclosure
and mandatory globalBoth certificate remain due. No newercompletecensus yet;
workingstate AFTERentry is UNKNOWN. finish() certifies globalBoth BEFOREexittelemetry,
so quiettail alone does NOTprove geometryrepair is still active. Use actualexit
counters/reason, not stale2Gcount, toidentify the blocker. Main35468directpollstillLIVE;
source/runtime/inputs frozen, no stopmarker/pressure/otherheavyjob. Cleanup NOTused.

First CONTROLdivergence vs274 atqualityownership24384:274gain318.325>2cost156.787
⇒migrate287947cells;278gain389.179<=2cost198.030 ⇒skip. ALLsixrankmath/previouscommon
decisions/sourcehash/prefixes sealed inownership24384_sensitivity274.json. No
schedulingcode/configchange. Firstpass q/J/g/work matched through24384, but later
geometryreturn topology/counts differ, so no isolated downstreamspeedup claim.
274returnexit24496:732475cells,G559,1112unknownq/J thenfresh215q/270J.
278returnexit24445:732471cells,G555,q/Jincomplete thenfresh215q/275J.
Later278length_split_exit24768:G21 with60UNKNOWNq/J; NEVERcallthatshapeclosure.
Latest24868allknown0q/J supersedes that intermediate state.

Passiveexact11handles allLIVE;sampledtreepeak13.937GB,MemAvailable~36.8GB,
hostload10.62 atlatestresource receipt. CPUincludesMPIspin; joincallwallshare is
NOTtotalMPIwait. Lastpushedqualityevidence684cd8251a; source2cf112b7ab.
A278/transfer_restart_readiness_reuse.json verifies8audited source/build/helperpaths
against CURRENTfrozenpins and workflowSHA164bf1c06ac5ba5d663aae871b357f5e7143a1d3bda1c8418efaeb9c3fa52074.
Read-only audit confirms Cycle2 includes final adaptedCFD and selected inspection
fixture does NOTrequire refusal. This is readiness only: NOactualtransfer/restartproof.
Source remains frozen; await outcome before any implementation change.

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
