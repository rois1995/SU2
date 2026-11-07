# Build237 terminal outcome and next diagnostic

Goal unchanged: two accepted same-seed corrected-L1 Euler50k meshes, conservative
admissible transfer, finite adapted CFD and actual isolated adapted disk restart;
then BL/MPI qualification and RANS500k. ZERO accepted meshes. All gates/budgets remain.

R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
A237=`R/m6_runs/euler_build237_signed_bounds_real_cfd50k`.

## Complete first-event outcome

Source4d1e2ec5b0; build237's only production delta from235 skips a signed residual
solve already proved nonpositive by outward intervals. Serial116cases/82379assertions,
MPI6all26cases per rank passed before this full attempt. The saved CFD seed, corrected
L1/50k target, constraints and native5400/outer14400 budgets are unchanged.

The first native event refused at native5347.003818s, round37505, with COMPLETE
cache/census/shape/geometry flags on every rank:1020222cells,296q-bad,327J-bad,
1366geometry-bad,0size-bad. Build235 ended3840q/2068J/1686geometry-bad. These are
actual complete final counts, not a same-time isolated causal speedup comparison.
At all common stages through round36800, counts/operations remain equal235;
last common quality traversal work is~4.84%lower. Construction traversal is5.04%
lower. Wall differences include host conditions and must not be attributed entirely
to the kernel change. More work fits the unchanged budget, but speed alone has not
closed the last required cells.

## The observed closure limitation

| Complete stage | Native seconds | q-bad | J-bad | Geometry-bad |
| --- | ---: | ---: | ---: | ---: |
| First quality entry | 705.179721 | 102058 | 101860 | 1997 |
| Exhausted first quality pass | 5291.79 | 44 | 17 | 1656 |
| Fresh quality entry after surface reconstruction | 5304.05 | 410 | 493 | 1379 |
| Final complete census | 5347.003818 | 296 | 327 | 1366 |

The first pass did NOT strictly close shape: all46shape-union cells were blocked,
queue0,missing_features0,567blocked feature entries.40of44q-bad and all17J-bad
have physical faces; four q-bad have no physical face. All46have an edge below.5.
No-face does not imply unconstrained vertices. This correlation does not establish
construction causality. No dedicated geometry_repair_entry was reached.

Existing fallback surface reconstruction reduced measured geometry-bad1656→1379,
then the fresh shape census found410q/493J. Construction's intermediate q/J caches
are incomplete: do not use their smaller printed counts as a fresh quality result.
The final window made193commits, retiring539/introducing425q and retiring563/
introducing397J: net114q/166J removed. At final,430shape-union cells remain; all430
have an edge below.5.164q/232J have physical faces,132q/95J have no physical face.
That final set is not all stuck: only17stuck_shape in the latest window. The
optimizer provided103net q/152net J removal in that window; coordinate moves
net-added2q while removing1J; collapse/reconnect removed13q/13J. Timers/rejection
flags overlap. These counters do not isolate all published effects by mobility class.

**Next target: repair power and coupled surface/volume reclosure**, with geometry
cost still measured. Inspect representative actual final boundary cavities and
controls using the existing bounded fixture before selecting a production change.
The original44/17blocked tail was changed by reconstruction and is not captured
as a faithful private historical replay. Preserve that limitation. Distinguish
placement, constraints, connectivity and conservative certification; test the
whole affected context plus neighbors for net q/J changes, geometry counts/excess,
metric conformity, heights and useful progress per second. No larger cavity,
quality-aware construction, scheduler or new geometry representation is selected.

## Evidence preservation and selected mesh

All30raw capture files sealed with SHA/count joins to all6summaries and final EXIT.
All1519source/runtime/input pins rehashed unchanged before one authorized pidfd
SIGTERM of verified launcher2862082/start7510715. First refusal made two accepted
meshes impossible in this attempt. Tool2688 terminal1 after5633.581992s; all11exact
owned handles absent. NEVER repeat this stop. A237/{capture_ready,source_freeze_before_stop,
owned_stop_once,terminal_handles,terminal_outcome_assessment}.json.

Selected offline export uses the existing validated inspector, low priority/libs1,
1200s bound and one-heavy-job discipline. This substantial improvement justifies
one new inspectable rejected mesh with the captured metric; it is not a routine
per-trial dump. Export tool13732 TERMINAL0,425.036783s,pins unchanged,all3sampled handles absent.
Independent meshio reader tool86406 TERMINAL0,5.542227s: all4VTUs readable with
exact point/cell array lengths;198941points/1020222tets;1725bad cores and25856
neighbors, none omitted. Counts296q/327J/1366g and430shape-union agree exactly.
All captured nodal tensors finite/symmetric/SPD,min eigen.0025878931;max edge
1.7999977268. SU2/VTU coordinates and tetrahedra match. No ParaView GUI check;
diagnostic SU2 marker-name fallbacks remain. A237/inspection_reader_status.json. No raw mesh/restart/binary/log is committed.

Read-only cumulative comparison now stops at the FIRST quality exit: construction
changes shape outside those quality-window churn counters. Latest-window location/
ladder analysis remains separate and validates its own window. An initially caught
cross-reconstruction churn assertion was an analysis-scope error, not solver failure;
no runtime code, choices, telemetry or counters were changed to bypass it.

Literature remains design guidance. The Springer paper supports coupled boundary/
volume operations and describes differing operator orderings; it does not establish
our gate equivalence. Both ScienceDirect links still return403 in this environment.
Existing hypotheses and retrieved-source limitations are recorded in
[repair support](ADAPTATION_REPAIR_SUPPORT_20261106.md). No competing implementation.

## Actual final residual selection and short-edge control

Existing support-census helper adapted read-only to237passes exact owned-row and
independent mesh joins (4.244196s). Final430shape-union:159with no physical face,
266with one face,5with two;71also geometry-bad. The two-face group has four geometry-
bad and one geometry-clean core. Five selected complete four-star unions fit the
existing64-cell cap:230716933(no-face,42cells),230222079(one-face/geometry-bad,52),
230694271(two-face/geometry-clean,48),230712740(two-face/geometry-bad,55),
230691583(two-face/geometry-bad,35). Other selected unions exceed64and remain excluded;
that does not prove larger support is necessary. Test existing original-star controls
before any expansion or production change.

All430shape-bad cells have an edge<.5. Clean controls also commonly do:
525251/954347(no physical face),54653/65234(one face),185/211(two faces).
Thus a short edge alone is not a useful causal diagnosis or authorization for
quality-aware construction. A237/support237_coverage_assessment.json retains exact
face counts, IDs, complete global star memberships, clean populations and source hashes.
No-face cells may still touch constrained boundary vertices. The reused probe initially
needed explicit boolean conversion for float-valued VTU indicator arrays; corrected
and completed without changing solver data or production code.
