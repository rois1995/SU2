# Isolated rotating-reservation correctness probe — 2026-10-12

Repository: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch: codex/native-3d-transport-accounting, production checkpoint
86bd7f78243cf1e4b38d54009ed18e6bd62bbdc5. Goal 1 active; Goal 2 not started.
This is an isolated prototype. Published C++/headers/configs, configured driver,
original 68 objects and 774 archive members are unchanged before/after the probe.

## Variant and safety scope

A private copy of CNativeDistributed3D.cpp changes only reservation arbitration:
first_rank=(matching reservation-call count-1)%worker_count. The winner at each
shared vertex minimizes its cyclic distance from first_rank. All other admitted
vertex dependencies, memory/record budgets, failed common votes, immutable
geometry/frozen fields and atomic publication stay in the original implementation.
The first call keeps priority 0, and a serial communicator keeps priority 0 always.

Each claim carries first_rank as an additional integer. The rendezvous owner
requires the received epoch/rank to match its own matching-call epoch; mismatch
throws into the existing common failure vote before reservation approval. No
new collective is introduced. The extra encoded word per claimed vertex is real
traffic and must be included in performance comparisons; requested-memory bounds
continue to use the full Claim and RecordStream representation. This is not a
no-cost priority change.

With one common total order, two proposals sharing a vertex cannot both win,
and the globally first active proposal wins all its claimed vertices. This is
the same disjointness/progress argument as the original minimum-rank rule. A
persistent active rank becomes globally first once per worker-count calls, but
changing proposals and feasibility do not imply equal useful edits or runtime.
No runtime epoch-mismatch fault injection or exhaustive new MPI safety matrix
has been performed for this prototype. Matching collective/object lifecycle is
required. A production implementation would need that stronger qualification.

run.py uses the actual configured compile/link recipes and all original test
objects/archives, placing only the new directory object ahead of the original
archive. Its link map verifies the original directory member is not extracted.
All original production source/object/archive hashes stay unchanged; source,
commands, compiler output, link map and disposable binary/object hashes are pinned.
The existing full test driver is not overwritten. The prototype uses the original
NativeEnvelope3D case and unchanged existing independent auditors.

## Actual N4/M2 result

The default 512 size 4 wedge, aspect 1.5, turning direction/spatial strength, actual
original-P1 target and marker junctions match the standard control's initial
config/mesh/target bytes exactly. Three actual refine/coarsen/refine replacements
pass C++ and a fresh complete independent audit:

- Accepted tetrahedra: 289/51/287; points 96/27/97.
- Exact volume 1/2; positive embedding/incidence/no-overlap/vertex links/domain/
  finite facet and physical marker preservation.
- Minimum q 0.24666457184881851, minimum J 0.08654324081190538,
  maximum full original-P1 metric-edge length 1.7957781168258844.
- Transported tensor residual 0; current/BDF residual<=3.5936476e-16.
- Actual SU2/CGNS output/readback and all parent/rank/transport accounting pass.

No original target, .20/.05/1.8 guard, boundary constraint, finer request, fade,
noise 0, donor/cavity cap or 512 limit is relaxed. Surfaces adapt as in the original
case. Accepted meshes, original/current/BDF states, exact audits, SU2/CGNS outputs
and three inspectable envelope_s0/s1/s2.vtu files are retained.

| Second coarsening work | Standard minimum rank | Rotating prototype |
|---|---|---|
| Private attempts, ranks 0/1 | 106/54 | 78/78 |
| Accepted, ranks 0/1 | 40/29 | 39/30 |
| Rejected, ranks 0/1 | 66/25 | 39/48 |
| Reservation vetoes, ranks 0/1 | 0/87 | 51/50 |
| Global transaction rounds | 141 | 129 |
| Final tetrahedra / points | 48/27 | 51/27 |

First coarsening uses 40 rounds rather than 39. Third coarsening uses 80 rather than 116,
but the final accepted mesh is 287 rather than 314 tetrahedra. The changed trajectories
must remain distinguishable. Initial inputs/target match; later original-P1
references are defined on different accepted meshes and are not automatically
identical targets. comparison.json records all operation and communication
counts and explicitly does not calculate a timing ratio. More even attempts and
fewer rounds in this example are promising, not a net speedup/scaling certificate.

The full auditor passes 8 input/accepted/readback frames and 3 current/BDF state pairs.
No old certificate is substituted for the new geometry. Production QoS, weights
and priority remain unchanged. The prototype is not mixed into the prepared
fixed-work donor comparison; that would change its numerical trajectory.

## Execution and next qualification

This is one sequential one-CPU/nice19/j1/thread1 compile/link/MPI4 correctness
probe after real host/memory/space checks, with four unrelated FSI ranks active.
No local profiling, repeated timing or cluster submission. All owned jobs are
terminal. Reproducible disposable object/executable are removed only after their
hashes/recipes/terminal evidence are retained; production binary remains available.

Before promotion, test repeated shared/disjoint claims, epoch mismatch refusal,
empty owners, MPI1/2/4 and M=N/M<N, complete atomic veto/stale/admission behavior,
strong/thin/production-limit cases, actual mesh/state/restart and new independent
trajectory audits. Then run repeated matched-target cost/scaling/traffic and
instrumentation controls separately from the fixed-work donor campaign. Full
Goal 1 performance/calibration/default 512limits remain open. This proves feasibility
for one declared faceted manufactured case, not production/general MPI robustness,
curved geometry, production 3D BL, full unsteady CFD or affordability.
