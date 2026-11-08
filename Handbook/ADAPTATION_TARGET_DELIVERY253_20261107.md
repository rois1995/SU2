# Same-seed shock/leading-edge diagnosis — active253 cycle

One production correction selected after diagnosis: base quadratic conditioning254; qualified; full M6 terminal REFUSED. Zero accepted meshes; all Euler transfer,
adapted continuation/disk restart, dedicated BL/MPI and RANS500k obligations remain.
R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
Evidence: `R/target_delivery253_v1/{target_diagnosis,radial_reproduction,leading_edge_diagnosis}.json`.
Reproduction scripts and exact surface Mach/Cp plane cuts accompany these records.

## Provenance and terminal253

253 first event refused at native5346.554979s/round34761 (time budget), still
first shape pass. Final census unknown on693003/1032510 cells; partial30q/20J
cannot be compared as whole-field results. Last complete34752:1641q/695J/1044g,
exact251 counts/work decisions; quality time37.66s slower despite7.32% fewer upper
work units. No causal timing improvement. Raw30files preserved and all1520pins
verified before one exact owned MPI stop; all11handles absent. See A253 terminal
receipts linked by HANDOFF_Codex.md. No owned M6 run remains.

A bounded existing-output replay (`R/metric_target253_hessian_trace_v1`) added ONLY
HESSIAN to VOLUME_OUTPUT; same real seed, one continuation, Mach-only L1/50k,
constraints and current qualified253 binary. Terminal0/57.39s, all pins unchanged,
all owned host handles absent. All22 old fields and positions BIT-IDENTICAL to
244; only six finite raw Mach Hessian fields added. No remeshing/new target or
baseline campaign. Early analysis attempts failed for unavailable scipy and a
numpy integer JSON conversion; corrected to numpy-only with assertions, no solver
rerun or source change. Large restart/mesh/raw logs are local, not staged.

## Confirmed target behavior

Independent reconstruction of the single-sensor L1 metric from the saved raw
Hessian gives complexity50000. Inferred common eigenvalue scale.3853048201 has
398 unchanged major directions and reproduces11961.3627 pre-gradation complexity
(printed production11961.4); final49999.999626. This is a checked reconstruction,
not a newly exposed exact intermediate production array.

Beyond radius20:34.872% final complexity,99.164% dual volume. Raw normalized
L1 already puts62.699% there; common scaling gives7500.734 pre-gradation versus
17436.023 final. Near radius<2:raw4535.577,pre1084.774,final23958.974. Gradation
therefore redistributes/increases near-field demand strongly; it is NOT evidence
that gradation alone erased the shock. The large-volume L1 objective gives weak
nonzero farfield curvature substantial weight. Its physical/error justification
remains open. Nearly constant Mach is not proof that the curvature is numerical.
HMAX100 alone would give only about2.09 complexity over this domain, not17436.

Actual upper/lower surface plane cuts at y=.2,.4,.6,.8,1.0 are saved in CSV.
Strongest upper compression segments move upstream from x/c≈.64 to≈.33 with span;
Mach falls and Cp rises. Individual cut-segment width is NOT complete shock
thickness or a resolved lambda-shock identification. Integrated forces did not
establish shock resolution.

Reproducing eleven old gradient-selected strips: final transverse spacing
≈.044–.068 chord, raw normalized≈.046–.070, bounded before gradation≈.074–.112.
Thus coarse shock demand is already present in the raw normalized target; it is
not solely a late bound/gradation loss. Original triangles can already be finer.
Changing p, complexity or sensors is not justified by this observation alone.

Bounded independent SVD quadratic fits on65points/two rings (130fits): sampled
surface compression directions generally agree with production; many two-ring
Hessians agree to floating-point accuracy. Near-nose residuals are high
(two-ring median.379), and some stencil-dependent magnitudes differ. Interior
compression median direction dot≈.970, relative H difference≈.498. Farfield
samples are sensitive too. This is not localization of every39786/61330 warning:
production thin-wall cubic/quartic selection and fallback are not reproduced by
this pure-quadratic cross-check. No proven Hessian recovery bug yet.

## Confirmed delivery on actual247 terminal mesh

All six owned/metric shard hashes match preserved seals; complete census remains
1148q/484J/1044g. Actual physical wing faces are joined to reference facet/body/
sheet, rather than classifying a volume-centroid proxy. 1043geometry-bad cells
have wing faces;795 lie in the stated approximate LE band (swept x-xLE<.024,
y.15..1.15). All795 have near_feature=false. This approximately reproduces the
side796 count with a different explicit band; no individual-face causal claim.

At y=.2,.4,.6,.8,1.0 first3%chord, triangle count is7.84–10.73times original;
median span/nose geometric extent ratio1.37–1.80. EXACT cached production metric
face-edge lengths (not endpoint tensor estimates) have median span-dominant
.119–.154, nose-dominant.169–.205. More than99.5% sampled face edges are below.5.
Every sampled face-vertex occurrence has mobility dimension2; zero faces are
near genuine features. This rules out a simple fixed-boundary explanation for
these sampled smooth bands, not every association/certification constraint.

These are full-target phase cached measurements at their valid census state;
no separate invented surface metric. Exact length provenance follows
CellDiagnostic→FailureCapture and validated edge identities. Transported nodal
fields are not a proof of pointwise frozen-target equality: existing fixed-point
drift telemetry reports maximum relative difference≈.06089 on rank0. No claim
that similar regional distributions imply zero drift.

## Paper and decision

The local CDT3D paper DOI10.1007/s00366-024-01994-0 (§§3.2–3.5,4,5.1/5.4,
AppendixA,Fig20) supports assessing proximity screening, pre/post collapse and
coupled surface/volume candidates. Its CAD/normal criteria do not establish our
strict no-CAD geometry certificate. L2 projection for recovery is distinct from
the Lp interpolation objective. Fig20 is an iterated50k/100k/200k sequence, not
first50k adaptation. No scheduler/curvature-metric/policy change follows from it.

Likely mechanisms: already broad flow target explains little shock refinement;
geometry-driven insertion and limited subsequent collapse/repair may explain
short, insufficiently anisotropic LE delivery. The latter is NOT established
causally: next inspect representative complete actual surface/volume cavities,
existing collapse/reconnection/relocation, geometry versus association refusal,
and whole-patch/neighbor q/J/geometry consequences at unchanged work limits.
Reuse existing actual-cavity harness and prior controls. No larger cavities,
blanket minimum edge gate, quality-aware construction or new geometry authority.
Select ONE correction only after that evidence; qualify affected behavior then
immediately resume the full same-seed two-event M6 workflow.

## Follow-through: actual cavities and selected condition correction

Existing complete-cavity compound test on actual LE cores197029133/213247623
passed35836assertions,103.969s including harness,all pins unchanged/handles absent.
Eight original-star controls,24directed collapses. Many collapse directions fail
exact positive-cell obstruction;some fail body/feature contraction permission
(despite broad two-dimensional mobility). One51cell context constructs collapse plus
retained-star optimization removing2of4 guidance-geometry bad-cell counts with no shape
failures,where raw collapse introduces3q/2J. Other candidates worsen shape or
geometry;one40cell context has a useful original-star shape control already.
Do not add an operator/scheduler based on this mixed evidence. Although boundary=false,geometry-constrained smoothing DID execute:302candidates/7moves.
Smooth dispatch ignores that flag when geometry is supplied;the flag gates generic
boundary topology. The earlier interpretation was wrong and is corrected from saved results. Recomputed guidance
geometry scores differ from terminal census certificate excess;this is an open
certification-semantics question,not a successful cached/recomputed equality check.
The worst sampled core has115cells and a90cell vertex star;excluded at unchanged64.

Eight largest-curvature trailing-edge nodes have first-ring polynomial design
condition2.9million–145million;production Hessians match those fits. Several
residuals are below.05,so current first-ring growth never asks for the existing
well-conditioned second ring. Independent two-ring quadratic conditions≈3–6;
curvature norms drop by orders of magnitude. Smooth manufactured cubic perturbation
also gives first-ring Hxx10.60 instead of2 while relative residual2.55e-6,versus
2ring1.99999968. This establishes a condition-admission defect,not a claim that all
farfield curvature or missing shock refinement is caused by it.

Selected ONE correction:reuse the existing richer-fit Frobenius QR condition<=1000
check for base quadratic fits;grow through existing owner-complete two rings and
preserve fallback if both unsupported. Shared2D/3D path,one controlled-stencil
regression. No physical sensor filtering,target norm,complexity,gates,geometry
representation,construction policy or scheduling changes. R/condition254_plan.json
sets evidence/acceptance;build254 and focused31serial/168175assertions plus ONE MPI6(17cases each rank) passed. After
focused checks and one MPI6 milestone compare actual same-seed target/flow,then
immediately resume complete M6. Independent surface/volume cleanup and certificate
questions remain hypotheses in backlog.


Actual corrected target replay254:all flow fields/coordinates bit-identical;71raw
Hessians change. Max raw H norm1.3025e13→8.5484e7;minimum QR pivot ratio
1.1528e-8→.00290566;zero WLS fallbacks. Final complexity49999.999976;independent
SPD/bounds/AR and directed-gradation audit passed. Sampled compression target
5–12%finer (about.041–.063chord),farfield share34.872→40.398%;farfield allocation
remains unresolved. No claim of resolved shock or M6 closure. Qualified correction
returns immediately to full same-seed two-event M6;R/condition254_qualification.json.


Refusal classification:all SIX body/feature contraction refusals among24trials
remove a boundary-associated vertex toward an UNASSOCIATED INTERIOR retained
vertex. These are not evidence of unnecessary seam constraints. Broad surface
mobility does not authorize embedding the physical boundary in the interior.
Seven directed collapses construct,eleven fail cell obstruction,six fail this
association check. Guidance bad-cell retirement can also reassign unchanged
physical faces to fewer cells;it does not prove fewer failed face certificates.


Admission clarification:only ONE of22bounded result rows passes the GeometryRefine
METRIC-change validator:it is a worsening coupled candidate(1→2q,0→1J,5→7guidance
geometry bad-cell counts),not a successful geometry repair. The two shape-clean
51cell collapsed+optimized candidates fail the exercised MetricChange::Repair
validator(no size/shape debt gain) and GeometryRefine validator(requires cell
count growth). Replacement/interface pass,but these are separate checks.
The fixture DOES NOT evaluate MetricChange::GeometryRepair,whose metric admission
has no cell-growth requirement. No conclusion about that production mode follows.
Private guidance/cell-count improvements are insufficient proof of production
admission or fewer failed physical-face certificates. Boundary smoothing was exercised(302candidates/7moves),but
nonlocal publication remains untested by this diagnostic. Preserve these gaps;
no further implementation while254 is frozen.


## Confirmed Euler metric/geometry separation (source review while frozen254)

Primary checked CSolver.cpp2665–69 and CNativeRemesher3D.cpp349–374:the geometric
BL metric and composition are gated by actual ADAP_BL requests. This M6 has none.
FrozenField::Query starts target=sensor and only applies composition if present
(CNativeField3D.cpp524–544,638–645). Runtime full_target=1 matches empty composition.
Thus current q/J,metric edge length and complexity use the Mach tensor;surface
geometry is separately certified/refined. ADAP_SURFACE=YES permits surface work,
but does not create a geometry metric. CAD remains optional.

The passed TangentialSize tolerance/coarsening test is the 2D PolylineReference
helper (CNativeMesh2D_tests.cpp568+);no production caller was found. It is not
proof of a ready3D M6 geometry-metric intersection. Hypothesis only:geometry-required
short nose chords combined with flow-metric shape constraints may require short
spanwise/volume spacing. Existing data do not establish the necessary spacing,
point count or a new metric policy. Do not introduce a mandatory curvature metric,
change complexity/gradation/p,or relax q/J from this source observation.


Boundary-setting correction (2026-11-08):earlier source notes inferred that
options.boundary=false disables boundary smoothing. Primary checked Smooth at
CNativeEngine3D.cpp6169–6178:fixed nodes with geometry call SmoothBoundary
independently of that flag. The saved22result rows contain302boundary candidates
and7moves;the original-star and combined controls therefore DID exercise boundary
relocation. Generic unbound BoundaryFlip/BoundaryCollapse are separately gated by
options.boundary&&!options.geometry (6495/6509). No test/runtime rerun or source
change was needed;le_compound_assessment.json records the correction explicitly.


Existing-cut flow-resolution diagnostic (no solver rerun):connected half-peak
negative-Mach-gradient bands at y=.2,.4,.6,.8,1.0 span .0830,.1157,.1333,.1422,
.1067chord,with Mach drops≈.16–.20 and Cp rises. These are broad discrete
compression bands,NOT a complete lambda-shock identification or certified physical
shock thickness. Together with requested4–7%chord spacing versus existing3–4%
triangles,this supports a coherent flow-to-target explanation for little first
surface refinement. It does not prove shock resolution. One saved-seed continuation
changes maxMach.004369,maxCp.005852;forces alone do not establish local convergence.
Same saved seed remains authoritative;no change of CFD solution,sensors,p,complexity
or adaptation decisions. Reproduction:target_delivery253_v1/analyze_flow_cuts.py
and seed_flow_resolution.json (all input/output hashes,explicit band definition).


Frozen254 has now demonstrated the existing guarded geometry-construction return,
not full shape closure: first quality exit24832/native3274.720 has49q/29J/974geometry.
Reconstruction and fresh shape remeasurement repeat;at round26176/native3658.670,
all737632cells measured:72q/49J/196geometry/0size. Current shape/geometry failures
remain;no accepted mesh/transfer/adaptedCFD/disk-restart claim. The first-pass
quality-comparison series is intentionally frozen to23windows;all later transitions
are retained in A254/phase_recovery_progress.json and complete stage evidence.


Independent A247 tensor reproduction now also matches the provisional LE demands:
median h_span/h_nose at five stations=2.713,2.796,2.550,3.274,3.284;median long-axis
span alignment abs cos=.99901,.99984,.99994,.99943,.99973. Actual triangle extents
are median.073–.107of requested span spacing,.165–.204of nose spacing. VTU/SU2
coords/tets match;all201203captured tensors available/SPD;actual53500wing faces
and station counts472/459/488/527/549 agree with exact-edge analysis. These are
mean nodal-tensor restrictions,not production quadrature/certification or absence
of pointwise drift. Reproduction script/JSON:target_delivery253_v1/
leading_edge_tensor_reproduction.*;bounded≈4.9s,one thread/nice19,unchanged input
hashes. This does not justify a curvature metric or prove which short edges are
geometrically necessary. 254 is now terminal REFUSED; the complete residual capture drives the active bounded diagnosis.


## Terminal254 outcome and saved mesh (2026-11-08)

The actual same-seed corrected-target attempt refused at native5332.870105s,
round42620, under the unchanged elapsed-time limit. Complete terminal census:
741236cells,251q-bad,205J-bad,95geometry-bad,zero oversized-edge demand.
Best complete shape counts34q/20J had728geometry failures; minimum60geometry
failures coincided with133q/122J. They never closed together. Late repair windows
can retire fewer bad cells than they introduce despite reduced hinge debt.
This supports testing repair power/admission on actual residuals; it does not
justify another speed baseline, larger supports or quality-aware construction.

All30raw capture files are sealed,all1520pins verified before the one exact
owned MPI stop,and every solver handle is terminal. Export and independent
reader are also terminal,with unchanged pins/hashes. Actual saved rejected mesh:
150080points/741236tets,PointData captured_nodal_metric9row-major,CellData q/J/
geometry/metric-edge bounds. All260bad cells,2400neighbor cells and reference
are preserved. VTU/SU2 coordinates/connectivity match;all nodal tensors SPD,
all tetrahedra positive. These integrity checks do not certify strict acceptance.
See A254/inspection_reader_status.json and terminal handle receipts. No accepted
mesh,conservative transfer,adapted CFD or actual adapted disk restart is established.

The next bounded selection must classify adjacent-volume cells using CURRENT
physical-wing vertex IDs,not original reference vertex IDs. Original IDs miss
newly inserted/moved boundary nodes;reference facet IDs remain valid associations
but do not equal output face vertex triples. A provisional selection caught at
primary review is preserved explicitly and must not drive cavity tests.
