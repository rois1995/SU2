"""Extract contiguous owner scopes from the accepted phase-C Adapt implementation."""
from pathlib import Path
import re
import subprocess

here = Path(__file__).resolve().parent
source = Path('Common/src/adaptation/CNativeEngine3D.cpp')
base = source.read_text()
lines = base.splitlines(keepends=True)


def span(first, last):
    return ''.join(lines[first - 1:last])


assert 'EngineResult Adapt(' in span(1524,1526)
assert span(2220,2220) == '}\n'
decl = re.compile(r'^[ \t]*(.*?&)[ \t]*(\w+) = (\w+)_lifetime.Get\(\);([^\n]*)\n', re.M)
owners = dict(re.findall(r'ContextValue\s+(\w+)_lifetime\(\s*([\w.]+)',span(1527,2218)))
map_owner = lambda target: target.replace('setup.control_prefix','context.setup.control_prefix').replace('control.','context.control.',1) if target.startswith(('setup.','control.')) else target
aliases = {}
for kind, name, lifetime, _ in decl.findall(span(1527,2218)):
    aliases[name] = f'{kind.strip()} {name} = *{map_owner(owners[lifetime])};'
for name in ('world','geometry','accepted','original','composition','options','intent','bidirectional','surface_target'):
    aliases[name] = f'auto& {name} = context.{name};'
aliases.update(control='auto& control = context.control;', stats='auto& stats = context.control.result->stats;',
               wall_counters='auto& wall_counters = *context.setup.wall_counters;',
               lane_counters='auto& lane_counters = *context.setup.lane_counters;',
               round='auto& round = round_state.round;', slot='auto& slot = slot_state.slot;')

mask = re.compile(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'', re.S)


def used(name, code):
    return bool(re.search(rf'(?<![.\w>]){name}\b', mask.sub(' ',code)))


def drop_unused_aliases(code):
    for match in reversed(list(decl.finditer(code))):
        if match[4].strip() and not match[4].lstrip().startswith('//'):
            continue
        if not used(match[2],code[:match.start()]+code[match.end():]):
            code=code.replace(match[0],'',1)
    return code


def referenced_aliases(code, signature):
    code=mask.sub(' ',code)
    own={m[2] for m in decl.finditer(code)}
    own.update(re.findall(r'\b(?:auto|size_t|bool|std::optional<[^>]+>)\s*&?\s+(\w+)\s*=',code))
    own.update(re.findall(r'\bCLocalFailure\s+(\w+)\s*;',code))
    return [alias for name,alias in aliases.items() if name not in own and used(name,code)
            and all(root not in alias or root in signature for root in ('slot_state','round_state'))]


def replace_exact(text, before, after):
    assert text.count(before)==1,(before,text.count(before))
    return text.replace(before,after,1)


parts=[]
finish='EngineControl::Finish(context, curved, working, atlas_budget, started, started_cpu)'


def method(result, signature, comment, code, tail='', template='', replacements=None):
    code=code+tail
    code=code.replace(finish,'Finish()')
    for old,new in (replacements or {}).items():
        code=replace_exact(code,old,new)
    code=drop_unused_aliases(code)
    bindings=referenced_aliases(code,signature)
    parts.append(f'// {comment}\n'+(template+'\n' if template else '')+
                 f'{result} {signature} {{\n'+'\n'.join(bindings)+'\n'+code+'\n}\n')


setup = span(1582,1607).replace('return rejected;','return rejected;')
parts.append('')
method('EngineResult','Run()', 'Binds the control and census owners before frontier setup.',
       span(1611,1657),'return InitializeFrontier();\n',
       replacements={'const bool curved=!geometry.ExtensionComponents().empty();':'curved = !geometry.ExtensionComponents().empty();'})
method('EngineResult','InitializeFrontier()', 'Owns the directory, source and frontier state through the event.',
       span(1659,1718),'return InitializeObservers();\n',
       replacements={'  constexpr size_t diagnostic_rejection_limit=64;\n':''})
method('EngineResult','InitializeObservers()', 'Keeps the original observer votes and quality ledger alive.',
       span(1719,1758),'return InitializeWorkingMesh();\n',
       replacements={';\n;\n  ContextValue transaction_captured_lifetime':'  ContextValue transaction_captured_lifetime'})
method('EngineResult','InitializeWorkingMesh()', 'Admits observation and creates the initial mesh and indexes.',
       span(1760,1786),'return InitializeRegion(setup_finish);\n')
method('EngineResult','InitializeRegion(SetupFinish& setup_finish)', 'Runs the initial region and first-row stages.',
       span(1787,1803),'return SeedAndRun(setup_finish);\n','template <class SetupFinish>')
method('EngineResult','SeedAndRun(SetupFinish& setup_finish)', 'Keeps the ownership state through seeding and rounds.',
       span(1804,1820),'return RunRounds(setup_finish);\n','template <class SetupFinish>')
method('EngineResult','RunRounds(SetupFinish& setup_finish)', 'Runs successive rounds and certifies after their state is destroyed.',
       '''for (RoundState round_state;;) {
         const auto decision = RunRound(round_state, setup_finish);
         if (decision == RoundDecision::Finish) return std::move(*finished);
         if (decision == RoundDecision::Break) break;
       }
'''+span(2203,2219),template='template <class SetupFinish>')
method('RoundDecision','RunRound(RoundState& round_state, SetupFinish& setup_finish)',
       'Keeps the phase and lane scopes active through the full round.',
       span(1822,1846)+'''auto pick = [&] { return PickRound(round_state); };
       SelectRound(round_state);
       if (!*context.control.ready) return Done(Finish());
       RoundHead round_head(context, round_state, atlas_budget, curved);
       const auto decision = round_head.Decide(pick);
       if (decision == RoundDecision::Finish) return Done(std::move(*round_head.finished));
       if (decision != RoundDecision::Proceed) return decision;
       return ReserveHistoryAndAdvance(round_state);
''',template='template <class SetupFinish>',
       replacements={'return std::move(*prepared);':'return Done(std::move(*prepared));'})
method('bool','PickRound(RoundState& round_state)', 'Selects the next candidate with the existing vote.',
       span(1848,1855))
method('void','SelectRound(RoundState& round_state)', 'Measures pending census and refills within the scheduling timer.',
       span(1857,1879),replacements={'        pick();':'        PickRound(round_state);'})
method('RoundDecision','ReserveHistoryAndAdvance(RoundState& round_state)',
       'Admits the history and advances the epoch before acquiring dependencies.',
       span(1886,1908)+'return AcquireRound(round_state);\n')
parts[-1]=parts[-1].replace('return Finish();','return Done(Finish());')
method('RoundDecision','AcquireRound(RoundState& round_state)',
       'Keeps acquisition and its reserved containers alive through slot publication.',
       span(1909,1945),'return PreparePrivateRound(round_state, acquisition);\n',
       replacements={'return std::move(*acquisition.finished);':'return Done(std::move(*acquisition.finished));'})
method('RoundDecision','PreparePrivateRound(RoundState& round_state, RoundAcquisition& acquisition)',
       'Allocates private-round state at the original admission points.',
       span(1947,1978),'return RunSlots(round_state, acquisition);\n')
parts[-1]=parts[-1].replace('return Finish();','return Done(Finish());')
method('RoundDecision','RunSlots(RoundState& round_state, RoundAcquisition& acquisition)',
       'Visits each slot and joins the repair batch after every slot has unwound.',
       span(1979,1979)+'''const auto decision = RunSlot(round_state, slot_state, acquisition);
       if (decision == RoundDecision::Finish) return decision;
       if (decision == RoundDecision::Break) break;
       }
'''+span(2197,2201)+'return RoundDecision::Proceed;\n',
       replacements={'return std::move(*batch_commit.finished);':'return Done(std::move(*batch_commit.finished));'})
method('RoundDecision','RunSlot(RoundState& round_state, SlotState& slot_state, RoundAcquisition& acquisition)',
       'Owns the acquired payload and diagnostics through the slot callback.',
       span(1980,2002),
       'return PrepareCandidate(round_state, slot_state, wall_choice, reserve_dependencies);\n',
       replacements={'if (slot_decision == SlotDecision::Next) continue;':
                         'if (slot_decision == SlotDecision::Next) return RoundDecision::Proceed;',
                     'return std::move(*slot_acquisition.finished);':
                         'return Done(std::move(*slot_acquisition.finished));'})
method('RoundDecision','PrepareCandidate(RoundState& round_state, SlotState& slot_state, wall_diagnostics::Choice& wall_choice, Reserve& reserve_dependencies)',
       'Owns trial communication, patches and failure state through the remaining slot.',
       span(2003,2048),'return PrepareIdentifiers(round_state, slot_state, wall_choice, reserve_dependencies, trial_comm, trial_checks);\n',
       'template <class Reserve>')
method('RoundDecision','PrepareIdentifiers(RoundState& round_state, SlotState& slot_state, wall_diagnostics::Choice& wall_choice, Reserve& reserve_dependencies, TrialComm& trial_comm, TrialChecks& trial_checks)',
       'Binds IDs and preserves the active cavity through evaluation and commit.',
       span(2049,2079)+'''auto observe_transaction = [&](const char* stage, bool passed, const DonorPatch* donors = nullptr) {
         ObserveSlotTransaction(round_state, slot_state, stage, passed, donors);
       };
'''+span(2085,2092),
       'return RunSlotTrial(round_state, slot_state, wall_choice, reserve_dependencies, trial_comm, trial_checks, observe_transaction);\n',
       'template <class Reserve>')
method('void','ObserveSlotTransaction(RoundState& round_state, SlotState& slot_state, const char* stage, bool passed, const DonorPatch* donors)',
       'Passes the same live transaction payload to the observer.',
       span(2081,2083))
method('bool','BoundaryReplacementReady(SlotState& slot_state)',
       'Checks the full replacement while preserving the existing exception routing.',span(2105,2126))
method('RoundDecision','RunSlotTrial(RoundState& round_state, SlotState& slot_state, wall_diagnostics::Choice& wall_choice, Reserve& reserve_dependencies, TrialComm& trial_comm, TrialChecks& trial_checks, Observe& observe_transaction)',
       'Dispatches the existing private and collective trial paths.',
       span(2093,2103)+'''auto boundary_replacement_ready = [&] { return BoundaryReplacementReady(slot_state); };
'''+span(2128,2129)+'''if (*context.control.repairing)
       return EvaluatePrivateTrial(round_state, slot_state, candidate_trial, boundary_replacement_ready);
       return EvaluateCollectiveSlot(round_state, slot_state, candidate_trial, reserve_dependencies, observe_transaction);
''',template='template <class Reserve, class Observe>')
method('RoundDecision','EvaluatePrivateTrial(RoundState& round_state, SlotState& slot_state, Candidate& candidate_trial, BoundaryReady& boundary_replacement_ready)',
       'Runs the private candidate and publishes its proposal without a collective slot commit.',
       span(2131,2136)+'''if (evaluated) evaluated = AdmitPrivateCoverage(slot_state, boundary_replacement_ready);
'''+span(2157,2160)+'''try {
         StoreRepairProposal(round_state, slot_state);
       } catch (const std::exception& e) { private_failure.Set(2, 0, e.what()); return RoundDecision::Break; }
       return RoundDecision::Proceed;
''',template='template <class Candidate, class BoundaryReady>')
parts[-1]=parts[-1].replace('break;', 'return RoundDecision::Break;')
method('bool','AdmitPrivateCoverage(SlotState& slot_state, BoundaryReady& boundary_replacement_ready)',
       'Accounts for the retained coverage inside the original private-trial catch boundary.',
       span(2139,2155)+'return evaluated;\n',template='template <class BoundaryReady>',
       replacements={'          evaluated=admit_coverage();':'          bool evaluated=admit_coverage();'})
method('void','StoreRepairProposal(RoundState& round_state, SlotState& slot_state)',
       'Moves the live private slot into its original batch record.',span(2162,2171))
method('RoundDecision','EvaluateCollectiveSlot(RoundState& round_state, SlotState& slot_state, Candidate& candidate_trial, Reserve& reserve_dependencies, Observe& observe_transaction)',
       'Preserves the exact collective slot route and observer ordering.',
       span(2175,2195),'return RoundDecision::Proceed;\n',template='template <class Candidate, class Reserve, class Observe>',
       replacements={'return std::move(*slot_commit.finished);':'return Done(std::move(*slot_commit.finished));'})
parts[-1]=parts[-1].replace('return Finish();','return Done(Finish());')

header='''// Adapt orchestration. Every helper owns one original lifetime block through its continuation.
struct EngineRun {
  EngineContext& context;
  SU2NativeGeometry::AtlasBudget& atlas_budget;
  const SU2NativeGeometry::BoundaryCoverage& no_coverage;
  bool curved = false;
  std::optional<EngineResult> finished = std::nullopt;
  static constexpr size_t diagnostic_rejection_limit = 64;
  using Phase = EnginePhase;

  // Finalizes the result while the originating timer and cavity scopes are still active.
  EngineResult Finish() {
    return EngineControl::Finish(context, curved, *context.ownership.working, atlas_budget,
                                 *context.setup.control_prefix.started, *context.setup.control_prefix.started_cpu);
  }

  // Carries the already-produced result across exactly one round/slot control boundary.
  RoundDecision Done(EngineResult result) {
    finished.emplace(std::move(result));
    return RoundDecision::Finish;
  }
'''
prelude='''// Admits private boundary parameters before constructing the engine context.
std::optional<EngineResult> SetupBoundaryParameters(
    EngineSetup& setup, World& world, const Owned& accepted, const std::vector<DonorCell>& original,
    const PlanarReference& accepted_geometry, EngineOptions& options, SU2NativeGeometry::AtlasBudget& atlas_budget) {
  auto& private_parameters = *setup.private_parameters;
  auto& private_geometry = *setup.private_geometry;
  auto& atlas_geometry = *setup.atlas_geometry;
  auto& parameter_failure = *setup.control_prefix.parameter_failure;
  auto& parameter_bound = *setup.control_prefix.parameter_bound;
  const auto atlas_scratch = *setup.control_prefix.atlas_scratch;
'''+setup+'''return std::nullopt;
}
'''
standalone = header + '\n'.join(parts) + '};\n\n' + prelude
main=span(1524,1581)+'''  if (auto rejected = SetupBoundaryParameters(setup, world, accepted, original, accepted_geometry, options, atlas_budget))
    return std::move(*rejected);
'''+span(1608,1610)+'''  return EngineRun{context, atlas_budget, no_coverage}.Run();
}
'''
for label in ('wall_counters','lane_counters','private_parameters','private_geometry','local_atlas_budget',
              'restore_budget','atlas_geometry','target_geometry_lease'):
    match=re.search(rf'^  auto& {label} = (setup\.[\w.]+\.emplace(?:\([^\n]*\)));$',main,re.M)
    if match and not used(label,main[:match.start()]+main[match.end():]):
        main=main.replace(match[0],'  '+match[1]+';',1)
main=drop_unused_aliases(main)
new=source.parent/'native_engine3d/EngineAdapt.cpp'


def format_for(target,code):
    return subprocess.check_output(['clang-format-18','--assume-filename='+str(target.resolve())],input=code,text=True)


new_main=''.join(lines[:1523])+'#include "native_engine3d/EngineAdapt.cpp"\n\n'+format_for(source,main)+''.join(lines[2220:])
(here/source.name).write_text(new_main)
(here/new.name).write_text(format_for(new,standalone))
