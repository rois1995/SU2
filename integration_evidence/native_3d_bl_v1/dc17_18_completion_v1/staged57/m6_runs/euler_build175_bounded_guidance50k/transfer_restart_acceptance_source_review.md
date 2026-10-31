# Actual M6 transfer and disk-restart acceptance — source review

Read-only preparation while175 remains frozen/live. No accepted mesh, transfer or
adapted restart is claimed. This supplements existing acceptance facilities, not a
new validation campaign or implementation cycle.

The current double-precision conservative backend records the true residual for
every field in CDistributedProjection.cpp:1438; SolveActive fails above
TransferTol(1e-10,1e-4) in ConservativeKernels.cpp:455. CConservativeTransfer.cpp:997
collectively checks the FULL final conservative admissibility predicate at every
point/history and projection-target totals (1e-12 double tolerance). PrintSummary
at1394 prints supermesh/fill/sliver/domain data, per-field true residuals and
final donor/new relative integral changes. A printed "conservative" label alone
is not acceptance. Default sliver policy is CLOSED: closed wall/symmetry content
is compensated, while open-boundary totals follow actual domain changes. Inspect
actual per-field residuals and integral changes; do not confuse limiter-target
preservation with a proof that arbitrary moving domains keep donor totals.

Require actual successful transfer sections for BOTH accepted events, no skipped
or rejected event, finite subsequent CFD, actual published mesh/native_ref pairs.
Initial restart00000 is only the61330point input-state provenance. Raw transfer
residuals/admissibility are assessed at transfer time, not by comparing integrals
after a subsequent evolving CFD iteration.

After the full workflow is terminal, COPY the accepted final mesh, native_ref and
matching final solution into a fresh isolated directory; never mutate originals.
Pin current binary/runtime/config and copied inputs. Reuse capcheck.read_su2 and
read_restart and audit_native_unsteady.state_values to check header dimensions,
point count, coordinate order, finite density/momentum/energy, rho>0 and positive
internal energy. Actual CFD continuation must use RESTART_SOL=YES, ADAP_LOOP=NO,
correct steady SOLUTION_FILENAME and a distinct RESTART_FILENAME/output basename.
Steady names do not gain an unsteady iteration suffix merely via RESTART_ITER.
Run and verify actual finite continuation/output; matching initial files alone
cannot establish a usable adapted disk restart. This avoids input collisions and
does not claim the general resumed-adaptation cycle-numbering risk is fixed.

Reuse existing integration_evidence/audit_native_unsteady.py::audit_3d_transfer
ONLY when exact donor and post-transfer mesh/state histories are available. Its
P1 integral audit is not applicable to states after further CFD evolution. Re-pin
helpers before actual execution; no Python audit was run on unaccepted artifacts.

Frozen C++ source SHA256 from the active run pins:
- SU2_CFD/src/adaptation/CConservativeTransfer.cpp: 9da510e846516899f222b5421d72e09fd05d89e978f8d592c02003c63604fee2
- SU2_CFD/src/adaptation/CDistributedProjection.cpp: b651e393ee2897167a45c990c214ae2c77a602444361813970fa5620bf3944bb
- SU2_CFD/src/adaptation/ConservativeKernels.cpp: e81a533c852898a39fa15889ad7bf7d0cb5571baae8a16ef42d892dc722634d9
- SU2_CFD/src/drivers/CSinglezoneDriver.cpp: 4979e78879292f03f6c910c7932b34bbac3d7075e6f6446ffb231c1c9ca6481f

Build metadata check: ConservativeKernels.cpp compile entry has no
USE_SINGLE_PRECISION define; meson-info/intro-buildoptions.json reports
enable-singleprec=false. Reverify build/binary provenance at actual acceptance.
