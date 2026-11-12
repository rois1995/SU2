# Requested-wall coverage in surface preparation

Active connected stage3 change, build432. Status: focused serial and connected MPI6 checks passed; actual saved-target M6 surface stage TERMINAL with valid provisional cells but surface_prepared=false. The cavity review has been delivered and pushed; the subsequent goal continuation resumes this existing change. Growth and semantic movement findings remain queued for their affected stage, not competing implementations.

## Missing capability and evidence

Actual428 and nativeMPI431 preparation both consumed the existing388.8s surface allowance in coarsening, before any split pass.431 had15,980 cached and285,203 unknown rows. These are complete captures but partial metric censuses. They establish that the existing full-volume coverage/phase contract remains incomplete, not that interior measurement dominates elapsed time. Existing wall action queues already restrict independent demand to requested walls.

The connected correction is to distinguish the surface stage's required demand coverage from final full-volume certification. It does not change the metric, construction policy, operators, scheduler or budgets. It also adds seed/update cell-class counts to measure actual work rather than apportioning overlapping timers.

## Stage contract

Input: the qualified complete396 composed target checkpoint, its saved CFD seed and original no-CAD authority, current nodal transported core, requested walls and unchanged physical settings. Use original partitions for the MPI stage boundary. No CFD advancement or target recomputation.

Output: private surface-preparation cells plus geometry parameters, explicit requested-wall coverage and generic whole-volume unknown flags. Readiness requires the existing requested-wall metric/geometry demand to close and the existing independent geometry/validity checks to pass. This is not accepted BL output: q/J and physical first rows/growth remain downstream obligations.

Acceptance of the implementation requires focused cached/recomputed coverage checks, no forged neutral lengths, unchanged complete cavity admission, migration preserving cached rows and unknown nonrequired rows, aligned empty ranks and retained work/resource refusal. An actual saved M6 stage must confirm the behavior and expose any remaining blocker. Save a credible reusable stage only after its relevant output/provenance checks pass; a timed-out capture remains failure evidence.

## Source audit and scope

`Selection::RequestedWall` selects cells incident to a requested physical face. These are the demand owners; their complete six-edge measurements remain unchanged. Initial census cursors are filtered at initial setup, region rebuild and migration rebuild. The directory still contains every cell and supplies complete versioned adjacent stars. Every published added cell is still measured, including interior or other-boundary cells. Shared edge coordinates, field identities, associations and exact measured lengths are not replaced.

`Selection::SurfaceCensus` checks required rows against current cell versions and separately reports geometry measurement coverage. Generic `StageProgress` retains its full-volume definition. Nonrequired rows remain unknown until actually measured. No surface coverage flag may be presented as full q/J, size or height certification.

Surface-only phase ownership gives nonrequired cells base ownership weight1 and zero independent geometry demand, but migrates the entire mesh. All existing cached records migrate; absent nonrequired records stay absent. Required coverage is checked after restore. Full-target quality migration still requires its original complete census. No repair history or full-target phase modes are changed.

Readiness checks test known remaining debt before scanning coverage, addressing cavity-review finding4. Full scans remain at possible readiness and phase/observation boundaries. No cached coverage scalar is introduced without invalidation machinery.

Affected files: Common Engine.cpp/.hpp; existing surface-preparation unit checks; native capture and saved-target stage fixture JSON. No new user configuration or persistence format.

## Qualification plan

Run the existing focused surface/height/direction/curved-transaction/feature/partition/deadline checks serially, then one connected MPI6 milestone. The incremental-seed case now requires20 requested-wall rows while40 unrelated volume rows remain unknown; recompute all available diagnostic rows independently. Exercise phase migration and originally empty ranks without claiming native-MPI FIRST_ROW generation.

After qualification, replay the existing saved-target surface stage at its unchanged work/time/memory allowance. Report readiness, required/measured demand coverage, generic unknown rows, seed/update classes, collapse/split progression, complete validity and geometry, counters, rank CPU/wall and sampled memory. Compare matching stages with431, accounting for changed trajectory and contention. No full RANS retry is needed until later stages have credible outputs.

Growth revision refresh, complete BL suitability and semantic curved-move corrections from the [review](ADAPTATION_CAVITY_OPERATOR_REVIEW_20261112.md) remain unimplemented. Do not declare complete BL capability from this census correction.

Build432 passed392.920s with all pins unchanged and all actual owned handles terminal.
Serial39cases/8801assertions passed11.468s. ONE connected MPI6 milestone passed17.977s,
35cases perrank/2774–5595assertions. Allcasepins and actual wrapper/worker identities
verified terminal. `R/surface_demand432_qualification.json` records receipts. These
checks cover scoped coverage, independent recomputation, empty ranks, phase migration,
curved transactions/features, height/direction preservation, deadlines and full-target
ownership/census regressions. They do not qualify nativeMPI FIRST_ROW generation.

Actual integration: `R/cases/curved432_saved_target_surface_mpi6_20261112T092307_730261`,
execsession30161 TERMINAL; wrapper730261/start51982678 and MPI730472/start51983011
plus actual worker identities verified terminal. Unchanged396target/original6partitions
and388.8s/233220work/12GiB. Wrapper532.794s, native max389.353s at the same deadline.
Complete payload validation:60,805unique points/301,908positive tetrahedra with core,
1,146,827exact shared coordinate/tensor checks. All pins/target profiles/CFD unchanged.

`R/surface_stage432_assessment.json` and `R/assess_surface432.py` own the assessment.
The first assessment attempt failed on an interleaved stdout counter; the diagnostic
did not fail. The parser now explicitly records missing rank2/4 retention/cache
fields. Separate rank JSON and all saved manifests/payloads validated. No replacement
campaign was launched.

| Matching surface allowance |431 |432 |
|---|---:|---:|
| Native wall, maximum rank |389.056s |389.353s |
| Commits |784 |525 |
| Global work |5104 |2589 |
| Field requests |77,727 |24,351 |
| Geometry work |39,465,074 |25,194,164 |
| Summed rank CPU, includes polling |2321.204s |1798.320s |
| Geometry-work max/min rank |13.23 |6.94 |
| Cached/full-volume unknown cells |15,980/285,203 |4,221/297,687 |
| Completed collapse/split passes |0/0 |0/0 |

432 required-wall coverage is2,394/37,753cells. Only864initial rows were seeded,
all requested wall:9 batches of16perrank. Changed-cell measurement remains complete:
1806interior/21otherboundary/2062requested-wall evaluations. Partial known geometry
bad68/sizebad0 are not whole-field improvement; all q/J remain unknown. No final
first-height/layer claim. Largest sampled treeRSS4.665GB/individual.922GB, not true
physical peak. Targetload max61.265s versus43142.048s; not part of native stage.

This is a qualified coverage-contract correction, **not an established performance
or construction success**. Fewer calls and lower geometry work/CPU did not deliver
more accepted surface work. Timed trajectories and coverage differ; contention and
MPI polling prevent a causal wall-time claim. Do not count the smaller known-bad set
as net repair.

The remaining blocker is within requested-wall processing: coarsening service and
coverage advance too slowly to reach a direction change/refinement. Removing unrelated
initial rows does not resolve it. Next localize the existing coarsening private-work,
refusal and collective/accounting paths on this saved outcome. Existing upper-query
work is uneven; broad election timers are overlapping and are not total MPI wait.
Do not select another optimization, change eligibility or increase budgets from
commit counts alone. Reuse430 actual cavity evidence and432 captures to distinguish
geometric refusal, preflight work and synchronization before a further correction.

This failed preparation capture plus complete396 policy is retained for diagnostics;
it is not a credible successful surface checkpoint or exact search continuation.
No new VTU was generated:432 has not demonstrated substantial mesh-delivery progress
over the existing428 inspection artifact.
