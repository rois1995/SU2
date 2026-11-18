"""Generate a source-preserving Remesh draft from the audited continuation boundaries."""
from pathlib import Path
import hashlib
import json
import re
import subprocess
from textwrap import dedent, indent

ROOT = Path(__file__).resolve().parent / 'drafts'
SOURCE = Path('Common/src/adaptation/CNativeRemesher3D.cpp')
PRIVATE = Path('Common/src/adaptation/native_engine3d')
base = SOURCE.read_text()
lines = base.splitlines()
assert len(lines) == 1126 and lines[200].startswith('CRemeshResult CNativeRemesher3D::Remesh')


def span(first, last):
    return dedent('\n'.join(lines[first-1:last]))


def bind(body):
    renames = {'check_job_memory': 'CheckJobMemory', 'remaining_time': 'RemainingTime',
               'phase': 'Phase', 'vote': 'Vote', 'admit': 'Admit', 'partition_bytes': 'PartitionBytes'}
    # Rename only the moved calls, not prose, string constants or qualified calls.
    lex = re.compile(r'//[^\n]*|"(?:\\.|[^"\\])*"|\b(?:' + '|'.join(renames) + r')(?=\s*\()')
    body = lex.sub(lambda m: renames.get(m[0],m[0]) if body[max(0,m.start()-1):m.start()] not in ('.',':')
                   else m[0],body)
    return body.replace('CURRENT_FUNCTION','caller')


def block(first, last, *, early_return=None):
    text = span(first,last)
    if early_return is not None:
        text = re.sub(r'\bbreak\s*;', early_return, text)
    return bind(text)


def method(name, body):
    return f'{name} {{\n{indent(dedent(body).strip(),"  ")}\n}}\n'


def portion(name, first, last, prefix='', suffix='', **kwargs):
    return method(name, prefix + '\n' + block(first,last,**kwargs) + '\n' + suffix)


def aliases(stage='', mesh='', worker=''):
    mapping={
        'failure':'preparation.failure','pending':'preparation.pending','retained':'preparation.retained',
        'normalizer':'input.normalizer','imported':'input.imported','composition':'input.composition',
        'frozen_core':'input.frozen_core','base_caller':'input.base_caller',
        'working':'mesh.working','migrated':'mesh.migrated','worker_source':'mesh.worker_source',
        'repartition':'mesh.repartition','ready':'mesh.ready',
        'workers':'trial.workers','kernel':'trial.kernel','worker_seconds':'trial.worker_seconds',
        'worker_start':'trial.worker_start','valid':'trial.valid','detail':'trial.detail',
        'stage_source':'trial.stage_source','begin':'trial.begin',
    }
    a=[]
    for name in (stage+' '+mesh+' '+worker).split():
        a.append(f'auto& {name} = {mapping[name]};')
    return '\n'.join(a)


def held(text):
    return text.replace('held()', 'Held(preparation)')


def insert(name, body, context=''):
    return method(name, held(context + '\n' + body))


members = [
    portion('double RemainingTime()',232,233),
    portion('void Phase(size_t i)',237,239),
    portion('bool Vote(bool valid, const std::string& message)',242,246),
    portion('bool CheckJobMemory(size_t peak, const char* message)',249,255),
    portion('bool Admit(size_t bytes)',258,260),
    portion('size_t PartitionBytes()',263,267),
    method('size_t Held(const ReferencePreparation& preparation)', '''
return transfer_memory::Add(preparation.retained, ReferenceStorageBytes(preparation.pending->state), PartitionBytes());
'''),
    method('bool WithinTime() const', '''
return !control.maximum_seconds ||
       std::chrono::duration<double>(std::chrono::steady_clock::now() - deadline_start).count() <
           control.maximum_seconds;
'''),
    portion('void Run()',275,278,suffix='''
TryRemesh();
Finalize();
ReportRemesh(config, control, parent, stats, reason, target_event_budget);
'''),
    portion('void TryRemesh()',280,305,suffix='PrepareReference(failure);',early_return='return;'),
    portion('void PrepareReference(CLocalFailure& failure)',306,333,suffix='Import(failure, pending);',
            early_return='return;'),
]
members[-2] = members[-2].replace('CheckSupport(config, geometry);', 'CNativeRemesher3D::CheckSupport(config, geometry);')
assert 'CNativeRemesher3D::CheckSupport' in members[-2]
import_body = block(334,335) + '\nReferencePreparation preparation{failure, pending, retained};\n'
import_body += block(339,372,early_return='return;') + '''
if (!PromoteReference(pending)) return;
CreateTarget(preparation, normalizer, base_caller, imported, ready);
'''
members.append(method('void Import(CLocalFailure& failure, std::shared_ptr<Publication>& pending)', import_body))
members.append(portion('bool PromoteReference(const std::shared_ptr<Publication>& pending)',373,393,
                       suffix='return true;',early_return='return false;'))
members.append(insert('void CreateTarget(ReferencePreparation& preparation, NormalizationStage3D& normalizer, '
                      'size_t base_caller, ImportedGeometry& imported, bool& ready)',
                      block(394,423,early_return='return;') + '''
RemeshInput input{normalizer, imported, composition, frozen_core, base_caller};
LeaseTarget(preparation, input, ready);
''', aliases('failure pending retained')))
lease = block(424,444,early_return='return;')
lease = lease.replace(
    'target_event_budget.within_time=[&](){return !control.maximum_seconds || std::chrono::duration<double>(std::chrono::steady_clock::now()-deadline_start).count()<control.maximum_seconds;};',
    'target_event_budget.within_time = [this] { return WithinTime(); };')
members.append(insert('void LeaseTarget(ReferencePreparation& preparation, RemeshInput& input, bool& ready)',
                      lease + '\nAdaptImported(preparation, input, ready);', aliases('failure imported')))
members.append(insert('void AdaptImported(ReferencePreparation& preparation, RemeshInput& input, bool& ready)',
                      block(445,448) + '''
WorkerMesh mesh{working, migrated, worker_source, repartition, ready};
Partition(preparation, input, mesh);
// Index and weight teardown is part of source preparation.
Phase(3);
std::unique_ptr<DonorImport> return_source;
RunStages(preparation, input, mesh, return_source);
FinalizeMesh(preparation, input, mesh, return_source);
''', aliases('imported')))
members.append(insert('void MigrateSource(RemeshInput& input, WorkerMesh& mesh, size_t baseline)',
                      block(450,481), aliases('imported','migrated worker_source ready')))
members.append(insert('void Partition(const ReferencePreparation& preparation, RemeshInput& input, WorkerMesh& mesh)',
                      block(484,493) + '''
PrepareWeights(preparation, input, mesh, source, weights, geometry_weights);
Phase(4);
RepartitionMesh(preparation, input, mesh, source, weights, geometry_weights);
''' + block(530,541).replace('migrate_source(baseline)', 'MigrateSource(input, mesh, baseline)'),
                      aliases('imported','working repartition ready')))
members.append(insert('void PrepareWeights(const ReferencePreparation& preparation, RemeshInput& input, WorkerMesh& mesh, '
                      'DonorImport& source, std::vector<uint32_t>& weights, std::vector<uint32_t>& geometry_weights)',
                      block(494,507), aliases('pending imported composition','working repartition ready')))
members.append(insert('void RepartitionMesh(const ReferencePreparation& preparation, RemeshInput& input, WorkerMesh& mesh, '
                      'DonorImport& source, const std::vector<uint32_t>& weights, const std::vector<uint32_t>& geometry_weights)',
                      block(509,529), aliases('imported','working repartition ready')))

stages = block(546,547) + '''
  if (ready) RunWorkers(preparation, input, mesh);
''' + indent(block(733,736),'  ') + '''
  std::shared_ptr<const PreparedComplexity3D> prepared;
  Reassess(preparation, input, mesh, return_source, prepared);
''' + indent(block(758,764),'  ') + '''
  if (!Normalize(preparation, input, mesh, return_source, prepared)) break;
}
'''
members.append(insert('void RunStages(ReferencePreparation& preparation, RemeshInput& input, WorkerMesh& mesh, '
                      'std::unique_ptr<DonorImport>& return_source)',stages,aliases('normalizer','ready')))
members.append(insert('void RunWorkers(ReferencePreparation& preparation, RemeshInput& input, WorkerMesh& mesh)',
                      block(549,562)+'''
if (parent.rank < stats.workers && ready)
  AdaptWorker(preparation, input, mesh, comm, idle_peak, worker_seconds, valid, detail);
JoinWorkers(comm, valid, detail);
''' + block(719,731),aliases('pending imported','working migrated ready')))
members.append(portion('void JoinWorkers([[maybe_unused]] SU2_Comm& comm, bool& valid, std::string& detail)',693,718))

adapt_worker = block(564,579) + indent('''
WorkerTrial trial{workers, kernel, worker_seconds, worker_start, valid, detail, stage_source, begin};
valid = SnapshotSource(mesh, trial, *worker_source);
if (valid && construction) ConstructRows(preparation, input, mesh, trial);
if (valid && construction) valid = SnapshotSource(mesh, trial, stage_source);
''','  ') + indent(block(656,659),'  ') + '\n' + indent('if (valid) ApplyMetric(preparation, input, mesh, trial);','  ') + '\n' + block(683,691)
members.append(insert('void AdaptWorker(ReferencePreparation& preparation, RemeshInput& input, WorkerMesh& mesh, '
                      'SU2_Comm comm, uint64_t idle_peak, double worker_seconds, bool& valid, std::string& detail)',
                      adapt_worker,aliases('imported','worker_source')))
members.append(insert('bool SnapshotSource(WorkerMesh& mesh, WorkerTrial& trial, const std::vector<DonorCell>& supplied)',
                      block(581,603),aliases('','working','workers kernel detail stage_source')))
members.append(insert('void ConstructRows(ReferencePreparation& preparation, RemeshInput& input, WorkerMesh& mesh, '
                      'WorkerTrial& trial)', block(607,616) + '\nAdjustPublicationCeiling(preparation, input, mesh, trial);\n'
                      + block(643,653), aliases('pending composition','working','workers kernel worker_seconds worker_start valid detail stage_source begin')))
members.append(insert('void AdjustPublicationCeiling(const ReferencePreparation& preparation, RemeshInput& input, '
                      'WorkerMesh& mesh, WorkerTrial& trial)', block(617,642), aliases('imported','working migrated','valid detail')))
members.append(insert('void ApplyMetric(ReferencePreparation& preparation, RemeshInput& input, WorkerMesh& mesh, '
                      'WorkerTrial& trial)', block(661,681),
                      aliases('pending composition','working','workers kernel worker_seconds worker_start valid detail stage_source')))

within = 'auto within=[&](){return !control.maximum_seconds || std::chrono::duration<double>(std::chrono::steady_clock::now()-deadline_start).count()<control.maximum_seconds;};'
members.append(insert('void Reassess(const ReferencePreparation& preparation, RemeshInput& input, WorkerMesh& mesh, '
                      'std::unique_ptr<DonorImport>& return_source, std::shared_ptr<const PreparedComplexity3D>& prepared)',
                      block(740,757).replace(within,'auto within = [this] { return WithinTime(); };'),
                      aliases('pending imported composition normalizer','working migrated ready')))
normalize = block(765,769) + '''
if (!ProposeCorrection(preparation, input, mesh, prepared, proposal, nodal_correction,
                       correction_source, normalization_live))
  return false;
return ApplyCorrection(preparation, input, mesh, return_source, prepared, proposal, nodal_correction);
'''
members.append(insert('bool Normalize(ReferencePreparation& preparation, RemeshInput& input, WorkerMesh& mesh, '
                      'std::unique_ptr<DonorImport>& return_source, std::shared_ptr<const PreparedComplexity3D>& prepared)',
                      normalize, aliases('imported','working migrated')))
members.append(insert('bool ProposeCorrection(ReferencePreparation& preparation, RemeshInput& input, WorkerMesh& mesh, '
                      'const std::shared_ptr<const PreparedComplexity3D>& prepared, NormalizationProposal3D& proposal, '
                      'bool nodal_correction, std::vector<DonorCell>& correction_source, size_t normalization_live)',
                      block(770,797,early_return='return false;').replace(within,'auto within = [this] { return WithinTime(); };')
                      + '\nreturn true;', aliases('failure normalizer frozen_core imported','ready working')))
members.append(insert('bool ApplyCorrection(ReferencePreparation& preparation, RemeshInput& input, WorkerMesh& mesh, '
                      'std::unique_ptr<DonorImport>& return_source, std::shared_ptr<const PreparedComplexity3D>& prepared, '
                      'NormalizationProposal3D& proposal, bool nodal_correction)', block(798,828,early_return='return false;')
                      .replace('migrate_source(', 'MigrateSource(input, mesh, ') + '\nreturn true;',
                      aliases('failure pending retained imported base_caller composition frozen_core','ready working migrated repartition')))
members.append(insert('void FinalizeMesh(ReferencePreparation& preparation, RemeshInput& input, WorkerMesh& mesh, '
                      'std::unique_ptr<DonorImport>& return_source)', block(830,875)
                      .replace('[deadline_start,duration]', '[deadline_start = deadline_start, duration]'),
                      aliases('pending imported','ready working migrated')))
members.append(portion('void Finalize()',877,900))


prefix = '''
// Remesh owns the timer and public transaction. These stages borrow its controls and live inputs.
namespace {
struct Publication {
  ReferenceState state;
  bool published = false;
};

// Shared geometry ownership is counted once even while pending and live state both refer to it.
struct ReferencePreparation {
  CLocalFailure& failure;
  std::shared_ptr<Publication>& pending;
  size_t& retained;
};

// Import and target lifetimes span the worker run and return packing.
struct RemeshInput {
  NormalizationStage3D& normalizer;
  ImportedGeometry& imported;
  MetricComposition& composition;
  double& frozen_core;
  size_t base_caller;
};

// All workers operate on the original working map and selected original donor source.
struct WorkerMesh {
  Owned& working;
  std::vector<DonorCell>& migrated;
  const std::vector<DonorCell>*& worker_source;
  bool repartition;
  bool& ready;
};

// Borrows a single worker trial, including its private policy and deadline accounting.
struct WorkerTrial {
  World& workers;
  EngineOptions& kernel;
  double worker_seconds;
  double worker_start;
  bool& valid;
  std::string& detail;
  std::vector<DonorCell>& stage_source;
  double begin;
};

// Executes one remesh while the caller's parent communicator and remesh timer stay alive.
struct RemeshRun {
  const CConfig& config;
  const CGeometry& geometry;
  const su2activematrix& sensor;
  const std::shared_ptr<ReferenceState>& reference;
  const EngineOptions& options;
  const CNativeRemesher3D::CompositionFactory& compositionFactory;
  const CNativeRemesher3D::NormalizationFactory& normalizationFactory;
  const CNativeRemesher3D::ConstructionFactory& construction;
  const std::function<void()>& constructionAccepted;
  RemeshStats& stats;
  World& parent;
  CRemeshResult& result;
  EngineOptions& control;
  double start;
  std::clock_t cpu;
  double& now;
  std::chrono::steady_clock::time_point deadline_start;
  const char* caller;
  std::string reason{};
  SU2NativeGeometry::AtlasBudget target_event_budget{};
  bool reset_need_at_start = false;

'''


def render():
    out=ROOT/'remesh'
    out.mkdir(parents=True,exist_ok=True)
    report=(ROOT/'RemeshReporting.inc').read_text()
    start='const RemeshStats& stats,'
    assert report.count('void ReportRemesh(') == 1
    head, tail=report.split('void ReportRemesh(',1)
    tail=tail.replace(start,'RemeshStats& stats,',1)
    brace=tail.index('{')
    tail=tail[:brace+1]+'\n  const auto report_started = parent.seconds();'+tail[brace+1:]
    end=tail.index('\n}',brace)
    tail=tail[:end]+'\n  stats.report_seconds = parent.seconds() - report_started;'+tail[end:]
    report=head+'void ReportRemesh('+tail
    intents = {
        'RemainingTime': 'Agrees the remaining parent-event time from the original deadline origin.',
        'Phase': 'Charges the preceding interval to the selected remesh phase.',
        'Vote': 'Elects the synchronous failure and stores its reason for the event.',
        'CheckJobMemory': 'Elects the whole-job memory refusal at the existing safe point.',
        'Admit': 'Records the peak modeled bytes before voting on the current allowance.',
        'PartitionBytes': 'Accounts for the retained partition vectors used throughout the event.',
        'Held': 'Accounts for the shared authority once while both reference states remain live.',
        'WithinTime': 'Reads the same frozen deadline for the local geometry and metric queries.',
        'Run': 'Owns the event geometry ledger through final reporting, including failed transactions.',
        'TryRemesh': 'Agrees event controls before making any private reference changes.',
        'PrepareReference': 'Loads and validates the pending reference before geometry import.',
        'Import': 'Retains the pending mobility and normalization state through the private mesh stages.',
        'PromoteReference': 'Uses one authority kind on all parent ranks before staging first-row work.',
        'CreateTarget': 'Composes the initial target from the imported frozen sensor and wall policy.',
        'LeaseTarget': 'Keeps the geometry workspace reserved through packing and private teardown.',
        'AdaptImported': 'Owns the working mesh and donors until the remesh result is packed.',
        'MigrateSource': 'Moves immutable donors under the existing migration storage vote.',
        'Partition': 'Keeps the donor index and weights alive through partition and migration.',
        'PrepareWeights': 'Measures routing work only when more than one destination can use it.',
        'RepartitionMesh': 'Changes working-cell owners and emits the corresponding partition row.',
        'RunStages': 'Runs worker adaptation, then at most one agreed normalization correction.',
        'RunWorkers': 'Keeps the communicator alive through worker activity and the parent join.',
        'JoinWorkers': 'Waits for all ranks before measuring the peak and freeing the worker communicator.',
        'AdaptWorker': 'Keeps private shape admission and donor state through both worker stages.',
        'SnapshotSource': 'Replaces the private stage donor snapshot only after the existing vote.',
        'ConstructRows': 'Accepts first-row cells provisionally and charges their shared work.',
        'AdjustPublicationCeiling': 'Accounts for first-row multiplication under the existing automatic cap.',
        'ApplyMetric': 'Uses the remaining worker work and time budgets for strict metric adaptation.',
        'Reassess': 'Reuses the parent donor index for the correction proposal and later return packing.',
        'Normalize': 'Keeps the correction source alive through both the proposal and application.',
        'ProposeCorrection': 'Agrees the proposal while the original metric timers and source are live.',
        'ApplyCorrection': 'Releases assessment indexes before changing the current nodal field.',
        'FinalizeMesh': 'Packs the private result and binds publication to CFD acceptance.',
        'Finalize': 'Accounts for attempted work and applies the last whole-job memory check.',
    }
    documented=[]
    for text in members:
        name=re.search(r'\b([A-Z][A-Za-z]+)\(',text).group(1)
        documented.append('// '+intents[name]+'\n'+text)
    files={PRIVATE/'NativeRemeshRun.inc':prefix + indent('\n'.join(documented),'  ') + '};\n}  // namespace\n',
           PRIVATE/'NativeRemeshReporting.inc':report}
    main='\n'.join(lines[:200])+'\n#include "native_engine3d/NativeRemeshReporting.inc"\n'
    main+='#include "native_engine3d/NativeRemeshRun.inc"\n\n'+'\n'.join(lines[200:230])+'''
  RemeshRun run{config, geometry, sensor, reference, options, compositionFactory, normalizationFactory,
                construction, constructionAccepted, stats, parent, result, control, start, cpu, now,
                deadline_start, CURRENT_FUNCTION};
  run.Run();
  return result;
}
'''
    files[SOURCE]=main
    manifest={}
    for path, content in files.items():
        target=out/path.name
        assumed=str(path.resolve())
        if path!=SOURCE:
            content=subprocess.check_output(['clang-format-18','--assume-filename='+assumed], input=content.encode()).decode()
        else:
            start=content.index('CRemeshResult CNativeRemesher3D::Remesh')
            line=content[:start].count('\n')+1
            content=subprocess.check_output(['clang-format-18','--assume-filename='+assumed,
                         '--lines='+str(line)+':'+str(content.count('\n'))],input=content.encode()).decode()
        target.write_text(content)
        manifest[str(path)]={'base':hashlib.sha256(path.read_bytes()).hexdigest() if path.exists() else None,
                             'candidate':hashlib.sha256(content.encode()).hexdigest()}
    (out/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')


if __name__ == '__main__':
    render()
