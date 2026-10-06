# Native integration completion audit

2026-10-06. This audit uses the unchanged objective and the five deliverables in
NATIVE_INTEGRATION_GOAL.md. It covers local/cached branch reconciliation and the
observed experimental native2D envelope. It does not redefine completion as
universal mesh generation, native3D/CAD or converged CFD/sensitivity accuracy.

The runnable evidence check is
`python3 integration_evidence/check_native_integration_completion.py --output <fresh-output.json>`.
Its negative-contract self-check is `--self-check`. The recorded actual result
is integration_evidence/integration_completion_v1.json. The checker reads
saved logs/reports, rechecks archived binaries and raw input/output hashes,
current sources, refs, draft and checkout guards; it runs no simulations.

## Requirement-by-requirement result

| Requirement | Authoritative evidence inspected | Result and scope |
| --- | --- | --- |
|1. Inspect local branches/worktrees and reconcile meaningful adaptation/MPI/transfer/output/geometry work |BRANCH_RECONCILIATION.md; refs_initial.txt/worktrees_initial.txt; stage_g_draft.json; periodic_branch_assessment_v1.json; reconciliation_guard_v2.json; current refs/diffs/merge ancestry |PASS. Native, Stage G/draft, sensors, BL/corner/capability/transfer work and CGNS output combined. B0 research preserved separately; historical/superseded and periodic/deformation work explicitly assessed/excluded from unsupported paths. Cached refs only, no network freshness claim. |
| Separate reviewable branch; leave feat_adap_noExt and its checkout untouched |Current codex/native-integrated worktree; main_initial.json; final HEAD/branch/tracked status and .gitignore FILE hash; Stage G draft SHA |PASS. Main remains88828474db with only its exact pre-existing .gitignore change. Other tracked production/test drafts are unchanged. Integration test-only patches committedffb3fb4c46; dependency symlinks unstaged. |
|2. Fresh integrated assertions-enabled primal MPI/MMG/CGNS build and focused1/2/4 checks |primal_build_v6.log; integrated_output_controls_v6(7cases/rank), integrated_primal_controls_v6(49), memory/default-MMG groups, integrated_auxiliary_v9(58); current Meson registrations/build options |PASS. Saved raw Catch logs and executable/source hashes checked. Current source coverage is executed_source_coverage_v2.json, not inferred merely from manifest labels. |
| MPI interpolation/transfer and partition changes |integrated_general_transfer_v1(40cases/rank1/2/4); native reader/import/field/distributed controls; actual replacement/conservative lifecycle controls |PASS. Distributed barycentric/conservative partition equivalence, donor read-only/fallback/admissibility, chunk/memory controls, reader ordering and corner metric tested. Different remeshing ownership layouts additionally measured. No arbitraryN→M active-rank runtime feature is claimed. |
| SU2 and CGNS adapted mesh writing/reload/config and exact boundaries/order |7outputcases/rank; independent_outputs_v6(84reports); native CGNS reload/reference fixtures; actual production WRT_ADAP_MESH outputs |PASS. In-memory boundary handling, mixed order,17-digit SU2 coordinates and CGNS marker descriptors retained. Independent geometry/field-target checks distinguished from writer-only checks. |
| Stage G separate AD build, counters, transfer/tape lifetime and interruption |ad_build_v9.log; ad_repair_controls_v14(3cases/26assertions per rank,ten lifecycles); goal_runtime_v7; goal_remaining_v1; static diagnostic/classification |PASS for integration/lifecycle scope. Halo-seed duplication fixed79ce4bc81d; exact same-ownership continuation differences0, counters past LIMITER_ITER. Four own-descendant interruptions collectively cleanstop/checkpoint/no-remesh. cold2 divergence classified by131-row exact-state static replay; warm4 growing residual and nonconvergence reported, not hidden. |
|3. Refinement/coarsening, thin/opposing BLs, boundary adaptation, target changes |Fresh smooth/step1/2/4 runtime;192independent P1/height/flat-reference reports; baseline and actual production NACA9targets each; two accepted partial-grid audits |PASS within tested envelope. Eight moving-height/demand cycles and both transfer policies; actual NACA h0=.0002 and accepted h0=.0001 mesh. Abrupt nodal request retains continuous P1 interpolation. Larger/thinner next-remesh timeouts and unrun axes preserved. |
| Sharp geometry, anisotropy, explicit admission/collision failures |Native geometry/collision/engine/distributed/fixed-adaptive controls in49-case matrix; matchedAR10/100/1000 independent9; incompatibleAR25 independent3 |PASS focused controls. Exact features/nesting/contacts/remote collisions, oversized-star rejection and rejected accepted-record equality checked. Matched affine anisotropy does not prove curved high-demandAR1000 robustness. Incompatible h0_metric2.5>lengthcap1.8 is valid incomplete output, not an integration failure. |
| Immutable original geometry and adaptive boundary samples/BL contracts |CNativeRemesher::PrepareReference/Remesh, deferred onAccepted; native reference IO/restart fixtures; all frozen-P1/original-reference audits |PASS. Original marker polyline is immutable while samples adapt. q>=.18,length<=1.8,h0 relative error<=1e-8; NACA original-reference deviation<=1e-6, exact declared feature retention and arc coverage independently checked. No CAD projection claim. |
| Accepted mesh/solution state on rejected/incomplete results |ReplaceMesh complete-status gate before mutation; remesher has no flow mutation and defers reference binding; fault/admission snapshot tests; accepted-reference/solution controls |PASS for tested rejection and reviewed common gates. No incomplete candidate is silently published. Timeout artifacts do not prove interrupted candidate topology or arbitrary allocation-failure recovery. |
| Preserve MMG default and reject unsupported combinations |CConfig defaultMMG; shared MakeRemesher; default-MMG1/2/4; native continuous-adjoint/missing-reference exact negatives1/2/4; actual native+TWO_PASS1/2/4; native-in-AD negative2 |PASS. Error exits1 with exact diagnostics; timeout/signal exits not accepted. Native remains static single-zone primal-double2Dtriangles; periodic/mixed/moving/time/adjoint paths remain unsupported. |
|4. Controlled size/rank scaling, repeats, first cost boundary |pilot12; low-cut2048/4096/8192 groups9; repeated8192 layouts/ranks/samples27; capacity_v2; ROBUSTNESS_SCALING_PROTOCOL.md stop rules |PASS observed envelope. Largest common complete size8192; four-rank16384complete; serial/two-rank16384 and four-rank32768whole-job timeouts. Remaining sizes/demand ranks NOTRUN under predeclared stop policy. Shared-host variability and rank-dependent work prevent strict strong-scaling claims. |
| Separate remeshing/transfer/CFD/output/audits and report per-rank memory/work/imbalance/communication limits |native scaling raw JSON/counters; partition work/memory summary and plot/CSV; instrumented NACA9MAX/local timing rows,18paired immediate flow fields, checker |PASS. Remesh and transfer independently measured; transfer lies inside replacement; per-phase MAX values are nonadditive. Init/reference/snapshot/audit costs excluded as documented. RSS includes startup; local elements overlap. Encoded traffic includes self; World counters omit direct MPI/votes. No whole-process memory-bound claim. |
| Explicit failed/limited case preservation |Raw timeout folders/logs; complexity/height cost classifications; cold2/static replay; original failed builds/harness/parser states; native_closure_failure_classification_v1 |PASS. Final closure bookkeeping failure retained: missing helper return after successful rank1negative. One-line fix committed; remaining2/4negative checks completed separately. No failed full chain relabeled successful. |
| Resource policy |Sequential runner implementations/commands/environment and recorded processes; maxranks4,build-j2,OMP/OPENBLAS1; final host process check |PASS observed execution. One own heavy job at a time; queued supervisors do not imply concurrent computations. Foreign idle PID918696 untouched. No own solver/build/audit job remains active. |
|5. Updated handoff, exact provenance/evidence and inspectable grids/solutions |HANDOFF_Codex.md; consolidated RESULTS/PROTOCOL/RECONCILIATION; GRID_GUIDE.md; per-case inputs/configs/mesh/metric/restart/flow and archived tools |PASS. Input grid and pre-adaptation solution explicitly linked. Immediate donor/transferred CSV fields and actual production ParaView/restart files distinguished; short solves not advertised as converged aerodynamics. |

## Protocol closure and practical limits

All eight original campaign steps are accounted for: fresh integrated gates;
12pilot cases; independent raw target/geometry audits; controlled escalation and
27largest-common repeats;9matched and3incompatible anisotropy cases; real CFD/
transfer/output baseline1/2/4; serial increased-demand/thinner diagnosis stopped
at recorded cost boundaries with accepted partial meshes independently verified;
and separate AD/lifecycle/interruption controls with inherited solver limits
classified. Predeclared higher-rank strip follow-through, abrupt nodal targets,
actual main-output and phase/field closure are also finished.

No required implementation or validation gate remains for this scoped goal.
Unproved extensions are explicit results, not quietly reduced requirements:
native3D/CAD/mixed/moving/time/adjoint capability; more than4ranks; successful
high-demand thin-BL MPI escalation after failed serial diagnoses; general
large-mesh scaling; converged aerodynamic/sensitivity accuracy. Transaction
caps do not bound whole-process memory. The measured strip hasfixedny4 andB∝N.

Primary completed gates: native_production_complete_v1.json,
native_step_support_complete_v1.json, native_phase_partial_complete_v1.json,
capacity_ad_followthrough_complete_v1.json, executed_source_coverage_v2.json
and the final integration_completion_v1.json. Original intermediate pending
flags are historical and superseded by these stronger completed gates.
