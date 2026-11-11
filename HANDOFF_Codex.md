# Native adaptation — current handoff

Updated2026-11-11 Europe/Rome. Goal ACTIVE. ONEconnected curved-BLcycle/oneownedheavyjob.
R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
Read AGENTS.md, [ordered stages](Handbook/ADAPTATION_CURVED_BL_STAGE_INTEGRATION_20261111.md),
[current accuracy/exclusion design](Handbook/ADAPTATION_BL_EDGE_EXCLUSION_20261111.md),
[normalization](Handbook/ADAPTATION_CURVED_BL_NORMALIZATION_20261111.md),
[timing/work ledger](Handbook/ADAPTATION_TIMINGS.md), [HowToProceed](HowToProceed.md).
History [pre394](Handbook/ADAPTATION_HANDOFF_HISTORY_PRE_394_20261111.md)/
[pre393](Handbook/ADAPTATION_HANDOFF_HISTORY_PRE_393_20261111.md)/
[pre388](Handbook/ADAPTATION_HANDOFF_HISTORY_PRE_388_20261111.md)/
[pre384](Handbook/ADAPTATION_HANDOFF_HISTORY_PRE_384_20261111.md); historical RUNNING/
next-action text is superseded by this live state.

## Actual source/build/owned workload — authoritative

Current workload: NONE. All394build/serial/MPI/3pointsets/originaledge/accuracy
identities reverified terminal on actual host; full source/build/input pins intact.
R/prepared394_qualification.json.394source/evidence in this reviewable checkpoint.
Previous393checkpoint7689c8a942PUSHED; earlier failures preserved in linked history.
Build394PASS36.244s. CFD260fa2887c808d9d94828a1052a63634f7397b9303d26bf01ae912bfdec24c6c;
testb5fd5e2657570bc4aa9e2185f222ed289e691a68431848bf371dfc6f6deaa428.
Expandedserial83cases/22594assertions241.533s; MPI6 56cases EACHrank/5526–5729assertions
66.436s. All3actualpointsets pass:18default/tighter pairs bit-identical foot/normal/
FULLtensor/chordspeed/hn. Residualpointwork5117vs3937116, wrapper dominated byloading.
Native MPI BL GENERATION remains unqualified; FIRST_ROWnative1guard stays. Broad380
MPI84/85cases perrank: unrelated unchanged planar fixedcount3vs6failure in backlog.

394uses SAMEexisting KKT criterion during strict-interior Newton iterations after
positive derivative Gram determinant. Existing snap/active-edge logic, caps, seeds,
canonical interval upper/global lower/clearance proof unchanged.393immutable-region
cache reuse remains. No lower-precision measurement or classification substitution.

## Stage2 measurement blocker resolved; return to target/checkpoint

ORIGINAL394M6edge PASSES207.818swrapper under SAME300s atlas/600souter: converged
numerical length42.06899590894017;6375evals (4185initial/2190refinement), prep69.4869s,
composition78.0256s,totaledge147.5169s. CPU202.49user+1.10system. Error estimator
4.12715e-8 <=4.20700e-8allowance; G7/K15difference is NOT a conservative enclosure.
393refused atdeadline after16831successfulsamples/12960refinement. Numerical witness/
cut trajectory changes, so no fixed-work or accepted-mesh time comparison. Prepared
672202704B; largest sampled individualRSS2039324672B, not wholejob peak.
R/target_panels394_assessment.json. NO construction/acceptedmesh/fulltargetcheckpoint.

Independent existingaccuracy diagnostic PASSES217.869s: whole reparameterized G7/K15
42.068995904246883 agrees with direct integral to1.116e-10relative. WholeGL10estimate
39.199324065 underestimates6.82135%; widestinterval error.318579%. Both split/coarsen
choices same ON THISoverlongedge, not proof for other edges/ranking/insertion/final.
R/edge_accuracy394_assessment.json. No GL10replacement or precision relaxation.

Next: finish ordered stage2physical/directional target assessment and COMPLETEtarget
checkpoint/fresh-process agreement before surface generation. Reuse original saved
CFD seed/no advance, same Mach-onlyL1/nominal500k and normal law. Do not launch another
speculative query optimization cycle now. Existing diagnostic reconstruction from
finalcore/scales is NOT original371state: preserve exact full/base outer profile,
transition support/history and OptionA identity/radius as well as core/sourcegeometry.
R/target_checkpoint393_audit.json maps existing formats and gaps; primaryclarification:
WallData::vertexLog/maxima derive from immutable wall geometry and may be rebuilt;
raw private serialization is not automatically needed. Need actual same-target
point/chord/authority agreement after a fresh process, not sensor tensors alone.

Classification/ranking/insertion/conformity/final accuracy remain separate contracts.
Core nodal gradation excludes BLtensors; fixed-ray/profile continuation is not global
continuous composed gradation proof. Distancewidth does NOT bound normal/tensorerror.
Keep physical h0/g/T and strict geometry/q/J/size/height gates.383whole-edge exclusion
remains qualified in BOTHfactories; original5M6edges nonexcluded, outsidecontrols
exactbackground. R/edge_exclusion383_qualification.json. Coulaud/MAdLib geometry/
target/realized BL investigation remains queued inside this cycle; no new scheduler,
mandatoryCAD, geometry representation, permanent columns or configoptions.

## Stage inputs and credible-output requirements

Stage1reusable exact metric-entry seed: R/m6_bl_inputs/rans_metric_entry364_v1.
Load WITHOUT another CFD advance. All9flow+SA fields finite/admissible, positive rho/
internal energy, SA>=1e-16exact wall floor. Coarse CL.15573/CD.05027reasonable,
not convergence/shock/wall-resolution proof. Original303seed/checkpoints untouched.
Hashes: mesh8c839170eb5e359f7de6a41434aa6a9c2d620053f37de6d9360bd04d2a63088b;
reference2e99fb7870e2c718f110da8509e4c0f7e746ce138e7f27ab57aa0dedbb6c42de;
entry fieldaf7ade468dc40cd39dbe9238248e50057cdd151a64fef92fda957677aa3f6cae;
config0a7de022729f69b170deaee2eaabbd5347550da8134cbdaa39587f7173e78742.
Original303seedc90000ea804c7e08a54dbec93f218b5bd3ae3dce3db8c287db6783fd138f1c78.

Stage2ACTIVE:371saved-CFD MPI6metric-only passed1739.083s/429364assertions, no CFD
advance or construction; exact source61330nodes/303359tets unchanged. Same Mach-only
L1; nominal500k, provisional composed504040.450±48545.704empirical indicator, NOT a
bound. Tangent scale.28717458874925855/core.0047263575870155755; h0/g/T unchanged.
Core-only complexity140142.106;50.42%ofCORE demand beyondr20 is not a composed
additive decomposition or proof of recovery error. Core directed gradation0bad at
tolerance after256cap; composed nodal audit133745bad/worst2.71976e6. Continuous
combined-field gradation and physical target reasonableness remain unproven.

Exact371core export at R/cases/curved371_saved_cfd_metric_stage_20261111T111921_1888531/
selected_core_native.bin, SHA25f03abdbe5e3406af6725e093bedd94c8b1d05cd32586d35d74829e1f1b9cae.
This is CORE-only, NOT a complete composed-target checkpoint. Fresh diagnostic recipe
SelectTangentialScale→Update(final core) omits original family and monotone support/
bounds history. Need credible physical/directional/height/gradation assessment, then
complete disk reconstruction and fresh-process agreement before stage3surface work.
Reuse native_ref/record/donor persistence first; no complete codec implemented yet.

Full364ended before first row: surface391.619s/18commits/122work and shared deadline;
partial capture61312used nodes/303310tets, incomplete strict census. No usable curved
M6 BL exists. Splitting/coarsening/relocation/reconnection/growth, constrained/blended
directions, coupled cavities and heights must still be qualified on actual output.
Surface polish or a target solve alone is not BLsuitability. Coulaud/MAdLib hypotheses
remain queued inside this cycle; unrelated ideas stay in backlog.

## Resources, preserved completion and remaining objective

One owned heavy job; nice19,j3(2–4build workers), OMP/BLAS/MKL1.4TB compile-tmp
integration_evidence/build-integrated-v2/compile-tmp. Native5400s/outer14400s,
12GiB/rank; actual chord diagnostics300s atlas/600souter, existing point probe600s
outer. Verify PID+start identities on actual host (namespace may hide processes)
before edits/restarts; preserve source/binary/input/helper/runtime pins and failures.
No raised limits/relaxed gates or full retry merely because a focused stage passes.

Euler293 COMPLETE: TWO strict Euler50k meshes149051points/736323tets and172001/
870422; conservative admissible transfer, finite continued CFD and isolated adapted
disk restart. Continued CFD was one iteration, not convergence. Preserve
R/m6_inputs/euler_adapted293_event{1,2}_v1 and
R/m6_runs/euler_build293_adapted_event1_disk_restart_v1. All22payloads/384685699B
preservation receipt euler293_preservation_during_bl358.json; no payload edits.

Remaining: strictly accepted usable curved BL, dedicated native-MPI constraint
qualification, TWO accepted RANSadaptations, conservative admissible flow+SA transfer,
finite continued adapted CFD and actual isolated adapted disk restart.500k is NOMINAL
complexity, not exact nodes or rejection gate. Geometry1e-4, exact positivity/body
separation,q>=.2,J>=.05,maxedge1.8,h0=2e-6,g1.2,T=.01,height tolerance.1. CADoptional,
original no-CAD authority sufficient. Validity-first provisional stages may have
diagnosed quality debt; strict closure is mandatory for final acceptance.
