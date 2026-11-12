# First-height continuation: implementation and qualification

Current connected curved-BL cycle. Read HANDOFF_Codex.md for live handles.441build,
focused codec/constraint qualification (with resumed MPI failure correction), explicit
seed initialization and complete stage0 target fresh-process reconstruction have
passed. Physical BL construction/height progression and factor10 remain unqualified.
Euler293 payloads and original saved CFD/396 target
remain untouched. No construction policy, final gate or budget change.

## Physical request and stage numbering

For configured final h0 and N halving transitions, stage k=0..N requests
h0(k)=2^(N-k)h0. Thus there are N+1 distinct heights, starting2^N h0, and exactly
N halvings. Later events retain h0 but keep their absolute event ordinal. The
existing ADAP_BL_H0_RAMP option already implements this. Saved364run.cfg omits it,
so396/434/437 use the final2e-6 immediately; they are not continuation evidence.

The diagnostic reuses existing x32/N5 staging, not a selected normalized target:

| k | Requested h0 | g | T | Ideal geometric rows reaching T |
|---:|---:|---:|---:|---:|
| 0 | 6.4e-05 | 1.2 | .01 | 20 |
| 1 | 3.2e-05 | 1.2 | .01 | 23 |
| 2 | 1.6e-05 | 1.2 | .01 | 27 |
| 3 | 8e-06 | 1.2 | .01 | 31 |
| 4 | 4e-06 | 1.2 | .01 | 35 |
| 5 | 2e-06 | 1.2 | .01 | 38 |

The last ideal row may intersect T; the table neither changes T/g nor certifies
constructed coverage. Actual curved/oblique NextLayer steps must be measured.
Its unit-metric stepping is not an algebraic guarantee of physical geometric
progression. At each stage the existing continuous normal law uses that stage's
h0 with unchanged g/T; inherited tangent scaling/core are held fixed only in the
causal diagnostic. Production normalization must assess its full composed field
again for a changed stage, preserving the prescribed normal law and nominal500k.
Two FINAL-height adaptations need at least N+2 remesh events; intermediate stages
do not count as completion of the two-event RANS objective.

## Confirmed gaps and implemented correction

Existing stage selection reaches metric composition, normalization, wall_heights,
region_walls, first-row/growth/repair and independent height checks through active
CConfig controls. final_cycle is report-only, not an acceptance exemption. Original
row sidecars retained topology but no stage; fresh config/driver restarted at0.
Original complete-target codec retained effective heights but no N/k, so exact
resume was unavailable (factor1 alone cannot distinguish later events).

The existing .native_rows payload is now wrapped with NEXT absolute event,
N/schedule/true-request signature, final saved mesh byte length/fingerprint and a
checksum. RowRegistry's coordinate/connectivity digest remains independent: later
quality changes may invalidate topology reuse without invalidating recorded event
progress. Load publishes state only after successful decoding/binding; writer uses
a temporary file and rename; seed initialization refuses existing progress. Metadata
promotes with pending rows only on successful replacement, not private trials.

Driver checks rank agreement/errors before reference preparation, restores active
height before dependent import/metric work, and resumes absolute cycle numbering.
Only FUTURE mesh filenames are checked against the input; a past checkpoint with
the same prefix is not overwritten. Terminal next_event==nCycles runs its continued
CFD solve with true height; it does not repeat adaptation. Resumed flow iteration,
residual and configured CFL settings come from the recorded schedule.

Complete target format v2 stores CURRENT event (not next event), N and signature,
in addition to the original live policy/reference/core data. Strict read checks
exact event. Explicit ReadRestoringStage/DecodeRestoringStage validate metadata,
restore k and roll back configuration on failure. Existing v1396 reads unchanged
with rampOFF; rampON refuses its ambiguous stage. This does not turn a sensor-only
export into a complete target checkpoint.

## Explicit saved-seed initialization and stage checkpoints

A ramp-enabled CFD restart without a stage envelope is ambiguous. Never infer its
stage from case names, Euler/RANS labels or mesh appearance. After verifying the
saved CFD/mesh/reference provenance, create an absent stage0 envelope explicitly
with WriteInitialContinuation(path,meshPath,MakeRampStage(config,0),config).
The helper fingerprints the supplied mesh, checks matching event0 controls and
refuses to overwrite an existing sidecar. It does not independently qualify CFD,
geometry or target suitability; those verified inputs remain part of the checkpoint.

A usable stage bundle includes matching configuration, native_ref/associations,
current mesh+transported nodal core/owned data, stage envelope, full composed-target
policy/family where used, CFD fields and source/build/helper/input receipts. The
row sidecar alone is not that bundle. Provisional output must be labelled and
geometrically valid with stage height enforced; final BL acceptance still requires
original h0/progression/coverage and every unchanged whole-mesh gate.

## Bounded actual comparison and limits

438d comparison uses complete434 current cavities/associations/nodal core and396
policy, reconstructs each stage through existing metric APIs, holds core/tangent
scaling and candidate coordinates/connectivity fixed, and checks finalk=N tensors
against the original final-height target. Reports physical height admission, full
centroid q/J and tangential planning edge distributions for complete affected
patches. It does not generate new first-row candidates or layers. Exact positivity
of fixed cells cannot change merely because h0 changes.

The same difficult6032–6072 edge gets <=20s per height inside the original aggregate
300s atlas deadline/work ceiling, with independent per-edge uncertainty fallback.
Report geometric preparation, nested composition/integration, evaluations/work and
CPU/wall separately. An unresolved length remains unknown. Newly composed fields
are not newly normalized500k targets; their physical allocation and estimated
complexity require the subsequent stage2 assessment. No speedup claim is established.

Qualification pending: focused serial behavior, ONE appropriate MPI milestone and
actual M6 comparison. Metadata MPI agreement is not distributed BL generation or
new cavity-publication qualification. RunEvent still has its existing native1
construction guard. The full strict BL/RANS/transfer/continued-CFD/restart goal remains.

## Actual440 bounded result

TerminalPASS284.555s, pins/actual handles verified;301171Owned records byte-identical
434.461records and all60old/fresh patch-height evaluations measured. Finalk5 centroid
tensors are BITIDENTICAL to396 policy on these points (not merely same statistics).
Maximum tangential planning edge relative change1.5062346e-6 across the six heights;
no sampled0.5/0.9/1.6/1.8classification change. No substantial tangential-demand change
is demonstrated. All60strict height checks fail: no layer was constructed by this
fixed-coordinate diagnostic. q/Jbad counts remain largely unchanged as h0 varies.
Do not interpret parameter progression as achieved physical layer progression.

| k | h0 | Exact edge result6032–6072 | Edge wall s | Geometry prep s (nested) | Integrand compositions | Stage atlas work |
|---:|---:|---|---:|---:|---:|---:|
|0|6.4e-5|0.9943139649742233|10.3465|9.9874|45|329646|
|1|3.2e-5|unknown,20s window|20.0002|20.0001|0|872346|
|2|1.6e-5|unknown,20s window|20.0002|20.0001|0|701527|
|3|8e-6|unknown,20s window|20.0001|20.0000|0|721711|
|4|4e-6|unknown,20s window|20.0002|20.0001|0|757679|
|5|2e-6|unknown,20s window|20.0002|20.0001|0|718150|

Stage work also includes point target evaluations; it is not edge-only work. Geometry
preparation is nested in edge/integral timers, never additive. Coarse stage0 additionally
spends.359s in composed evaluation; all finer windows expire before integrand work.
Profile construction.70–1.12s/stage is separate. This demonstrates first-height influence
on preparation work, not complete cost at finer stages or an end-to-end speedup.
Projection replay identifies many certified distance calls during chord preparation,
not a cheap Mach Hessian or numerical edge integrand as the measured20s blocker.

Useful cavity alternatives remain separate: original visibility failures are exact
fixed-coordinate positivity obstructions, unaffected by a scalar target change;
New height-driven placement behavior was not tested here. Leading-edge geometry-guidance
refusals also do not become demonstrated outside witnesses. A larger h0 alone has
not supplied suitable wall cells. Next evaluate the normalized proposed stage0
field from the SAME saved CFD/authority, initialize explicit stage0 provenance, and
return to surface preparation/first row in order. N5 is justified as a bounded next
proposal, not final-height feasibility or a reason to relax any physical request.

The report exposed an existing Json(string)=std::quoted bug: C0 characters in replay
exceptions produce invalid JSON. Raw440 remains preserved; a normalized copy retains
identical values.441 fixes the existing serializer using the same escaping convention
as native failure capture, adds a focused regression and exposes explicit verified-seed
stage initialization through a hidden stage helper. No query/admission decision changes.

## Latest user question: tenfold rather than halving

User asks whether factor10 would reduce elapsed development/workflow time. No
production factor change is authorized by evidence yet. Keep the same initial and
final height when testing:32→16→8→4→2→1 versus32→3.2→1, final step capped. A100→10→1
sequence changes the initial target too and is a separate comparison. Stage0 at32h0
is the common initial field and remains useful while this choice is evaluated.

Read-only actual path audit: dc17::rebuildFirstRow defaultsTRUE in BLRebuild17.inl:28;
RebuildFirstRow is production, HalveFirstRow→ColumnInsert is the test-only comparison
branch. Rebuild/fallback and HalveFace use the current requested h and old altitude>
h*(1+tol), not an exact1/2 assumption (BLRebuild17:180–198,248–275; BLVolume13:37–59).
Shared lids satisfy ColumnLidFraction>=1.5*incident h; lower prism/upper slab rebuilding
is temporary coordination, not permanent prism topology. These facilities express an
arbitrary height reduction, but tenfold feasibility/coverage/quality is unqualified.
ColumnInsert computes configured geometric-law positions up to.85existing lid and
retains the outer stack (BLVolume13:126–145). That stack need not match the new profile.
CompleteFirstRow gates positivity and first height, NOT full g/T progression/coverage.
NextLayer consulting g/T is not a proof of successful shrink-band reconstruction.

After credible surface/initial-row output, compare actual cumulative work and first-
height errors, normal layer coordinates, progression, coverage/gaps, old-layer
preservation, complete-patch q/J/geometry/size and outer transition on both schedules.
Fewer outer events alone is insufficient. Do not add a new scheduler/configuration
or silently change ADAP_BL_H0_RAMP's existing power-of-two semantics before a justified
correction. Preserve every stage/final physical request and all final gates.

Saved actual inputs for the later bounded test: checkpoint_front408_smooth_context.dat
(smooth_upper), used by416; first row5/5columns, geometry/heights pass,25q/Jbad and no
accepted mesh. LE context checkpoint_front416_le_context.dat:416zero fresh cells/
82cell cap,419three strict whole-patch geometry refusals. LE needs credible surface
preparation before a shrink comparison; these old outputs are not accepted BLs.

## Stage0 normalization producer441 (reload pending)

Separate seed-stage envelope initialized successfully5.661s from original364CFD/ref/
mesh bytes; JSON-export regression1case3assertionsPASS4.845s. R/ramp5_seed_stage441.json
binds the derived configuration. Original files remain unchanged. This is metadata,
not generated BL. Driver stage0 currenth=6.4e-5 with final physical2e-6,g1.2,T.01.

Actual producer terminalPASS624.400s, pins/all9actualhandles terminal.61330source
points/303359tets, no generated mesh. Nominal500000, estimated486914.7222 with empirical
indicator41525.0034 (NOT bound), bracketed1. Selected core.0054996988679568166 and inherited
tangent.37775074746138892 versus original396.004607250072/.25. Normal profile is NOT
scaled globally to hit complexity. A coarser normal request permits different adjustable
flow/tangent allocation; do not reuse396's selected core for this target. Full checkpoint
payloads were written; independent fresh-process reload is currently frozen/running.

Metric total max529.708s, construction462.297s, Hessian~.5s/setup66.9s; original396 total
531.651s/construction480.951s/setup50.305s. Different targets/contention and nested work
prevent a simple speedup claim. Producer checkpoint chord9.6480553402102753 differs
physically from39642.0666. Only matched new-field reload can test checkpoint agreement.
Per-rank metric peakRSS~.56–.62GB; sampled later individual.852GB not whole-job peak.
10.67GB geometry reservation is conservative admitted workspace, not resident memory.

Fresh441reload terminalPASS435.543s:61330/61330owned tensors and production chordBITIDENTICAL; R/target_checkpoint441_qualification.json. Complete303788608B/12payload checkpoint retained. Nodal core export companion verified, not fulltarget substitute.42original-coordinate directional samples have normaldensity~15625 as stageh0prescribes; normalizedtangent densities11–23%higher than396, with local curvature proxies still noncertifying. Core median-dual densityallocation52.066% beyondr20 is CORE-only, not the composed-volume fraction. Physical target/constructed BL suitability remains unqualified; next bounded surface stage uses the NEW complete policy/sourcevalues.
