# Native adaptation — current handoff

Updated2026-11-12Europe/Rome. Goal ACTIVE. One connected curved-BLcycle/one heavy job.
R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
Read AGENTS.md, [ordered stages](Handbook/ADAPTATION_CURVED_BL_STAGE_INTEGRATION_20261111.md),
[checkpoint](Handbook/ADAPTATION_BL_TARGET_CHECKPOINT_20261111.md),
[current target assessment](Handbook/ADAPTATION_BL_TARGET_ASSESSMENT_20261111.md),
[normalization](Handbook/ADAPTATION_CURVED_BL_NORMALIZATION_20261111.md),
[timings/work](Handbook/ADAPTATION_TIMINGS.md), [HowToProceed](HowToProceed.md).
Historical live handles are superseded by this file; [pre397history](Handbook/ADAPTATION_HANDOFF_HISTORY_PRE_397_20261111.md)
preserves detailed394/396provenance; earlier histories remain linked there.

## Source/build/owned workload — authoritative

No owned heavy workload. Build399 and actual399 diagnostic are terminal/PASS;
actual-host PID+start identities rechecked in R/terminal_recheck399.json. All1652
actual-case source/binary/input/helper pins reverified before documentation edits.
Unrelated host Python collector left untouched. R/target_patches399_qualification.json.
Build399210.225s; CFD BYTE-IDENTICAL398:
307c62b04384cbf7f2d7ace740d540e0c64edac30ed66dfa76902701225080a0;
test6b81a43a4c3755a8c5d4e60749b01d0d691fd939371553d6bea73b31d08d0558.
Source/evidence398eeeb02f562PUSHED;399diagnostic checkpoint798db15fffPUSHED.
Actualcase R/cases/curved399_checkpoint_target_patches_20261111T231039_3485346
PASS115.188s. Single fixture adds bounded25+84 current metric-frame export using
ALREADYcomputed tensors/samples; no added geometry queries. Saved patch measurements
BYTE-IDENTICAL398, same146337atlaswork. Reuse398MPIpolicy qualification; no new
MPIcampaign for diagnostic output. NativeMPI BLgeneration remains unqualified.

Independent R/saved_row399_sensitivity.json reproduces centroid q/J to1.943e-16.
All25saved-row cells remain q/Jbad despite positivity/geometry/firstheight passing.
All5wall cells have frozen-centroid fixed-face/allowed-plane-height optimum q>.2:
this CONTROL does NOT prove target incompatibility. Existing apex normal alignment
>.99999992. Rotating only tensor frame toward face normal remains insufficient;
finer tangent tensor controls improve wall cells but20other cells staybad. Controls
are NOT position-dependent target evaluations, actual moves or admissible replacements.
Saved coordinates are older358construction, original396P1source is a controlled field,
not a reconstruction of old generated-node transported tensors. Union84has incomplete
adjacent layer heights; do not relax them. Existing MoveVertex handles one fullstar;
RepairMetricConnectivity fixes cavity skin, so cannot test coordinated wall motion.
Reuse existing SplitSurfaceStrip/RefitWallSplit/rowtransactions for any justified
coupled proposal, with live complete support/current nodal transport/globalclearance.

NEW read-only R/composed_gradation399_assessment.json independently reproduces ALL
123186nodal composed transport violations/max3115659.1964465; core0bad. Wall→wall52955,
wall→nonwall2145, reverse5780, nonwall→nonwall62306. Nonwall includes otherboundary.
52945wall→wallbad pairs have eigenratio>1e4 at bothends; median dominant-axis change
.29975degrees, maximum89.4494. Worst ratio pair64→4184 turns30.08degrees nearroot/LE.
Dominant axis is not a certified normal; no continuous gradation/causal noise proof.
No target correction selected from counts or local curvature proxies alone.

Edge investigation disposition:383whole-edge exclusion remains qualified in BOTH
factories.394shared projection fix already resolves original measurement deadline;
no lower-precision/classification scalar substituted. R/edge_measurement399_audit.json
and Handbook/ADAPTATION_BL_EDGE_ACCURACY_20261112.md audit classification/ranking/
insertion/conformity/final callers.394saved trace has248/279initial intervals narrower
than1e-10parameter consuming3720/6375evaluations; cuts retain ownership/transition
information. G7/K15difference is not a conservative interval. No exposed conservative
composed-speed bound justifies skipping these samples or early classification yet.
No extra baseline/perf/GDB or new scheduler/config/limit. Remain in ordered stage2.

All396producer/reload/build/serial/MPIhandles actual-host terminal; full pins verified
BEFORE edits. Qualified396 source/evidence2652ccf84aPUSHED; sourceimplementationb2d18d8f30.
Qualified complete target checkpoint396, R/target_checkpoint396_qualification.json:
producer ...T214613_3294100 terminal0/733.225s; freshreload ...T220013_3323327 terminal0/
546.920s.61330/61330owned composed tensors BITEXACT, same full/base profiles/reference/
sourcecore, no metric recompute/CFDadvance. Same chord42.06659984296543BITEXACT,
4635evals; producer6258743work/152.860sbusy, reload6205073/151.447sbusy(owner5).
Fullpaths under R/cases/curved396_saved_{cfd_metric_stage,target_reload}_... .
All12target/chord files303787984B local and hashed in target_checkpoint396_producer.json.
Checkpoint controls/family/source ownership/reference must match. Witness tensors
are comparison data, NEVER interpolation/transport authority. Original CFD/mesh
payloads intact. Replay uses the five successful profile mutations, not finalscale recipe.

Build396CFD01fcb08eb89209f69e50bb1b6b1efc684092703c252d99c0928208d6773f215e;
testbb3223c1ea5330b3b72836fcd5ddb143d4cd811b8096dda006362a7c22cdcec1.
Sourceimplementation/focused evidence b2d18d8f30PUSHED; previouslivehandoff b78730fc18.
Build396PASS208.210s; serial4cases/221assertions4.373s, broader39525/3120/65.497s reused
for unchanged behavior. MPI6 25cases/2866assertions EACHrank4.445s. Short MPIworkers
missed by1s sampler: no memory peak claim. Actual producer/reload are separate MPI6
stage evidence, NOT native MPI BL GENERATION (FIRST_ROWnative1guard stays).

## Current stage2 output and next bounded check

Same saved CFD/config, Mach-onlyL1, nominal500k, h0=2e-6,g1.2,T=.01 unchanged.
396provisional composed estimate461495.99094 with47864.00778empirical indicator,
NOT bound. Tangent factor.25/core.004607250072048262. Physical target suitability
UNQUALIFIED. Source61330points/303359tets unchanged; no generated BL. Core nodal
complexity119826.626;49.525%ofCORE allocation beyondr20 is not a composed decomposition.
Core directed gradation0bad at1+1e-5 after256cap; composed123186bad/worst3.11566e6.
Rapid prescribed normal variation is not itself a bug; no continuous combined proof.

Metric531.649smaxrank/setup50.307/build480.941; geometrywork45356241vs37171299713.
Preparation140.557svs612.375s with same6516query/8192proposal counts.2distinctfield
trials vs3; selected scale/core differ from371. NOT a fixed-target timing comparison.
R/metric_stage396_assessment.json and timing ledger retain overlapping timers/gaps.
Checkpoint write+join41.695s; reload42.681smax/load +293.775squery+election(join includes
wait), then151.447sowner chord. Chord result_join excludes earlier election wait.

Read-only witness inspection checks all61330core/coordinates against producerJSON;
42actual localcurvature samples/5contexts in R/target_directions396_assessment.json.
Leadingedge medians combined tangent density108/32 vs local osculating proxy640/34.
That proxy is NOT a conservative geometry bound, admissible replacement or sufficient
cause. Mapped BL wall evaluation currently has no analytic curvature cap; final
geometry certification remains independent. Do NOT add a curvature metric from this
alone. Existing unproven alternatives remain in Handbook/ADAPTATION_BACKLOG.md.

Exact saved-policy5-context/25-cell/84-neighbor assessment completed398/399,
not pending. Next bound actual coupled placement/construction feasibility using
complete existing supports and operators; distinguish physical target requirements
from incomplete transition and constrained skin. Do not choose a new curvature target
from sampled proxy or matrix controls. The stage2 suitability limitation remains
explicit before declaring surface input credible. Current diagnostic/control evidence
and scope are in Handbook/ADAPTATION_BL_TARGET_ASSESSMENT_20261111.md.

R/cases/curved396_saved_cfd_metric_stage_20261111T214613_3294100/selected_core_native.bin
is an exact existing-format derived CORE-only export, SHA in core_allocation_assessment.json;
use together with complete checkpoint, NEVER as complete target by itself. Independent
Mach error6.66e-16; sourcecoordinates and9flow+SA admissibility stage1 unchanged.

## Reusable inputs and preserved completion

Stage1 R/m6_bl_inputs/rans_metric_entry364_v1. Load without another CFDadvance.
Mesh8c839170eb5e359f7de6a41434aa6a9c2d620053f37de6d9360bd04d2a63088b;
reference2e99fb7870e2c718f110da8509e4c0f7e746ce138e7f27ab57aa0dedbb6c42de;
entryfieldaf7ade468dc40cd39dbe9238248e50057cdd151a64fef92fda957677aa3f6cae;
config0a7de022729f69b170deaee2eaabbd5347550da8134cbdaa39587f7173e78742.
Original303seedc90000ea804c7e08a54dbec93f218b5bd3ae3dce3db8c287db6783fd138f1c78.
All9fields finite/admissible, positive rho/internalE, SA>=1e-16wallfloor. Coarse
CL.15573/CD.05027reasonable, NOT convergence/shock/wall-resolution proof.

Euler293 COMPLETE: TWO strict Euler50k meshes149051points/736323tets and172001/
870422; conservative admissible transfer, finite continued CFD and isolated adapted
restart. ContinuedCFD one iteration, not convergence. Preserve original293event1/2
checkpoints and disk restart, all22payloads/384685699B preservation receipt unchanged.
Full364 surface stalled before row; saved358row5columns/25cells has geometry/height/
positivity but q/Jdebt,84-cell union has34neighbor-only height failures. No usable M6BL.

## Constraints and remaining objective

Incremental stages2target→3coupled surface→4directions/firstrow→5growth→6transition/
strictclosure→7transfer/CFD/second adaptation/restart. Physical progression must be
measured: isolated current UnitStep gives~1.199402677 at g1.2, not exactly1.2;
normal_progression396_source_review.json is an analytic control, NOT actual growth.

One owned heavy workload, nice19/j3(2–4workers), OMP/BLAS/MKL1.4TB compile-tmp;
verify actual-host PID+start terminal before C++/helper edits or new job. Sourcefreeze,
bounded active-reference/transient accounting, checkpoints/pushes, CADoptional/noCAD
authority, validity-first intermediate meshes remain mandatory. Native5400s/outer14400s,
12GiB/rank; diagnostic atlas300s/outer600s unchanged. No config/scheduler/geometry
representation/permanentcolumns/raised budgets.500k nominal, NOT node count/gate.
Strict geometry1e-4, exactpositive/bodyseparation,q>=.2,J>=.05,maxedge1.8,h0/g/T,
heighttol.1 final gates unchanged. Intermediate diagnosed quality debt allowed.

Remaining: strictly accepted usable curved BL, dedicated native-MPI BL generation/
constraint qualification, TWO accepted RANS adaptations, conservative admissible
flow+SA transfer, finite continued adapted CFD and actual isolated adapted disk restart.
