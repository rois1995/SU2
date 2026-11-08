# Native adaptation — current handoff

Updated2026-11-08 Europe/Rome. Goal ACTIVE, **ZERO accepted meshes**.
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

## LIVE — qualified283 fullsame-seed TWO-event M6 FROZEN

ONE owned heavyjob: tool63535, launch_m6283.py→run_m6.py→MPI6 test_driver
[NativeSelectedFailureWorkflow3D]. This is the real CFDdriver with refusal observer,
NOT a fixture that requires rejection. Source checkpoint **61db48895b PUSHED**.
Command expects2events, Mach-onlyL1/50k, native5400s/outer14400s unchanged.
A283/owned_startup.json verifies ALL1520source/build/binary/runtime/config/inputpins.
ConfigSHA5ad2dc1d9a1cd8bff87f633617b191b50defbc261ab3d0b2ed0987a34722b89e.
CFDSHA b885585c5229d67bb01929c5737d175d39d7949c7a098a0789ef960758a1204c.
Workflowtest_driverSHA d8057cf0e9bba284f16422c58312cf4b00525dda80310f538f12988311c6578d.

Exact owned identities (PID/start_ticks), confirmed live in startup receipt:
wrapper859900/21881293; runner860220/21882037;
time860477,timeout860478,MPI860479 ALL /21882642;
ranks860482–860486 ALL /21882645, rank860487/21882646.
NEVER restart from stale logs/observation timeout: verify actual owned handles.
NEVER stop these without preserved outcome or authorized demonstrated bottleneck/
error/resource pressure. Before any stop recheck exact MPI identity and seal evidence.
No repeat signals to historical runs. No other active agents/build/tests/heavyjobs.

Starting REAL savedCFDseed R/m6_inputs/euler_original_force_seed_v1 reused.
61330points/303359tets, M.8395/AOA3.06. CL.269535/CD.013536 unchanged. All61330initial
coordinates/22fields BIT-IDENTICAL256 and qualified254target; SENSOR_MACHonly.
A283/initial_cfd_target_equivalence.json and initial_verifier_reuse.json seal it.
This is INITIAL original-grid CFD/target proof, not transfer/adaptedCFD acceptance.
Initial ownership migrated139925cells. Qualityentry round23306/native783.250s:
690804cells,27052q/27499J/1204geometry/0size, ALLknown/flagscomplete.
Latest sealed qualitywindow23424/native1342.748s:704561cells,20067q/19324J/1146G/0size,
allknown. Netq6985/559.498s=12.484/s,J8175/559.498s=14.611/s; slower than274near562s
(17803q/16890J/1117G). Private commits3640,netq/commit1.919; substantialchurn remains.
No stuckshape/missingfeatures/private per-trialcollectives. Debtimax/mean1.343;
mostremainingq interior(13676 vs6391surface; interior may have boundaryvertices).
Still progressing; keepfrozen. No acceptedmesh or transferproof.
Startup hostload1.92/8CPUs,MemAvailable48.39GB; later43.54GB. Sampled treeRSS~4.79GB
at232s, NOTphysical peak. Cache hostquota4GiB/rank715827882bytes; native working
ceiling12288MiB/rank is a limit, not measured use. No actual memory pressure.

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
No fullM6 speedup established yet. Reverse cost and remaining6.77% are unmeasured.

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
Already strict meshes can accept without optionalcleanup.274/278 NEVER ran it, so
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
