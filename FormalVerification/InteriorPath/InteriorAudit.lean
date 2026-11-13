import SU2Interior
import Lean.Util.CollectAxioms

set_option pp.universes true
set_option pp.fullNames true
set_option pp.explicit true

/-! Worker-authored aid; final acceptance and independent audits belong to the evaluator. -/
open Lean Elab Command in
run_cmd do
  let env ← getEnv
  let mut logical := 0
  let mut proofs := 0
  let mut internalRuntime := 0
  let mut roots : Array Name := #[]
  let constants := env.constants.toList.toArray.qsort (fun a b => a.1.toString < b.1.toString)
  for (name, info) in constants do
    if name.toString.startsWith "SU2" || (`CFDLean).isPrefixOf name then
      if info.isUnsafe then
        unless name.isInternal do
          throwError "Unexpected unsafe public declaration {name}"
        internalRuntime := internalRuntime + 1
        continue
      roots := roots.push name
      logical := logical + 1
      if info.isTheorem then proofs := proofs + 1
      if (`SU2Interior).isPrefixOf name && info.isTheorem && !name.isInternal then
        let axioms ← Lean.collectAxioms name
        for ax in axioms do
          unless #[`propext, `Classical.choice, `Quot.sound].contains ax do
            throwError "Unapproved axiom {ax} in {name}"
        let record := Json.mkObj [("name",toJson name.toString),
          ("type",toJson (toString (← liftTermElabM (Lean.Meta.ppExpr info.type)))),
          ("axioms",toJson ((axioms.map Name.toString).qsort (· < ·)))]
        logInfo m!"DECL {record.compress}"
  -- One shared visited set audits the UNION of all transitive closures without
  -- repeatedly traversing the same imported dependency for every generated helper.
  let collect := do
    for name in roots do Lean.CollectAxioms.collect name
  let (_, closure) := ((collect.run env).run {})
  for ax in closure.axioms do
    unless #[`propext, `Classical.choice, `Quot.sound].contains ax do
      throwError "Unapproved axiom {ax} in connected dependency closure"
  if proofs == 0 then throwError "No connected theorems audited"
  logInfo m!"AUDIT logical={logical} proofs={proofs} transitiveConstants={closure.visited.size} axioms={closure.axioms} internalCompilerRuntime={internalRuntime}; all proof dependencies checked"
