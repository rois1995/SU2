# M6 transaction replay — diagnostic56

Current: an accepted LOCAL replacement now exists for the captured first M6 failure.
All26 recorded construction calls and11 shape refusals replay exactly. Adjusting the
split parameter passes strict shape/edges, curvature progress, prescribed interface
and external contact. Full M6/DC-17/DC-18 remain open. The implementation direction
is now the user's reaffirmed [staged construction and movable-boundary optimization](STAGED_NATIVE_ADAPTATION.md), not more strict construction fallbacks.

## Verified capture

Evidence: `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/diagnostic56`.
The source/runtime/input-pinned six-rank run is terminal (session94073, exit0):
72.287s native, 80.475s launch, work2048 rejection. All ten diagnostic55 numerical
work/state counters match on every rank. This does not prove identical full trajectories.
Sampled summed process RSS peaks at3,636MiB, largest sampled driver619MiB;
shared pages are counted repeatedly, so neither is a physical whole-job peak.

At round4, edge23754–24333 on rank2 has:

- A saved global current mesh of303,428 cells and61,353 nodes, with unique owners
  and consistent shared coordinates. The complete initial edge star is cells114549
  and136288. Independent checks verify722 dependency records across the70 events.
- A byte-identical first split and exact first shape-failure message on replay.
  Initial patch minima q=.232448088/J=.062308272; candidate minima
  q=.183971960/J=.042789630. Maximum integrated candidate edge1.598362316 passes1.8.
- A reconstructed directory confirms the initial dependencies, versioned reservation
  for the single candidate, and outside-contact clearance. This does not replay
  competing peer reservations or publish a replacement.
- Twelve `ReconstructSplitPatch` calls and twelve successful growth reservations,
  followed by thirteen `Reconnect` calls. Two reconstructed fans report retained-skin
  visibility failure; other fans fail shape. One reconnection passes construction
  but fails geometry-error progress. The transaction remains rejected.

The previous rejection-only snapshots could not show this complete outer trial
history. The12-growth cap is now observed on this occurrence; exhausting it does
not prove that more growth would help or that the requested mesh is impossible.

## Implementation and scope

`EngineOptions` has an opt-in first-feature observer, forwarded by the native
remesher and consumed only by hidden tests. It snapshots every worker's current
cells before construction and records the selected outer geometry/metric trials,
versioned dependencies and donor batches. Callbacks cannot enter collectives;
diagnostic selection/error votes occur in the engine only when enabled. The
ordinary numerical policy and all mesh gates are unchanged.

The test helper freezes boundary parameters rather than retaining the live mutable
view. The six captured parameter payloads agree byte for byte. Storage is bounded
and truncation invalidates the capture. Serialization occurs after the timed native
event. Large whole-mesh/parameter snapshots stay local with hashes and reconstruction
scripts; compact reports, code and receipts are reviewable checkpoints.

The first compressed archive exceeded the25MiB output limit because replicated
parameter payloads were stored six times. The failed size check is preserved in
`analysis.log`. Hash-checked lossless deduplication reduces the complete archive to
25,114,331 bytes, below25MiB; `capture_archive_receipts.json` maps every logical file
to its stored bytes. `analysis_final.log` passes. This did not drop captured state.

Independent exact-containing-donor centroid analysis (`candidate_ranking.json`)
checks all11 geometrically constructed split/fan candidates. The initial four-cell
split is best by min(q/.20,J/.05)=.85579; the best grown fan scores only.55782.
Some better-looking first-failed cells therefore conceal worse whole-patch minima.
Growing these particular fans did not approach a fully admissible replacement.

The initial standalone launch (offline85224) refused reference decoding before
any construction replay: a1GiB conservative allocation allowance was incorrectly
used for the intended1GiB actual-RSS limit. The corrected test uses the existing
2GiB decoder allowance, while the serial launcher monitors process RSS and terminates
its owned process group above1GiB. This setup failure is not meshing evidence.

Offline2 reproduces all26 construction calls and11 shape-refusal messages exactly
(2,404 assertions pass before a setup exception). It does not replay growth selection
or the MPI schedule. The repair phase then fails its harness setup: wrapping an
unchanged sensor in a deadline composition requires a geometry edge policy. The
correction retains the original uncomposed field and enforces CPU/wall limits in
the launcher; it does not add dummy geometry bands or change metric arithmetic.

## Bounded repair result and next experiment

Offline3 (session18011, exit0) passes2,408 assertions, including all26 construction
calls and11 exact shape-refusal messages. Its repair attempts make no change:
6 connectivity trials and9 optimizer trials, branch wall0.00265s/CPU0.00399s;
maximum process RSS324,316KiB. No time/work/RSS limit is reached. The q/J=1 and
maximum-edge=0 fields in this historical receipt are unmeasured defaults, not a
candidate certificate; the updated reporter emits null for unmeasured values.

The initial four-cell split has no interior vertex that can move while its complete
skin remains fixed. The next existing reconstruction (event3, eight cells) introduces
one interior vertex. Test that saved seed with the same existing repair/optimizer
and unchanged final gates. This isolates whether staged construction supplies the
missing freedom; it does not justify accepting the intermediate mesh. The alternatives
share256 trials and60s. Restore Absolute admission for terminal transaction checks;
check the fixed interface, integrated edges, source queries and external contact.
Report transaction and final-patch acceptance separately. No repaired witness exists yet.

The production diagnostic defect is fixed and diagnostic56/concurrent2 passes all5
selected tests on both ranks (81/121 assertions,2.680s launcher): a peer's successful
final reservation clears an inactive worker's local construction refusal. The engine now preserves
that refusal across contact and reservation votes without changing protocol failure
agreement or admission.

No second full-M6 MPI prefix, larger construction caps, production shape-policy switch,
new partitioner, full1800s campaign, conservative transfer or restart is justified
by this result. Preserve source/build pins throughout each running stage.

## Staged seed result and fixed-vertex obstruction

Offline4 could not initialize Open MPI inside the sandbox; no replay ran. Offline5
runs on the host and passes2,411 assertions. The eight-cell seed has one interior
vertex and the same old footprint, complete skin coordinates and physical faces as
the direct split (`seed_context_check.json`). Connectivity uses64 trials and10 moves;
the separate interior optimizer uses64 trials and supplies the reported candidate.
Its full minima q=.096410391,J=.022544997 fail; max edge1.598362316 and fixed interface
pass. Branch wall.03076s/CPU.03305s; no time/RSS cap, launcher14.697s. The connectivity
sub-budget was reached, while the separate optimizer did not exhaust the shared256
trial cap. Neither changed candidate establishes acceptance or impossibility.

Independent enumeration in `fixed_vertex_skin_check.json` checks all15 possible
nondegenerate tetrahedra on the initial six vertices, with exact original-donor
containment for every centroid. The physical child face23754–24461–119505 has no
q>=.20,J>=.05 apex among the other three existing vertices. Even allowing exterior
or overlapping cells cannot satisfy this necessary condition. Thus flips using only
these fixed vertices cannot fill this fixed skin admissibly. Added vertices or
changed surface placement are not excluded; this is not general infeasibility.

Next connected construction question: the current curved geometric-refinement path
keeps the split position fixed while growing volume fans. The existing
`PlaceSplitPatch` can coordinate surface placement but is used for size-driven
refinement. Probe its best geometry-valid candidate against GeometryRefine, full
shape/edge checks, curvature progress and current-mesh contact, using captured state
and unchanged256-trial/60s/1GiB-RSS limits. This is test-only; no new production
construction path is enabled. The serial directory uses production's12GiB conservative
storage allowance, independently of the enforced1GiB measured process RSS cap.

Offline6 (session8203, exit0,19.158s launch) uses59 surface-placement trials but
retains the initial split exactly. Independent original-donor measurement confirms
q=.183971960,J=.042789630 and identical new-point coordinates. The helper's terminal
interface check incorrectly supplied an unsorted expected-face vector; this diagnostic
setup failure is corrected by canonical face-key ordering. No acceptance is inferred
from the exit status or the unmeasured q/J fields. Next:15 deterministic interior
parameters along the authoritative original edge, rebuilding each with existing
SplitEdge and checking actual shape plus curvature progress. This isolates whether
the placement search misses an available admissible position. It is not a cap increase
or a production fallback. Sources remain pinned through build96356 and its replay.

## Accepted local parameter witness

Offline7/session83140 is terminal/pass with immutable source/runtime/input pins:
15 parameter trials,4-cell candidate, q=.24412178036264912,J=.055704157122262989,
maximum integrated edge1.5983623161096352. Curvature progress, prescribed interface,
GeometryRefine admission, complete final-patch metric and current-mesh contact all
pass. Parameter t=.8125 supplies the best normalized shape score in this bounded
scan; t=.6875 and.75 also pass q/J. All other node coordinates and physical facet
identities remain unchanged. No extra interior point, relaxed gate or new MPI prefix.

Branch wall2.4828s/CPU2.4812s includes the serial full-current-mesh directory/contact
check; launcher16.9202s. Largest sampled process RSS905,027,584 bytes stays below
1GiB; no cap reached. Conservative directory allowance12GiB is distinct from that
measured-process limit. The accepted local witness is not an accepted global mesh.

`verify_repair.py` independently confirms positive exact orientations, original-donor
P1 centroid q/J and the single moved node; native full-edge and contact certificates
remain separately attributed. `offline7/repair_candidate.json` is readable; small
candidate binary, all15 scan rows, status/pins and independent receipt are retained.
Earlier failed repairs and the offline6 interface-ordering harness failure remain.

User2026-10-26 reaffirmed the existing final-only quality decision and explicitly
allowed boundary-node movement. Production still forces Absolute admission. The
next implementation is the connected validity-first construction, geometry/BL-aware
quality improvement and distributed repair lifecycle, reusing this accepted witness
as a regression. Original mapped chart restrictions are a separate mobility gap;
do not confuse artificial original-mesh edges with actual sharp physical features.
