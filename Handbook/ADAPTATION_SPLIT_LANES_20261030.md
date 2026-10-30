# Finite split lanes and progress handoff

Current implementation: build157. Focused serial11cases/5306assertions and
MPI6 18cases/rank pass; this is diagnostic evidence, not M6 acceptance. The complete
two-event `euler_build157_split_lanes50k` attempt is running with unchanged gates,
50k complexity and5400s/event budget. Read its live receipts rather than assuming
this historical status is current. Build155 was stopped on the user's explicit instruction to
start this work immediately: terminal3798.033s, all1395runtime/input pins unchanged,
all9owned handles gone. Its22completed split generations left484886private cells,
46763accepted splits, maximum metric edge3.8544 and unique-edge counts not measured.
Size-incidence debt16466.18; no quality stage, accepted mesh or transfer. Detailed
evidence: staged57/build155_m6_summary.json and the stopped attempt's receipts.
The full strictly accepted Euler CFD→mesh→CFD→mesh/restart objective remains,
followed by BL generation verification and RANS. Euler50k/RANS500k targets and
all final gates and resource budgets remain unchanged.

## Missing capability and connected candidate

Pass-long retirement limited the attempted fraction of the entry long edges;
surface work held cheap interior work in the same collective rounds. Initial
geometry balancing improved real M6 throughput about1.9× at comparable early
states, but still did not reach quality. This candidate addresses attempt coverage
and phase coordination, not geometry-checker primitives or cache eviction.

Bidirectional splitting uses surface then interior scheduling. Selection counts
local physical-face users for each edge and maintains separate ordered queues.
Geometry demand stays in the surface lane. Collapse and standalone Refine retain
their policy. In the surface lane, Directory pass claims remain active and bulk
retirement affects surface-class neighbors. Once the complete global census has
no active surface choice, EndConstructionPass clears pass claims and interior
entry edges can reuse vertices across rounds. Full fresh lookup, exclusive
reservation, expansion re-reservation, geometry/metric/height checks and atomic
versioned publication remain unchanged in both lanes. No cell-only reservation
shortcut is introduced.

Physical incidence is a local scheduling hint. Imported physical edges and final
surface changes found in the interior lane are counted and keep all existing
checks. If interior publication reactivates entry-edge surface work, the global
quiescent scheduler resumes a fresh surface claim sweep before ending the same
generation. The entry frontier is never reseeded by a lane transition; descendants
still wait for the next generation. Rejected directions are blocked/served;
ordinary simultaneous conflict losers remain eligible. Touch clears its block
before lane-aware requeueing. Flat bidirectional split lanes also use the existing
fresh one-candidate/rank path; old multi-slot leases are not extended to this policy.

## Progress and compact diagnostics

New CNativeGenerationProgress3D.hpp reuses bounded World exchanges for a unique
canonical-edge census, routing each edge to a deterministic owner. Replicas merge
length by maximum, physical classification by OR and tracked age by maximum;
last disposition comes from the latest recorded round. Only fixed summaries/top8
are replicated, not the resident edge graph. Requested storage is admitted and
failure leaves the output unchanged. No acceptance predicate uses these diagnostics.

The census reports unique long surface/interior edges, unique edge-hinge debt,
global maximum, bins <=1.8/(1.8,3.6]/>3.6 and top8longest IDs/age/disposition.
The current long-edge history is pruned to current edges and not persisted across
adaptation events. Codes:0pending,1ordinary conflict,2pass deferral,3bulk retirement,
4construction/geometry/height refusal,5accepted,11metric measurement refusal,
12shape refusal,13metric progress refusal. Edge ages are tracked generation ages,
not proof that an edge was attempted. Diagnostic rounds label census freshness.

Stage rows report lane, generation, lane rounds/time/conflicts, late physical
discoveries, geometry debt, cache work and child/parent metric-length ratio when
both actual placed child-edge certificates are available in the existing cache.
The child ratio requires no extra donor/metric query. Scope preparation remains
nested in union time; collective elapsed includes communication and waiting.

After three split generations, compare a sliding window of unique-edge debt,
maximum length and fully measured geometry debt. If none falls beyond numerical
noise, enter the existing fresh quality census even if edits were accepted. A
productive edit alone no longer keeps construction running. Work/time/memory
budgets are cumulative and never reset by lane or quality transitions. Final
geometry, q/J, size and requested first-height gates remain strict. A handoff is
not acceptance and does not guarantee that quality can close the remaining debt.

## Qualification still required

New checks cover duplicate-edge census/metadata merge, atomic storage refusal,
fresh full-star reservation after claims end, and neighboring interior entry
edges serviced within one distributed generation without q/J measurements.
Existing descendant-generation, frozen-field, curved publication and boundary/
BL expansion checks remain relevant. Compile and run focused serial/MPI6 checks,
then the complete same M6 case. Passing fixtures and this candidate do not fulfill
the capability objective. Respect source/test/helper pins throughout each campaign.

## Review follow-up, 2026-10-30

The dedicated neighboring-edge lane fixture uses candidate_batch=1 and therefore
misses concurrent proposal selection. Existing bidirectional MPI tests use the
default MaximumCandidateBatch, but their parallel-commit assertion is cumulative
and may be satisfied by the preceding collapse pass. They exercise concurrent
adaptation without proving concurrent split commits; add a split-specific
assertion rather than treating that statistic as complete split-lane coverage. Add it at the next safe source-edit boundary, together
with printed construction_stall_handoffs and explicit handoff coverage. Do not edit
pinned tests or engine while the current M6 attempt runs. Build156's sole serial
failure compared Directory snapshots before and after intentionally clearing the
pass-active state; build157 corrects that assertion.

The stall threshold detects near-zero progress, not inadequate throughput. Slow
nonzero progress may still consume the quality budget. Inspect real unique long
surface counts, lane times and late boundary discoveries before changing policy.
Local boundary misclassification is counted and fully certified, not automatically
deferred. The census is bounded but lacks separate timing; its histogram and child
ratio are compact diagnostics, not a full length distribution or spoke certificate.
Surface-first remains the endorsed scheduling policy; do not remove it based on
incomplete provenance of older memory records. No complete Euler loop is proven.

## Live M6 evidence after first split generation

Checkpoint split_lanes157_checkpoint.json records private progress, not acceptance.
Coarsening gives exactly build155's292694cells,6078proposals, geometry trial/cache
counts, prepared bytes and size debt. Time1726.614s versus1180.126s:46.3% slower
execution despite identical work. A late five-second sample has78.49% inclusive
Allreduce cycles,17.10% surface union and12.92% TriangleUnion construction.
Percentages overlap; this is not a whole-run or isolated speed benchmark.

The unique census has4160long surface/41088long interior edges at collapse exit.
Split generation1 accepts41131private splits:679during169s surface sweep and
40452during50.08s interior sweep. Unique hinge11956.707->1706.429, maximum
7.083878->4.540239.515287private cells,3890geometry-bad cells, no quality stage;
construction q/J remain unmeasured. Interior boundary discoveries0. This supports
the attempt-coverage design and does not establish final mesh quality or closure.
Generation2 surface exit has4145geometry-bad cells; boundary closure remains open.
Preserve50k complexity separately from actual cell counts. M6 remains running on
frozen sources. Scope preparation/cache saturation remain distinct open costs.
