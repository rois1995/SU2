# Work order status

FP2-C2 step 0 is complete on test-driver build648. N1 and N6P had zero sanity differences against C1. The observed whole-chain counts were 66,380 in N1 and 71,085 in N6P pass 2, with dry one-sweep estimates of 9,578 and 9,766 removed points. N1 evaluated 88.5% of scheduled strict queries at the time cap. N6P evaluated all of them. Production and the solver still match C1. [FP2-C2 step 0 report](integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FP2_C2_0/reports/FP2_C2_0_648.md)

FP2-C1 S passed on build647. Its five wall-collapse fixtures, the G1 identity check, and two trials of each hand-over case passed. Both full S events and isolated restart passed. Native times were 606.213 and 387.779 s with 467,139 and 464,565 final cells. The default proof and full repeat are pending. [FP2-C1 report](integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FP2_C1/reports/FP2_C1.md)

FX2 remains the accepted reference at build645. Its G5 OFF and S workflows passed both events and isolated restart with matching payload hashes and exact timer/storage counts. The surface option defaults to NO. [FX2 report](integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FX2/reports/FX2_645.md)

Build640 established the accepted FP2-B surface-on reference with identical S1 and S2 payloads. [FP2-B acceptance report](integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FP2_B/reports/FP2_B_640_continue_1.md) · [Surface-on reference](integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FP2_B/on_surface_reference.json)

FP2-B ON failed S1 on build633 after the corrected lab passed. Event 1 passed and published all three artifacts. Event 2 rejected incomplete quality-refresh edge measurements. The item was reverted to build627 production sources and the pre-FP2-B test tree. Both rebuilt binaries matched their baseline hashes. Build633 stopped before its default proof and S2. [FP2-B correction 1 report](integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FP2_B/reports/FP2_B_633_correction_1.md)

FP2-B0 completed on test-only build629. At six ranks, the frozen lab measured 2,538 bad cells with surface splits off and 11,099 with them on. X6 replayed the best serial variant (X3) and measured 3,741 bad cells at c4. The production source pins and solver hash match build627. FP2-A ON remains failed and the surface option defaults to NO. [FP2-B0 report](integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FP2_B0/reports/FP2_B0_629.md)

FP2-A Parts A and B and the full Part C default passed on build627. The surface option defaults to NO. S1 failed the native deadline with unresolved quality and geometry. S2 is unevaluated. [FP2-A report](integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FP2_A/reports/FP2_A_627.md)

FP2-0b measured the feature-edge split prototype on test-only build620. P3 accepted its 84,478 faces and P4 accepted all four entities in 56.148 s. Strict Classify flagged zero final faces. G1 matched 375 / 114 / 228 records. [FP2-0b report](integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FP2_0B/reports/FP2_0B_620.md) · [Earlier FP2-0 measurement](integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FP2_0/reports/FP2_0_619.md)

FX1 closed with Part M accepted on build617. Part P was omitted under R4 because its budget values feed later admission and decode exception classification. Both G5 modes passed and matched their six reference hashes. Minimum named coverage was 99.008% / 99.529% OFF and 99.065% / 98.738% ON. This also closes the FP1 Part S OFF proof. The default is NO. [FX1 report](integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FX1/reports/FX1_617.md)

FP1 Part S passed gates 1 to 4 on build616. Its G5 OFF proof passed in FX1. FP1 ON accepted on build616. Both ON runs passed the standing workflow and isolated restart gates. All six payloads match. Minimum named coverage was 99.041% / 98.658% in ON1 and 99.043% / 98.650% in ON2. The default is NO. [Resume 6 report](integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FP1/reports/FP1_616_resume6.md) · [ON payload reference](integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FP1/on_reference.json)

N22 result gates passed. The user accepted both purpose timing flags on 2026-11-19. Both parts matched all six N21 W payload hashes. [Step 8.5u report](integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N22/reports/STEP_8_5U_591.md)

N21 Parts P and W passed. Its build589 carried the qualified build588 CFD binary. The six wide payloads matched across two runs and became the payload reference. [Step 8.5t report](integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N21/reports/STEP_8_5T_588.md) · [Adopted payload and time references](integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N21/accepted_reference.json)

N20 Part A PASSED. Part W FAILED its purpose rule. Build586 was restored at that checkpoint. [Step 8.5s report](integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N20/reports/STEP_8_5S_585.md)

Work order 2 ended at N7. D/E passed at `3524e11a75`. F failed. Its first difference was event one NATIVE3D complete=0 against the required 1, with rank 3 q_bad=1 against 0. The F enabling change was reverted at `12690f1d99`, with build579 restored at that checkpoint. [N18 report](integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N18/reports/STEP_8_5R2_580.md)

Work order 3 governs the next item. At session start, read AGENTS.md and the progress record. [Progress](integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/progress.md) · [Reviewer questions](integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/QUESTIONS_FOR_REVIEWER.md)
