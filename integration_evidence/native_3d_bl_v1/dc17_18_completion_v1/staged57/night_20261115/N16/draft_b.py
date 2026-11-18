"""Prepare the reviewed Part B source overlay while Part A's build is pinned."""
from collections import defaultdict
from pathlib import Path
import hashlib
import json

ROOT = Path(__file__).resolve().parent
NE = 'Common/src/adaptation/native_engine3d/'
ENG = 'Common/src/adaptation/CNativeEngine3D.cpp'
HEADER = 'Common/include/adaptation/CNativeEngine3D.hpp'
edits = defaultdict(list)


def replace(path, old, new, count=1):
    edits[path].append((old, new, count))


replace(HEADER, 'constexpr size_t MaximumCandidateBatch = 8;',
        '''constexpr size_t MaximumCandidateBatch = 8;
// One curved repair candidate can require four of the directory's support stars.
constexpr size_t MaximumRepairBatch = 64;
constexpr size_t MaximumRoundBatch = MaximumRepairBatch > MaximumCandidateBatch ? MaximumRepairBatch
                                                                                : MaximumCandidateBatch;''')
replace(HEADER, '  MetricSizeControls requirement_size;',
        '''  size_t repair_batch = 0;  // Candidates on one rank per repair round. Zero uses candidate_batch.
  MetricSizeControls requirement_size;''')
replace(HEADER, 'struct SplitRecord {',
        '''inline size_t RoundBatch(const EngineOptions& options, bool repairing) {
  return repairing && options.repair_batch ? options.repair_batch : options.candidate_batch;
}

inline size_t RoundCapacity(const EngineOptions& options, bool repairing) {
  return repairing && options.repair_batch ? options.repair_batch : MaximumCandidateBatch;
}

inline size_t WindowRounds(const EngineOptions& options, bool repairing) {
  const auto round_batch = RoundBatch(options, repairing);
  if (round_batch == options.candidate_batch) return options.stagnation_window;
  // Scale the window without overflowing the intermediate product.
  const auto whole = options.stagnation_window / round_batch;
  const auto remainder = options.stagnation_window % round_batch;
  return std::max<size_t>(1, transfer_memory::Add(transfer_memory::Mul(whole, options.candidate_batch),
                                                remainder * options.candidate_batch / round_batch));
}

struct SplitRecord {''')
replace(ENG, 'namespace SU2Native3D {\nAdaptationRequirements',
        'namespace SU2Native3D {\nstatic_assert(4 * MaximumRepairBatch <= MaximumDirectoryFeatures,\n'
        '              "Curved repair support must fit in one directory query.");\n\nAdaptationRequirements')
replace(NE+'EngineSetup.cpp',
        '''  const auto batch_lo = TimedAllreduce(options.candidate_batch, CPassiveComm::Op::MIN, world.comm);
  const auto batch_hi = TimedAllreduce(options.candidate_batch, CPassiveComm::Op::MAX, world.comm);
  if (batch_lo != batch_hi || !batch_hi || batch_hi > MaximumCandidateBatch)''',
        '''  const bool valid_batches = options.candidate_batch && options.candidate_batch <= MaximumCandidateBatch &&
                             options.repair_batch <= MaximumRepairBatch;
  // The sentinel lets the existing agreement pair reject any out-of-range input on every rank.
  const uint64_t batches = valid_batches
                               ? (uint64_t(options.repair_batch) << 32) | uint64_t(options.candidate_batch)
                               : UINT64_MAX;
  const auto batch_lo = TimedAllreduce(batches, CPassiveComm::Op::MIN, world.comm);
  const auto batch_hi = TimedAllreduce(batches, CPassiveComm::Op::MAX, world.comm);
  if (batch_lo != batch_hi || batch_hi == UINT64_MAX)''')
replace(NE+'EngineRound.hpp', '  size_t round = 0;',
        '''  size_t round = 0;
  size_t batch = 0;       // Agreed width of the current round.
  size_t token_base = 0;  // Sum of rank count times width for all preceding rounds.''')
replace(NE+'EngineAdapt.cpp', '    if (decision != RoundDecision::Proceed) return decision;\n',
        '    if (decision != RoundDecision::Proceed) return decision;\n'
        '    round_state.batch = RoundBatch(options, repairing);\n')
replace(NE+'EngineAdapt.cpp', '    batch_bytes = 0;\n    ++round;',
        '''    round_state.token_base =
        transfer_memory::Add(round_state.token_base, transfer_memory::Mul(context.world.size, round_state.batch));
    batch_bytes = 0;
    ++round;''')
replace(NE+'EngineAdapt.cpp',
        '''    auto& round = round_state.round;
    auto& slot = slot_state.slot;
    ContextValue epoch_lifetime(slot_state.epoch,
                                transfer_memory::Add(transfer_memory::Mul(round, world.size), world.rank));
    const size_t& epoch = epoch_lifetime.Get();
    ContextValue token_lifetime(slot_state.token,
                                transfer_memory::Add(transfer_memory::Mul(epoch, options.candidate_batch), slot));''',
        '''    auto& slot = slot_state.slot;
    ContextValue token_lifetime(
        slot_state.token,
        transfer_memory::Add(round_state.token_base, transfer_memory::Mul(world.rank, round_state.batch), slot));''')
replace(NE+'EngineSlot.hpp', '  std::optional<size_t> epoch;\n', '')
replace(NE+'EngineQueues.cpp', 'size_t repair_slots = context.options.candidate_batch;', 'size_t repair_slots = round_state.batch;')
replace(NE+'EngineQueues.cpp',
        '''bool RoundAcquisition::SetRepairSlots(CLocalFailure& failure, size_t& repair_slots) {
  auto& world = context.world;
  auto& working = *context.ownership.working;
  auto& options = context.options;''',
        '''bool RoundAcquisition::SetRepairSlots(CLocalFailure& failure, size_t& repair_slots) {
  auto& world = context.world;
  auto& working = *context.ownership.working;''')
replace(NE+'EngineQueues.cpp', 'std::ceil(options.candidate_batch * share)', 'std::ceil(round_state.batch * share)')
replace(NE+'EngineExchange.cpp',
        'bool RoundAcquisition::AdmitBatchStorage() {\n  auto& options = context.options;',
        'bool RoundAcquisition::AdmitBatchStorage() {')
replace(NE+'EngineExchange.cpp', 'options.candidate_batch', 'round_state.batch', 4)
for path,count in (('EngineRound.hpp',2),('EngineAdapt.cpp',4),('EngineQueues.hpp',1),('EngineCommit.cpp',2)):
    replace(NE+path, 'MaximumCandidateBatch', 'MaximumRoundBatch', count)
replace(NE+'EngineExchange.cpp', 'refused_receipt_count == MaximumCandidateBatch', 'refused_receipt_count == MaximumRoundBatch')
replace(NE+'EngineQueues.cpp', 'key.slot >= MaximumCandidateBatch', 'key.slot >= MaximumRoundBatch')
replace(NE+'EngineQueues.cpp', 'world.size, MaximumCandidateBatch),', 'world.size, RoundCapacity(options, repairing)),')
replace(NE+'EngineQueues.cpp', 'choices.size() > MaximumCandidateBatch', 'choices.size() > MaximumRoundBatch')
replace(NE+'EngineQueues.cpp', 'std::array<RepairReceipt, MaximumCandidateBatch>', 'std::array<RepairReceipt, MaximumRoundBatch>')
replace(NE+'EngineQueues.cpp', 'MaximumCandidateBatch + 1),', 'RoundCapacity(options, repairing) + 1),', 2)
replace(NE+'EngineQueues.cpp', 'count > MaximumCandidateBatch', 'count > MaximumRoundBatch')
replace(NE+'EngineQueues.cpp', 'receipts.size() > transfer_memory::Mul(world.size, MaximumCandidateBatch + 1)',
        'receipts.size() > transfer_memory::Mul(world.size, MaximumRoundBatch + 1)')
replace(NE+'EngineQueues.cpp', 'refused_receipt_count == MaximumCandidateBatch', 'refused_receipt_count == MaximumRoundBatch', 2)
replace(NE+'EngineControl.cpp', 'round % options.stagnation_window', 'round % WindowRounds(options, repairing)')
replace(NE+'EngineOwnership.cpp', 'stats.rounds - quality_ownership_round < options.stagnation_window',
        'stats.rounds - quality_ownership_round < WindowRounds(options, quality)')
replace(NE+'EngineCommit.cpp',
        '<< " private_work_used=" << private_work_used << " busy_seconds=" << private_busy_seconds << \'\\n\';',
        '<< " private_work_used=" << private_work_used << " busy_seconds=" << private_busy_seconds\n'
        '              << " choices=" << round_state.choices->size() << \'\\n\';')


def main():
    draft = ROOT / 'B_draft'
    draft.mkdir(exist_ok=False)
    manifest = {}
    for name, replacements in edits.items():
        source = Path(name)
        base = source.read_bytes()
        text = base.decode()
        for old, new, count in replacements:
            assert text.count(old) == count, (name, old, count, text.count(old))
            text = text.replace(old, new)
        target = draft / source
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(text)
        manifest[name] = {'base': hashlib.sha256(base).hexdigest(),
                          'draft': hashlib.sha256(target.read_bytes()).hexdigest()}
    (draft / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')


if __name__ == '__main__':
    main()
