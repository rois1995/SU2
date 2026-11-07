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

## Completed five-context operator assessment

Existing current_compound covers the three two-face contexts,36470assertions/1case,
tool89903TERMINAL0,121.090848s. Existing current_residual covers only the other two,
34392assertions/1case,tool49679TERMINAL0,153.948427s. All source/runtime pins unchanged,
all sampled host handles absent; all five old q/J fields match exactly. No MPI milestone
repeated because no production C++ was changed. The first launch failed at MPI startup
before any test: missing previously qualified OMPI_MCA_osc=pt2pt; preserved as terminal53
and retried only after owned handles absent, restoring that environment/private4TBscratch.

| Actual core/context | Useful original stars | Useful tested alternatives |
| --- | ---: | ---: |
| 230716933/no-face42 | 3/4 | Not tested |
| 230222079/one-face52 | 0/4 | Not tested |
| 230694271/two-face48 | 2/4 | 4/4 |
| 230712740/two-face55 | 0/4 | 0/4 |
| 230691583/two-face35 | 1/4 | 2/5 |

Useful means net whole-context q/J reduction with neither count increasing, valid
metric/interface/replacement and non-increasing geometry excess. Alternatives overlap.
For230712740, shell+projected geometry split+optimizer reduces5q/3J→1q/0J but geometry
7→11,excess1.3994e-11→9.1785e-11(6.56x), correctly refused. Collapse+retained-star
reduces5q/3J→4q/2J,geometry7→6 but excess1.3994e-11→1.4424e-11, also refused. Neither
proves actual distance violation or an overly conservative refusal. A different core
already has an original star closing1q/1J→0/0 and reducing geometry8→4.

One-face230222079has four failed net-count original-star controls; one lowers debt
and geometry, but4q/3J remain. **Next bounded hypothesis:** test existing directed
collapse plus retained-star optimization in its52-cell context. The current compound
diagnostic globally skips non-two-face cores, so it has NOT assessed that combination
there. Move the diagnostic filter to only the shell-specific branch if source review
confirms that change is sufficient; no production change/new operator/budget follows
until a useful whole-context candidate is demonstrated. Do not extrapolate this final
residual to faithful replay of the earlier blocked tail.

A237/current_residual_assessment.json retains all20original-star controls,13alternative
candidates, churn identities, counted query work, overlapping timers and source hashes.
Requested heights are zero in these actual Euler contexts; no nonzero-height/MPI/nonlocal
publication/accepted mesh/transfer/restart proof follows. Geometry-clean labels describe
cores, not neighbors. Context-only current nodal fields with fresh associations match
old q/J; captured edge equality is disabled, candidate edges freshly checked.

The two-control subdirectory used a byte-identical symlinked config. run_cases resolves
that path for extra receipt lookup; fixture reads the raw environment directory, and
result rows prove only the two selected cases executed. Supplemental input checks use
all49actual-directory input hashes from its PRELAUNCH preparation and verify unchanged
at terminal. Future selected subdirectories should copy run.cfg physically so extra
receipt lookup follows selection. No rerun is needed solely to replace an identical
config symlink after the completed, independently pinned run.

## Completed bounded follow-up238 — hypothesis rejected

Only native_current219_witness.inc changed: moved current-capture two-face filtering
from the top of the compound loop to its shell-only branch. Directed collapse still
receives a complete removed-vertex star; scoring uses the complete removed+retained
star union, unchanged64-cell cap, replacement/interface/metric/geometry guards.
No production C++/config/gate/budget change. Primary/source second opinion found no
concrete unsafe assumption; source review is not correctness qualification.

Build238tool2530 TERMINAL0,234.181180s,all1302pins unchanged,both targets current,
all sampled compiler handles absent. Existing compound fixture on SAME five actual
contexts tool45667TERMINAL0,125.348654s,37975assertions/1case;allpins unchanged/all4
observed handles absent. All five old q/J fields match exactly. All33previous237
candidate outcomes are unchanged (including counts, certified excess, metric/interface,
trials; work/timers excluded).15alternatives now assessed in addition to20star controls.
R/assess_current238.py validates source joins, churn identities and preserved outcomes;
A237/current_residual238_assessment.json retains every candidate and raw refusal.

One-face230222079has12directed collapse trials:seven obstructed-cell refusals,three
body/feature refusals,one leaves/reverses geometry support;only one constructs.
Its collapse+retained-star optimizer gives4q/3J→4q/2J with geometry count6unchanged,
but certified excess1.7369589e-12→2.8218985e-12(+62.46%). The optimizer finds no further
progress. Metric, replacement/interface and size pass; geometry merit correctly refuses.
No useful alternative on this core. The new no-face control's additional constructed
collapse also gives no useful admitted improvement; its existing original stars suffice.

**Reject production integration of this hypothesis.** Keep the diagnostic coverage fix,
not a new production fallback or support expansion. No extra MPI campaign or full-M6
baseline repeat solely for a fixture change. The next same-cycle analysis must inspect
the refused changed faces and separate actual geometric displacement from conservative
certification using existing reference/query export and rigorous lower/upper facilities.
Sampling alone is insufficient. Do not add another local variant, larger cavity, new
scheduler or construction-policy change based on these refused results. After a justified
connected production correction and affected serial/ONE MPI qualification, immediately
return to full same-seed two-event M6. All acceptance/transfer/CFD/restart obligations
remain open. These final contexts are not historical replay of the earlier blocked tail.
