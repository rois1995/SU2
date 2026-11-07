# Historical continuation assessment — NOT LIVE

Archived while full244is frozen. Read root HANDOFF_Codex.md and HowToProceed.md for current work.

# How to proceed

Updated 2026-11-07. This is a consolidation checkpoint, not a claim that the
adaptation workflow or the new diagnostic is complete.

## Objective and current state

The objective remains two strictly accepted ONERA M6 Euler adaptations at **50k
metric complexity**, with conservative admissible transfer, finite continued CFD
and an isolated adapted disk restart. Then qualify BL construction and constraint
preservation under MPI and complete the corresponding RANS workflow at **500k**.
Actual mesh point/cell counts are separate from metric complexity. Keep geometry
1e-4, q>=.2, J>=.05, maximum metric edge1.8 and requested heights unchanged. CAD
remains optional.

The current diagnostic outcome is in
[actual240geometry evidence](Handbook/ADAPTATION_CAVITY_GEOMETRY240_20261107.md).
Both refused collapsed faces contain genuine violations of the stored no-CAD cubic
surface: certified lower248.334µm/112.330µm versus100µm. All six old/collapsed queries
validate fresh associations/cached-vs-recomputed results. Exact rational checks verify
source-point rounding correction; reconstructed geometry-merit delta agrees with the
native candidate to3.73e-16relative. Diagnostic240passes443assertions; production
code remains unchanged. This does not establish that all intermediate upper-bound
merit is tight or that every refusal is physical.

The existing-constructor assessment is now complete:241all four paths refused;
242all four normal-displacement proposals inverted cells. The connected243change,
now locally useful but awaiting integrated qualification, adds positivity-only free-apex visibility placement to
the existing split path and reuses complete-star reservation/publication. Native
projection and exact strip/association/interface/height checks decide admission;
clipping only proposes coordinates. Validate the actual18-cell moving star joined
to its complete52-cell context, affected serial/MPI behavior, then immediately run
the full same-seed two-event50k attempt. Actual243 native placement constructs the18-cell free-apex star in3trials.
Complete context4q/3J/6geometry becomes3q/3J/6geometry, excess−74.68%; existing
optimizer then gives1q/1J/5geometry, excess−82.97%, with interface/replacement and
metric gates passing. This is a local repair result, not whole-mesh acceptance.
244passes the connected MPI6 milestone,55cases on every rank. Full two-event
M6 is now ACTIVE/FROZEN: euler_build244_visibility_real_cfd50k, tool82306,
source236e03ed64, same saved real CFD seed/L1 target and unchanged budgets.
The overbroad legacy serial campaign FAILED8cases; documented historical scenario/
donor-count assumptions and an unattributed wall-policy budget refusal remain
preserved, not waived. Use actual M6 outcome to choose further work.
No quality-aware construction is authorized
by these results; all construction/final gates and budgets remain unchanged.

Authoritative handles/source pins are in [HANDOFF_Codex.md](HANDOFF_Codex.md).
Latest full237first native event refused at the unchanged budget with COMPLETE
final census:296q/327J/1366geometry-bad/0size,1020222cells. All30capture files sealed,
1519pins unchanged, one authorized owned stop, all11solver handles absent.
See [237outcome and next target](Handbook/ADAPTATION_BUILD237_OUTCOME_20261107.md).
No mesh accepted; transfer/adapted CFD/restart still unproved. The selected198941point/1020222tet rejected mesh WITH metric is saved and independently
read: all1725bad cores/25856neighbors included, arrays/counts/SPD tensors verified.
Use237inspection files; no ParaView GUI check. Verify handles in HANDOFF before work.

First quality pass exhausted with44q/17J,46shape-union ALL blocked, empty queue.
Existing surface reconstruction reduced geometry1656→1379 but fresh shape entry
became410q/493J. Thus the next diagnostic targets repair power and coupled surface/
volume reclosure using representative ACTUAL residual cavities and existing operators.
Five actual final contexts have now been tested with existing original-star controls
and applicable compound alternatives; three have useful original-star repairs, two
show no tested net q/J improvement. The bounded238follow-up now tests existing collapse plus
retained-star optimization for one-face230222079/52cells. Only one of12collapses
constructs; it removes one J failure but raises geometry excess62.46%, so the
hypothesis is REJECTED for production. The fixture coverage fix passes37975assertions;
all33previous outcomes are preserved. That replay has now completed240above; next assess existing projected splitting/
reconstruction of the actual under-resolved chord and complete neighboring volume. Two-face230712740already shows strong q/J gains correctly refused
for increased geometry excess. See237outcome for exact complete-context results/limits.

All46blocked first-pass cells had short edges<.5; this correlation is not construction
causality and does not authorize quality-aware construction. Dedicated geometry-only
phase was not reached. Final set is not all stuck; preserve that distinction.

Current same-cycle proof/qualification is in
[the screening assessment](Handbook/ADAPTATION_GEOMETRY_SCREEN236_20261107.md).
237omits only a signed lift solve proven nonpositive by fresh intervals;116serial
cases/82379assertions and MPI6all26cases/rank pass. Matching construction traversal
falls5.04%, common quality~4.84%, all common stage outcomes equal235. More work fits
the unchanged budget; wall differences are descriptive under contention.236screening
was REJECTED/reverted; no slack cache, larger cavity, new scheduler or representation
is selected. Use the actual blocked tail and reconstruction neighbors to select ONE
connected hypothesis; then qualify and immediately rerun the complete same-seed loop.

Historical227saved198364points/1017523tets, but its final measurement caches were
invalidated and its printed final zero counts were UNKNOWN. Separate centroid
shape diagnostics recovered2368q/1101J there; they did not recover final geometry.
Those artifacts remain preserved in [227outcome](Handbook/ADAPTATION_BUILD227_OUTCOME_20261107.md).
Use235COMPLETE measured state for current inspection; do not repeat old recensus
work just because227needed it.

Corrected production L1at50k was selected over L2using the SAME saved real CFD
seed and unchanged constraints; L2was not assumed better. Keep this seed/target
for subsequent attempts. [Metric assessment](Handbook/ADAPTATION_METRIC_FLOOR_20261106.md).

The qualified225coupled shell admission correction preserved exact entity identity
across released seams and non-increasing whole-ring geometry guidance. Qualified227
shared geometry screening then avoided a lower search that cannot prove outside,
using an existing rigorous witness upper bound; full domain certification remains.
On matching construction work it reduced traversal17.6%. The first19quality windows
match225; later deadline-dependent ownership migration differs, so elapsed-time
comparisons cannot isolate a kernel speedup. [Geometry evidence](Handbook/ADAPTATION_GEOMETRY_WITNESS_20261106.md).

Historical227shape diagnostic: residuals are not uniformly free interior:
1414bad cells have no physical face but touch wing vertices;1341are free interior,
452have wing faces,39other physical faces,12touch other boundary vertices. Some
existing single-vertex stars exceed64cells; that alone does not justify larger
cavities. Select representative ACTUAL bounded contexts and test existing operators
and complete neighbors before adding a strategy. Distinguish placement, constrained
movement, connectivity limits and certification refusals. Preserve fresh association
reconstruction and do not claim historical private publication replay.

Historical227last complete110.6795s window net-removed853q/502J failures, with1211commits.
Boundary smoothing recorded12063candidates/445moves and72.60summed-rank seconds;
interior5641/676and3.26seconds. Timers overlap and local debt credits are not global
net improvement. Geometry-bad fell only2. Prioritize boundary repair efficiency,
separating expensive trials from low useful yield using current cavities. This is
a measured diagnostic priority, not an established new repair hypothesis.

Build229contains a small collective AND for existing ownership admission:
rank-local timing estimates must not split collective repartition participation.
The generalized current-star diagnostic passed36241assertions: four useful stars
each in the free/interior-wing-touch contexts, zero useful stars in both selected
physical-wing-face contexts. Build230existing compound diagnostic on those two cells finds shelling improves
shape but raises geometry excess27–32x, correctly refused;23of24collapses fail
construction, the remaining collapse+optimizer worsens whole-context counts.
One connected hypothesis tested in build231: existing shell followed by
existing relocation inside the complete50-cell context before checking against
the ORIGINAL patch. The66-cell context is excluded from this test;64-cell active
bound,160optimizer-work/30s limits and all gates remain unchanged. It closed local q2→0/J1→0 but increased geometry excess23.8x; still refused.
Eight old/replacement-face queries reproduce native guidance deltas and show one
raw exposed face per shell rigorously outside queried support (125.5/104.3µm vs
100µm). Cheaper certification alone cannot admit these two raw candidates.
That231candidate remained inadmissible; later232/235results below address its geometry failure.
The resulting geometry-conforming candidate232uses existing shell, reference-projected
split of its new diagonal and existing optimizer. On the complete50-cell context,
q2→0/J1→0/geometry4→2 with89.39%less geometry excess and unchanged maximum edge.
This supports connected production quality integration with work/ID/transient
accounting and fresh edge/metric/geometry/height checks, followed by the affected
serial/ONE MPI6 milestone and immediately the full M6. It is still one local
candidate, not accepted-mesh or publication proof.
Do not add local variants or raise bounds because one fixture stalls. Production guard behavior remains unqualified until
the appropriate connected serial/MPI milestone passes. No new scheduler/configuration,
larger cavities, quality-aware construction or geometry representation is selected.
Return immediately to full same-seed L1/50k two-event M6 after the connected change
is justified and qualified. Shape closure, residual geometry/size closure and shape
reclosure must all fit the unchanged budgets and gates before downstream transfer.

Historical build210 naturally rejected64q/39J/2283geometry-bad with complete census.
Its terminal evidence remains useful for coupled closure, but219changes the target;
comparisons must identify that difference. See
[terminal210 findings](Handbook/ADAPTATION_BUILD210_OUTCOME_20261106.md).

The shared failed-direct-bound correction is implemented and scoped-qualified on
the current-host GCC13.3/runtime193files: actual witness38938assertions/1case,
geometry2666assertions/34cases and MPI6all6ranks23cases each PASS. Existing rigorous
one-sided union bounds can tighten a failed correspondence bound; preserve the
better bound and independent affine maximum. All other gates and fail-closed
resource handling remain independent. This did not establish workflow closure.

The first quality tail had26q/11J, all26shape-union cells blocked and queue0.
Geometry construction reduced geometry-bad2535→2286 while reopening356q/488J.
Reclosure reached64q/39J before time budget, still2283geometry-bad. Full45192
published repairs remove49672q/48667J; subtract construction reopenings for exact
net49316q/48179J. Final80shape-union include10blocked, queue770, so final search
was not globally exhausted. Faster bulk alone does not prove that coupled closure.

After the corrected-target219outcome, select repair power or geometry/shape reclosure
from that run; old-target evidence identifies a coupled-closure risk. Reuse existing saved witnesses and query/context
facilities before selecting another connected correction. Postterminal212 tested
existing exact subdivision: no additional bad face was certified beyond210's union
refinement; do not add it to production. No speedup is established. 217controls show three smaller postcollapse stars also close the saved witness;
broader cavities are not justified. The selected217whole-mesh/metric capture is complete; current state is in HANDOFF_Codex.md. Construction soft q/J policies remain unproved and must not be introduced
under validity-only construction. Qualify a demonstrated change, then immediately
full two-event M6 with the same real CFD seed/50k/5400s and unchanged gates.

Geometry closure after shape uses the existing dedicated geometry-repair phase:
`clean_quality` plus zero size debt enables `EnableGeometryRepairs`. Those trials
must preserve strict q/J, size and requested heights while reducing complete
geometry guidance. Build210 never entered this phase: all six sealed first-event
prefixes contain no `geometry_repair_entry`. Its26-cell blocked shape tail instead
triggered geometry construction and reopened shape. If dedicated geometry repair
stalls, bounded validity-only geometry construction still requires subsequent
shape/size/height reclosure. Final whole-mesh two-sided geometry and transfer gates
remain independent; neither a zero-guidance cavity nor this plan proves closure.
See210/geometry_phase_readiness_assessment.json for source/prefix receipts.

Previous206terminal was809684cells/281q/341J/2129geometry/0size. Current has fewer
shape failures but154more geometry-admission failures; this is mixed, not strict
closure. Host/compiler/runtime and initial populations differ.32complete210quality
windows and actual EXIT are preserved; last window144q/126J/2287geometry is not
terminal64q/39J/2283geometry. Peak sampled descendant RSS14.736GB(decimal), not
physical RAM/true job peak.16metric samples stay supported, max drift6.1285%; the
original continuous-target fidelity remains unproved. All current handles,
provenance and downstream audit obligations are in HANDOFF_Codex.md.

The selected inspection build202 finished naturally at the unchanged native time budget. Its
complete terminal census contains **810163cells,295q-bad,294J-bad,2110geometry-bad
and zero size-bad**. All ten exact owned processes are absent;1403pins unchanged.
No adapted mesh was accepted. Complete SU2/VTU are now saved under
`inspection/rejected_mesh.su2` and `inspection/rejected_mesh.vtu`:164526points and
810163tetrahedra,2476bad cells and35426complete neighboring vertex-star cells.
See the [actual inspection and next witness test](Handbook/ADAPTATION_M6_INSPECTION_20261101.md).

Authoritative entry point: [HANDOFF_Codex.md](HANDOFF_Codex.md). Current terminal
receipts are in `staged57/m6_runs/euler_build210_real_cfd50k_bound_refinement/`;
selected inspection files remain in `staged57/m6_runs/euler_build202_real_cfd50k_inspection/`.
Build199 figures below remain historical comparison evidence, not the current
terminal state.

## What the evidence actually says

| Observation | Implication and limit |
| --- | --- |
| Construction entered polishing with727818cells,54081q-bad,53392J-bad and3770geometry-bad. | There is a substantial repair burden at entry. This alone does not prove construction is wrong: shape was deliberately deferred. |
| Quality removed net49713q-bad and50855J-bad in4131.48s, but failed the final gates. | The measured blocker is useful closure within the time budget. Neither commits nor debt percentage is completion. |
| Final q-bad:3623interior/745surface; J-bad:1881interior/656surface. | This is not exclusively a boundary problem. An interior cell can still contain constrained boundary vertices. |
| Private boundary smoothing used3367.75summed rank seconds; interior smoothing203.82. Their private merit gains were906.10and4623.38. | Boundary search/certification is expensive per unit of private merit. These timers overlap geometry work and are not independent global wall times or published bad-cell gains. |
| Geometry-bad fell only3770to3098. | Shape improvement does not establish geometry closure. Surface and adjacent volume must eventually satisfy both constraints. |
| All6132q-or-J-bad cells have an edge below.9;6045have an edge below.5. | Short edges are strongly correlated with surviving bad cells. This is not proof that additional coarsening would fix them. |
| Only33bad cells are fully blocked;67428features remain queued. | There is no evidence that every remaining repair is impossible. Queue size also does not prove that the available operators are powerful enough. |
| Most residual q/J misses are small:4001of4368q-bad and2264of2537J-bad are within10%of their respective gate. | A very small remaining debt can coexist with thousands of failing cells. Count whole-field q/J failures explicitly. |
| Shared winding screening cut construction traversal work52.9%, while observed timing gains in quality were much smaller and varied with host conditions. | Counted predicate operations have unequal cost. Do not extrapolate another large runtime gain from a work-count reduction. |

Build199 and the latest198retry1 have matching selected states through quality
window20. Ownership decisions then differ, followed by the first measured state
difference at window21. Compare actual time brackets and gate vectors after that;
do not assume identical trajectories or attribute differences solely to code.

The197finite residual-geometry continuation was **not exercised** in build199.
Do not declare it successful or unsuccessful from this run. Metric transport also
changes values at fixed samples;16supported samples do not prove fidelity to an
immutable original continuous CFD target. Keep that limitation visible.

## Saved state and inspection policy

The user wants an actual saved mesh for inspection, including an eventual
unapproved mesh. **Do not write mesh files for every trial, candidate, or routine
M6 attempt.** Capture must be explicitly installed for a selected substantial
implementation milestone when examining the resulting mesh is useful. Keep the
normal production path free of automatic failure-dump I/O.

For that selected run, obtain:

1. The actual complete final working mesh as `rejected_mesh.su2`, with coordinates,
   tetrahedra and physical boundary markers. Also provide VTU, exact native ID
   maps, point/cell counts and an explicit unapproved status. Never substitute the
   original input mesh or an original-grid restart.
2. Bad-cell and neighboring complete vertex-star views, joined across MPI ranks,
   carrying cached q/J, geometry excess, six metric edge lengths, node mobility/
   associations, genuine features and requested-height policies.
3. Bounded historical rejection evidence with feature/cavity IDs and its state
   context. A past rejection is not automatically the reason a final cell is bad.
4. The exact geometry authority/provenance and a reference surface for inspection.
   Reference tessellation is a visualization, never a geometry certificate.
5. Explicit missing, truncated, unmeasured and incomplete-cavity counts. Do not
   infer that a topology change is impossible from an incomplete cavity.

If practical in the same selected run, save the construction-to-polishing arrival
mesh too. This pair is more informative than another aggregate counter campaign:
it shows whether the same clusters survive, are introduced by polishing, or move.
Do not add a sequence of mesh files for intermediate trials.

Build199 did not retain its private working mesh. It cannot be recovered from
its counters. A new, deliberately selected capture is necessary.

## Inspect before choosing a repair strategy

Classify representative surviving cavities, including severe and near-threshold
q/J failures, curved boundaries, genuine features, seams, and apparently interior
cells. Inspect actual node constraints, not just whether a cell has a physical
face. For each class ask:

- Are points poorly placed even though feasible movement exists?
- Is useful movement restricted by a genuine feature, requested height, body
  separation or geometry tolerance, or by an unnecessarily restricted proposal?
- Does the cavity need a topology change that the current move/optimizer/collapse/
  reconnect ladder never proposes or cannot certify?
- Was the defect already present after construction, or introduced by a later
  committed cavity? Which measured trial failed, and on which unchanged gate?

Use the saved cavities for bounded replay with the same metric, active reference
and constraints. Include nearby clean cavities as controls. Measure net q/J-bad
retirement/introduction and geometry counts, not only local minimum quality or
accepted moves. Do not turn a small fixture success into a capability claim.

## Construction quality: a hypothesis to test, not an approved change

It is plausible that better construction proposals would leave a cheaper polishing
problem. It is also plausible that the main limitation is the cost and power of
the existing repair search. Current counters do not distinguish these sufficiently.
My recommendation is to inspect the arrival/final pair and replay representative
cavities before changing the construction policy.

Soft quality preferences could be worth testing **after** that evidence exists:
choose a better placement or topology among already valid alternatives rather
than vetoing valid progress because q/J are temporarily low. Possible experiments
include alternative split placement, collapse placement/direction and local
surface/volume topology. These are hypotheses, not demonstrated improvements or
instructions to implement all of them.

The current explicit policy excludes construction q/J measurements and ranking,
not merely hard quality vetoes. Testing a soft q/J preference would therefore be
a deliberate proposed policy change. Do not silently introduce it under the name
of polishing or a surrogate. Positivity, nondegeneracy, topology, association,
genuine features, body separation and requested-height validity remain mandatory.

A construction experiment should be retained only if thorough tests show that it:

- Preserves valid construction, constraints, coverage and all final strict gates.
- Reduces the total construction-plus-closure cost and net whole-field failures,
  including geometry, rather than merely producing a prettier entry histogram.
- Does not create a collapse/split oscillation, hide small cells, weaken transfer,
  or drift the target to make the mesh appear acceptable.
- Works on the actual difficult M6 cavities and meaningfully different curved,
  feature/body/height cases, including MPI ownership cuts and resource refusal.

Do not start several competing construction or smoothing implementations. Select
one connected hypothesis from the diagnostic evidence, validate it, then return
to the full M6 attempt. Keep unrelated alternatives in the linked backlog.

## Work saved in this checkpoint

The following is **unfinished diagnostic work**, not a qualified new solver build:

- `CNativeEngine3D.hpp/.cpp`: read-only cached cell-diagnostic view; retention of
  actual already measured q/J in the selection cache and its migration payload;
  a bounded ring of historical rejected-proposal metadata when admitted observer
  storage is installed. No per-trial files, new collectives, gate changes or
  automatic production writer were added.
- `staged57/inspect_native_failure3d.py` and its focused test file: an offline
  prototype for bounded rank shards, full SU2/VTU mesh output, native ID maps,
  bad-cell/cavity/reference views and explicit coverage. **Eleven synthetic-format
  tests pass.** This is not producer interoperability or actual M6 export evidence.

The opt-in C++ producer and selected full-driver wiring are now qualified in
build202:6845serial assertions/14cases,22MPI6cases on every rank, and24actual
producer/reader combinations. These checks cover curved/body/feature/height
metadata, cached values after migration, empty ranks, cross-rank cavities and
resource refusals. The actual416-byte little-endian RecordStream layout uses
8-byte booleans; the initial381-byte assumption was incorrect. Qualification is
saved in staged57/failure_export202_qualification.json. It proves diagnostic
behavior, not M6 acceptance or a construction/repair improvement.

The selected build202 full-driver M6 has naturally rejected event1. The existing
continuation was exercised:3quality stages and2construction returns. Shape reached
a small predominantly boundary tail before geometry reduction and later shape
reopening; actual EXIT is295q/294J/2110geometry. Inspect the saved actual state and
phase counters before attributing this to cycling, construction or lack of speed.
The subsequent authorized owned stop preserved first-event evidence; all exact
handles are absent. No further signal is needed.

Related plan: [bounded export](Handbook/ADAPTATION_BAD_CAVITY_EXPORT_20261101.md).
The [independent assessment review](Handbook/ADAPTATION_TECHNICAL_ASSESSMENT_REVIEW_20261101.md)
adds three verified interpretation limits: geometry-bad can mean conservative
failure to certify, interior cells can contain constrained vertices, and this
inspection snapshot is not a faithful rejected-trial replay. Actual witness data
must distinguish those cases before selecting another change.
Existing source facilities include the stage/rejected-state hooks and the
`NativeStageCapture` fixture. Prefer extending those over a new capture framework.

The historical205/206correction is the strict retained-feature proof described in
[the inspection record](Handbook/ADAPTATION_M6_INSPECTION_20261101.md). Build205
reproduces all52oldq/J exactly and identifies a missing unchanged companion-face
contribution. Build206passed the narrow proof correction and distributed regression,
then failed the full M6 at the unchanged budget. Serial/actual-witness checks and
one MPI6 milestone remain scoped evidence. The complete result did not establish
closure; no weaker geometry or shape gate is proposed.

## Resume and validation order

1. Read AGENTS.md and the current HANDOFF_Codex.md. Verify exact current host
   handles, source/binary/runtime/config/input pins and resource headroom. Full235
   session38789 is terminal; preserve its sealed final measured mesh/counter evidence.
   Verify current export/analysis handles before another workload. Never repeat stops.
2. Reuse235focused qualification and the existing capture/export facilities. Do
   not repeat passing suites or launch an old-code baseline. MPI fixtures qualified
   adjacent protocols; the full M6 is the pending end-to-end qualification of the
   exact new support-selection path.
3.235full run/export and bounded235/236queries are terminal. Keep the CURRENT
   implementation/helpers/binary/config/inputs frozen through build/validation;
   consult handoff for237handles. Require useful measured savings and affected
   serial/ONE MPI6qualification before a full same-seed retry. Selected235mesh and
   metric are already exported; no per-trial mesh dumps or repeated baseline runs.
4. Judge complete all-rank whole-field q/J removal per second, geometry/size counts,
   churn and counted work. Call timers overlap. A local private candidate, debt
   reduction or commit rate cannot replace accepted meshes. Select the next tested
   hypothesis from the terminal evidence; unrelated ideas stay in the backlog.
5. If accepted, complete the second event, conservative admissible transfer,
   continued CFD and isolated actual adapted disk restart, then BL/MPI and RANS500k.
   Current production distributed transfer checks every state/time level and scaled
   target-integral defect<=1e-12 collectively BEFORE setting new solver arrays
   (CConservativeTransfer.cpp final-check block). Its per-field summary prints true
   mass-solve residuals; require<=1e-10. Printed old/new-domain integral changes are
   a separate quantity. Preserve source/observed-path evidence and finite matching-
   grid disk states; the run_m6 finite-integral helper alone is insufficient.

One owned heavy job; nice19; single-threaded libraries;2–4build workers. Check
current RAM/load/disk, use4TBcompile-tmp for build/MPI/export scratch, and leave
unrelated jobs/files alone. Preserve resumable receipts and pushed reviewable
checkpoints. Do not raise the native time limit or relax a gate.

## Connected235integration qualified; full M6 refused

The complete geometry-conforming candidate is now production code, with bounded
four-star prefetch before reservation and private shell/split/optimization before
admission. Original64/replacement128cell limits, remaining work/time/sweep bounds,
strict q/J count progress, geometry/height/edge/interface checks and full publication
remain. Failed primitive233and interface-packaging234attempts are preserved;235
fixes canonical interface packaging without relaxing it. Actual full-context235
regression q2→0/J1→0/geometry4→2passes in143/160trials.116serial cases/78351assertions
and ONE MPI6milestone26cases/rank pass, pins unchanged. This is focused qualification,
not accepted meshes. Adjacent MPI protocols pass; exact support-selection predicate
was included in the full same-seed correctedL1/50k attempt, which refused at the
unchanged budget with3840q/2068J/1686geometry-bad cells and a COMPLETE final census.
No global closure/speedup is established. Preserve/export this actual final mesh
and use its terminal evidence before another connected correction. See [current design](Handbook/ADAPTATION_GEOMETRY_CONFORMING_REPAIR_20261107.md)
and HANDOFF_Codex.md for authoritative owned handles.
