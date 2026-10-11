# Native 3D first practical size limit and termination diagnostics

The diagnostic review passes; both adaptation runs remain **FAIL**. The frozen
n8 wedge has405 vertices,1536 tetrahedra and512 planar boundary triangles.
N1M1, targets64/16/64, aspect1.5, spatial rotation/variation0.125, quality>=0.20,
sliver>=0.05, length<=1.8, rounds512 and noise0 are unchanged.

Cycle0 refines successfully privately (457 edits,3552 tetrahedra). Coarsening
uses512 attempts:272 accepted and240 rejected, leaving2489 private tetrahedra.
The newly emitted counters show2489 cached measurements, zero cached invalid
cells,91 pending edge candidates and zero pending repairs. Engine completion
explicitly requires the coarsening queue to be empty. This is budget/queue
incompletion, rather than a recorded remaining frozen-field violation.
The original and diagnostic binaries have identical input/config/source-field
files and all compared integer work/communication counters; see validation.json.

The entire remesh is rejected. The original C++ fixture passes accepted CFD
current/BDF/reference preservation and no-acceptance-callback checks before its
sole COMPLETE assertion fails (5038 assertions pass, one fails). No private
rejected grid is exported; envelope_input.su2 and envelope_s0_initial.json are
inspectable input evidence. No accepted-geometry/current-BDF/output audit ran,
and no later cycle ran. Cached validity is not an independent mesh certificate.

Five O(1) local termination counters were added to EngineStats, captured at every
exit and emitted in existing detailed engine rows. No new loop/collective,
operator, target or acceptance change was introduced. The real full68-object,
774-member MPI/MMG/CGNS build passes. Actual MPI1/2/4 each pass69 selected cases
per rank and independent bridge audits:12/24/48 frames,6/12/24 state pairs and
6/12/24 actual SU2/CGNS output/readback checks. Serial MMG2D/3D passes48 assertions
in two cases.30 package parser/mocked checks pass, including missing/fractional/
negative/corrupt termination counters. These are sequential contended correctness
checks, with sources/objects/archives/tools pinned before and after.
The newly built driver also passes the actual serial lifecycle: six OFF/ON
controls compute the metric/Hessian, remesh, replace geometry/solver/BDF and
write meshes; its independent audit validates six successful engine rows with
all termination counters and complete coverage/empty queues.

The n8 failure is a practical limit of the current bounded attempt policy. Do
not remove the pending-queue acceptance check, increase rounds or soften the
frozen gates to relabel this campaign as passing. The next implementation step
must distinguish scalable useful coarsening from repeated blocked proposals,
retain bounded transactions and validate its changed workload independently.
Local elapsed times are diagnostic only. Goal1 remains active: repeated cluster
performance, instrumentation overhead, larger accepted envelope and full MPI
wait/traffic/vendor/allocator calibration are still unproven.

Recheck saved evidence without numerical launches:

    PYTHONDONTWRITEBYTECODE=1 python3 integration_evidence/native_3d_core_v1/termination_limit_review_v1/review.py

This also checks the currently qualified binary and source/object/archive pins;
run before replacing that build. Historical raw FAIL logs are preserved.
