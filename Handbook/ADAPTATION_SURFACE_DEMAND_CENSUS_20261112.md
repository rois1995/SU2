# Requested-wall coverage in surface preparation

Active connected stage3 change, build432. Status: focused serial and connected MPI6 checks passed; actual saved-target M6 surface stage is frozen/running. The cavity review has been delivered and pushed; the subsequent goal continuation resumes this existing change. Growth and semantic movement findings remain queued for their affected stage, not competing implementations.

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
execsession30161, wrapper730261/start51982678, MPI730472/start51983011. Unchanged
396target/original6partitions and388.8s/233220work/12GiB. Preserve until terminal or
an evidence-backed authorized stop; no pinned source edits while this runs.
