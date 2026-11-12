import SU2BoundaryMemory
import Lean.Util.CollectAxioms

/-! Elaboration-time dependency audit. Evaluators should run their own audit as well. -/
open Lean Elab Command in
run_cmd do
  let env ← getEnv
  let mut checked := 0
  let mut declarations := 0
  let mut runtimeDeclarations := 0
  for (name, info) in env.constants.toList do
    if (`SU2BoundaryMemory).isPrefixOf name then
      if info.isUnsafe then
        unless name.isInternal do
          throwError "Unexpected unsafe public declaration {name}"
        runtimeDeclarations := runtimeDeclarations + 1
        continue
      let axioms ← Lean.collectAxioms name
      for ax in axioms do
        unless #[`propext, `Classical.choice, `Quot.sound].contains ax do
          throwError "Unapproved axiom {ax} in {name}"
      declarations := declarations + 1
      if info.isTheorem then
        logInfo m!"AUDIT {name}: {axioms}"
        checked := checked + 1
  logInfo m!"Excluded {runtimeDeclarations} internal unsafe compiler-runtime declarations; no proof dependencies exempted."
  if checked == 0 then throwError "No SU2BoundaryMemory theorems audited"
  logInfo m!"Audited {declarations} SU2BoundaryMemory logical declarations ({checked} theorem declarations); standard logical axioms only."
