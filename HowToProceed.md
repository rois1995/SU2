# How to proceed

Updated2026-11-09. TWO strictly accepted Euler50k meshes, conservative admissible
transfers, finite continued adaptedCFD and actual isolated adapted disk restart
are VERIFIED in293. Actual meshes149051points/736323tets and172001points/870422tets;
allq/J/geometry/size failures zero/known. Target50k is metric complexity, not counts.
CFD continuation was one iteration per adapted mesh; no converged-physics claim.

[HANDOFF_Codex.md](HANDOFF_Codex.md) is current source/build/input/handle authority.
Both actual mesh/reference/matching-CFDrestart checkpoints are independently copied
under staged57/m6_inputs/euler_adapted293_event{1,2}_v1; metadata/hashes pushed, large
actual assets retained locally. Isolated restart on event1 passed MPI6 with same
qualified standaloneCFD, unchanged inputs/pins, exact coordinates and finite/admissible
changed output. All293 fullworkflow/restart owned handles are terminal. The active boundary-reuse
qualification job and source freeze are recorded in HANDOFF_Codex.md.

FIRST296 is now TERMINAL ACCEPTED and independently preserved:149051points/
736323tets, allknown0q/J/G/size, conservative admissible transfer and finite continued
CFD. Mesh/reference/restart are byte-identical293event1 and independently copied to
staged57/m6_inputs/euler_adapted296_event1_v1. All11actualHOST handles absent; all1520
pins unchanged. Actual firstAdapt3689.76versus2933819.85s (-3.4%), with identical
expensive geometrywork and outputs: no causal material performance gain established.
Qualified296source/binaries archived locally before BL edits;293two-event/restart
proof and BOTH checkpoints remain primary regression baseline.

Current next cycle is the [connected curved BL milestone](Handbook/ADAPTATION_CURVED_BL_INTEGRATION_20261109.md).
Latest user: no further Euler optimization cycle for small timings, reuse counters
or speculation. Missing capability/evidence/acceptance are stated there before
implementation. Combined flow/BL target plus explicit wall-aware construction and
constrained repair; audit/reuse first-row/joint-wall/metric/height facilities without
assuming complete multilayer extrusion. Address original no-CAD cubic distance/
normal, composition, quadrature and chord integration together. Do not simply remove
the guard or replace curved walls with planar ones. Measure BL suitability beyond
q/J and after split/coarsen/move/reconnect/migration. Temporary coordinated layers
allowed; no permanent frozen columns/coords/prisms or Euler/RANS-label dispatch.

Focused checks plus ONE connected MPI milestone, promptly actual M6 BL, then saved
reasonable RANS500k two-event flow+turbulence transfer/continuedCFD/actual adapted
diskrestart. Existing120iteration M6SA state is only an initial-guess candidate;
qualify/continue it rather than blindly restart. Connected curved-BL implementation is active but unqualified; current frozen
build/job and preserved failures are in HANDOFF_Codex.md and the linked BL design. Full goal ACTIVE; Euler acceptance is not BL completion.

The remaining notes are historical evidence; the active next action is above.

Prior287certificate change is QUALIFIED (retained in289): preserve all original early vertex
coverage guards; replace general segment clipping in the planar boundary-interior
proof with exact separating axes, reusing existing face orientation. No new geometry
representation/config/scheduler or skipped final certificate. Earlier285/286point-
winding removals regressed reverse work and are REJECTED, not layered into287.

Same actual283query bounds/order remain exact; common work falls only0.59%forward/
0.95%reverse. BOTH20Mqueries still incomplete. Do not claim substantial closure or
whole-run speedup. Focused12cases/7281assertions; serial90cases/67446assertions; ONE
MPI6 milestone124cases EVERYrank passed. Source/binary/runtime/input pins unchanged,
allowned handles terminal. Qualification explicitly is NOTmeshacceptance.

Full287 is TERMINAL REFUSED, notlive. All11ownedidentities areterminal, all1520pins
unchanged, all30rawfiles complete736323cells/metrics/noomissions. Exactlyoneverified
ownedMPIstop after explicitrefusal/capture; NEVERrepeat stophelpers. Nativeexit
5346.697s hasallknown0q/J/LOCALgeometry/size, but mandatory globalBoth remains
incomplete. Noacceptedmesh/transfer/adaptedCFD/restart. Fullcertainty is unchanged.

Full289 is now TERMINAL REFUSED at736323tets,allknown0q/J/localG/size.
Originalfailure: atlasstorage13482375068requested vs12884901888ceiling, caller5.381GB/
scratch8.101GB; wrapper also reports timebudget. All30rawfiles complete/all1520pins
unchanged, exactlyone verifiedownedstop, all11actualhandles absent. All24nonmanifest
rawpayloads SHA-identical287. PrimitiveCPUgain enabled3.64x traversalwork in similar
finalwindow; still NO globalcertificate/acceptedmesh/transfer/adaptedCFD/restart.
Preparedresidentcache remainedbounded; hostRAM/PSI showed no actualpressure.

NextONEconnected candidate: reuse fresh rigorous per-face mesh→model bounds from
existing entity ParameterError path, retain whole model→mesh proof, fullBoth fallback
for uncertain localbounds. Accepted-face association/orientation cert is NOTdistance
proof; localG0 alone is insufficient. Same exactsource/support/body/sheet/marker,
currentcoordinates/bindings and resourceadmission required. Factored existing
logic into EntityFaceError; candidate290/291 built and affected qualification passed.
Fresh captured-face replay is explicitly partial:17776/69544 at unchanged20Mwork,
0overTol; no wholefield proof. Serial91cases/67534assertions and ONE MPI6 125cases EVERYrank passed; fullsame-seed
TWOeventM6 immediately. No widercleanup/targets/norms/schedulers/budgets/gates.

Counts/operations matched283 ONLYthroughround24000,firstdiverged24064; latermatching
rounds aredifferentworkloads. Initial61330coords/22fields matchqualifiedMach-only
L1/50k seed exactly. Do not attribute walltimedifferences solelytocode or useold283
physicalquery ascurrent287geometry. Native5400/outer14400/allgates unchanged.

283refused final globalBoth proof after localclosure of736019cells,allknown0q/J/
localgeometry/size. Final~720s/rank dominated traversal, not ongoing localpolishing.
All30rawfiles complete; all1520pins sealed before exactlyone ownedMPIstop; all11old
identities terminal. NEVER repeat historical stop helpers. Noacceptedmesh/transfer/
adaptedCFD/restart. Complete283rawgeometry query supports same-input diagnostics.

[Cleanup investigation](Handbook/ADAPTATION_POST_REFINEMENT_CLEANUP_20261108.md):
original clean-star selection/admission gap confirmed; fouractual262clean contexts
prove localpointremovals with strictchecks and better directional/shortedge measures.
Other directions fail positivity/support/geometry. Integrated273cleanup qualified274
is finite, conditional on stalledshape withgeometry/size/heightsclosed; NOTautomatic
postrefinement cleanup or minimumedgegate.274/278/283/287NEVERran it; no wholefieldLE
benefit established. Preserve it while activecertificatecycle finishes.

Latest meaningful verified mesh remains274/inspection/rejected_mesh.vtu (+su2),
148781points/734806tets, captured nodalmetric9 SPD and knownq/J/localgeometry in
CellData. Refused globalproof, notaccepted.262hasfourUNKNOWNq/J; no shapecertificate.
No pertrial VTU required; export a new finalmesh when meaningfully useful.

After TWOaccepted: verify perfield/timelevel true massresidual<=1e-10, scaledtarget
integraldefect<=1e-12 and admissibility; require finite continued ADAPTED CFD.
Then isolate actualacceptedmesh/native_ref/matchingrestart in a newdir, ADAP_LOOP=NO,
RESTART_SOL=YES,READ_BINARY_RESTART=YES; input/output restartbasenames must differ.
Verifycoords/order<=1e-12,finitefields,positive density/internalenergy and continued
CFD. Source readiness is not runtimeproof. Then dedicatedBL/constraints/MPI/RANS500k.

Shockallocation stays separate: sampledtarget~4–6%chord versus existing~3–4%.
Cleanup/certificate speedup does not strengthen shockdemand. Other hypotheses stay
in[the backlog](Handbook/ADAPTATION_BACKLOG.md), not a competing cycle.
