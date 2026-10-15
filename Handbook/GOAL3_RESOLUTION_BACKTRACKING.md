# Goal3 — preserve resolution while repairing connectivity

Repository: `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`;
branch `codex/native-3d-bl`. Full Goal3 remains ACTIVE and incomplete.

## Demonstrated mechanism

The559-cell stronger planar reproduction had valid exact embedding/unit volume,
first altitude/normal feet and shape, but a physical wall edge measured1.855981.
Its18-cell edge/apex-star union was not geometrically closed: the new normal child
needed four actual neighboring cells. At22 cells the retained reconstruction was
geometric but shape-invalid. Its actual-field hint requested the complete support
star of vertex50319, yielding31 cells. The common single-background fan did not
resolve its corresponding grown closures; limits were not increased.

The original retirement-first shape repair completed in19 trials (3 removals,
2 flips), q .206430/J .052889,40 cells. It introduced two long edges1.989505 and
1.810532. Independent full composed integration reproduces changed excess:
removed .003133829, added .036023066. This is an operator-choice blockage; the
prior admissible mesh and unchanged field rule out blaming a newly relaxed target.

## Connected implementation

`RepairMetricConnectivity` now permits a resolution-preserving retry that skips
point retirement. Engine callers retain the original private reconstruction only
while repairing, in the existing bounded multi-proposal scratch. If a retirement
proposal completes shape but fails the final frozen progress gate, the engine
restores that reconstruction and retries without retirement. The two attempts
share the original per-worker allocation of at most256 trials and unchanged
4096 event work ceiling. Collective branches/imports/validation remain matched;
nonretrying workers retain their previous candidate result. Moved original nodes
still receive reserved fresh identities before publication. Complete dependencies,
first altitude/footprints, actual-query target and strict final gates remain.

The exact31-cell replay succeeds on the alternate path in70 more trials (7 flips,
2 moves), q .209831/J .051600, changed-edge added excess0. The mandatory success
assertion passes on current source. No config mode or fixture dispatch in production.

## Complete declared qualification

Native construction/removal plus independent exact geometry/coverage and analytic
original-P1 sensor-plus-square-wall field audits pass MPI1/2/4:

| workers | BL cells | released cells |
|---|---:|---:|
| 1 |591|252|
| 2 |591|250|
| 4 |591|243|

BL coordinates/connectivity/markers are exactly identical across workers. BL
qmin .213044/Jmin .050377/Lmax1.798735, altitude error0 and contained normal feet.
The maximum centroid tilt1.05409 is retained as a diagnostic; no complete normal
alignment/multilayer claim. Source is the original six-cell cube with scalar
`4+12*x`, h0=.1/growth1.2/thickness.3/core4/unchanged fade. This is engine N=M
qualification, not current+BDF/CFD solution-transfer/restart or viscous accuracy.
No speedup/scaling/affordability claim from these correctness runs.

Evidence under `integration_evidence/native_3d_bl_v1/wall_reconstruction_v1`:
`backtracking_validation.json`, `backtracking_MPI_consistency.json`, `sensor`,
`sensor_mpi2`, `sensor_mpi4` and their independent audits. Accepted grids are
`finer_grouped_constructed_BL_n1.su2`, `_n2.su2`, `_n4.su2` in those folders.
Exact immutable regression input: `planar_wall_input.json`; fixture and metadata
`native_bl_planar_wall.inc`, `planar_wall_fixture.json`; exact replay
`planar_wall_replay`, rejected retirement `planar_wall_retirement_rejected_v1`.
`diagnose_planar_wall.py` reproduces the rejected debt independently against those
closed hash-pinned inputs. All earlier failed policy experiments remain preserved.

## Remaining difficult target

Current full cold-cylinder run still returns the300-second kernel ceiling with
52 edits/all8 wall reconstructions,438 rejected tetrahedra. Its SU2 output is
byte-identical to the earlier recorded residual-audit input. Relevant original
query/metric/import/test hashes match; `curved_residual_reuse.json` explicitly
retains the old executed-binary measurement scope rather than claiming a fresh
full field audit. Measurements: no native shape failures,q .229233/J .056062;
281 oversized edges of624,max4.201315. No release or accepted CFD replacement.

The next missing curved capability is coordinated transition construction across
retained non-star-shaped interfaces. Repeating common-fan growth or raising work
limits is not justified. Preserve actual-query geometry, immutable finer sensor,
fade and all admission/rejection/return contracts while addressing this mechanism.
Interacting volume and composed gradation still need work. Full M<N/BDF/SA/CGNS/
reference/restart, wider geometry, actual RANS/M6/accuracy and complete performance
gates remain required in `GOAL3_REQUIREMENTS.json`. General CAD, structured
multilayer/prism/hex support and full physical unsteady3D remain separate scope.
