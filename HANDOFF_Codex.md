# Native adaptation — current handoff

Updated2026-11-07(Europe/Rome). Goal ACTIVE: TWO strictly accepted ONERA M6 Euler
CFD→mesh→CFD→mesh at50k METRIC COMPLEXITY, conservative admissible transfer, finite
continued adapted CFD and ACTUAL isolated adapted disk restart. Then dedicated
BL/constraint/MPI qualification and corresponding RANS500k. ZERO accepted meshes.
Actual point/cell counts are separate from metric complexity. Fixtures are not completion.

Geometry1e-4,q>=.2,J>=.05,maxedge1.8,requested heights remain strict. Construction
validity-first: exact positive/nondegenerate/topology/body/feature/height validity;
NO fresh q/J measurement/ranking/veto. CAD optional; no-CAD authority sufficient.
ONE connected cycle/ONE owned heavy job. Freeze compiled/executing/pinned C++,
fixtures/helpers/binary/config/inputs. Verify exact HOST handles before edits/restarts.
Preserve evidence before authorized stop/fix/retry; never repeat historical stops.

R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
A244=`R/m6_runs/euler_build244_visibility_real_cfd50k`.
A237=`R/m6_runs/euler_build237_signed_bounds_real_cfd50k`.

## ACTIVE244 — full same-seed two-event M6, frozen

Production checkpoint236e03ed64, build244; launch metadata07272ec108, live quality
checkpointf1ca3da200. Newer metadata commits do not change pinned production.
Tool82306 ACTIVE, runner3440151/start8863722; MPIlauncher3440421/start8864247,
outer timeout3440420;6solver workers. R/m6_244_{launch,owned_startup}.json exact
HOST PID/start/argv/cwd identities. No stop sent. Native5400/outer14400 unchanged,
nice19/libs1/private4TBscratch. Do not launch another heavy job or change sources.
Host /proc is distinct from the sandbox: use escalated host inspection for identities.

Same saved real CFD seed and corrected-L1/50k target: run.cfg, original mesh/reference
and initial restart are byte-identical237; A244/initial_metric_input_provenance.json.
Initial coarsening and early split counts/query work match237exactly. Later trajectories
diverge slightly (new reserved node identities and placement), so same rounds are not
identical batch histories. Construction entry871576cells/2008geometry/0size;
shape deliberately UNMEASURED. Fresh quality entry native691.245338:
102043q/101849J/2008geometry/0size, ALL flags complete,871576cells.
237entry native705.18/871607cells/102058q/101860J/1997geometry/0size.

Latest complete window round35072/native1881.357870:916768cells,70567q/69979J/
1948geometry/0size, ALL flags complete. Cumulative net q-removal
26.45/s versus23727.01/s at common round; traversal ratio0.9623.
Ages and trajectories differ; no isolated speedup or predicted acceptance claim.
Latest location:63843/70567q-bad have no physical face (does not prove free vertices),
stuck_shape0,missing_features0,private_tracked_collectives0.
Net retired−introduced exactly matches field change.
Initial quality redistribution preserves ALL census totals/flags; q+J debt max/min
6.575→1.485(max/mean1.757→1.125). Later committed redistribution also retains flags.

Reuse R/analyze_m6244_quality.py for first-event stages, first uninterrupted quality
churn and exact-census location/ladder joins. It reuses existing validated parsers and
assertions, hashes complete six-rank prefixes and does not change runtime decisions.
A244/{construction_progress_assessment,quality_comparison237,quality_counter_assessment,
quality_entry_observation,quality_ownership_observation}.json current evidence.
Nearest measured quality ages are stated explicitly; no interpolated counts.
Timers overlap; join-call wall share is NOT CPU idle. Sampled summed RSS is not
physical RAM or whole-job peak. Last host check28GiB available,21GiB disk free;
last sampled tree about9.4GiB. Observe resources.jsonl while the frozen run proceeds.

Let this attempt finish unless preserved demonstrated failure/bottleneck/resource
pressure justifies authorized stop. If first event refuses, two accepted events
become impossible; before stopping ONLY verified owned processes, preserve complete
terminal census, sealed capture, source/runtime/input pins and exact handles. Do not
repeat old237/235/227/225/219/217/210 stops. No competing implementation now.

## Connected placement change and qualification

[Design, actual evidence and limits](Handbook/ADAPTATION_VISIBILITY_PLACEMENT243_20261107.md).
240proved real stored-cubic violations of chord7868–7876: certified lower248.334µm
and112.330µm exceed100µm. Cheaper certification alone cannot fix these faces.
241all four existing constructors refused;242all four normal-displacement proposals
inverted cells. Do not enable that shortcut. Private PlaceSplitPatch accepted apex
inputs but only tried no-move construction; current fix adds bounded positive-
visibility proposals for free interior apices, reusing existing convex clipping.
Exact strip/association/interface/body/feature/height checks remain decisive. Existing
complete-star lookup/reservation/version checks/publication now support geometric
splits, within64cells/8apices and existing work/time/memory budgets. Reuse9reserved
construction node IDs for split+up to8movers. No quality-aware construction, new
operator/scheduler/config/cavity cap or geometry representation. Signed-bound/cache/
ownership improvements retained. Clipping header is a mechanical Common extraction,
CFD forwarding include retained; source body identical after include-path edit.

Actual243 native projection constructs18→20cell star in3trials/1interior move.
Whole52→54context:4q/3J/6geometry→3q/3J/6geometry; excess−74.68%,maxedge unchanged.
Existing optimizer gives59cells/1q/1J/5geometry,excess−82.97%; replacement/interface/
metric admission pass. Eight old constructor refusals preserved. Raw GeometryRefine
passes; raw Repair correctly refuses. A237/visibility243_assessment.json. Local result,
NOT whole-mesh acceptance or historical replay. Only one mover/no requested heights.

Build243363.243376s/build244168.129617s TERMINAL0,both targets current,1303pins unchanged,
all compiler handles absent.244production identical243; only distributed fixture added.
Actual serial cavity93.042200s TERMINAL0/pins unchanged/handles absent. Connected MPI6
milestone101.983105s TERMINAL0:55cases on EVERY rank,pins unchanged/all10handles absent.
Actual directory check covers initially empty owners, stale/veto refusal and complete
cell publication; native result agrees all6ranks. It does NOT prove Engine selection
or commit distributed parameter edits; existing curved runtime/height tests pass
separately. Full244must establish usefulness. R/visibility244_qualification.json.

A mistakenly selected legacy serial_full FAILED:305.529924s,344cases/336pass/8fail,
1161529assertions/27fail,pins unchanged/all handles absent. Do not repeat generic suite;
reuse individually passing affected cases. Historical warning in
[geometry-query handbook](Handbook/ADAPTATION_GEOMETRY_QUERY_REUSE_20261031.md)244–258.
No old-code baseline or weakened assertion/gate. Donor/scenario-count assumptions are
historically documented; zero-work support returns before changed placement. RANS
wall-policy fixture exhausts4096work,95placement trials/0moves; attribution is NOT
established, baseline equivalence NOT claimed. Preserve as required downstream BL
qualification issue. R/visibility244_broad_serial_failure.json exact failures.

## Seed, previous full outcome and saved inspectable mesh

R/m6_inputs/euler_original_force_seed_v1:61330points/303359cells,M.8395/AOA3.06,
CL.269535/CD.013536 stable force window,user accepted reasonable forces, not full
residual convergence. ITER1continues this REAL saved solution, not startupCL.059.
Corrected219local Hessian floor/log normalization/flat sensors qualified40cases/
207605assertions,MPI6every4cases. Same-seed production L1/L2 comparison at50k selected
L1; L2coarsened4/11shock points,tip allocation6.44→13.59%,maxAR38.26→199.62.
R/metric_target219_production/{selection,comparison}.json; do not reselect/reseed.
Fixed16sample drift is not a whole-target norm; retain nodal transport semantics.

[Full237outcome](Handbook/ADAPTATION_BUILD237_OUTCOME_20261107.md):first event refused
native5347.003818,1020222cells/296q/327J/1366geometry/0size,ALL flags complete.
First shape pass exhausted44q/17J/1656geometry,46shape-union all blocked/queue0.
Geometry reconstruction reduced geometry but fresh shape became410q/493J; reclosure
ran out of budget. Actual geometry/volume construction obstruction selected after
[240exact face diagnosis](Handbook/ADAPTATION_CAVITY_GEOMETRY240_20261107.md), not from
literature alone. Complete final context is not faithful replay of earlier blocked tail.

Latest meaningful SAVED mesh remains A237/inspection/rejected_mesh.vtu and .su2:
198941points/1020222tets, captured_nodal_metric9row-major. All1725bad cores/25856
one-node neighbors included; exact array lengths/CellData vs PointData, finite SPD
metric, coordinates/connectivity and whole-field counts independently verified.
No ParaView GUI check; diagnostic SU2marker fallbacks. Not accepted/adapted restart.
Do not export each trial. Assess meaningful244terminal mesh for a new saved inspection.

## Remaining acceptance obligations — all open

Shape closure, strict residual geometry/size closure and required shape reclosure
must fit unchanged budgets. Two accepted WHOLE meshes on all ranks, not private
proposals, balanced weights, lower memory or passing fixtures.

True CV mass-solve residual<=1e-10 for every field/time level; all projected states
admissible; scaled TARGET integral defect<=1e-12 BEFORE publication. Printed old/new
DOMAIN integral change is separate. CConservativeTransfer.cpp997–1039 checks before
arrays set; PrintSummary1420+ reports true per-field residual. Finite-CFD helper alone
is insufficient.

Actual adapted_mesh_adap_00001/2.su2+.native_ref and matching
restart_flow_adap_00001/2.dat, exact point order/coordinates1e-12, finite conservative
states,rho/internal energy positive. Refused driver may write an adap-named restart
on ORIGINAL grid; not adapted proof. Fresh ISOLATED adapted disk load with ADAP_LOOP=NO,
RESTART=YES,READ_BINARY=YES and distinct input/output prefixes, finite admissible
continuation. Resume-cycle0overwrite/rejected-event consumption remain open issues;
isolated restart does not claim those fixed. Then dedicated BL/curved/junction/
interfaces/subsequent-ops/ownership/heights MPI qualification, then RANS500k workflow.

## Working rules and linked records

[How to proceed](HowToProceed.md),[backlog](Handbook/ADAPTATION_BACKLOG.md),
[integrated task](Handbook/ADAPTATION_INTEGRATED_NEXT_TASK.md)(historical155numbers
are superseded),[repair-support references](Handbook/ADAPTATION_REPAIR_SUPPORT_20261106.md).
Literature is hypotheses, not gate equivalence or authorization for a new scheduler,
larger-cavity campaign, quality-aware construction or geometry representation.
[Prior detailed handoff archive](Handbook/History/HANDOFF_Codex_before_compact244_20261107.md)
is historical, NOT LIVE.

Origin github.com/rois1995/SU2.git,branch codex/native-3d-bl. User authorizes fork pushes
with EXACT whitelists; preserve unrelated dirty logs/.orig/untracked work. Git metadata
outside workspace at SU2_AdapNoExt/.git requires escalation; use gc.auto=0 and
submodule.recurse=false,ignore-submodules=all. Never broad stage raw meshes/binaries/
restarts/logs. Build scratch on4TB,build3workers,nice19/libs1,one heavy job. Verify actual
HOST owned handle terminal before edits/restarts; observations/timeouts/stale logs are
not termination. No extra perf/GDB or old-code baseline campaign.
