# FP0 test and helper corrections

- Isolated-source setup first tried to locate historical qualification paths
  inside the source tree, starting with a missing .clang-format. It now copies
  the root source and build inputs and verifies the accepted production, test
  and external pins. The full accepted pin manifest remains linked by SHA256.
  No build or test had launched at the setup failure.

- The A9/A11/A12 syntax-check launcher initially had a malformed unused Python
  f-string. Removing it allowed the exact-flags syntax check to pass.

- The isolated build and link completed. The first seal classified every
  warning as new and then treated Meson diagnostics as compiler diagnostics.
  The corrected seal matches those messages to the pre-fixture configuration
  log and binds each compiler warning to an unchanged source, exact compiler
  command and dependency list with no FP0 test input. The original receipts
  and logs are preserved. No build or numerical check was repeated.

- A's first fixture reached the correct frozen CFD point IDs, then stopped
  before any measurement because the no-BL remesher factory allocates an
  empty reference without loading the sidecar. The test now loads the pinned
  entity reference using the production sidecar reader before timing the
  one metric import. The failed fixture and build592 evidence are preserved.
  The A9 wall monitor also receives its start marker immediately after mesh
  load, before region setup, so a setup stall stays inside its 900 s limit.

- A10's first test passed the absolute admission flag and stopped at cell
  360 with q=0.18024384633056384 and J=0.028573106646635871. It had measured
  only the completed batches, so its counts were incomplete. The default
  production measurement still computes q, J and edge lengths and returns the
  failure counts. The fixture now uses that call with unchanged thresholds and
  target. The original attempt is kept under recoveries/A_census_593.

- A12's first cubic order check matched all 1,000 samples. Some finite-skin
  closest-point projections produced a nonfinite distance, and the resulting
  mean was not valid JSON. The fixture now retries a nonfinite distance with
  a cross-product projection and segment distances on the same flat triangle.
  It records the number of such samples. Cubic controls and sample locations
  stay fixed. The original log is kept under recoveries/A_float_surface_593.

- Build594 stopped before linking because the new thin-triangle helper used
  the nested patch namespace for vector functions defined in its parent.
  The fixture now imports the four qualified functions from their actual
  namespace. No measurement was launched with build594.

- The first standalone Refine returned valid on all six ranks, with zero
  rounds and zero accepted changes. Its scope printed the job peak as
  -1,297,141,760 B. The independently summed six process peaks total
  7,292,792,832 B, which wraps to that signed 32-bit value. The test had called
  World::sum(int) via an implicit conversion. It now calls the existing typed
  64-bit collective. The replay and its six payloads are reused, and only
  the standalone fixture is repeated. Evidence is under recoveries/B_refine_595.
