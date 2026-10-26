# Historical validation47–54 handoff

These are historical results copied from the handoff on2026-10-26. They do not
validate the subsequent staged-adaptation changes. Current work is in
[HANDOFF_Codex.md](../HANDOFF_Codex.md) and the
[staged adaptation handbook](STAGED_NATIVE_ADAPTATION.md).

54 full M6 Euler6 session64848 terminal (driver0, native REJECTED).
Native1805.12s, whole1980.41s, largest process736,780KiB (not whole-job peak).
24,972 private edits/31,133 attempts;427,852 cells;235,717 cached invalid;
235,227 queued edges;76,739 shape refusals;2113 optimizer calls/115 validated;
max6 simultaneous commits. Original CFD mesh/solution retained, no transfer accepted.
Source990/runtime19,81 loaded MPI/library pins, inputs and connected receipts verify.
54 connected qualification remains passed: serial127/1,082,802; MPI6 118/rank;
all-worker11/rank incl cubic Euler/RANS BDF2+SU2/CGNS reload; focused4/31.
Implementation e9263060cf and docs68621318f5 pushed to approved fork/branch.
Independent review of f5e9456f06 reconciled:
[findings and implementation qualifications](Handbook/M6_REVIEW_RECONCILIATION.md).
Witness table now separates failed-cell q/J from patch minima; MPI1.428 is only a
hypothetical accounting quotient. That review itself established no production numerical defect.
Do not add local fallbacks or raise limits blindly. M6 restart_adap_00000.dat
contains exact original 61,330-node sensor metric (Metric_XX..ZZ,23 fields), usable
with existing hidden NativeFrozenVolume3D replay. Verify coordinate/ID correspondence.
Investigate fixed cavity/surface constraints and bad boundary triangles, using
actual frozen M6 field and bounded witnesses; do not substitute toy-case success.
Profile13781 terminal:491 user-CPU samples, about74% inclusive in MPI_Allreduce,
active reconstruction/donor measurement. Decoder91717 interrupted after DWARF errors;
symbol-only reports pass. Raw54/m6_euler/live_profile.data remains LOCAL ONLY.
No raw sampled stacks or large restart binaries added to git; pin their hashes.

Historical checkpoint895ab7274a: empty initial FVM ranks fix; four-point sphere
import passes on6 ranks.4933586627 fixed optimizer work accounting;47 serial125
cases/1,082,524 assertions pass.48 MPI6 passes114/116 cases/rank: cubic runtime
work4096 and collapse-filter counter fail. All-worker48 affine stagnates with1 bad
cell; cubic exhausts4096.49 diagnostic build95165/probe6364 terminal, pins verified:
whole-mesh topology cannot fix affine last cell; one insertion/flip can. Cubic45bad
->34 topology, ->88 full-interior/boundary at8-sweep cap, normalizedminimum0.0394.
No diagnostic output is an accepted mesh. These are historical validations47–49;
the current full-M6 failure is validation54 above.

50 connected implementation checkpoint7113fd984d (WIP;51 correction checkpoint b214ff210b): Runtime Absolute shape policy;
bounded optimizer repair of private candidates before publication, both resident and
streamed paths. Swaps/insertion; no smoothing/boundary operations. Construction
preserves old vertices; repair retains collapse. Candidate parameters/interface,
original-field metrics, height, geometry and progress checks retained.160 extra node
IDs separate from original construction range; actual trial/node guards. Reserve
work only for optimizer phase, refund unused immediately; cumulative160/proposal and
global limits unchanged. Collapse-filter fixture retains its two-owner local witness
layout while all ranks participate; every original counter/mesh assertion retained.
49 adds test-only complete rejected-cell dumps and optional bounded probe using
SU2_NATIVE_MAPPED_REPAIR_PROBE with SU2_NATIVE_MAPPED_ALL_WORKERS=1. Preserve its
failure evidence. Do not introduce further late fallbacks or raise limits blindly.
