# MPI audit request and Loseille paper assessment — 2026-10-26

This assesses the user-supplied audit specification and paper against implementation
`e9263060cf` and validation54. It is not completion of the proposed seven-phase audit,
a new benchmark, or evidence that DC-17/DC-18 are closed.

## Decision

The audit asks the right questions, particularly separating useful computation from
MPI waiting and testing workload predictors before changing partitioning. Correct
its architectural assumptions and reuse existing instrumentation. The paper supplies
useful construction and parallel scheduling hypotheses, not a demonstrated fix for
the current strict full-M6 acceptance failure.

Prioritize two connected diagnoses: whether actual failed M6 regions can be repaired
under the unchanged contracts, and how much synchronization those repairs impose.
Do not replace this with a broad partitioning study that never resolves construction.

## Inputs and prior awareness

User files, read without modification:

- `MESH_ADAPTATION_MPI_AUDIT_AGENT_PROMPT.zip`, containing one 35,052-byte Markdown
  specification; SHA256 `c2e7e9cb847ff5533c7004df5632ee4f419c0990acae9d0fbd646851f70fa97f`.
- Loseille, Alauzet and Menier, *Unique cavity-based operator and hierarchical domain
  partitioning for fast parallel generation of anisotropic meshes*, Computer-Aided
  Design 85 (2017), 53–67, [DOI](https://doi.org/10.1016/j.cad.2016.09.008).
  Supplied PDF SHA256 `560d55d5cbe14e41da49cc21d14d6f00e68ee0abc190266fded0f21bb4b0c63a`.
  Assessment uses the local PDF; online DOI retrieval was unavailable.

The paper was already consulted: the October 10
[reading record](../integration_evidence/native_3d_core_v1/papers_reference.json)
records sections 2.2–2.6 and the opening of section 3, and the
[design notes](../integration_evidence/native_3d_core_v1/DESIGN_REFERENCES.md)
discuss cavity growth, coupled surface/volume changes and work-proportional fronts.
[Construction reassessment](GOAL3_CONSTRUCTION_REASSESSMENT.md) also cites it.
The earlier PDF has a different file hash; this is the same bibliographic paper,
not a claim of byte-identical copies. The detailed workload formula, hierarchical
interface strategy and published quality distributions are assessed here explicitly.

## Corrections to the audit specification

| Request or premise | Verified current state and assessment |
|---|---|
| Complete regeneration | The current native path incrementally modifies a private imported mesh through refinement, coarsening and cavity reconstruction. It can change topology extensively; it is not a fresh, topology-independent generator. Evaluate predictors accordingly. |
| Separate CFD/adaptation partitions might be needed | This already exists. `CNativeRemesher3D.cpp:321–388` optionally computes working weights, repartitions cells and migrates original donors separately. Validation54 M6 enabled `ADAP_NATIVE_REPARTITION=YES`. |
| Weighted ParMETIS might be needed | Native `CNativePartition3D.cpp:94` builds a tetrahedron face-dual graph and calls PartKway with two constraints: predicted work and cell count, both at 1.15 balance tolerance. Migration is implemented. It differs from CFD's node graph, whose weights are point cost plus adjacency count in `CPhysicalGeometry.cpp:7420–7463`. |
| Metric complexity as a new weight | `WorkWeights`, `CNativePartition3D.cpp:15–45`, already combines a baseline, sensor metric volume, target long/short edge pressure and shape deficits; weights are positive integers capped at 1024. This is an uncalibrated proxy, not a proven cost predictor or the paper's formula. Compare against it, including saturation effects. |
| MPI decomposition | Worker communicator selection already exists. Splitting communicators does not itself migrate data. Current distributed cavities import dependencies and can cross ownership boundaries; they do not permanently freeze partition interfaces. |
| Add phase/counter instrumentation | Reuse `EngineStats`, `PartitionStats`, native per-rank phase logs, CPU scopes and perf evidence first. Remaining gaps include representative compute/wait attribution, spatial operation costs, predictor calibration and accepted full-cycle cost. |
| BL extrusion as one parallel stage | Trace the actual requested-wall-height construction and distinct FIRST_ROW path. Some paths remain serial/guarded. The Euler54 failure does not measure a completed BL construction or transfer. Avoid solver-label production dispatch. |
| Benchmark many rank counts | Use at most six ranks and one owned heavy job. Begin with frozen M6 input plus uniform and rotating-anisotropy controls; run ranks 1/2/6 where feasible, with matched acceptance and resource conditions. |

Retain the audit's evidence labels, no extra barriers in production, held-out
prediction tests, migration accounting, diagnostics-overhead checks and explicit
missing values. Its seven reports and CSV schemas describe a substantial research
campaign, not a prerequisite to every fix. Start with one linked decision artifact
and existing raw receipts; expand when a specific decision needs more measurement.
The supplied prompt is assessed here, not silently adopted as authorization to
implement every instrumentation or architecture proposal it contains.

## What the paper contributes

### 1. A more informative work estimate (section 3.1)

For each existing tetrahedron K, construct its natural metric M_K, in which its
edges have unit length, and compare it with target metric M. Let d_K, d_M and d_I
be sqrt(det) densities of M_K, M and their metric intersection, respectively.
The proposed model is

`W(K) = |K| [alpha (d_I - d_K) + beta (d_I - d_M) + gamma d_M]`.

The terms estimate insertion, collapse and optimization work. Equal determinant
and equal anisotropy ratios do not imply low work: rotated metric directions can
require both insertion and collapse. This makes the formula a useful additional
candidate beside the audit's target-complexity predictor and our current proxy.

Table 1 reports partition times of 4.6–5.1 seconds with their anisotropic work
estimate versus 0.9–9.6 seconds with target-density weighting on the eight-part
blast example. These are their results, not our measured speedup. Their equal
operator-cost coefficients are specific to their cavity kernel; ours include
variable geometry, donor, rejection and certification work. Fit or reject the
model against measured regional costs, with surface/BL contributions separate.
Their log-Euclidean averaging is an estimator choice, not a reason to change our
authoritative frozen P1 field or actual composed-field validation.

### 2. Coarse-grained remeshing and interface recovery (sections 3.2–3.5)

They perform many independent local operations with temporary fixed partition
interfaces, then gather the unfinished interface regions into subsequent levels.
Hilbert ordering is used initially; BFS with restart is used for later interface
meshes, with connected-component corrections. Repeating the initial partitioning
alone does not expose the old interfaces adequately.

Crucially, the next interface region includes the cavities needed for predicted
collapse/insertion operations, not merely a one-cell halo. This speaks directly
to whether an artificial local boundary prevents a necessary replacement.

Our current fine-grained synchronized transactions are different. Batching more
independent local work could reduce collective frequency, but adopting the full
paper strategy requires complete region ownership, geometry/source dependencies,
interface scheduling, consistent publication and eventual recovery of every
temporary constraint. A weight change or communicator split does not provide it.
Our unchanged final acceptance gates remain mandatory.

### 3. Coupled cavity construction (sections 2.3–2.6)

Their unified cavity formulation can express a sequence of elementary operations
as one connected replacement and grow through obstructing faces. Our code already
has cavity insertion, obstruction-driven growth and joint surface/volume repair
(`CNativeEngine3D.cpp:2563–2756`). Current limits include 12 growth iterations,
64 old cells and 128 replacement cells. The missing question is whether these
particular regions and constructions can resolve the recorded difficult target.
Neither adding another local fallback nor raising the caps answers that question.

Algorithm 1 addresses positive reconnection and explicitly contains a surface-point
rejection branch. It is not a guarantee of arbitrary curved BL reconstruction,
global embedding or q >= .20. Keep complete-interface, contact, source, physical
feature and height checks.

### 4. Construction and quality optimization are distinct

Sections 2.2–2.3 first adjust metric lengths, then improve quality with swaps.
Our production runtime explicitly selects `ShapeAdmission::Absolute`
(`CNativeRemesher3D.cpp:452–454`), so fresh metric changes must already meet the
shape thresholds. This may restrict paths through temporarily lower-quality but
valid intermediate configurations; it is a hypothesis to test on actual failed
regions, not an established cause of the M6 failure.

Validity-only machinery already exists, and earlier experiments did not establish
reliable completion: validation49's cubic global repair still failed. Therefore
simply toggling the policy back is not a complete solution. Any staged design
must deliver a fully accepted final mesh, with bounded cost and rollback on failure.

### 5. Published speed does not establish our acceptance contract

Their Q uses 1 as ideal and increases with worse shape; our q increases toward 1
with better shape. Do not compare their numerical thresholds directly. Table 6
contains tetrahedra with Q above 1000 and edges longer than 5 metric units; it does
not establish our hard q/J/edge gates. Nor does the paper demonstrate our first
height tolerance, absolute curved-wall tolerance independent of h0, conservative
BDF transfer or restart workflow. Their experiments use discrete surface support.
Out-of-core storage is useful at their billion-cell scale, but current M6 evidence
does not identify RAM capacity as the active failure.

## What current measurements actually establish

Source: validation54 [analysis](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/validation54/m6_euler/refine_analysis.json)
and [status](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/validation54/m6_euler/status.json).

| Rank | Attempted proposals | Accepted private edits | Shape refusals | Optimizer calls / validated |
|---|---:|---:|---:|---:|
| 0 | 5,191 | 5,062 | 401 | 131 / 2 |
| 1 | 5,186 | 5,129 | 120 | 58 / 1 |
| 2 | 5,186 | 4,246 | 10,952 | 331 / 16 |
| 3 | 5,183 | 5,154 | 213 | 17 / 0 |
| 4 | 5,192 | 2,773 | 31,981 | 790 / 52 |
| 5 | 5,195 | 2,608 | 33,072 | 786 / 44 |

Shape refusals count candidate evaluations, not unique cells or failed top-level
proposals. Ranks 4–5 account for 84.8% of these refusals. This establishes unequal
repair demand/success, not a quantified useful-compute imbalance or its geometric
cause. Recorded reconstruction CPU scopes are approximately 330–344 seconds across
ranks; do not infer proportional CPU cost from rejection counts.

The event stopped at the 1800-second native deadline, with 24,972 private edits,
235,717 cached failing cells and no accepted mesh/transfer. Global work was only
199,331 of 19,414,976. The five-second, 491-sample CPU profile puts about 74% of
inclusive samples in MPI_Allreduce. That short sample is not 74% of full-event
wall time, does not separate polling/waiting from data transfer, and gives no
valid 3.8x speedup prediction. Whole-process CPU also includes MPI busy polling.
Nested timer fields must not be added as independent phase times.

## Next decisions and acceptance

1. Reuse the exact saved M6 metric with `NativeFrozenVolume3D`. Verify coordinates
   and global IDs before conversion. Capture bounded rejected regions with existing
   callbacks, including geometry/source bindings and rejection reasons. Distinguish
   fixed-interface obstruction, shape-policy restriction, unsupported construction
   and genuine invalidity. A solved witness is diagnostic, not full-M6 acceptance.
2. On the same workload, measure local construction/certification separately from
   collective entry/exit and remote dependency waits. Associate cost with regions,
   not just six rank totals. Reuse current logging; add only missing diagnostics.
3. Compare uniform, current WorkWeights and paper-style mismatch estimates offline.
   Include equal-density rotated anisotropy and surface/BL demands, held-out data,
   weight clipping, partition cut and migration/rebuild cost. Do not claim an
   offline balance improvement as a measured remesher speedup.
4. Select a connected construction or scheduling change from that evidence. If
   local repairs work but coordination dominates, assess independent regional
   batches and interface recovery. If the serial region cannot close, repair its
   construction first; redistribution alone cannot create an admissible topology.

No C++ or runtime policy changed for this assessment. No new timing campaign was
run. The full audit's predictive calibration, scaling and net-benefit measurements
remain open, as do complete full-M6 Euler/RANS acceptance and DC-17/DC-18.
