# Goal3 — remaining physical-boundary reconstruction

Repository `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`, branch
`codex/native-3d-bl`, following published00e3ee4fd81c41a58040f7c4559bacfdc1a34d49.
Full Goal3 ACTIVE and incomplete. The published1984-cell mesh still has25 long
physical edges (13 cylinder/12 rim); geometry/height/shape pass, generation rejects.

## Actual blockage and connected implementation

The first diagnostic replay is terminal and preserved in `curved_boundary_step`:
zero edits, zero metric solves, refit50→57 cells before exceeding64; no normal-row
attempt. The direct normal-row50-cell probe does execute coupled repair but stops
atq .14912/J .03677, blocked at interior vertex29. Its complete accepted star adds
14 cells: union64 with the original normal patch. Refit's monotonic branch growth
preempts and consumes admission needed by this alternative.

The pending change makes refit, normal-row and background reconstruction use
independent private dependency branches under the same shared12-growth/global
work/time ceilings. It restores the original complete versioned payload/reservation
before a different branch. Normal reconstruction runs actual coupled repair before
requesting another star; a useful partial private replacement is joined to newly
admitted accepted neighbors, retaining its physical-constructor certificate and
checking exact subset/interface/contact/positive topology before another solve.
All publication metric/geometry/height/version checks remain unchanged. Added
scratch is256KiB, with existing cell/replacement bounds64/128 unchanged.

Diagnostic rejection labels and exclusive engine phases remain. Initial one-round
rejection costs6.61s engine-only:6.07s seed, .517s metric, .0168s reconstruction.
Nested field costs overlap phase totals and must not be summed with them.

The first connected build is terminal. `private_extension_controls` PASS167
assertions, including exact union, contact/positive volume and rejected-input
output preservation. The actual edge replay still FAILS:2 dependency backtracks,
1 successful private extension50→64,66 linearizations/3324 probes,512 private
trials, q .199513856/J .062281963 at the last normal proposal;0 accepted edits.
The25-edge accepted state remains unchanged. This is operator progress only.

Progress-aware continuation accepts the actual blocked edit in the same1984-cell
mesh:64 admitted→95 replacement cells,1 accepted insertion,74 linearizations/3504
probes/560 joint trials, work566; global squared length excess .041918515→
.031924794 (~23.84% reduction). All final native geometry/height/q/J/progress gates
remain strict. This first eager recovery's **full**1402-cell run FAILS/regresses:
1706 cells/79 long edges/excess .401507583 at4096 work/92.56s, versus published
1984/25/excess .041918515 at300s/3985 work. Different stops forbid speedup claims.
The negative whole run and its complete source snapshots are preserved.

The connected final scheduling keeps the published ordinary reconstruction/hint
policy until the ordinary split queue is exhausted or an existing observation
window has search attempts but no certified mesh progress. It then requeues blocked
metric/geometry splits and enables independent branches/carry/polish once. No new
production option, scenario mode, target relaxation or larger work/time/cell bounds.

Build68752 and its final bookkeeping correction are terminal. Decisive full-target
run87048 is live. The actual edge probe uses the existing observation window1
and two rounds to demonstrate cheap rejection followed by actual escalation; this
is an explicit diagnostic cadence, not a weakened mesh gate. The full target keeps
the original default window64 and4096/300s bounds. A matched MPI2 protocol will
require real dependency backtracking, private continuation and coupled solves
within512 work, plus unchanged accepted input/source/reference; final mesh/transfer
qualification remains separate. Earlier owned sessions are terminal.

These are engine-only costs: CFD-rank repartition/migration, return/rebuild, solution
and BDF transfer/output are outside this replay. Full accepted lifecycle and
adaptation/CFD ratio remain Goal3 requirements. Diagnostic timings on rejected
meshes do not establish production affordability. All remaining cold/geometries/
gradation/MPI/current+BDF/SA/restart/viscous/performance gates stay intact.

## Required next milestone — user instruction2026-10-15

A complete accepted curved mesh under unchanged field/geometry/height/q/J/full-edge
constraints and original run bounds. Local accepted edits and residual decreases
are diagnosis only. If the final staged recovery still rejects, preserve evidence
and reconsider the construction strategy before adding another local fallback.
Do not pursue more local operator variants as substitute milestones. This does
not remove any full Goal3 MPI/transfer/accuracy/performance requirement.

## Final staged result — rejected

Run87048 is terminal:1918 cells/30 long edges/excess .042646951, work4096,
168.76s; q .200019435/J .050003463,107 accepted internal edits. Recovery executes
once with4 dependency backtracks/3 private continuations. Full acceptance FAILS.
No further fallback is being added. Read
[the construction reassessment](GOAL3_CONSTRUCTION_REASSESSMENT.md).
The residual export PASS is diagnosis only. Independent exact1918-cell geometry/no
overlap/91 normal feet/height/exported-tensor shape arithmetic PASS;30 lengths
still reject. All owned jobs are terminal; no new MPI or CFD campaign.
The recovery implementation and complete negative evidence are pushed on
`codex/native-3d-boundary-recovery-study` at39a1f487a628b72b896a0df1bd4069e7b7a610fa.
Production source is restored to00e3; this branch retains evidence/decision only.
The existing build still belongs to the study; verify/rebuild before future use.
