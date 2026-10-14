# Physics and transfer questions to revisit

User request, 2026-10-14: preserve these questions for later investigation. They
remain open; the current SA/ideal-gas workflow does not qualify these combinations.

- **SST:** does remeshing preserve admissible k/omega and the solver's conservative
  turbulence representation, current state and BDF histories through interpolation,
  limiting, recovery, rebuild and restart?
- **Transition models:** qualify the supported SST/SA and transition-model
  combinations. Which additional transported variables, bounds, couplings and
  history/restart requirements must transfer preserve? Establish the actual
  supported combinations from configuration and solver code before choosing tests.
- **Non-ideal gases:** is interpolation and conservative projection robust with
  each supported equation of state? Assess the actual fluid-model admissibility
  checks and recovery, including valid density/internal-energy states, temperature
  and pressure, model validity ranges and thermodynamic inversion failures.
- **Combined cases:** validate turbulence/transition/EOS combinations explicitly;
  success of each component separately does not establish the combined workflow.

Use representative serial and MPI N=M/M<N adaptation/restart cases with current
and BDF histories. Preserve strict physical bounds, conservation and diagnostic
failures. State the validated combinations and unsupported limits; do not infer
robustness for non-ideal gases from ideal-gas positivity checks.

These are deferred questions, not changes to implement during the current
wall-reconstruction qualification. Revisit them before broad physical-model
support or interpolation robustness is claimed.
