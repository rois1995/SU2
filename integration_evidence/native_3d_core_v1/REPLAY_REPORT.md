# Exact native3D size-limit replay — 2026-10-11

Working repository `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`, branch
`codex/native-3d-core`, predecessor `ce942b5d6fba97985577b2416e1d4256018b6e7c`.
Implementation/evidence checkpoint `2463e523c2827a5182139fcdd4a71ff7083053fe` is pushed and
remote-verified. Full Goal1 stays active. This checkpoint changes tests/audits/runner only;
production operators, target, criteria and default512-round budget are unchanged.

## Qualified evidence

`REPLAY_MATRIX.json` freezes two serial direct Refine diagnostic allowances,
512 and1024. Each uses byte-identical input SU2/config from the failed actual
n3 driver case and the identical original imported mesh, facets, markers and
sensor JSON. The target is9*(1+0.125*x) times the45deg rotation ofdiag(2.25,1,1).
Actual complete configured archives/test objects are pinned; no standalone
kernel override. Direct-engine diagnostic allowance is not a CFG option or
changed production default. CFD geometry/solution/reference and original
input/source remain unchanged. No ReplaceMesh or actual CFD iteration occurs.

| Qualified folder | Rounds | Private accepted/rejected | Cells | qmin | Jmin | Oversized edges/cells | Lmax |
|---|---|---|---|---|---|---|---|
| replay_size3_r512_v2 |512|59 /453|229|0.267933|0.146700|25 /65|2.464292|
| replay_size3_r1024_v1 |1024|79 /945|231|0.211317|0.098595|21 /51|2.464292|

The512 replay matches the original backend's counts exactly, including18 moves
and26 reconnections. The1024 replay reaches29 moves/35 reconnections, but still
rejects. Both private meshes independently pass positive volume, no overlaps,
incidence/manifold links, oriented finite-facet coverage, marker and domain
volume1 checks. Their measured q/J remain above frozen0.20/0.05. Strict edge
completion fails: unchanged maximum2.464292 exceeds1.8. Longer budget gives
some progress, not completion or proof that further rounds cannot complete.

Both folders contain `replay_rejected.su2` and full original/private JSON.
The SU2 file is an independently signature-checked diagnostic private grid,
not an accepted house/CFD mesh. `independent_audit.json` records every oversized
edge, its coordinates/length and incident cell IDs. For512, the longest edge
2,23 joins(0.6666666666666666,0,0) to(1,0.3333333333333333,0.3333333333333333),
incident cells12,13,14,52390,52392. For1024, edge6,27 of equal length remains.

The512 diagnostic Refine wall time is8.046308s (metric6.425151s,
reconstruction0.812530s);1024 is12.942995s (metric10.307921s,
reconstruction1.570562s). Nested edge tracing is6.734453s and10.517806s,
spanning multiple phases; do not add nested times to exclusive totals.
Single runs under foreign four-rank FSI contention do not establish a speedup
or controlled timing comparison. Both whole events reject: useful accepted
CFD work is zero even though private edits were accepted.

## Failures retained and corrected

REPLAY_PREPARATION_FAILURE.json preserves an initial runner closing-parenthesis
error caught before folder/build/MPI. replay_size3_r512_v1 preserves the first
fixture's crash after Refine returned: querying the driver's native reference
without creating Backend first. Backend initialization was added only in the
test; production envelope setup already does it. Original receipt/logs/source
remain; no production crash is inferred.

The corrected v2 passes all17 C++ assertions. Its first Python audit compared
frame-derived facet dictionaries against raw JSON and failed. Corrected auditing
requires stronger equality of the entire raw import/source JSON, then unchanged
independent geometry/residual checks. No refinement rerun; AUDIT_RECOVERY.json
binds original FAIL, failure log, corrected audit and rejected-grid export.
The1024 pipeline is terminal PASS. The ordinary runner control also passes;
all11 accepted/input/readback mesh/source and state files match the published
N1 varying-strength case. Production archives/members match the reader checkpoint. PASS here qualifies diagnosis/rejection and
state preservation; it does not mean strict adaptation completed.

## Next implementation experiment

Current Refine enters its single recovery queue after all available splits
are blocked. It then remains in repair mode until all cells pass. Successful
movement/reconnection can unblock the ordinary split queue through Drop/Add,
but that queue is suppressed while repairing. Thus an improving repair is
not followed by a newly feasible split. This is a verified control-flow
limitation and a causal hypothesis for the stall, not a demonstrated fix.

Test alternating back to available splits after successful recovery edits,
while retaining the single seeded repair queue, its dependency-based blocked
state, existing round/storage bounds, immutable donor/geometry and strict
final checks. Separate selection of the current operation from tracking repair
features; do not reseed the queue blindly or raise production sweeps first.
Collective operation choice must remain identical on all workers. Preserve
Coarsen and pure Repair contracts, including current single-recovery tests.

The segment-clipping hotspot still needs a separately qualified optimization.
IntersectsSegment already has certified sign filters while TraceSegment uses
exact rational clipping for each AABB candidate. A filter must preserve invalid
source detection, coverage, conformity and tiny/tied intervals; do not bypass
exact clipping or alter target samples based on these timings alone.

Inspect/reproduce with a fresh folder:

```bash
python3 integration_evidence/native_3d_core_v1/run_envelope.py \
  integration_evidence/native_3d_core_v1/my_replay \
  --ranks 1 --workers 1 --size 3 --anisotropy 1 --varying 1 --replay-rounds 512
```

Local-first one-job policy still applies.
No cluster launch requested tonight. Full traffic/wait/memory calibration,
stronger/direction-changing/thin/wedge/restart limits and closed SGE campaigns
remain required. No full Goal1, BL, curved, unsteady3D or affordability claim.
