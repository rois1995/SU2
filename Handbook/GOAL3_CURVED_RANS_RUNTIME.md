# Goal3 — actual curved RANS/native BL lifecycle

Branch `codex/native-3d-bl`; full Goal3 ACTIVE/incomplete.
Prior qualified source-refresh baseline9bd5fb76e4 remains separately scoped.
This lifecycle matrix qualifies the907646e00e constructor. The subsequent
[metric-directed front/core change](GOAL3_METRIC_REGION_CORE.md) needs its own
updated workflow campaign; do not promote these old-source receipts to it.

## Recorded outcome at907646e00e

The complete actual short RANS/SA lifecycle passes all six N/M combinations:
1/1,2/1,2/2,4/1,4/2,4/4, each with independent saved-data audit and SA projection replay.
All construct1368 BL cells from24 coarse tetrahedra; released meshes contain
68/70/74/70/72/72 cells respectively. Actual dual-time CFD, pressure-derived metric,
conservative replacement, SU2/CGNS/reference reload, actual restart and further CFD
execute. Sensor return residual0. The pressure sensor clips to the coarse bound;
no stronger-flow-feature or arbitrary-field claim. All owned jobs are terminal.

`rans_lifecycle_summary.json` pins the six receipts, twelve independently audited
frames and six reference/clipping replays. Maximum physical flow conservation error
2.53e-15, SA correction-accounting error<3e-15, maximum independent SA nodal replay
error4.22e-15 of field scale, and exact flow/SA restart. Initial BL complexity differs
only48.71302686078762..48.71302686078766 across ranks. Native shape/full-edge gates
remain enforced, but these actual-flow outputs do not yet have independently
re-integrated full composed edges. Section clocks are retained only as diagnostics.

The **v5 workflow receipt remains FAIL** on its four all-field conservation assertions.
Independent saved-mesh audit passes both complete exact embeddings, wall height/feet,
curved authority, physical P1 flow/open ledger and exact flow/SA restart. Height error
5.14e-15, flow residual<=2.53e-15; no independent composed-edge quadrature claim.
Its auditor initially failed only while writing a relative helper path; corrected
report and exact previously executed helper are preserved. No CFD rerun for that fix.

## SA diagnosis and unchanged production contract

Saved-state replay v1 passes17810 assertions in.418s, without CFD. The existing serial
reference projector preserves the raw SA target to<=1.91e-15. Independently applying
`max(1e-16,raw)` reproduces every saved target SA value<=4.22e-15 of field scale, with
exactly200 clips on construction and56 on removal. Python independently checks all
raw/bounded/saved CSV rows and rational-volume totals. Integral changes (~0.037% and
~0.369%) are fully explained by the existing solver-bounds step, not roundoff or a
projection conservation defect. No recovery patches occur in these two transfers.

The new lifecycle helper incorrectly demanded zero change after the documented
solver clipping. It now enforces exact flow totals and explicitly accounts for the
reported turbulence correction for each level; zero/actual field scales replace
max(1,scale), strengthening residual normalization. It also checks correction counts
and exports the correction ledger. Production transfer/mesh gates are unchanged.
The earlier attempted edit was rejected by automatic review for lacking independent
proof; the accepted correction follows the saved-state nodal/integral proof above.
Do not rewrite v5 as PASS, or claim strictly conserved post-clipping SA integrals.

Build10146 and every sequential case/audit/replay wrapper are terminal PASS.
Cases use `native_curved_rans_nN_mM_v6`; corresponding saved-state replay casesv2..v7
follow the six combinations above. Original failed v5 and diagnosticv1 remain intact.
The exact executed Python helpers are retained beside their prior evidence. Current
replay takes saved-case and new-diagnostic-case arguments and verifies unchanged
production objects. Passing case reuse requires exact source/build/binary/config/
input/helper provenance; current quadrature scoped reuse is separately pinned in
`annular_reuse_after_SA_test_correction.json`. No broad regression was repeated for
this test-only correction; the full actual lifecycle MPI matrix is the integration
campaign. Production behavior is unchanged from the accepted-domain checkpoint.

## Connected implementation

Positive cylindrical-annulus complexity integration passes v7:3 selected families,
877746 assertions, all8 sectors/wall-first permutations, independent prism integral,
sphere and rotated/sheared polynomial moments. Prism native6.32953372249 vs independent
6.32953579782,13635 retained points over3 tetrahedra. Actual RANS complexity also
completes; the irreducible BL floor48.7130268608 exceeds requested complexity1.
This is reported rather than changing the field or wall-height constraints.

The preceding actual v4 run successfully constructed1368 cells, but unconditional
legacy coarsening then exhausted a dependency-star bound. Complete region construction
already resamples surface and volume bidirectionally. EngineResult now explicitly
marks a valid changed complete domain rebuild. The backend only schedules incremental
coarsening for other results, with the existing shared work/deadline/return/transfer
gates. No fallback after failed coarsening or bypass of final mesh validation.
Ordinary incremental path/work-limit/time-limit/atomicity control passes18 assertions
in `native_complete_domain_controls_v1`. v5 exercises the changed complete path.

Quadrature source, executed arithmetic/geometry objects and volume-test object remain
byte-identical across this backend change. Scoped v7 reuse is verified in
`annular_controls_reuse_after_domain_api.json`; it does not claim that v7 reran with
the new whole binary. No repeat volume campaign is needed for this unrelated flag.

## Unchanged acceptance and limits

- Geometry authority, first height.05/growth1.2/thickness.2/fade, Hausdorff.04,
  wall surface ceiling.025, q>=.20/J>=.05/full composed edge<=1.8.
-4096 charged construction work/300s per event; v5 construction3028/removal168 work.
- Sensor-only original P1 donors; geometric BL at every actual query, finer requests
  preserved, no hard-normal reset and Hessian noise0.
- No accepted mesh until complete reconstruction and all native gates pass; no local
  fallback chain or budget ladder in place of a construction strategy.
- Serial and MPI2/4 N=M/M<N now pass this declared workflow; wider construction and fields remain open.
- One visible radial wall/planar caps region constructed on one worker. General
  exterior/multiwall/high-ratio multiple fronts and scalable region generation remain open.

The advance clock includes Postprocess automatic metric calls under
COMPUTE_METRIC=YES/ADAP_LOOP=NO. Its solve_seconds is not CFD-only; no affordability
ratio or performance conclusion is justified. No converged viscous force/wall-shear
or M6 accuracy claim. Existing broader Goal3 gates remain in the requirement audit.

## Preserved failures and provenance

The collector retains source snapshots/build/binary/thin-object/config/input/helper
pins and actual grids/solutions/restarts. Do not rewrite prior failed receipts.
- Initial invalid HMIN.1/height.05 request; corrected to HMIN.025.
- RANS v1 generic complexity point-budget exhaustion; rejected axis-fan experiment.
- Volume v2 duplicate/unrepresentable angular partition; v3 incorrect unit-axial oracle.
- One-prism v4/v5 controls pass, but actual rotated-cell RANS v2/v3 fail.
- Exact house Polygon::Clip self-clipping changes5/8 rotated projected triangles;
  source/probe/output preserved in RANS v3/projection_diagnosis.json.
- Expanded v6 sector test exposes subtractive chord cancellation; v7 positive
  projective rays/barycentrics/vertex heights solve it without merging/clamping.
- RANS v4 redundant legacy coarsening failure; v5 accepted meshes, failed SA assertions.

Positive projective charts use exact shared-base triangle intersections, strict
compensated products, exposed geometric radial bands, positive retained weights and
original-source queries. Bounds remain65536 points/16384 work,1e-4 density error and
1e-10/2e-10 volume error/closure. General interacting/outside-slab integration has its
previous explicit limits. Full evidence index: `region_construction_v1/annular_integration_progress.json`.


## Next construction limitation: one kernel cannot cover stronger sensor demand

A source-linked exact norm certificate is preserved in
`region_construction_v1/single_kernel_metric_span_certificate.json`. For the same
unit axial span, a constant original-P1 sensor diag(1,1,16) gives metric distance>=4
between any vertices on opposite end-cap planes. Any single kernel has distance>=2 to at least one cap,
exceeding the unchanged1.8 edge limit. BL composition preserves this finer demand
and cannot reduce that lower bound. Current CNativeRegion3D closes the final core
by joining every boundary face to one kernel. More wall variants or moving that
kernel cannot solve this limitation. This does not prove global mesh infeasibility.

After the current actual lifecycle MPI qualification, the next connected construction
step must support a metric-driven core with several interior vertices/regions and
shared interfaces, while retaining joint wall/transition construction, actual queries,
private validation and atomic publication. Use a complete accepted curved mesh with
this stronger frozen sensor as the difficult target, alongside a distinct varying
field. Do not substitute another local repair ladder or raise the current limits.
This certificate is a static diagnosis, not an executed stronger-field adaptation
or a qualified implementation of the next strategy.
