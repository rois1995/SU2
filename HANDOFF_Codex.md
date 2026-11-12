# Native adaptation — current handoff

Updated 2026-11-12, Europe/Rome. Goal ACTIVE. One connected curved-BL cycle and one owned heavy job.
R = `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
Read AGENTS.md, [ordered stages](Handbook/ADAPTATION_CURVED_BL_STAGE_INTEGRATION_20261111.md),
[complete checkpoint](Handbook/ADAPTATION_BL_TARGET_CHECKPOINT_20261111.md),
[current assessment](Handbook/ADAPTATION_BL_TARGET_ASSESSMENT_20261111.md),
[normalization](Handbook/ADAPTATION_CURVED_BL_NORMALIZATION_20261111.md),
[timing ledger](Handbook/ADAPTATION_TIMINGS.md), and [HowToProceed](HowToProceed.md).
[Pre404 history](Handbook/ADAPTATION_HANDOFF_HISTORY_PRE_404_20261112.md) preserves older
provenance and failures. This file supersedes historical live handles.

## Current source, build and workload

NO owned heavy workload. Build404, serial, MPI6 and actual404 PID/start identities
are verified terminal on the actual host. All 1,656 actual404 source/binary/input/helper
pins were reverified before documentation/checkpoint work. Qualified404 source/evidence f4ba305a3c is PUSHED.
Earlier798db15fff/ef3dd43e0d remain preserved. Keep unrelated working changes intact.

Build403 (production fix) passed in 418.466 s; build404 (test only) in 40.123 s.
CFD is identical between403/404:
`16d88dd02469c169aa852e444719c960bb97cd114f16e4bb746c85818eb73dbd`.
Test404: `e8859478926642bd0accad875c497782dd23b06c9f2bfa948b1b7db1cb05e2e7`.
Serial: 11 cases/390 assertions, 4.460 s. ONE connected MPI6 milestone: 11 cases on
EVERY rank, 250–271 assertions/rank, 4.455 s. Covers preservation beside unfinished
rows, migration, empty ranks, curved geometry and deterministic resource expiry.
This does NOT qualify native MPI BL generation.

Actual404 passed in127.173 s:
R/cases/curved404_checkpoint_target_repair_20261112T000440_3612660.
R/height_preservation404_qualification.json binds the comparison and qualification.
Same402 baseline patch/target repeats exactly. Repair:64 trials,6 coordinated steps,
36 vertex moves,1,442 fresh compositions,540,608 geometry work,11.874 s; two height
rejections. All25 cells positive; strict geometry passes (max6.404e-7); first height
passes (max relative error.0839683<=.1). BUT all25 remain centroid q/J-bad: net retirement0/0.
Independent q/J error<=1.388e-16. No speedup, closure or accepted-mesh claim. Exact edge
conformity, external clearance and publication remain unmeasured. Largest sampled
individual RSS1,501,868,032 B is not whole-job peak.

## Qualified correction and preserved failure

RepairMetricConnectivity used provisional AdmitFirstHeight for every quality trial.
ValidityOnly intentionally allows resolved height/footprint debt, so repair could
undo passing rows. Actual402 returned lower loss43.06597→41.16487 but height error
.297083>.1 and zero net q/J retirement. It failed two independent checks and was
never published. Terminal129.280 s; all1,654 pins/actual-host handles preserved in
R/patch_repair402_assessment.json. Exact fixtures: R/source_snapshots402.

403 fixes all FIVE candidate paths: initially height-clean inputs use strict checks;
partial resolved debt uses existing admission PLUS PreservePassingFirstHeights
(working,proposal). The working baseline also protects children repaired during
previous steps. Final independent checking remains; no extra final height check for
empty-height Euler inputs. Unfinished layers remain diagnosed debt. No target,
normal law, construction policy, operator, scheduler, config, gate or budget changed.
404 ends at loss41.89264, with height preserved. Its changed trajectory is not a speedup.

Preserved development failures:400 compile include-interface error18.155 s;401 passed
212.224 s;402 required-input guard build208.213 s (CFD identical399).403 serial failed
one new fixture reachability assumption (390/391 assertions), with preservation checks
passing.404 supplies a deliberate interior defect and wall-aligned metric; unchanged
preservation assertions pass. Exact403 test: R/source_snapshots403. Read-only review
found no concrete production semantic defect. Full details live in the linked assessment.

## Active stage2 and next bounded test

Same saved CFD, Mach-only L1, nominal500k, h0=2e-6,g=1.2,T=.01,height tolerance.1.
Complete396 checkpoint fidelity is QUALIFIED; physical target suitability and usable
BL are not. Estimated complexity461495.99094, empirical indicator47864.00778 (NOT a
bound); tangent factor.25, core.004607250072048262. Source61330 points/303359 tets.
Core nodal complexity119826.626;49.525% beyond radius20 concerns CORE only.

Next target: repair power/coupled construction and target delivery, not another speed
cycle. The fixed25-cell patch retires no q/J failures and reports blocked interior
vertex39424. Its existing saved complete neighbor star forms an84-cell union inside
the current128-cell constructed-patch bound. Reuse that ONE support for a private
actual-field test, preserving five passing rows while retaining34 unfinished neighbor
rows as debt. Do not raise limits, start a cavity campaign or publish from incomplete
saved ownership state. Compare complete-patch net q/J, geometry, passing heights and
work; lower loss alone is insufficient. If wall shape remains blocked, distinguish
coupled surface modification from physical geometry/height target deficiencies using
the queued Coulaud/MAdLib assessment. No new metric is selected from proxies alone.

Current controls use original396 P1 on older358 coordinates, NOT evolved nodal metric
transport.402 shows all five unrestricted optimal feet outside their triangles;
constrained constant-tensor planar-foot q maxima.11509–.18838 are below.2. This is NOT
a varying curved-field impossibility proof. R/row_footprint402_assessment.json.
Local LE density proxy640/34 versus target108/32 is not a conservative geometry bound.
399 reproduces123186 composed nodal transport violations (max3115659.196), core0.
Wall-to-wall52955, cross7925, other62306; normal-profile/frame/noise causality is unresolved.

## Reusable checkpoints and measurement evidence

Stage1: R/m6_bl_inputs/rans_metric_entry364_v1. Reuse without another CFD advance.
Mesh SHA8c839170eb5e359f7de6a41434aa6a9c2d620053f37de6d9360bd04d2a63088b;
reference2e99fb7870e2c718f110da8509e4c0f7e746ce138e7f27ab57aa0dedbb6c42de;
entry fieldaf7ade468dc40cd39dbe9238248e50057cdd151a64fef92fda957677aa3f6cae;
config0a7de022729f69b170deaee2eaabbd5347550da8134cbdaa39587f7173e78742.
Nine flow/SA fields finite/admissible; CL.15573/CD.05027 is not convergence/shock proof.

R/target_checkpoint396_qualification.json: producer733.225 s, fresh reload546.920 s;
61330/61330 composed tensors BITEXACT, same profiles/reference/source core and chord
42.06659984296543/4635 evaluations. No CFD advance or metric recomputation. Full/base
history, OptionA, original family/owned layout and source geometry are required.
Twelve payloads303787984 B local, hashed in R/target_checkpoint396_producer.json.
Witness tensors are NEVER transport authority. The producer's selected_core_native.bin
is CORE ONLY; use WITH the complete policy checkpoint. Full paths in pre404 history.

383 whole-edge BL exclusion is qualified in BOTH factories; outside controls use exact
background.394 projection fix resolved the difficult edge:147.517 s/6375 evaluations.
Ten-point quadrature underestimates6.82%;248 tiny ownership intervals consume3720
evaluations but lack conservative composed-speed bounds. No approximate classification,
ranking, insertion or final scalar was enabled. See [edge audit](Handbook/ADAPTATION_BL_EDGE_ACCURACY_20261112.md).

## Preserved completion, constraints and remaining goal

Euler293 COMPLETE: two strict Euler50k meshes149051 points/736323 tets and172001/870422;
conservative admissible transfer, finite continued CFD and actual isolated adapted
restart. Continued CFD was ONE iteration, not convergence. Preserve22 payloads/384685699 B.

Ordered stages:2 target →3 coupled surface →4 directions/first row →5 growth →6 outer
transition/strict closure →7 transfer/CFD/second adaptation/restart. No usable curved
BL yet. Actual physical progression remains unproven; isolated metric law~1.199402677
at configured g1.2. Keep this stage5 obligation.

One heavy job, nice19,j3 (2–4), OMP/BLAS/MKL1,4TB compile-tmp. Verify actual-host handles;
freeze pinned sources; retain bounded active-reference/transient accounting and pushes.
Native5400 s/outer14400 s/12GiB per rank; diagnostic atlas300 s/outer600 s unchanged.
500k is nominal, not node count/gate. Strict geometry1e-4, positivity/body separation,
q>=.2,J>=.05,max edge1.8 and requested heights remain. CAD optional/no-CAD authority,
validity-first construction; no new scheduler/config/representation/permanent columns.

Remaining: usable strictly accepted curved BL, dedicated native-MPI BL generation and
constraint qualification, TWO accepted RANS adaptations, conservative admissible flow/SA
transfer, finite continued adapted CFD and actual isolated adapted disk restart.
