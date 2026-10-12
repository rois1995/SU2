# Selected static-assessment fixes

2026-10-12. Working repository: `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`.
Branch: `codex/native-3d-transport-accounting`, based on `7746fd53fa2f`.
The user separately authorized these fixes while Goal 1 is paused.

## Changes

1. Initial boundary import and reference decoding now use a global closed,
   oriented manifold validator. It rejects duplicate/repeated-node faces,
   open/nonmanifold or inconsistently oriented edges, and disconnected vertex
   links. Components and genus are unrestricted; the original skin no longer
   inherits the cavity-only 512-triangle restriction. Source embedding remains
   an accepted-input precondition. Existing finite-facet, coordinate and marker
   checks remain, as do the local sphere/disk and 64/128-cell cavity limits.
2. Owner publication admits the combined payload of independently validated
   cavities by bytes, rather than applying one cavity's cell limits to it.
   Version, duplicate-ID, approval, memory and atomic staging checks remain.
   Retired-edge scratch uses six edges per actual removed owner cell, with
   saturating byte arithmetic, instead of the same single-cavity limit.
3. Parent source construction and work-weight queries run only when repartition
   is requested, including M<N. Controls are checked collectively BEFORE entering
   conditional source collectives. Engines and reader return still validate/use
   the immutable sensor source. Non-repartitioned tests assert zero parent source
   builds, weight queries and weight collectives. No measured speedup is claimed.
4. Native 3D documentation is corrected. New core cluster manifests declare 100
   selected cases; legacy bundles default to 97. The runner validates and uses
   that count. Existing frozen scheduler/query comparison bundles remain tied
   to their earlier sources and require refresh before comparing these changes.

## Verification

The new topology tests accept 1,024 triangles, multiple components and a
closed genus-one skin while the cavity validator retains its stricter bounds.
Malformed skin and a vertex pinch are rejected. Actual CFD-owner import and
sidecar readback pass on a cube with 768 original boundary triangles.

The MPI publication test stages 65 independent one-cell cavities and 260
validated children on one owner. Memory and approval vetoes preserve both
stores; subsequent publication updates the complete center star. MPI1/2/4
exercise the owner and empty peers. This reproduces the aggregate-limit issue
without needing high rank counts. No >64-removal engine trajectory was executed.

| MPI ranks | Selected cases per rank | Assertions per rank | Bridge frames / state pairs / SU2-CGNS round trips |
|---|---:|---|---|
| 1 | 101 | 12,951 | 12 / 6 / 6 |
| 2 | 101 | 13,852; 16,304 | 24 / 12 / 12 |
| 4 | 101 | 15,264; 16,790; 18,348; 19,971 | 48 / 24 / 24 |

All C++ cases, independent bridge audits and seven distributed-record suites
per allocation pass. Serial MMG 2D/3D controls pass (48 assertions, two cases).
The mixed selection includes the actual driver case; its C++ geometry, metric,
state, output and accounting checks pass. No independent runtime certificate
is claimed for the mixed selection. The 34 lightweight package checks pass,
including legacy/new count controls; actual registration confirms 100 core cases.

MPI1 data are in `static_assessment_fixes_v2/mpi1`; its raw log is in the parent.
MPI2/4 data are in `static_assessment_fixes_v5/mpi2` and `mpi4`.
Completed MPI1 numerical execution was reused, not rerun.
Host checks, one CPU, nice19, j1, one library thread and sequential MPI<=4
were used. All owned jobs are terminal. No local performance campaign ran.

`STATIC_ASSESSMENT_FIXES_VALIDATION.json` pins the compiled sources, all 68 test
objects, production archives/members, exact binary and 2,193 retained artifacts
(about 53.5 MB). No copied binary/object/archive or symlink is retained.
Binary SHA256: `108d978568adc8b6b6d449a16da9ecaab0c2d7e97c11863876e41ff49e6899fc`.

## Failures retained

V1 stopped at a missing test <map> include, before linking or execution. V2
built and passed MPI1, then its collector used a Python 3.9 string method on
Python 3.8. V3 selected a serial-probe auditor for MPI records. V4 passed the
bridge audit; its runtime-only auditor reached a mixed-log assertion that
assumes every rejection has zero private accepted edits. The separate default
fixture deliberately tests rejection after private progress. No runtime PASS
is manufactured. V5 uses the existing seven MPI-record adapters and passing
bridge scope. All failures, original controllers, logs and failed test source
are retained. No numerical guard changed in response to a collector failure.

The postqualification cluster runner change only adjusts count declaration and
validation; its distributed_audits function is AST-identical. The exact earlier
tool source is retained as `static_assessment_fixes_v5/used_cluster_run.py`;
current tooling is separately checked and pinned. A listing wrapper initially
omitted Catch's word "matching"; the unchanged listing confirmed 100 cases.

## Remaining limits

Goal 1 remains paused and incomplete. Default 512-round completion, batching,
dense communication, repeated performance/scaling, actual weight calibration,
wait/traffic/memory qualification, 2D scan costs and strong-anisotropy generation
remain open. Driver rejection is still fatal; its policy is unchanged. The
1 MiB reference-file limit and global byte ceilings remain. No curved projection,
body-in-farfield adaptation, 3D BL generation or physical unsteady affordability
certificate is implied. Rotating scheduling remains isolated. AdapNoExt and
foreign ClusterResults are untouched.
