# Goal3 — reconsider construction before more fallbacks

User instruction2026-10-15: the next meaningful milestone is a complete accepted
curved mesh under unchanged constraints. A local accepted patch or lower residual
is diagnosis, not a substitute. Full Goal3 and its later workflow gates remain open.

## Decisive evidence

The unchanged1402-cell target uses its original sensor/geometry/BL/fade, window64,
4096-work/300s ceilings, q≥.20/J≥.05 and maximum full composed edge1.8.

| Construction/search policy | Cells | Long edges | Squared excess | Stop |
|---|---:|---:|---:|---|
| Published00e3 checkpoint |1984|25|.041918515|300s/3985 work|
| Eager independent recovery |1706|79|.401507583|4096 work/92.56s|
| Stagnation/queue-exhaustion recovery |1918|30|.042646951|4096 work/168.76s|

Both new whole runs FAIL. Different stopping points do not establish speedups.
The staged run executes recovery once,4 dependency backtracks/3 private
continuations,191 linearizations/8448 probes, and107 accepted internal edits.
These counters prove execution, not complete construction. The isolated1984-cell
worst edit accepts with64 admitted→95 replacement cells/560 joint trials; it does
not rescue the complete run. No new MPI/CFD campaign is warranted for this rejected
scheduling policy. The newly prepared MPI probe remains unqualified.

## Source-level construction limitation

`Common/src/adaptation/CNativeBoundary3D.cpp`:

- `ConstructFirstLayer` starts with a single background cone, then inserts one
  normal apex per physical face into that inherited fan.
- `ReconstructWallSplit` first subdivides the surface in inherited connectivity,
  then inserts normal apices sequentially. `protected_faces` prohibits a later
  visibility cavity from consuming an already completed wall child. Thus face
  order and old volume connections constrain the resulting transition topology.
- `RepairMetricConnectivity` can improve that candidate with movement, insertion,
  retirement and reconnection. Its demonstrated ability to repair one difficult
  patch does not establish that this construction workflow closes a complete mesh
  within affordable work. Neither failure proves the target infeasible.

The attempted recovery fixes a real branch-preemption defect, but the whole result
rejects. More retries, a larger cavity number, another local proposal or weaker
contracts are not the next implementation step.

## Construction strategy to investigate next

Make a connected wall region and its transition the construction unit, with surface
and volume connectivity designed together. Reuse geometry authority, original-P1
sensor donors, actual-query BL composition, edge quadrature, predicates, versioned
MPI dependencies and atomic whole-event rejection. Replace sequential protected
face insertion as the primary construction mechanism in that region.

Before production code, establish a coherent first-row/transition decomposition:
normal-height wall-face apices, compatible connections across shared edges and
vertex links, and a volume-fill/closure mechanism that does not inherit a bad
coarse cone. Face/edge/vertex dual cells are one candidate to assess; they are not
implemented or proven. They must cover the entire region without overlap, retain
exact prescribed altitudes, handle cap/junction constraints and admit subsequent
surface adaptation. A smooth offset surface alone does not ensure exact altitude
to every faceted wall triangle or positive tetrahedra.

Do not freeze a newly meshed anisotropic surface and assume an independent volume
mesher will recover it. The available Loseille/Alauzet/Menier2017 paper, section2,
explicitly discusses front-closure and anisotropic boundary-recovery failures and
motivates simultaneous surface/volume adaptation. That warning is relevant to the
new design; it does not prove that any proposed region construction will succeed.
Local PDF: `SU2_AdapNoExt/Papers/1-s2.0-S0010448516301142-main(1).pdf`.

A region proposal needs explicit physical/junction constraints, a certified outer
interface and a complete distributed dependency set. Shared planned vertices and
faces must have consistent identities/coordinates across regions. Import and
construction work must be admitted by memory/work budgets; merely increasing the
existing64/128 limits is not a construction strategy. Keep old accepted CFD mesh
and current/BDF solution untouched until the complete replacement is validated.

## Acceptance and cost

The next milestone requires complete native acceptance of the unchanged curved
target, independent exact embedding/no-gap/no-overlap/height/quality and full
composed-length checks. No saved coordinates, cylinder-specific dispatch, fixed
physical boundary, target smoothing/spreading, relaxed threshold, changed input or
budget ladder may substitute for that result. Existing artificial partition
interfaces need an ownership/dependency mechanism, not permanent freezing.

Measure construction work, memory, metric/integration calls and validation cost
on the complete candidate. Rejected-run timings are diagnostics only. General
features/interacting layers, cold construction/removal, current MPI1/2/4 N=M/M<N,
current+BDF/SA transfer/restart, actual RANS/viscous accuracy and complete lifecycle
cost still require the full Goal3 qualification after this milestone.
