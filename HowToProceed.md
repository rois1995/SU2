# How to proceed

Updated2026-11-07. The full M6 attempt drives development. ZERO meshes accepted.
Read [HANDOFF_Codex.md](HANDOFF_Codex.md) for live handles and authoritative source,
build, input and evidence provenance; older build/status notes never override it.

## Goal and active work

Complete TWO strictly accepted ONERA M6 Euler CFD→mesh→CFD→mesh events at **50k
metric complexity**, conservative admissible transfer, finite continued adapted CFD
and an actual isolated adapted disk restart. Then dedicated BL/constraint/MPI
qualification and the corresponding RANS workflow at **500k**. Report actual mesh
points/cells separately. Keep geometry1e-4,q>=.2,J>=.05,maxedge1.8 and requested
heights strict. CAD remains optional; existing no-CAD authority must suffice.

The connected visibility-placement change is implemented and tested, but full244
REFUSED at native5347.387821. Last first shape pass42q/18J/1717geometry; late geometry
work reduces geometry to1418 but fresh shape rises414q/472J. Final cache UNKNOWN,
not a pass. [244outcome](Handbook/ADAPTATION_BUILD244_OUTCOME_20261107.md).

The user now requests **MACH ONLY**, removing PRESSURE from ADAP_SENSOR. The run
config and generator are updated and checked; same saved real CFD seed,L1,50k and
unchanged gates/budgets. Production metric-only check passed,tool18760/32.579044s:
`staged57/metric_target244_mach_only`,complexity49999.999626,finite SPD,zero
gradation violations and bit-identical CFD fields. Use existing implementation244
for the full two-event Mach-only M6; no C++ rebuild or new quality implementation.
Combined-target timing comparisons do not describe an unchanged Mach-only target.
Keep the coupled-quality hypothesis deferred until this changed-target outcome.

The prior combined-target production L1/L2 comparison selected L1: L2coarsened4/11shock
points, increased tip allocation and maximum anisotropy. Do not assume L2 is better
or launch another baseline. L1 stays fixed for the requested sensor-only change;
that prior comparison does not establish an optimum norm for every sensor set.
Corrected219metric floor and flat-sensor handling are
integrated; seed CL.269535/CD.013536 meets the user's reasonable-force criterion,
not a claim of full residual convergence. Initial restart/config/mesh/reference
are byte-identical237. Reuse the seed rather than recomputing it.

## What the actual cavities established

[Full237 outcome](Handbook/ADAPTATION_BUILD237_OUTCOME_20261107.md): first shape pass
reached44q/17Jbad but left1656geometry-bad cells. Geometry reconstruction reduced
geometry failures while fresh shape rose to410q/493Jbad. Final strict refusal had
296q/327J/1366geometry/0size. Thus both geometry closure and shape reclosure matter;
a low intermediate debt percentage is insufficient.

[240 exact query diagnosis](Handbook/ADAPTATION_CAVITY_GEOMETRY240_20261107.md)
proves actual stored-cubic violations on chord7868–7876: certified lower248.334µm
and112.330µm exceed100µm. Cheaper certification alone cannot close these faces.
Existing241split/insertion/reconstruction paths all refused. Existing242normal
strip proposals all inverted inherited cells; do not enable that shortcut.

[243/244 connected design and results](Handbook/ADAPTATION_VISIBILITY_PLACEMENT243_20261107.md):
PlaceSplitPatch accepted movable-star inputs but only tried no movement. Restore
bounded positivity-only free-interior-apex visibility proposals using existing
clipping, native projection and exact strip/association/interface/height admission.
Reuse existing complete-star reservation/version/publication and9node identities,
within64cells/8apices and unchanged work/time/memory limits. No q/J construction
measurement/ranking/veto or new operator was added. Earlier cache, signed-bound and
phase-ownership improvements remain intact.

Actual18-cell star constructs in3trials/one apex move. Full52-cell neighboring
context4q/3J/6geometry→3q/3J/6geometry, excess−74.68%; existing optimizer then gives
1q/1J/5geometry, excess−82.97%, valid replacement/interface/metric admission.
This is useful local repair power, NOT a complete accepted mesh or historical replay.
MPI6passes55cases on every rank. The actual direct directory test proves stale/veto
refusal and complete cell publication with initially empty owners; Engine selection
and distributed parameter edits rely on separate existing coverage and full244.

A mistakenly selected legacy broad serial suite FAILED8/344cases. Preserve its
receipt and reuse individually passing affected cases; never label it passing.
Historical donor/scenario assertions and an unattributed RANS wall-policy4096work
refusal remain open. No baseline equivalence, weakened expectation or gate is claimed.
The connected MPI6 milestone passed; downstream BL qualification must revisit its
wall-policy issue. Do not repeat the generic suite or launch an old-code baseline.

## How to choose the next change

Use the terminal outcome, complete six-rank counters and representative actual
residual cavities with neighboring cells, metric, reference surface and rejection
history. Include nearby clean controls and both severe/near-threshold defects.
Distinguish poor placement, genuine constrained movement, missing connectivity
changes and conservative certification refusals. A cell without a physical face
may still share constrained vertices. A final captured context is not faithful
replay of an earlier rejected transaction.

Judge whole-field net q/Jbad removal per second, geometry/size counts and counted
work. Retired−introduced churn must agree with field change. Use existing counters
to separate underfilled/conflicted batches, rank imbalance/synchronization, expensive
geometry trials and low repair yield. Timers overlap; join-call wall share is not
CPU idle. Same round is not identical batch history after trajectories diverge;
nearest measured quality ages must state their gap. Do not attribute timing changes
to code from a contended run alone.

If residual boundary cavities demonstrate a missing capability, test ONE connected
hypothesis using existing surface/volume operators before adding operators or
larger cavities. Assess the complete affected patch AND neighboring cells for net
q/J failures, geometry failures/excess, metric conformity and useful progress per
second. Preserve exact support/coordinate/association invalidation, active-reference
accounting, private curved contexts at use, transient admission, body/feature/seam/
height protection and final certification. Uncertain bounds fall back to full checks;
sampling never establishes geometry acceptance. Literature is design reference,
not proof of equivalence to these gates or authority for a competing implementation.

## Is the mesh arriving at polishing the problem?

It may contribute, but current evidence does not establish that a soft construction
quality preference would solve closure.237polishing removed almost all initial
shape failures, then geometry reconstruction introduced new ones. Short edges occur
in both failing and nearby clean cells; correlation does not prove causality.
Current244must show whether the connected placement change improves actual closure.

The current validity-first policy excludes construction q/J measurements and
ranking, not only hard vetoes. A soft q/J preference is therefore a deliberate
policy experiment, not an already authorized production tweak. Do not introduce it
silently. If later evidence justifies proposing it, test total construction-plus-
closure cost, whole-field geometry/shape/size outcomes, oscillation/target drift and
constraint/coverage preservation on actual difficult cavities and varied curved,
feature/body/height/MPI/resource cases. A prettier entry histogram is insufficient.
Keep unrelated alternatives in [the backlog](Handbook/ADAPTATION_BACKLOG.md).

## Saved meshes and completion

Latest saved rejected mesh is244:198598points/1018217tets, with full nodal tensor,
verified array lengths/SPD/connectivity and runtime q/J/geometry UNKNOWN(NaN).
Previous237is the latest fully measured rejected mesh:198941points/1020222tets,
`rejected_mesh.vtu` plus `.su2`, with captured nodal metric9row-major. All1725bad
cores/25856neighbors, q/J/geometry/edge fields and reference/rejection context are
saved; array lengths, correct PointData/CellData, finite SPD metric and connectivity
were independently checked. No ParaView GUI validation. Assess a meaningful244
terminal mesh for the next inspection; do not export each trial.

Require TWO accepted whole meshes, true CV mass-solve residual<=1e-10 for every
field/time level, admissible states and scaled TARGET-integral defect<=1e-12 BEFORE
publication. Printed old/new-domain integral change is separate. Verify actual
adapted mesh/reference/restart point order and coordinates1e-12, finite conservative
states,rho/internal energy positive. Then fresh isolated ADAP_LOOP=NO,RESTART=YES,
READ_BINARY=YES disk load with distinct input/output prefixes and finite admissible
continuation. An adap-named restart on the original grid is not adapted proof.
Resume-cycle0overwrite/rejected-event consumption remain open issues.

Only after accepted Euler, qualify BL generation/curved/junction/interface/subsequent-
operator/ownership/heights behavior under MPI, then RANS500k. One heavy job,nice19,
libs1,build2–4workers,4TBscratch,verified host handles/resources and frozen inputs;
preserve resumable evidence and push exact reviewable whitelists. No raised native
time limit or relaxed gate. [Previous detailed assessment archive](Handbook/History/HowToProceed_before_compact244_20261107.md)
is historical, not the active plan.
