# Current-evaluation gradient reuse with MUSCL flow

The common residual wrapper now retains temperature/velocity gradients already refreshed
in the current evaluation when it runs boundary-only flow assembly for eligible first-order
auxiliary probes. Flow MUSCL fluxes are omitted on that path; the guarded boundaries use
nodal characteristic states and those current viscous gradients. Removing the blanket
MUSCL-flow exclusion avoids a redundant native gradient pass. It adds no cache across
states, probes, CFL retries, timesteps or configurations, and no new option or dependency.

Live primitive/transport properties, SST kinetic-energy effects, strong boundary closure,
prescribed masks, halo communication and numerical-history restoration retain their existing
paths. Species, bounded scalars, higher-order auxiliary transport, wall functions, hybrid
models, feedback and unsupported boundaries keep their previous fallback. The NACA SA deck
uses higher-order turbulence and therefore does not benefit from this implementation change.

## Correctness and application checks

The initial eleven regression jobs pass: 69 integration cases on both layouts, 92 ordinary
cases and species, failure/restart, turbomachinery and setup CLI checks on both layouts.
The extended test poisons unused pressure/density/enthalpy gradients, compares auxiliary
residuals with full assembly and retains characteristic states, frozen history and rollback.
A further test covers translated and rotated periodic MUSCL flow with SA, SST and SST–LM.
The final unit binary passes all 70 integration cases on each layout; its CFD binary is
byte-identical to the first candidate, so CLI/ordinary checks remain applicable.

All six original-control application pairs below retain identical configuration hashes,
complete Newton diagnostics, residual/linear work and byte-identical restart files. They
are supporting prefixes, not converged application endpoints. The [CSV](NewtonKrylovGradientReuse20261018.csv)
retains before/after observed times, memory, physical reports, hashes and evidence paths.

| Case | Layout | Mode | Updates | FLOW/TURB, before = after | Outer/inner K, before = after | Retries | Restart |
|---|---|---|---:|---:|---:|---:|---|
| naca_sa | 1x1 | segregated | 20 | 1810/786 | 907/0 | 2 | byte-identical |
| naca_sa | 2x2 | segregated | 20 | 3810/678 | 1606/0 | 1 | byte-identical |
| naca_sst | 1x1 | segregated | 20 | 3952/336 | 1226/0 | 0 | byte-identical |
| naca_sst | 2x2 | segregated | 20 | 3946/339 | 1234/0 | 0 | byte-identical |
| naca_sst | 1x1 | coupled | 20 | 3382/3818 | 430/0 | 3 | byte-identical |
| naca_sst | 2x2 | coupled | 20 | 3521/4022 | 500/0 | 3 | byte-identical |

## Measured computational work

GDB counted native primitive-gradient method entries on the main thread, over the same
original auxiliary fixture including setup. Counts fall from 395 to 380 per rank on both
1x1 and 2x2. The additional periodic test is separate and is not included in that counting
boundary. These are measured method entries, not counts inferred from outer iterations.

The existing PMPI/libgomp interposer then measured a matched 20-update NACA SST segregated
pair on 2x2. Both profiled restarts and all Newton telemetry match their unprofiled runs.
The counting boundary is the whole solver process, including setup/output, main thread
only. The [counter CSV](NewtonKrylovGradientReuseCounters20261018.csv) retains both ranks.

| Counter, rank 0 | Before | After | Removed |
|---|---:|---:|---:|
| MPI_Allreduce | 92966 | 92966 | 0 |
| MPI_Isend | 69446 | 68812 | 634 |
| MPI_Irecv | 69442 | 68808 | 634 |
| MPI_Waitany | 69195 | 68561 | 634 |
| MPI_Waitall | 69568 | 68934 | 634 |
| GOMP_barrier | 1276982 | 1272544 | 4438 |
| GOMP_loop_end | 167184 | 164648 | 2536 |

Rank 1 shows the same savings. These are API calls, not message bytes or global sums over
ranks. The change is modest: about 0.91% fewer sends in this application prefix. It does
not remove residual assemblies, independent derivative checks or the dominant accepted
Krylov work. No broad speedup follows from these counts.

An initial SA profile correctly showed identical MPI/OpenMP counts because MUSCL_TURB=YES
retains the fallback. Its two successful numerical runs and the subsequent failed analysis
assertion are preserved. Whole-process allocator counts include nondeterministic MPI/setup
activity; their differences are not attributed to this optimization. Profiled SST elapsed
times were 133.913 and 142.032 seconds, with different contention; they establish neither
a speedup nor a timing regression. No timing assertions were added.

## Reproduction

Evidence is under `newton_krylov_refactor_20261015/application_qualification_20261018/` in
the workspace parent:

- `muscl_boundary_gradient_v2_release/manifest.json` and `source.patch`: final source/build
  hashes and binaries. CFD SHA256 `c04ba1ec5a43783f029888bda17d2afc528651474026b29e494a1c91d1ae3610`.
- `muscl_boundary_gradient_build.py`, `muscl_boundary_gradient_v2_build.py`: guarded build,
  original checks, GDB counting and periodic validation commands.
- `muscl_boundary_gradient_checks/commands.json`, `muscl_boundary_gradient_v2_checks/commands.json`:
  eleven original passing jobs and two fresh 70-case integration runs.
- `muscl_boundary_gradient_20261018/application_commands.json`: eight application jobs,
  six complete matched pairs, exact configurations and byte/work assertions.
- `muscl_boundary_gradient_20261018/count_commands_{before,after}.json` and `capture_gradients.gdb`:
  measured gradient entries on both layouts.
- `muscl_boundary_gradient_20261018/profile_commands_sst.json`: exact release-profiler
  commands, shared-library hash, unchanged numerical checks and raw counters.
- `muscl_boundary_gradient_20261018/sa_profile_fallback_assessment.json`: preserved negative
  control and limits on allocation attribution.

The separately labelled [fixed-forcing study](NewtonKrylovFixedForcing20261018.md) changes
existing linear-accuracy controls. Its reductions in residual/Krylov work are not credited
to this gradient-reuse implementation. Full application qualification and eligible timing
comparisons remain separate requirements.
