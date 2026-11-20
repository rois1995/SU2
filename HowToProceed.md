# Work order status

FP1 Part N is pushed at `d76fa0d5bc`. Build615 passed the fixtures, G1, G2, GQ, G3, G4 and the saved-state safeguards. The fresh G5 OFF workflow passed both events, the isolated restart and all six reference payload hashes at `d242c9421a`. Minimum named coverage was 97.467% and 98.019%. FP1 ON FAILED. ON1 passed both events and isolated restart. ON2 event 2 rank 0 recorded 94.842% named coverage against 95%. The controller stopped before event 2 publication, so six-file repeat identity is unevaluated. Resume 5 ended without another run. The default is NO. [Resume 5 report](integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FP1/reports/FP1_615_resume5.md) · [Resume 4 baseline](integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FP1/reports/FP1_614_resume4.md)

N22 result gates passed. The user accepted both purpose timing flags on 2026-11-19. Both parts matched all six N21 W payload hashes. [Step 8.5u report](integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N22/reports/STEP_8_5U_591.md)

N21 Parts P and W passed. Its build589 carried the qualified build588 CFD binary. The six wide payloads matched across two runs and became the payload reference. [Step 8.5t report](integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N21/reports/STEP_8_5T_588.md) · [Adopted payload and time references](integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N21/accepted_reference.json)

N20 Part A PASSED. Part W FAILED its purpose rule. Build586 was restored at that checkpoint. [Step 8.5s report](integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N20/reports/STEP_8_5S_585.md)

Work order 2 ended at N7. D/E passed at `3524e11a75`. F failed. Its first difference was event one NATIVE3D complete=0 against the required 1, with rank 3 q_bad=1 against 0. The F enabling change was reverted at `12690f1d99`, with build579 restored at that checkpoint. [N18 report](integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N18/reports/STEP_8_5R2_580.md)

Work order 3 governs the next item. At session start, read AGENTS.md and the progress record. [Progress](integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/progress.md) · [Reviewer questions](integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/QUESTIONS_FOR_REVIEWER.md)
