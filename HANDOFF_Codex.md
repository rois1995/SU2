# Native adaptation — current handoff

Updated2026-11-08(Europe/Rome). Goal ACTIVE;ZERO accepted meshes.
TWO strictly accepted ONERA M6 Euler CFD→mesh→CFD→mesh events at50k METRIC
COMPLEXITY,conservative admissible transfer,finite continued ADAPTED CFD and
ACTUAL isolated ADAPTED disk restart. Then dedicated BL/constraints/MPI
qualification and corresponding RANS500k. Actual points/cells are separate.
Fixtures,private candidates,balanced weights and lower work/memory are not completion.

Strict geometry1e-4,q>=.2,J>=.05,maxedge1.8,requested heights. Validity-first
construction:NO unnecessary fresh q/J/ranking/veto;exact positive/nondegenerate
cells,topology,geometry authority/body/features/requested heights preserved.
Boundary redistribution on prescribed geometry allowed. CAD optional;no-CAD
geometry sufficient. ONE active cycle/ONE owned heavy job. All pinned C++,
binary,helpers/config/runtime/inputs FROZEN throughout build/validation/run.

R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
A256=`R/m6_runs/euler_build256_canonical_geometry_debt_mach_only_real_cfd50k`.
A254=`R/m6_runs/euler_build254_conditioned_mach_only_real_cfd50k`.

## CURRENT — full256 two-event M6 RUNNING FROZEN

Executing C++implementation42a3fedcc7;qualified/full-start checkpoint3b580b2a7e;
fifth-census checkpointd27d6dc35e. GitHEAD may be a later metadata-only checkpoint.
Tool10984 is LIVE (last poll returned running);A256/status.json running.
Launcher1752458/start14512699,runner1752768/start14513409,
time1753218/timeout1753219/MPI1753220 all start14514129.
MPI test-driver workers1753223/1753224/start14514142;
1753225/1753226/1753227/start14514143;1753228/start14514144.
Exact11argv/cwd/identities:A256/owned_startup.json. These are actual integrated
CFD-driver workers,not standalone CLI PIDs. Stop ONLY verified owned handles,
only after preserved evidence satisfies existing authorized stop conditions.
An observation timeout/stale log is NOT termination. Never restart from that alone.

Latest observed complete window round26112,native4406.013439s:
733808cells/0q-bad/0J-bad/192geometry-bad/0size-bad.
ALL cache/census/shape/geometry completeness flags true. No acceptance yet.
NEW actual shape closure:allq/J/size zero;geometry_repair_entry24512 has775g,
next complete24576has658g,and26112has192g withq/J/size stillzero. First pass exited24328with
2q/0J/969g,then existing construction returned187q/202J/813g for reclosure.
A256/shape_closure_geometry_repair_entry.json seals these transitions;
geometry_only_progress_snapshot.json records later complete geometry-only windows. This
refutes assuming new retained-star orchestration is already necessary for shape
closure;keep alternatives in backlog and wait for the actual terminal outcome.
Geometry-only mode now active;old254never reached that matching stage.
First-pass quality_progress/comparison254 must NOT be presented as live g-only data.
Current counter files:A256/phase_recovery_progress.json,quality_progress.json,
quality_counter_assessment.json,quality_comparison254.json. Read-only refresh:
`nice -n19 python3 R/analyze_m6256_frozen.py` (expandR;source-prefix hashes checked).
Comparisons use FIRST uninterrupted shape pass;ALL first-event phase transitions
are separate in phase_recovery_progress.json. Do not substitute partial/unmeasured
construction q/J or later phase frames for complete shape counts. First-event
parsers are not by themselves second-event acceptance proof.

ALL11identities/argv/cwd AND1520source/build/runtime/input pins match in
A256/geometry_phase_owned_receipt.json (latest verification). Sampled treeRSS9.71GB,
MemAvailable~30.1GB,diskfree10.801GB,no demonstrated RAM pressure. Monitor disk;
root/tmp unsuitable for scratch. Native5400/outer14400 unchanged,nice19/libs1,MPI6.
Host cache4GiB,rankquota715827882bytes EXACT254. No competing heavy job or source edit.

NEXT:let frozen256finish unless demonstrated failure/bottleneck/actual resource
pressure justifies authorized stop. Judge net WHOLE-FIELD q/J removal persecond
alongside geometry-bad counts,useful counted work,CPU/wait and rank spread.
Do not add overlapping timers,run a baseline/perf/GDB campaign,raise limits,
start another scheduler/operator/metric/geometry/construction implementation.
Preserve source and owned-handle proof before any later stop/fix/retry.

## Active correction and qualified evidence

[Current design/evidence](Handbook/ADAPTATION_GEOMETRY_DEBT256_20261108.md),
R/geometry_debt256_plan.json. Shared GeometryGuidanceExcess now uses complete-
census squared absolute-tolerance certificate excess,without face-area weights.
Actual raw contraction:q28→18,J25→17,failed PHYSICAL faces8→7,certified debt−15%,
maximum error unchanged,one fewer point. Old area weighting rises6% and vetoes it.
No operator,scheduler,support bound,metric,config,gate or budget change. Existing
monotone partial-sum early rejection,authority/association/invalidation/body/feature/
height checks and strict final certificates unchanged. Nonlocal production
selection/admission and collapse→retained-star orchestration are NOT established
by bounded controls;the isolated tip required that combination. Alternatives in backlog.

Build256TERMINAL0/480.354111s,all1303pins unchanged,both targets up to date,
all recorded build handles terminal. Focused serial58cases/46938assertions PASSED
165.939102s (current_compound_20261108T010915_1724241). ONE MPI6 milestone99cases
on EACHrank PASSED183.821737s (placement_mpi6_20261108T011332_1738581).
All source/runtime/input pins unchanged,all5serial/all10MPI handles terminal.
R/geometry_debt256_qualification.json seals both stages and3actual raw witnesses.
Do NOT rerun passed checks absent changed scope/failure evidence. Generic MPI99
is NOT dedicated BL qualification or M6 acceptance/transfer/restart proof.

## Same-seed comparison and actual target

Same saved REAL CFDseed euler_original_force_seed_v1 (61330points/303359tets,
M.8395/AOA3.06,user-qualified stable CL.269535/CD.013536,not full shock/residual
convergence). Actual initial22flow/metric fields+coords BIT-IDENTICAL254and qualified
corrected target:A256/initial_cfd_target_equivalence.json. ONLYSENSOR_MACH.
L1/50k,configSHA5ad2dc1d9a1cd8bff87f633617b191b50defbc261ab3d0b2ed0987a34722b89e EXACT254.
First quality23314:690715cells/27016q/27453J/1205geometry/0size EXACT254;all selected
per-rank geometry/support/cache/traversal/construction counters match,including
245091394upper-work. No pointwise absence-of-target-drift claim from these checks.

First31matched construction censuses have identical counts/flags/globalwork;
initial quality-entry rank counters match too. Native705.664versus254755.523 is
not isolated code-speedup evidence. At first2quality windows,new q20201/J19573/
g1144 at380.265s versusold20157/19485/1148 at388.130s;upper-work223125688vs
236680834(~6%less) for similar progress. At fifth window:new14069/12878/1078
at906.472s versusold14549/13445/1095 at925.586s. Explicit time gaps,not interpolation
or terminal forecasts. Later curve and strict closure determine usefulness.

[Flow/target diagnosis](Handbook/ADAPTATION_TARGET_DELIVERY253_20261107.md):sampled
compression target4–7%chord versusoriginal3–4%triangles explains little first
refinement THERE;not full lambda-shock identification/resolution. Corrected QR
base-fit condition bound254fixes71ill-conditioned raw Hessians,all flow fields
unchanged,sampled compression target5–12%finer. Farfield40.398%complexity allocation
remains physically unqualified;no norm/sensor/complexity change. Keep this target.
LE exact254edges.13–.19,station triangles8–15xoriginal;necessary geometry spacing
not established. All260terminal254bad cores own/touch CURRENT wing vertices:
117physical-face,143adjacent-volume,NONE free-interior. Provisional original-
reference-node classification was wrong and is preserved;use selectionv2ONLY.
Paper is a design reference,not gate equivalence or first50kbenchmark;recovery
L2projection differs from Lp objective. CAD remains optional.

## Preserved terminal254 and meaningful saved mesh

254REFUSED at last complete native5332.870105/round42620,time budget:
741236cells/251q/205J/95geometry/0size. All30raw sealed,capture_ready.json.
All1520pins verified after sealing BEFORE ONE pidfdSIGTERM to exact old
MPI1274730/start13602153;owned_stop_once.json attempts1/signal_sent,all11old
handles terminal. NEVER repeat this stop. Later original-grid CFD/adap-named
files are not accepted/adapted proof. No254solver alive;256is the LIVE run above.

Latest meaningful saved mesh remains A254/inspection/rejected_mesh.vtu and.su2,
150080points/741236tets,captured_nodal_metric9row-major PointData;
q/J/geometry_excess_l2/production edge bounds CellData. All260bad and2400neighbor
cells plus reference saved. Export55016TERMINAL0/329.411249s,1554pins unchanged,
all3handles terminal. Independent reader22015TERMINAL0/2.681038s:array lengths/
associations correct,VTU/SU2coords+tets identical,allmetrics SPD,alltetvolumes
positive,counts251/205/95exact,all input hashes unchanged. Receipts:
A254/inspection_{export_status,export_terminal_handles,reader_status,reader_terminal_handles}.json.
This is refused geometry,not acceptance. Large artifacts local;hashes/metadata
pushed. No per-trial export;only save another meaningful final/private checkpoint
when substantial implementation progress makes inspection beneficial or user asks.

254best shape34q/20Jwith728g;minimum60g with133q/122J. Guarded geometry
construction/reclosure executed,but late cycles churned to251/205/95. Source
conditioning254+qualified witness253cutoff improvements PRESERVED. Historical
253terminal censusUNKNOWN693003/1032510cells;its partial30q/20J are NOT whole-field
counts. All253handles terminal after one preserved stop;never repeat. Detailed
prior handoff preserved in Handbook/History/HANDOFF_before256_quality7_20261108.md.

## Acceptance,transfer and restart — ALL still required

Do NOT trust workflow_accepted alone. A256/transfer_restart_readiness.json records
verified current double/mixedOFFbuild/source guards;runtime transfer/restart
verified=false. ActualCV-mass true residual<=1e-10 EACHfield/timelevel;full
transferred-state admissibility and scaled TARGET-integral defect<=1e-12 BEFORE
publication. SU2_CFD/src/adaptation/ConservativeKernels.cpp454–457,
CDistributedProjection.cpp1434–1448,CConservativeTransfer.cpp997–1038(MPI)/
1275–1302(serial). PrintSummary gives true residuals,not numeric final target
defects. Distinguish executed hard-gate proof from an actually observed number;
old/new domain integral changes and post-CFD integrals are insufficient alone.

Require actual accepted mesh1/2+.native_ref and matching restart1/2 with exact
point/header/coordinate order<=1e-12,finite conserved fields,rho/internal energy>0.
Use existing run_m6_seed.validate_state through isolated matching-grid aliases.
After full owned workflow TERMINAL,pin/copy final ACTUAL adapted mesh/ref/restart
into NEW isolated directory;ADAP_LOOP=NO,RESTART_SOL=YES,READ_BINARY_RESTART=YES,
explicit actual checkpoint basename and DISTINCT input/output prefixes. Run actual
same-binary/runtime CFD continuation,validate disk read and finite/admissible state.
Resume-cycle0overwrite/rejected-eventconsumption bugs remain OPEN;isolation is
not a fix. Preserve/reuse independently qualified successful stages with provenance.

After TWO accepted Euler events+transfer+finite adaptedCFD+actual adapted disk
restart,do dedicated BL/MPI validation:curved walls,junctions,interfaces,subsequent
operations,ownership/memory,bounds/exhaustion,requested heights. Then RANS500k.
No Euler/RANS-label reconstruction modes;use actual target/geometry/heights.
One active cycle,one owned heavy job,resource/source-freeze/evidence/checkpoint
rules persist. Goal remains ACTIVE;never declare it complete from fixtures.
