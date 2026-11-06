# How to proceed

Updated 2026-11-06. This is a consolidation checkpoint, not a claim that the
adaptation workflow or the new diagnostic is complete.

## Objective and current state

The objective remains two strictly accepted ONERA M6 Euler adaptations at **50k
metric complexity**, with conservative admissible transfer, finite continued CFD
and an isolated adapted disk restart. Then qualify BL construction and constraint
preservation under MPI and complete the corresponding RANS workflow at **500k**.
Actual mesh point/cell counts are separate from metric complexity. Keep geometry
1e-4, q>=.2, J>=.05, maximum metric edge1.8 and requested heights unchanged. CAD
remains optional.

The previous selected corrected-target build219mesh is saved WITH its complete nodal
metric:197550points/1011596tetrahedra. It is unapproved. Its complete final census
is10286q/6872J/1753geometry-bad/0size-bad. All ten owned M6handles are absent;
first event naturally rejected at the time budget. Complete VTU/SU2 and independent
reader/tensor checks are recorded in HANDOFF_Codex.md and A219/inspection_artifacts.json.
Only the bounded cavity visualization is partial; full mesh and bad-cell view are complete.

The shared metric correction is reviewed and qualified in build219. Actual same-seed
production L1/L2targets at50k were measured before remeshing; correctedL1was chosen.
See [metric correction and target choice](Handbook/ADAPTATION_METRIC_FLOOR_20261106.md).
This corrected-target attempt has now supplied actual residuals. Most remaining
q-bad cells have no physical face; boundary certification is expensive. Current
bounded existing-operator witness tests compare complete neighboring q/J/geometry/
size effects on leading edge, blocked midspan, feature tip and an interior control.
These distinguish placement, constraints, connectivity and certification. The
existing root4-cell shell was useful but suppressed by an extra strict-new-face
veto; the selected connected fix reuses exact entity identity across released seams
and non-increasing whole-ring geometry guidance in validity-only quality repair.
Actual223regression enables2of4root stars (best q5→3/J3→2/geometry8→6). Midspan
shell can now construct but is correctly refused for increased geometry excess.
The connected225milestone is qualified (116serial cases;26MPI6cases per rank),
including a demonstrated geometry-version provenance fix in common import. Full
same-seed two-event M6 is now TERMINAL225:987865cells/24200q/21065J/1784geometry/0size.
All owned solver/export handles absent; complete capture preserved, selected mesh+metric
export/independent reader checks PASS:193855points/987865tets. Full VTU/SU2 complete;
bounded bad-cell/cavity views partial. Next is existing geometry-certification/boundary-cost analysis,
with speed the measured priority and repair power a residual risk. See
[225outcome](Handbook/ADAPTATION_BUILD225_OUTCOME_20261106.md). HANDOFF_Codex.md has exact handles,
source pins, initial-target equality and the authoritative run state.
Current227candidate is a bounded shared geometry-path correction, after terminal225:
reuse the existing rigorous sample witness upper bound to skip a complete-support
lower search that cannot prove outside. Full domain/final gates remain unchanged.
On twelve actual current faces, all bounds and production guidance stay EXACT;
traversal work falls45.7%, with no query regression. Connected serial102cases/MPI6each26cases qualification passes;
full M6 useful-progress measurement remains required; no full-run speedup claimed.
[Current proof and evidence](Handbook/ADAPTATION_GEOMETRY_WITNESS_20261106.md).
Context unions67/91cells do not enlarge active operator supports: selected core
vertex stars remain8–42cells.
Old218diagnostic remains deferred/unexecuted; preserve its unrelated local edits.
There are still zero accepted adapted meshes and no adapted transfer/restart proof.

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

The current connected correction is the strict retained-feature proof described in
[the inspection record](Handbook/ADAPTATION_M6_INSPECTION_20261101.md). Build205
reproduces all52oldq/J exactly and identifies a missing unchanged companion-face
contribution. Build206passed the narrow proof correction and distributed regression,
then failed the full M6 at the unchanged budget. Serial/actual-witness checks and
one MPI6 milestone remain scoped evidence. The complete result did not establish
closure; no weaker geometry or shape gate is proposed.

## Resume and validation order

1. Read AGENTS.md, HANDOFF_Codex.md and this file. Verify the current owned
   handles and terminal receipts before starting another workload. Historical
   build199and202handles must never be signaled.
2. Reuse the qualified build202 diagnostic and its exact source/runtime/helper
   evidence. Serial and one connected MPI6 milestone already passed; rerun only
   checks affected by actual changes or unresolved failures. Keep capture explicitly
   selected, not automatic on ordinary rejection or `WRT_ADAP_MESH` use.
3. Verify206terminal receipts and all10exact handle absences before source changes
   or sustained computation. Reuse the qualified local witness work and202saved
   SU2/VTU. Assess current counters and existing certification/reuse facilities
   before selecting one connected correction; do not launch another baseline campaign.
4. Use complete all-rank censuses to judge whole-field q/J retirement/introduction,
   geometry and size closure, then transfer/continued CFD. Diagnose a demonstrated
   failure before another correction. A local admissible private trial and its
   debt reduction cannot substitute for the complete workflow.
5. Use that evidence before pursuing the construction/repair hypotheses above;
   test one connected hypothesis thoroughly. If M6 accepts, continue the second
   accepted event, conservative admissible transfer, continued CFD and isolated
   adapted restart. Only after Euler passes proceed to dedicated BL/MPI and RANS500k.

One owned heavy job; nice19; single-threaded libraries;2–4build workers. Root/tmp
had only about1.5MBfree at the latest check. Put build, test, MPI and export scratch
on the4TBworkspace compile-tmp. Leave unrelated jobs/files alone. Freeze source,
binary, helpers and inputs during builds/runs; preserve receipts and push bounded,
reviewable checkpoints. Do not raise the native time limit or relax a gate.
