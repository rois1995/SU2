# Native adaptation — current handoff

2026-11-14 measurement v2 completed: Run A ended normally; both configured Run B adaptations hit the time budget, solver exit 0; see the [measurement report](Handbook/ADAPTATION_M6_MEASUREMENT_V2_20261114.md). No source/build/config changes or design work.

Updated 2026-11-13, Europe/Rome. The full curved-BL/RANS goal remains open. The requested nose-target cycle492 is **closed: tested size-only implementation rejected**. No owned heavy job remains; do not start another local nose variant or retry this cycle.

R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
Read [AGENTS](AGENTS.md), [492 outcome](Handbook/ADAPTATION_MAPPED_CURVATURE_OUTCOME_492_20261113.md), [frozen reviewed plan](Handbook/ADAPTATION_MAPPED_CURVATURE_TARGET_492_20261113.md), [ordered stages](Handbook/ADAPTATION_CURVED_BL_STAGE_INTEGRATION_20261111.md), [timings](Handbook/ADAPTATION_TIMINGS.md), [HowToProceed](HowToProceed.md). [History through491](Handbook/ADAPTATION_HANDOFF_HISTORY_THROUGH_491_20261113.md) retains earlier evidence; its LIVE labels are historical.

## Current result and next decision

One serial production PrepareSurface run, exact saved485 input and new qualified target, configured388.8s/233220work/12GiB: native389.573s, outer566.441s,305accepted splits (1.277s/split). Geometry-guidance evaluations1014/refusals705 (69.53%, above the <50% support threshold). Surface census31308/39853, incomplete; not ready.

Exact split-only ancestry and final topology agree. Of65published splits in the frozen nose set,36remain spanwise and29around-nose at their saved cubic geometry bindings. All incident wing sides agree; no near ties. This meets the frozen REJECT direction criterion. Both30s full-entity certificate passes completed zero faces, so before/after deviation and two-halving criteria remain unresolved. R/surface492_outcome.json consolidates the initial bound-only assessment and later direction evidence. No progress inference from changing bad-face counts or pre-BL q/J.

Brief item3 remains: split selection/direction and unweighted face-excess admission are unchanged; height/normal-foot/link coordination also refuses candidates. Item4 remains: old/fresh bound tightness differs, and fixed-face full-entity certification is expensive. This run does not separate genuine regression from conservative refusal. Review the coordinated construction/selection/admission and certification strategy before another run; no raised limits, relaxed gates or extra local variants.

**Additional metric debt:** final mathematical review proves that individual principal rank-one caps do not guarantee their joint envelope at oblique directions in doubly-curved regions (R/joint_curvature492_debt.json). The focused axis constraints/reload fidelity pass, but wider metric qualification is incomplete. Before another qualification cycle, correct the joint tangent-tensor intersection, test mixed directions and distinguish/rebuild target semantics; then review construction/certification. Do not silently reuse492 as a fully qualified joint-curvature field. No post-run source/build/native rerun occurred.

## Reusable state, source and target

Latest provisional state is **saved492:61855nodes/304593positive tets**, including305new splits on top of485. R/saved_surface492_checkpoint.json binds current Owned/live parameters/transported sensors, full new target, geometry/config and provenance. R/surface492_current_state is its adapter; R/surface492_adapter.json verifies links/hashes. Scheduler-exact continuation is not claimed. Preserve saved485 and surface486_current_state as the prior input; inspection_surface485 is historical, not a492export.

Mapped wall samples now retain both principal curvature caps from the existing cubic differential authority, using the analytic tolerance-derived chord law. Independent projections handle tilted feature-foot normals; tangent normalization preserves both caps. No CAD, tuned factor, feature averaging, normal-profile or engine dispatch change. Target schema3 rejects obsolete mapped sizing laws. Old441 cannot qualify the corrected target.

Build494 is terminal/pinned: test SHA e6c25520db86bdd212b500249420a7cf3ceddf4af5c17c40e02b1b0b743cd365; CFD SHA0311b5a2a4f871bd82f48cd7cef3eac49a932cd1d90012bbc7e3c0d887e74ee6. Nine focused cases covered with explicit source/object/runtime evidence. Build/test-fixture failures and corrections are retained in the outcome; no production fallback.

New target producer523.101s, fresh reload364.939s; all61330owned composed tensors and chord witnesses bit-identical. Twelve payloads303788608B; all6embed the same47601773B authority SHA9cdbc63cb6853f2c8b052bb0549f7b561299bfe82cd21fd6b792d2600dadf7e9. R/target_checkpoint492_qualification.json and target_reference492_fidelity.json own fidelity. Seed estimate492586.451 with empirical indicator49430.288, tangent.28717458874925855, core.0047263575870155755.500k is nominal, not node count/gate; evolved-mesh complexity remains unmeasured. Current transported sensors stay fixed. No CFD advance.

Actual production case R/cases/curved492_saved_production_surface_serial_20261113T163756_3724702 is TERMINAL0; all1794pins/five actual identities verified. Its pinned v1 assessor is preserved; v2 fixes only a manifest-reader assumption. Saved geometry/metrics and native result are unchanged. All jobs are terminal; verify hashes/acceptance before reuse and freeze pinned code/inputs during future campaigns.

## Complete objective and strict gates

Order: saved CFD/geometry → composed target → coupled surface → constrained directions/first row → growth → outer transition/strict closure → transfer/continued CFD/second adaptation/isolated restart. Stage3 remains incomplete. Final h0=2e-6, growth1.2, thickness.01, height tolerance.1; stage0 RAMP5 h6.4e-5 is already applied by config. Final geometry1e-4,q>=.2,J>=.05,maxedge1.8 and all resource/height/transfer gates remain strict.

Euler293 preserves two accepted50k adaptations, conservative admissible transfer, finite continued CFD(one iteration) and isolated adapted restart. Remaining: accepted curved BL, dedicated native MPI constraints and two accepted final-height RANS adaptations with conservative flow+SA transfer, continued CFD and isolated disk restart. The brief's final-height chord/normal-metric question remains outside492, before stage6.

Checkpoints41a16e1f07 and df0b1bdf4f are pushed. User explicitly approved this fork/destination and instructed keeping it updated; authorization persists for this task's reviewable checkpoints. The final492 outcome/debt checkpoint is kept on the same branch. Unrelated working changes remain preserved.
