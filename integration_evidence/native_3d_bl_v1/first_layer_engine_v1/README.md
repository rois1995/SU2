# First-layer distributed engine checkpoint

Run `python3 integration_evidence/native_3d_bl_v1/first_layer_engine_v1/run.py`
from NativeIntegrated after building the existing test target. The helper pins
source, objects, binary, compile commands, helper, command and environment; each
MPI stage is independently resumable. Threads=1, nice=19, MPI1/2/4 run serially.
CPU affinity is in selected_cpus.txt; select available CPUs before reuse elsewhere.

Latest validation.json is PASS. Earlier failed receipts/logs are preserved.
The initial MPI4 failure required local checks on workers without wall cells;
collective checks correct that assertion without weakening height/mesh contracts.
Tests also retain accepted input on inconsistent/nonfinite policies, impossible
height and a work ceiling after one private layer transaction.

Scope: height construction, edit protection, owner transactions and N=M/M<N.
The one-round probe does not assert full remesh validity. No composed BL target,
CFD, transfer, RANS, restart or performance qualification is claimed here.
