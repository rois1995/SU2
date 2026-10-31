# Native adaptation — current handoff

Updated 2026-10-31. Full goal ACTIVE: TWO strictly accepted ONERA M6 Euler
CFD → mesh → CFD → mesh events at 50k metric complexity, conservative admissible
actual transfer, finite continued CFD and an isolated usable adapted disk restart.
Then dedicated BL/constraint/MPI qualification and corresponding RANS500k workflow.
Report actual points/cells separately from complexity. Geometry1e-4, q>=.2,
J>=.05, maxedge1.8 and requested heights remain strict. Construction retains exact
positive/nondegenerate/topology/body/feature/height validity and omits q/J.
CAD remains optional. No accepted M6 mesh/transfer/adapted restart yet.

All staged57 paths below are under
`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/`.
AFTER = `staged57/m6_runs/euler_build183_real_cfd50k_after`.
BEFORE = `staged57/m6_runs/euler_build179_real_cfd50k_before`.

## Authoritative state —188full M6 LIVE, source freeze ACTIVE

ONE owned heavy job: full TWO-event `build188`, attempt
`staged57/m6_runs/euler_build188_real_cfd50k_feature_safe`, exec16914.
Helper4103308/start87384875; time4103462/timeout4103463/launcher4103464
start87385499; ranks4103468–73/start87385504. ALL10identities match;
ALL1401source/header/test/helper/binary/config/input pins unchanged; all82loaded
runtime files pinned/matching. Keep frozen until verified terminal or preserved
failure/bottleneck/actual resource pressure justifies the authorized owned stop.
No extra perf/GDB, baseline campaign, gate relaxation or native budget increase.
Nice19,libs1,MPI6,unchanged50k/5400s per event. CFD SHA
191ccb8869b3320de35a56ed91f608790897ebe7626741841e3edaf077eb4ca1.

Connected genuine-feature fix source checkpointb64674701a; qualified34f458f5ed;
live provenance5434e8b835, all pushed to fork. CheckReplacement locally reuses
existing noncommitting Prepare on complete old/fresh boundary deltas against the
replicated accepted state. Quality candidate guards/final private backstop refuse
invalid proposals before one atomic publication, preserving valid peers. Exact
unchanged accepted-face contracts skip delta rebuilding. No per-slot collective;
final combined clearance/reservation/version/parameter checks remain. Construction
unchanged. [Design/evidence](Handbook/ADAPTATION_FEATURE_PUBLICATION_20261031.md).

Build188PASS54.151s/all1296pins unchanged/exact handles absent before validation.
Affected serial187PASS21cases/6148assertions. Affected MPI187completed163cases/rank:
all passed5ranks; rank3only newfixture's erroneous local-directory-change assertion
failed on an unaffected rank. All other assertions passed. Only testpin187→188
changed: assert global directory change. Focused188serialPASS2cases/71assertions,
MPI6PASSbothcases/all6ranks; allpins unchanged/owned handles terminal before M6.
`staged57/feature_publication188_qualification.json` defines reused scope. No full
repository-suite claim. Legacy nonlinear PSLG extension refused before preflight;
original legacy case retained, nonlinear entity case validates actual M6 path.
Earlier wrong retained-ID test expectation corrected to existing new-ID movement
contract. Failed fixtures are preserved in qualification/backlog, not passing evidence.

Current188seed AND initial continued original-grid CFD are BYTEIDENTICAL179/183;
positive finite61330pointstate. Initial ownership matches183 across ALL17compared
fields,303359cells. Coarsening EXACT183:292709cells,2627collapses,size55180,
geometry3512,same14870uppercalls/work303014856; elapsed303.106vs586.967s.
Freer host affects timing; quality-only publication fix cannot explain construction
speed. Construction SIZECLOSURE COMPLETE after18gens at855.998native seconds:
727722cells,size0,geometry4059. ALL18generation measured totals EXACT183,
including query work/calls; current quality entry892.080vs1831500.747s. This
construction timing difference is host-dependent, not caused by quality-only fix.
Quality entry counts EXACT183:54071q/53377J/4059geometry,size0,debt25236.787403.
Ownership410245cells/11.124s, allflags/counts/debtweights preserved; rawdebtspread
15.800→1.456. First SHORT quality window8.856s:78commits,53858q/53136J/4054geometry,
size0,debt24685.285922—EXACT183counts. Not a throughput comparison window.
Private tracked collectives0/unmeasured churn0; all16globaldriftpoints supported,
lost0,maxcurrentnodaldrift.084754961 (replicated views,not96samples). Existing
original continuous-target caveat remains below. Current receipts:
matched_construction183_assessment.json,quality_entry_ownership_assessment.json,
quality_entry_assessment.json,quality_trend_assessment.json,quality_window1_counters_assessment.json.
Latest COMPLETE quality window3 at673.283quality seconds:739479cells,
45817q-bad/44474J-bad/3954geometry-bad,size0,debt14479.797316,4028commits
(5.983/s),8254netq-bad removed(12.259/s). ALLfirst3windows counts/debt/commits
EXACT183; itswindow3took808.030s. Host contention differs; no isolated speed claim.
Sincequalityentry upperwork988904585 EXACT183; addedpreflight atlaswork65113804
(~4.2%of prior quality atlaswork). Scope/support/facet/trial/guidance counters and
nested timers are separated in matched_quality183_assessment.json, never added.
Window3commits1855,qchurn33917removed/30650created,J31497/28080;
privatecollectives0/unmeasuredchurn0/stuckshape0. Rankjoin-callshares
[.414,.469,.597,.749,.626,.580] include protocol+wait,notpureidle. Global16drift
samples supported/lost0,max.084197787. Full diagnostics in window3counter receipt.
Actual full-case publication barrier PASSED: complete window4/round21760,
5837commits, beyond183fatalround21746/5421commits, no rejection. Wholefield743645cells,
43013q/41452J/3936geometry,size0,debt11960.093779 at957.208quality seconds;
commits6.098/s,netqremoval11.552/s. ALLcensus/cache/shape/geometry flags complete.
feature_publication_barrier_assessment.json records exact evidence, not mesh acceptance.
Window4commits1809,qchurn28559removed/25755created,J26555/23533; privatecollectives0,
unmeasuredchurn0,stuckshape0. Join-callshares[.558,.595,.564,.482,.486,.337] include
protocol+wait. All16driftpoints supported/lost0,max.084197787.
Closest actual179BEFOREquality-time comparison:188window3/673.283s vs179window1/
698.727s(-25.444s,3.64%). 18845817q/44474J/3954geometry vs17948483/47386/4112,
commits/s5.983vs2.715,netqbadremoved/s12.259vs6.920,debtspread2.053vs4.863.
Hostload/cache/ownership/entrycounts differ; never claim exact clock/isolatedspeed.
No interpolatedcounts. BEFORE1479scomparison pending current matchingregion.
matched_before_quality_comparison.json preserves scope and raw comparisons.
Live provenance1793460502:ALL1401pins unchanged/all82mapped runtime files pinned.
Latest passive1793460756:ALL10identities match,~29GiBavailable,treeRSS~8.64GiB
(sampled sum,not jobpeak),load10.85,no rejection. Source freeze ACTIVE; exec16914.
No accepted mesh/transfer/restart. Last pushed window2checkpointa17a554060;
window3/4receipts and publication-barrier/currenthandoff ready for checkpoint.

183 is HISTORICAL TERMINAL: first event naturally failed genuine-feature coverage
at2763.908refine seconds before5400s; original CFD retained. Saved raw6rank outcome
before verified launcher stop during event2setup. All10handles independently absent
and all1401pins unchanged BEFORE ANY edits. AFTER/event1_natural_failure.json,
event1_failure_owned_stop.json,terminal_handles.json. Never treat historical PIDs
or build183statistics as the current live attempt. Both agents finished/no jobs.

## Preserved actual AFTER183 progress

Original input61330points/303359cells. Initial partition matches BEFORE exactly
across all17compared per-rank cell/metric/geometry-work/graph fields.
AFTER/initial_partition_assessment.json. Curved mobility setup181938600work,
674protectededges/2corners/2fixedpoints/671curvepoints/23806surfacepoints.

Coarsening COMPLETE:292709cells,2627collapses,55180size-bad,3512geometry-bad,
EXACTLY matching BEFORE. All construction census/cache/geometry flags intact;
q/J intentionally unmeasured. Native586.967s vs1142.910s. Same14870upper queries,
2456747support facets,3748716piece-selection work and294071794traversal work.
Facet build work1491264vs18681774 (~92%less), misses63144vs296744.
Summed facet build time88.803vs1056.738s is nested within upper time500.257vs1507.165s;
never add them. This supports cache benefit, not private-slot quality throughput.
Splitting ownership moved118115cells, conserved292709, transition max9.449s;
placement differs with geometry/cache cost model. AFTER/coarsen_assessment.json.

Construction size closure COMPLETE after18generations at1453.454native seconds:
727722cells,size-bad0,geometry-bad4059. BEFORE17generations/3105.412s:
726930cells,size0,geometry4197. Counts are comparable, not identical trajectory.
All generation flags complete, q/J unmeasured. AFTER/split_generation_assessment.json.

Quality entry COMPLETE at1500.747native seconds:727722cells,54071q-bad,
53377J-bad,4059geometry-bad,size0,q+Jdebt25236.787402739737.
Quality ownership moved410245cells in14.006s max; pre/post all whole-field counts
and complete/cache/shape/geometry flags preserved; debt sum roundoff7.28e-12.
Separate quality weights conserved exactly12953598. Raw q+Jdebt max/min changed
15.800→1.456, post debts4662.403/4246.852/4764.376/3528.335/4761.761/3273.061.
AFTER/quality_entry_ownership_assessment.json / quality_entry_assessment.json.

First complete quality window is SHORT (global round alignment):9.084s since entry,
728099cells,53858q-bad/53136J-bad/4054geometry-bad,size0,debt24685.285922;
78commits(8.587/s),213net q-bad removed(23.448/s). This is NOT a matched-time
comparison against BEFORE698.727/1479.020s windows. Await matching quality times.
Private tracked collectives0,unmeasured churn0. Global16fixed samples (replicated
per-rank views, NOT96distinct points) all supported/lost0; max relative current
nodal tensor drift.084754961 after this window. Numerical target drift is an OPEN
risk, not a newly proven frozen continuous field; prior transport already had it.
Current Euler composition empty:full_target1. Composed cases report sensor-only.
Latest complete window3,808.030quality seconds:739479cells,45817q-bad/44474J-bad/
3954geometry-bad,size0,debt14479.797316;4028cumulative commits(4.985/s),
8254net q-bad removed(10.215/s). Per-window private collectives0/unmeasured churn0;
global16samples supported,lost0,max drift.084197787. Nearest BEFORE complete census is698.727s,109.3s earlier; actual rates/counts
shown without interpolating or claiming an exact time match. BEFORE1479.020s
comparison remains pending. AFTER/matched_before_quality_comparison.json.
AFTER/quality_trend_assessment.json,quality_window*_counters_assessment.json and
per-rank stdout preserve full passive diagnostics, including nonadditive join-call
wall shares. Global queue/blocked-feature sums may contain replicated features.

## Matched saved CFD and BEFORE evidence

Readonly saved seed:`staged57/m6_inputs/euler_original_force_seed_v1`.
User accepted reasonable stable CL/CD; do NOT reimpose an assistant residual threshold.
CL.26953535374/CD.01353585654,200iteration ranges1.114e-7/3.121e-8;
all finite,rho/IE>0,exact original-grid coordinate/order match. Not a residual-converged
aerodynamic reference or an adapted restart. Every first remesh copies/reuses it.
Seed SHA1eaee788bd62bd92a92ec539577dcbcfc0f8042dcbcec23adb04e825bb01a880.
Actual continued restart SHA6bbed8da75c40eca7364cc9d3fe96f13311fad69706e013acb68ba8feaf4240a
is BYTE-IDENTICAL BEFORE/AFTER; initial1flow iteration prints CL.269535/CD.013536.
AFTER/initial_real_field_assessment.json. No repeated freestream pre-solve.

BEFORE179real-CFD is TERMINAL:all6naturally time-budget rejected,max5409.754671s;
evidence preserved before exact launcher stop during next-event setup. All10handles
absent/all1401pins unchanged before ANY source edit. No accepted mesh/transfer.
Entry:53318q-bad/52464J-bad/4197geometry-bad,size0,debt24844.47974341604,
726930cells at3225.021native seconds. Last COMPLETE window2,1479.020quality seconds:
740798cells,43764q/42274J/4067geometry,size0,debt13087.09702308638;
4098commits(2.77075/s),net q-bad removal6.45968/s. Post-entry debt spread3.022,
window2spread7.481. Terminal next target SPEED, with rank4low yield/repair-power
uncertainty. No new churn/ladder/drift counters existed retroactively in BEFORE.
Exit partial/zero counts mean UNKNOWN. BEFORE/terminal_assessment.json and window
receipts preserve raw six-rank evidence. Old startup175/179 are different fields.

## Continue the full workflow

1. Observe the LIVE full TWO-event build188 M6retry from SAMEsavedCFD, keep all
   pinned sources/helpers/binaries/config/inputs unchanged. Its actual outcome
   drives the next connected fix. No new baseline/profiling campaign or gate relaxation.
2. Preserve complete all-rank phases/windows and compare matching quality-time
   regions with actual timestamps, counts/rates, geometry work, debt spread and
   inclusive call-wall. Cadence differs; never fabricate interpolated field counts.
3. Shape/height close first, then size and residual geometry-directed repair.
   Existing construction handoff/remeasurement recloses shape after geometry changes;
   same cumulative budgets. On actual failure preserve raw6rank outcome, stop only
   verified owned run, audit ALLhandles/pins before editing, one connected fix/retry.
4. Require TWO accepted meshes/native_ref pairs, conservative admissible actual
   transfer(double residual1e-10,targetintegral1e-12),finite continued CFD and all
   point/history rho/IE>0. Source/tests are not actual transfer.
5. After workflow terminal, isolated COPY final accepted mesh/native_ref and
   matching flow; audit header/order/coords/finite-positive state, ACTUALdisk CFD
   continuation RESTART_SOLYES/ADAP_LOOPNO/correctSOLUTION_FILENAME/distinct outputs.
   Original seed/restart00000 never proves adapted restart.
6. Then dedicated curved/junction/interface/subsequent-operation/redistribution/
   requested-height BL qualification under MPI and RANS500k. No solver-label modes.

Terminal183complete EXIT census:742749cells,43620q-bad/42099J-bad/3938geometry-bad/
size0 at1261.498quality seconds;debt12512.093264.5421commits(4.297/s),10451net
q-bad removed(8.285/s),debtspread2.309. Private collectives0/unmeasured churn0.
AFTER/terminal_assessment.json preserves ALL7diagnostic categories/per-rank timing.
Immediate next target REPAIR POWER/CORRECTNESS: genuine-feature-preserving
candidate admission/publication failed before time limit. Topology yield56/9740;
remaining bad union53420,98.84%haveedge<.5: construction correlation is backlog,
not proof/cause or immediate reprioritization. No acceptedmesh/transfer/restart.

Connected source preserves oldest fairness,worst-first, exact changed-support rejected
star invalidation, global full-vertex reservations/version checks, one complete repair
publication, unchanged-base curved contexts and exact retained coverage admission.
Local debt ceilings and separate zero-clean quality partition constraint direct effort.
All private slots are local; common clearance checks EVERYapproved repair before
final reservation/publication. Passive location/churn/stuck/ladder/short-edge/geometry/
drift counters are defined in the linked parallel plan; do not add their nested times.

Open correctness obligations: current-nodal P1 transport drifts numerically despite
same field identity. Final strict metric gates use transported current tensors,
without independent original-target recertification; empty identity is a cache tag.
AFTER/metric_authority_source_review.md records primary-reviewed exact paths.
Do not claim an immutable continuous-original-target certificate. Original conservative CFD
donors are separate. General resumed adaptation cycle0path collision/rejected-cycle
consumption remain open; isolated restart does not claim general adaptation resume
fixed. serial_full170 was NOT passing; unrelated envelope/count/timer assertions in
backlog. No full repository suite pass claimed. BL/RANS capability remains pending.

## Operating links

[AGENTS](AGENTS.md) · [complete task](Handbook/ADAPTATION_INTEGRATED_NEXT_TASK.md) ·
[parallel design/evidence](Handbook/ADAPTATION_PARALLEL_REPAIR_20261031.md) ·
[feature-publication fix](Handbook/ADAPTATION_FEATURE_PUBLICATION_20261031.md) ·
[saved CFD](Handbook/ADAPTATION_M6_CFD_SEED_20261031.md) ·
[quality-service history](Handbook/ADAPTATION_QUALITY_SERVICE_20261031.md) ·
[geometry/smoothing](Handbook/ADAPTATION_GEOMETRY_QUERY_REUSE_20261031.md) ·
[cache/phase ownership](Handbook/ADAPTATION_CACHE_PHASE_20261030.md) ·
[findings](Handbook/ADAPTATION_FINDINGS_ASSESSMENT_20261031.md) ·
[backlog](Handbook/ADAPTATION_BACKLOG.md) ·
[preserved prior handoff](Handbook/History/HANDOFF_Codex_before_build183_quality_entry_20261031.md).

One owned heavy job,nice19,libs1,build2–4workers(helper3),MPI6bounded;
TMPDIR on4TBcompile-tmp, not nearly-full root/tmp. Monitor actual available RAM/RSS.
Fork pushes authorized:https://github.com/rois1995/SU2.git branch codex/native-3d-bl.
Git metadata outside writable root:use escalated git -c gc.auto=0 -c submodule.recurse=false,
diff/status --ignore-submodules=all (unrelated broken Eigen worktree). Whitelist adds;
no binaries/meshes/restarts/rawperf; leave unrelated155logs/.orig/submodules/untracked
steering/history files untouched. Primary retains integrated acceptance. ownership_survey owns the bounded
BoundaryParameters API/test task; no independent builds/jobs. quality_queue_survey
completed read-only replicated-state/candidate completeness proof and has no edits/jobs.
