# Same-seed shock/leading-edge diagnosis — active253 cycle

No production correction selected. Zero accepted meshes; all Euler transfer,
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

Bounded independent SVD quadratic fits on57points/two rings (114fits): sampled
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
