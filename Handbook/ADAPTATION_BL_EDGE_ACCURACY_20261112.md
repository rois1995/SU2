# Curved BL edge measurement: decision accuracy audit

Same connected curved-BL cycle; source/build/handles in HANDOFF_Codex.md.
Current-source review after terminal428: see `R/edge_measurement428_review.json`.
All1453 build428 source pins and both binaries match. The retained raw394/399
diagnostic inputs also match their recorded hashes. No new baseline or profiling
campaign was launched, and no production measurement precision was changed.
Whole-edge outside-BL exclusion qualified383 in both production factories. Ambiguous,
stale, exhausted or incomplete bounds fall back. Outside controls use exact background
P1 integration with zero BL compositions. This is not evidence that the original
M6 difficult edge is outside; it was not excluded. Strict gates and physical law stay.

## Existing evidence and demonstrated correction

Reuse394 original difficult-edge/projection/accuracy diagnostics; do not rerun a
baseline. It used the older reconstructed371 policy, not qualified complete396 policy.
393 immutable prepared-region reuse plus394 earlier use of the SAME existing interior
KKT criterion resolves the actual measurement deadline. Expanded serial83cases/22594
assertions, MPI6 56cases/rank and18default/tighter actual point pairs pass. This is
numerical measurement qualification, not a mesh or accepted-workflow improvement.

On that saved edge: numerical length42.06899590894,6375evaluations,4185initial/2190
refinement. Geometry preparation69.487s and composition78.026s are nested in147.517s
edge total; wrapper207.818s. Geometry preparation includes discovery/ownership and
query work; composition includes distance/normal/target work. Existing counters do
not fully separate those subcosts. Do not add these timers to whole-run walltime.

Independent reparameterized G7/K15 reference agrees to1.116e-10relative. GL10whole
39.1993240651 underestimates6.82135%; widest smooth retained interval differs.318579%.
Split/coarsen decisions agree ON THIS overlong edge only. Neither G7/K15difference
nor sampling is a conservative enclosure; ten points are not a safe replacement.
The current396 checkpoint chord differs numerically42.06659984297/4635evaluations;
policy and cut trajectory differ. Do not claim fixed-target speedup from this change.

Saved394 panel trace contains279initial intervals;248have parameter width<1e-10.
Those248consume3720evaluations (15each) of6375total. All retained intervals need not
have the same precision, but these cuts encode ownership/transition information.
Removing them, merging across them, or ignoring their mass is currently unproved.
A future bounded treatment would need a conservative composed-speed majorant for
rounded queries, total error accounting and unchanged inverse behavior. Existing
EdgeGeometry carries cuts/pinned/sensor_only, not such a majorant. Tiny width alone
is not an integration error bound under strong anisotropy.

## Caller contracts and downstream dependencies

| Purpose | Existing caller/path | Required result |
|---|---|---|
| Split/coarsen classification | Engine selection from MeasurePatch/EdgeMeasure | Conservative interval may establish action, but no interval API exists today. |
| Candidate ranking | Engine edge queue; metric size-excess merit | Accurate comparable scalar; cannot insert a threshold witness as length. |
| Balanced insertion | FrozenField::BalancedSplitPoint | Converged whole length, panel inverse proposal, freshly evaluated prefix residual≤relative1e-8. |
| Candidate conformity | ValidateMetricChange/MeasurePatch/MeasureSurfacePlanning | Complete affected patch size excess/max and retained-edge comparisons, not one split classification. |
| Final size gate | Cell/census/final certification and regional field service | Existing complete numerical measurement; unresolved remains failure. |

Two direct boolean-only uses were found in the current source:
`SU2_CFD/include/adaptation/native_bl3d/BLFirstRow7.inl:147` caps a proposed lift
using length<=1.5; `BLState.inl:488` clusters normal sectors using length<=1.
They discard the scalar. The nearby fan query at `BLState.inl:509` needs its
magnitude because ceil(length) sets the fan count. Neither boolean-only check is a
demonstrated current bottleneck; both occur downstream of the unfinished surface
stage. A future action-bound implementation must distinguish them explicitly.

Audit reaches Common/src/adaptation/CNativeField3D.cpp, CNativeMetric3D.cpp,
CNativeEngine3D.cpp, CNativeRegionField3D.cpp, both BL composition factories,
CBoundaryLayerMetric.cpp and CSolver::ComputeMetric. Exact unchanged-coordinate/target
identity campaign/region length caches already exist. Plain P1 intervals and proven
outside-BL edges already have analytic sqrt-affine integration. Fixed-ray continuation
is graded; changing nearest-wall frames are explicitly not a globally graded field.
No unconditional fixed quadrature cap, tolerance relaxation or new config was added.

## Gradation and unresolved attribution

ComputeMetric grades the core/nodal contribution; recomposition applies wall normal,
tangent, feature/intersection and transition demands afterwards. The composed nodal
transport audit is diagnostic and does not spread wall corrections. It is not a
continuous combined-field gradation certificate.

Read-only399 audit on qualified396saved tensors reproduces123186composed directed
violations/max3115659.1964465; core0. Wall→wall52955,wall→nonwall2145,reverse5780,
nonwall→nonwall62306.52945wall→wallbad pairs have eigenratio>1e4 at bothends;
median dominant-axis change.29975degrees,max89.4494. Worstpair64→4184turns30.08degrees.
Nonwall includes symmetry/farfield; eigenaxis is not a certified normal. This localizes
strong frame/normal-profile variation, but does not distinguish required curvature,
feature switches, insufficient combined gradation and geometry-query noise causally.
Distance error alone never bounds normal/tensor error. Preserve h0/growth/thickness.

## Disposition

Current428 surface preparation uses analytic core edge integration plus sided
tangential/strip guidance, with ZERO full composed-field evaluations. Its shared
lengths still drive ordering and whole-patch merit. Therefore the full-BL quadrature
precision hypothesis does not explain the present surface-stage failure.

The connected428 reuse correction is already qualified: the same64work production
prefix retains identical owned/donor/parameter bytes and23commits, while requests
fall3438→1752, metric time11.401→4.093s, stage wall29.159→22.711s and CPU28.778→22.306s.
This removes repeated unchanged context, not precision. Full428 still exits at its
388.8s allocation with647collapses,0splits and an incomplete census. It establishes
neither surface readiness nor complete mesh/BL acceptance. Keep the active
current-cavity/operator investigation; do not start another quadrature cycle from
these historical edge timings.

Historical394 and current428 are not source-identical qualification campaigns:
CBoundaryLayerMetric gained admitted profile-history persistence/replay for396;
the integrator, target edge factory and shared curved-reference files match394.
Retain396 checkpoint qualification and428 integration evidence separately; do not
claim that the old edge timing was reproduced on the saved396 target.

The demonstrated geometry work correction394 is qualified and retained. The general
hypothesis that construction pays excessive scalar precision remains partly unresolved:
no conservative integral enclosure is currently available for the affected composed
callers, and classification alone cannot replace their shared scalar. Do not introduce
an unsafe early result to claim fewer evaluations. Record bounded tiny-panel treatment
as an unproved alternative; remain in ordered target/physical-patch assessment rather
than launch a competing performance implementation or profiling campaign.

R/edge_measurement399_audit.json pins source and reused evidence. No additional MPI
campaign is required for399diagnostic-only frame output/read-only analysis;398 shared
policy/reference MPI and396 checkpoint MPI remain relevant. Any future production
bound/invalidation/publication change needs focused curved/crossing/feature/height/
resource checks and one meaningful MPI milestone before stage integration.
