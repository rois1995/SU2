# Existing witness screening after terminal M6 build225

The full goal and strict gates remain in HANDOFF_Codex.md. This is one connected
geometry-cost cycle; no scheduler, new geometry representation, configuration,
construction policy or gate change is included. CAD remains optional.

## Captured evidence, rather than a new baseline

R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
Build225 naturally rejected its first event with24200q/21065J/1784geometry-bad
cells. All owned handles were terminal before compiled sources changed. The actual
full rejected mesh and its captured nodal metric are saved and independently read;
see [the terminal outcome](ADAPTATION_BUILD225_OUTCOME_20261106.md).

Existing support discovery, exact answer memo, prepared geometry, unchanged-face
association certificates and smoothing screens were reviewed first. An association
certificate does not imply zero numerical geometry error. Exact misses alone do
not distinguish nearby trial positions from evicted identical inputs. Historical
motion slack0of12 and optimistic subtree savings0.5765% did not justify another cache.
R/geometry225_source_review.json records source hashes and those limits.

The existing query diagnostic now accepts actual physical faces and nodal bindings
from the selected captured state. prepare_geometry225_queries.py validates all30
raw input hashes and streams actual physical tags; adapted IDs are never matched
against original reference-node IDs. Archive format7 and authority definition18
are distinct. Twelve actual faces cover root/midspan/tip, six geometry failures
and six proved-curved clean controls. No synthesized nearby trial positions.
Fresh exact entity context uses the production mobility angle60/marker roles and
support-discovery formula. It is not historical private transaction/publication replay.

Build226 passed with1302unchanged pins. Current-face exec30653 passed268assertions
in1case; exact saved v1packet replay exec26793 passed163assertions in1case. The
current SurfaceDeviation answers, cold/repeated/recomputed bounds agree. Source,
input and runtime pins were unchanged and all observed host handles are absent.
The initial MPI_Win_create setup failure is separately preserved; qualified pt2pt
and unique4TBscratch resolved it with identical source/binary/input.

Existing work trace, not perf/GDB sampling, gives the cold totals:

| Work | Counted units |
| --- | ---: |
| Domain upper calculations |458395|
| Sample lower-distance calculations |412553|
| Failed footprint coverage |239081|
| Successful footprint coverage |69740|
| Graph lift |40365|

Graph coverage/lift work is nested inside domain upper work and must not be added
again. There are7752failed and1536successful graph coverage checks. Five clean
near-tolerance queries dominate work. Exact memoized repeats cost23–292units.
The twelve faces are a bounded population, not a statistical distribution of all
smoothing candidates. Work units are not CPU seconds or isolated timing claims.
R/geometry226_trace_review.json preserves per-query cold/recomputed attribution.

## One candidate: avoid a lower search that cannot establish outside

SurfaceForest::Distance already computes a rigorous upper bound at the sample
using the prepared joined witness. A witness is a subset of the exact support;
its upper bound includes approximation error and interval roundoff. Therefore,
when that upper bound is <= the current query tolerance, the sample cannot prove
outside tolerance. Returning lower0 and that same upper is a valid interval.
Searching the complete support for a tighter lower bound cannot affect the only
lower-bound decision, lower>tolerance. Full domain upper certification still runs.
For an uncertain sample the complete-support path is retained unchanged. A sample
never accepts an entire face; two-sided final certification remains mandatory.

Build227 changes this shared local path only. The query's tolerance is stored on
its transient forest, never in a persistent cache. Reference/support identity,
association and coordinate invalidation, active shared references and transient
admission remain intact. Existing cumulative work and sticky exhaustion stay in
force; there is no new allocation or budget increase.

Affected callers are coarsening, splitting, boundary smoothing and quality through
ParameterError/BoundPhysicalSurface, plus final certification using the same
geometry facility. Their callers, metrics, requested heights, ownership and
construction decisions are unchanged. Existing full-support recomputation is the
oracle for the twelve real queries. Qualification must cover curved contexts,
holes, bodies/features, heights, cached/recomputed answers, exhaustion and MPI
empty/conflicting/migrated publication behavior. One MPI6 milestone, then return
immediately to the full same-seed correctedL1/50k/two-event M6 if the candidate
saves work and passes. Fixtures alone do not establish closure or a speedup.

After shape closes, dedicated geometry/size closure must preserve q/J and heights;
any validity-only geometry construction requires subsequent shape remeasurement
and reclosure under the same cumulative budgets. Two accepted meshes, conservative
admissible transfer, finite continued CFD and actual isolated adapted disk restart
remain unproved; BL/MPI and RANS500k follow those obligations.

## First actual-query result

Build227TERMINAL0,36.105134s,1302pins unchanged/both targets up to date; all host
build handles absent. Same twelve current faces:268assertions/1case PASS,
58.594722s, all pins unchanged/all observed host handles absent. Serialized exact
support packet is byte-identical to226; every bound and production guidance upper
is exactly equal. Cached/recomputed comparisons pass. Traversal870948→472780
(-45.7%); total cold work910366→512198(-43.7%). Eight queries improve,
four unchanged, none increase. Expensive clean query7 improves296672→113310.
Setup/context construction and uncached oracle are included in whole-test time;
no isolated wall-time speedup or whole-field closure gain follows from that time.
R/geometry227_query_comparison.json preserves each query and both case paths.
Affected serial and MPI qualification, then the full M6 attempt, remain required.

Connected qualification is now complete:102serial cases/35396assertions (actual
queries1case/268assertions,geometry88/30542,quality13/4586). MPI6:26cases on EACH
of six ranks,2874/3020/2869/3678/2841/2685assertions. All pins unchanged; all owned
host handles terminal. R/geometry_witness227_qualification.json records exact case
paths, hashes and scopes. This covers affected geometry, quality/heights and
publication/ownership behavior; it is not the separate later BL qualification.
Return immediately to full same-seed correctedL1/50k/5400 two-event M6. No full-run
speedup or acceptance established by these checks.
