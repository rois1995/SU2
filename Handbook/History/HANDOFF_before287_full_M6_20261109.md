# Native adaptation — current handoff

Updated2026-11-09 Europe/Rome. Goal ACTIVE, **ZERO accepted meshes**.
TWO strictly accepted M6 Euler CFD→mesh→CFD→mesh at50k metric complexity,
conservative admissible transfer, finite continued ADAPTED CFD and ACTUAL isolated
ADAPTED disk restart. Then dedicated BL/constraints/MPI and RANS500k. Actual point/
cell counts differ from complexity. Strict geometry1e-4,q>=.2,J>=.05,maxedge1.8,
requested heights. Validity-first construction, current nodal metric transport,
CAD optional/no-CAD authority sufficient. ONE active cycle/owned heavy workload.
Nice19/libs1/buildj3/4TBscratch. Freeze compiled/executing/pinned C++, binary,
helpers/config/runtime/inputs throughout build/check/run. Fork pushes authorized;
exact-whitelist staging, preserve unrelated dirty files. No new scheduler/config,
larger cavities, quality-aware construction, raised budgets or relaxed gates.

R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
A283=`R/m6_runs/euler_build283_pinched_graph_witness_mach_only_real_cfd50k`.
A278=`R/m6_runs/euler_build278_source_chord_certificate_mach_only_real_cfd50k`.
A274=`R/m6_runs/euler_build274_metric_cleanup_mach_only_real_cfd50k`.

## CURRENT —287 exact boundary-intersection revision BUILDING; all early guards restored

ONE connected geometry-certificate cycle. Original point-winding removal285passed
correctness but reverse common-domain work+28.42%. Adding exact planar boundary
separation286gave sourcework-13.57% but reverse+21.87%. BOTH versions performance
REJECTED despite lowerwalltimes/identicalcommonbounds. All285/286probes are TERMINAL,
pinsunchanged/allactualhandlesabsent. Patches and rejected_work_diagnosis receipts
preserved; NO MPI/M6of285/286. Serial28590cases/67446assertions passed236.428s but
must NOTbe attributed to revised287source.

287RESTORES ALL original early vertex winding checks, keeps ONLY exact planar
BoundaryCrossesTriangleInterior separating axes, reusing existing fresh face
orientation. No newrepresentation/index/cache/config/scheduler/gate/budget; no
point/face certificate skipped. Boundary-only contacts remain excluded from
open-interior crossing, holes/seams/body/feature/height/finalBoth gates unchanged.
Exact rational interval oracle140352checks/0mismatches +independentread-onlyreview
support logic; new C++ L-notch/plane/orientation/offplane controls and existing
curves/holes/resources still required. Build287 TERMINAL0/33.108s/allpinsunchanged/helper1192109+ninja1192195absent.
Focused287 TERMINAL0/52.692s,12cases/7281assertions/allpinsunchanged/all5handlesabsent.
Source287 TERMINAL0/81.718s,casegeometry_current_queries_20261109T001022_1200684,
allpinsunchanged/all5handlesabsent. EXACTcommon163356bounds/order, work-0.5894%
(19,648,840→19,533,020),164858domainsvs163356at20Mcap, parametercoverage81.9035%
vs81.8703% NOTphysicalarea/proof. Gain is SMALL, no substantialclosure claim.
Reverse287 TERMINAL0/188.780s,casegeometry_current_queries_20261109T001246_1206798,
allpinsunchanged/all5handlesabsent. EXACTcommon58737bounds/order,work-0.9464%
(15,307,265→15,162,399),60892domainsvs58737at20Mcap. Query177.592s versus193.060s;
walltime is not alone causalproof. BOTH directions incomplete; gain SMALL, do not
claim solved wholecertificate. Candidatecheckpoint cbaba0b675 PUSHED.
Serial287 TERMINAL0/198.989s,90cases/67446assertions/allpinsunchanged/all5handlesabsent,
casecurrent_production_20261109T001710_1218462. ONE MPI6milestone ACTIVE tool8729,
caseplacement_mpi6_20261109T002042_1226839,runner1227056. Hostload3.37/8CPUs,
availableRAM47.484GB; no pressure. Allcompiled/pinned code/helpers FROZEN. Nextseal
qualification→immediate fullsame-seed TWO-event M6. No extra MPI campaign.
No competingcycle, no unchangedbaseline.

ZERO accepted meshes. First nativeevent REFUSED mandatory final globalTWO-SIDED
geometry proof on elapsed-timebudget; feasibility UNDETERMINED. Exit25175/native
5344.368047s:736019cells,0q/0J/0LOCALgeometry/0size, ALLknown/flagscomplete. Local
closure is NOT anacceptedmesh/globalproof. No transfer/adaptedCFD/diskrestartproof.
Lastprequiet complete geometryentry25169/native4621.108681s had0q/J,G16. Sixrounds
closed16G (+33tets); final~719.5–719.9s/rank spent3fullqueries. Traversal121–128Mwork/
622–640s,scopebuild52–59s,facetprep24–40s. Same thirdentity bottleneck as278 byquery
order, but FORWARD vs REVERSE still unproved. Earlier2queries returning doesNOT
establish their bounds within tolerance. A283/terminal_certification_diagnosis.json.
Nexttarget SPEED, not newconstruction/repair operators: all localgates closed,
finalfull query consumes quiettail. Do not credit cleanup (NEVERran) or claimglobal
LEbenefit. 283qualification/controlledcoverage improvements did NOTcomplete M6.

ALL30rawfiles COMPLETE/736019rows/noomissions/metricscomplete within128MiB/filecaps.
A283/capture_ready.json + pin_check_before_owned_stop.json seal ALL1520pins unchanged.
EXACTLYONEpidfdSIGTERM sent to MPI860479/start21882642 afterexplicitrefusal+sealing.
A283/owned_stop_once.json. NEVERrepeat seal_stop_m6283.py or anyhistorical stop.
Main63535 TERMINAL1/5538.896861s,pins_verified=true,changed=[],workflow_accepted=false.
All11actualHOSTownedidentities TERMINAL inA283/terminal_handles.json. Codefreeze
from M6released. TEST-ONLY build284 TERMINAL0/51.084s, allpinsunchanged,
production CFD BYTE-IDENTICAL283; helper1109327/ninja1109359 verifiedabsent.
TEST-ONLY284 directional diagnostics now BOTH TERMINAL0/allpinsunchanged/all5handles
absent each; NO owned heavy workload live. Source case
geometry_current_queries_20261108T233340_1112519:20Mwork exhausted,19.649Mtraversal/
72.274s,query73.805s,81.8703%equal-facetparametercoverage (NOTphysicalarea or proof).
Reverse case geometry_current_queries_20261108T233546_1117571:19,999,935work exhausted,
15.307Mtraversal/121.810s,query193.060s; ALL53488rootfaces visited, then5249childdomains.
Forward nested graphcoverage10.886Mwork/lift1.974M; reversecoverage5.582M/lift6.399M.
These are NESTED within upper/traversalwork, never add overlapping counters.
No outsideproof; both remain incomplete. R/certification284_direction_diagnosis.json
and each direction_work_assessment.json seal actual work, caps, trace hashes and
terminal receipts. Only CNativeAffineReference_tests.cpp changed; production CFD
BYTE-IDENTICAL283, no productionAPI/config/operator changes or correction chosen.
Hidden fixture legacy model_bound denotes reverse upper bound in reverse-onlymode;
commandfilter/directionassessment disambiguates. No extra MPI milestone for this
test-only diagnostic. Next analysis: reduce measured footprintcoverage work using
existing supports/exact membership; reverse lift remains substantial too. Preserve
all holes/seams/features/body/height/fullBoth gates and resource accounting.
Actual /proc/ps MUSTrequire_escalated:
sandboxabsence previouslycaused false terminal inference (corrected beforeaction).

Qualifiedsource61db48895b PUSHED; lateststartup/progress checkpoint4c2f49a74c PUSHED.
Full283terminal/stop/diagnosis PUSHED fd9147dced. CFD SHA
b885585c5229d67bb01929c5737d175d39d7949c7a098a0789ef960758a1204c; workflowtest_driver
SHA d8057cf0e9bba284f16422c58312cf4b00525dda80310f538f12988311c6578d.
ConfigSHA5ad2dc1d9a1cd8bff87f633617b191b50defbc261ab3d0b2ed0987a34722b89e.
Mach-onlyL1/50k,native5400/outer14400 unchanged. StartingREALsavedCFDseed reused;
61330coords/22fields BIT-IDENTICAL256/qualified254target,SENSOR_MACHonly,
CL.269535/CD.013536. Initialoriginal-grid proof only; noneadapted afterrefusal.
Eight unchanged transfer/restartreadiness paths match actual283pins in
A283/transfer_restart_readiness_reuse.json; sourceauditONLY, notruntimeproof.

Current actualcapture is complete. Prepared actual283thirdentity:37202reference/
53488physicalfaces,69520totalphysicalfaces,736019rows; all raw hashes/coordinates
verified by prepare_final283_entity2.py in14.310s. Query and provenance are in
A283/final_certificate_entity2_v1. TEST-ONLY284adds source-only/reverse-only choices
to the existing hidden fixture; no productionAPI/config/operator changes.
Current actualcapture is suitable for bounded residualglobalquery export.
Reuse prepare_final278_entity2.py for actual283thirdentity, then existingAPI source-
only and reverse-only upper queries under same20Mwork/8GiB/600s caps. Need separate
costs before productionchanges; reverse upper-only conservative refusal is NOT an
outside proof. No newbaseline/perf/GDBcampaign. ExistingFullBoth hiddenfixture now also supports TEST-ONLY source-only and reverse-only
modes; productionCFD byte identity verified. Reuse completed284evidence rather than
repeating these queries. One connectedcycle, no new
scheduler/config/representation/largercavity/quality-awareconstruction/gate orbudget
change. Unproven containment/branch/phase-specificwitness ideas inbacklog, notchosen.

Timinginterpretation: atmatchinground23872, commits27417201 vs28317029; netq/commit
1.137 vs1.142, traversal700.8M vs682.8M, qualitytime1462.65 vs2219.91s. Repairyield
similar; walltime slower. Hostload~15/8CPUs,ranks~78–79%CPU/elapsed(includesMPIspin),
kernel schedstats flag0 so queueobservations do notestablish solecause. Sampledtree
RSSpeak14.0215GB, NOTphysicalpeak; no actualmemorypressure. Lastmidrun~9GB.

Read-only analysis: verify_m6283_initial.py already passed; analyze_m6283_frozen.py,
analyze_m6283_quality_live.py. Old_quality.py assumes measured matchingwindows;
livecopy preserveschurn/prefix/location assertions. Firstuninterruptedqualitypass
only for cumulativechurn; latergeometryreturns altercounts outsidecounters. No new
baseline. CurrentParserfirstevent only; do notmisinterpret original-grid continuation
afterevent1refusal asadapted CFD. CurrentM6was stopped becausefailedfirstevent leaves
insufficientcycles forTWOaccepted meshes, notbecause partialrateforecast.

## Connected correction and qualification

[Design/evidence](Handbook/ADAPTATION_PREPARED_SOURCE_CERTIFICATES_20261108.md).
Actual278 saved thirdentity37202reference/53406physicalfaces was expensive.
Exact offline topology reproduced physicalprojectiongroups21230/21103faces refused
because3/4group-boundary pinches; all7vertices have fullphysical manifold cyclelinks.
282/283 rebuild a refused OPTIONAL joined graph ONCE after omitting incident faces.
UNCHANGED strict FinitePlaneReference validates the subset. Every original triangle
remains in complete distance/index/singletriangleupper support; holes never filled.
Only kept faces map to graph slots/lifting. Remaining refusal uses original traversal.
Transient maps/vectors admitted/workcharged; no persistent metadata increase.
279/281prepared-domain variants were assessed and REPLACED, not layered.

At SAME20Mwork, disjoint certified equal-facet DYADIC PARAMETER area70.3478%→93.2310%
(NOTphysicalarea). Physicalgraphs recover3/omit33witnessfaces; model29/omit906pieces.
Actualquery STILL INCOMPLETE before reverse; no fullcertificate/capability claim.
283 ALSO stops PlanarReference convexity tests atfirst exact nonconvex counterexample;
later crossing/topology/coverage guards unchanged. ALL non-timequeryfields, counts,
graphresults EXACT282. Preparation420.931→85.541s,totalquery518.812→182.754s;
traversal97.881→97.212s. R/convexity283_bounded_comparison.json seals it.
No fullM6 speedup established yet. Reverse cost and remaining6.77% were unmeasured on that older278query; current283
directional diagnostics above are a DIFFERENT physicalmesh, not a timing baseline.

Build283 TERMINAL0/49.216s, allpinsunchanged/handlesabsent. Probe TERMINAL0/249.056s,
pinches/trueholes/storage/curves/cache/work controls passed; actual20Mproof incomplete.
Affectedserial86cases/63018assertions212.587s; ONE MPI6 milestone120cases EVERYrank
241.121s. ALLpinsunchanged/ownedhandlesabsent. R/certification283_qualification.json
explicitly qualifies implementation/boundedprogress ONLY, actual certificate_complete
false/full_mesh_accepted false. Current fullM6 must satisfy original finalBoth gate.
No further production variants while frozen; use full outcome to select next work.

Read-only analysis: verify_m6283_initial.py (already passed), analyze_m6283_frozen.py,
analyze_m6283_quality_live.py. Old_quality.py assumes a measured matching window
and can assert atentry; livecopy allows absentmatches/no windows, preserving all
churn/prefix/location assertions. MONITOR-only, no solver/pinned sourcechange.
Existing complete six-rank prefixes/counters versus274,
no new baseline/perf/GDB. Compare matchingstage work, net wholefield q/J-bad removal/s,
geometry-bad, rank busy/wait/debt/churn. Do not add overlapping timers. RankCPU can
include MPIspin; elapsed is contention/cache/ownership sensitive. Geometry bounds
may legitimately change construction decisions under unchanged validity policy.

## Preserved terminal evidence and cleanup scope

278 firstevent REFUSED final whole-surface TWO-SIDED geometry proof on elapsed time.
Exit24869/native5336.348s:734914cells,0q/0J/0LOCALgeometry/0size, ALLknown/censuscomplete.
One round closed last2localG, +3work/+2tets. Quiettail was globalcertification.
No acceptedmesh/transfer/adaptedCFD/restart. Cleanup NEVER executed. All30rawfiles
complete/734914rows,all1520pins sealed. ExactlyONE pidfdSIGTERM only after evidence,
MPI241348/start20630721. ALL11oldhandles absent inA278/terminal_handles.json; main
35468 TERMINAL1/5562.946s. NEVER repeat historicalstop/seal_stop_m6278.py.
A278/terminal_certification_diagnosis.json:3fullcalls/rank~1318.7s,~157–158Mtraversalwork.
Earlier2queries returned, not automatically proof they were within tolerance.

[Post-refinement cleanup](Handbook/ADAPTATION_POST_REFINEMENT_CLEANUP_20261108.md):
source eligibility gap confirmed, four actual clean262contexts selected/recomputed.
Three rawcollapses plus one private collapse+optimizer replacement strictly pass
local q/J/geometry/metric/height/interface and remove1point/decrease short deficit.
Opposite directions can fail positivity/support/geometry; not all collapses feasible.
Integrated273 finite one-shot improvement-only short-edge pass reuses existing
selection/claims/versions/publication/ordinaryCoarsen gates; no hard minimumedge.
Already strict meshes can accept without optionalcleanup.274/278/283 NEVER ran it, so
GLOBAL LE cleanup benefit/causal explanation remain unproven. Do not widen policy.

Latest meaningful fullinspection A274/inspection/rejected_mesh.vtu (+su2):148781pts/
734806tets,captured_nodal_metric9 SPD,q/J/localgeometry allknown inCellData. Exporter/
readers terminal; exact topology,positivevolumes,all rawID-array joins verified.
FinalBoth incomplete, REFUSED.278near-identical state need not trigger pertrialexport.
262full148608/734315 has4UNKNOWNq/J and4localG, NOTshape-certified.271partialcapture
644221/735624 must not be filled from other runs. Preserve unchanged128MiB/filecaps.

## Acceptance, transfer and actual diskrestart still required

Do not equate run_m6.py workflow_accepted alone with numerical transfer proof.
Audit trueCVmass residual<=1e-10 for EACHfield/timelevel, scaled TARGETintegraldefect
<=1e-12 and full admissibility before publication. Hardtargetdefect/admissibility
source gates exist; old/new-domain integral difference is NOTtargetdefect. Existing
PrintSummary exposes trueResiduals; targetdefect not numerically printed. Source/
runtime readiness audit is not actualproof. Driver cycles0..nCycles includes final
adaptedCFD afterevent2. Original-grid continuation after refusal proves none of this.

After TWOaccepted: isolate ACTUAL acceptedmesh+.native_ref+matchingrestart in newdir;
ADAP_LOOP=NO,RESTART_SOL=YES,READ_BINARY_RESTART=YES, explicit checkpointINPUTbasename
DIFFERENT from RESTARTOUTPUT, same qualified standaloneCFDbinary/runtime. Check
coords/order<=1e-12,finitefields,positive density/internalenergy and continuedCFD.
Cycle0 resume-overwrite/rejected-event consumption bugs remain open; isolatedrestart
avoids neither by pretending original-grid output is adapted. Then dedicatedBL/MPI,
then RANS500k. No completion narrowing or goal-complete from fixtures.

Missing shock allocation remains SEPARATE: sampled target4–6%chord vs existing3–4%;
cleanup/query speedup does not fix it. No norm/sensor/complexity/geometrymetric change.
L2 recovery projection differs from Lp objective; CAD/paper criteria are not our gates.
Unrelated/unproven hypotheses in [backlog](Handbook/ADAPTATION_BACKLOG.md).
Prior detailed handoff [archive](Handbook/History/HANDOFF_before283_full_M6_20261108.md),
[final certificate history](Handbook/ADAPTATION_FINAL_CERTIFICATION_20261108.md).
