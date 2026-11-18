"""Execute and seal the phase-D identity gates using the qualified phase-C adapters."""
from pathlib import Path
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'phase_C'))
import run_cut_step as cut


def bind(name, previous, build, workspace, build_dir):
    """Bind the existing tests and exact references to this build and stage."""
    workspace.mkdir(parents=True, exist_ok=False)
    build_dir.mkdir(exist_ok=False)
    source = cut.PHASE / 'M27/validation'
    names = ('run_g1_570.py', 'run_g2_gq_prefix_570.py', 'validate_g2_gq.py',
             'run_g3_570.py', 'validate_end_b.py', 'launch_g3.py', 'run_g4_570.py')
    for filename in names:
        replaced = (source / filename).read_text().replace('M27/validation', f'{name}/validation')
        replaced = replaced.replace('570', str(build)).replace('N17_G3', f'{name}_G3')
        target = workspace / filename.replace('570', str(build))
        compile(replaced, str(target), 'exec')
        target.write_text(replaced)
    (workspace / 'run_g2_gq_prefix.py').symlink_to(f'run_g2_gq_prefix_{build}.py')
    prev = cut.PHASE / f'build{previous}'
    pins = cut.read(prev / 'source_pins.json')
    changed = subprocess.check_output(['git', 'diff', '--name-only'], text=True).splitlines()
    production = [path for path in changed if path.startswith('Common/src/adaptation/')]
    assert production
    for path in production:
        full = cut.REPO / path
        pins[str(full)] = cut.sha(full)
    all_paths = subprocess.check_output(['git','ls-files','--others','--exclude-standard',
                                         'Common/src/adaptation/native_engine3d'], text=True).splitlines()
    for path in all_paths:
        if path.endswith(('.hpp','.cpp')):
            production.append(path)
            full = cut.REPO / path
            pins[str(full)] = cut.sha(full)
    # The committed work-order amendment changes a provenance pin outside the C++ inputs.
    order = cut.REPO / 'AGENTS.md'
    committed = subprocess.check_output(['git','show','HEAD:AGENTS.md'])
    assert order.read_bytes() == committed
    pins[str(order)] = cut.sha(order)
    differences = [path for path,digest in pins.items() if cut.sha(path) != digest]
    assert not differences,differences
    cut.save(build_dir / 'source_pins.json', pins)
    cut.save(workspace / 'candidate_source.json', {
        'base_commit': subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip(),
        'source_paths': production, 'build': build,
    })
    (build_dir / 'build.py').write_text((cut.PHASE/'build570/build.py').read_text().replace('570',str(build)))
    aliases = (cut.PHASE/'build570_aliases.py').read_text().replace('570',str(build))
    (cut.PHASE/f'build{build}_aliases.py').write_text(aliases.replace('M27/validation',f'{name}/validation'))
    for prefix in ('run_g1_', 'run_g2_gq_prefix_', 'run_g3_'):
        subprocess.run([cut.PYTHON,str(workspace/f'{prefix}{build}.py'),'--self-check'],check=True)


def seal(name, step, build, workspace, build_dir):
    """Save passed gates and a scoped list of evidence for the checkpoint commit."""
    summary = {'state':'PASSED','build':build,
               'source_pins':len(cut.read(build_dir/'source_pins.json'))}
    paths = list(build_dir.glob('*.json')) + [build_dir/'build.py',cut.PHASE/f'build{build}_aliases.py']
    summary['G1'] = {}
    for cohort in ('serial','mpi2','mpi4'):
        result = cut.read(workspace / f'G1_candidate_{cohort}_summary.json')
        attempt = Path(result['candidate_attempt'])
        assert result['identity_pass'] and result['changed_case_count']==0
        summary['G1'][cohort] = {'records':result['expected_cases_per_rank'] * result['ranks'],
                                 'elapsed_seconds':result['elapsed_wall_seconds']}
        paths += [attempt / file for file in ('status.json','command.json','n11_terminal_receipt.json')]
        paths += [cut.pack(attempt / file) for file in ('run.log','resources.jsonl')]
    for gate in ('G2','GQ'):
        summary[gate], artifacts = cut.seal_prefix(workspace, step, build, gate)
        paths += artifacts
    case, = (workspace/'cases').glob(f'{name}_G3_*')
    summary['G3'] = cut.read(case/'g3_end_b_gate.json')
    assert summary['G3']['passed']
    reference = cut.NIGHT/'N14/section6_12/validation/n14_g3_first_gate.json'
    subprocess.run([cut.PYTHON,str(cut.PHASE/'phase_C/report_g3_integers.py'),
                    str(reference), str(case),'--output',str(workspace/'G3_integer_report.json')],check=True)
    paths += list(case.glob('*.json')) + [case/'run.cfg']
    paths += [cut.pack(case/file) for file in ('run.log','resources.jsonl')]
    if step == 11:
        summary['G4'] = cut.read(workspace/'g4_case_paths.json')
        assert summary['G4']['passed']
        for row in summary['G4']['new_cases']:
            case = Path(row['case'])
            paths += list(case.glob('*.json'))
            paths += [cut.pack(case/file) for file in ('run.log','resources.jsonl')]
    assert all(cut.sha(path)==digest for path,digest in cut.read(build_dir/'source_pins.json').items())
    cut.save(workspace/'acceptance_summary.json',summary)
    paths += list(workspace.glob('*.json'))+list(workspace.glob('*.py'))
    cut.save(workspace/'checkpoint_paths.json',[str(path) for path in sorted(set(paths))])


def main():
    name, previous, build = sys.argv[1:]
    assert name in ('D1','D2') and Path('/proc/1/comm').read_text().strip()=='systemd'
    previous, build = int(previous), int(build)
    step = 10 if name=='D1' else 11
    workspace = cut.PHASE/name/'validation'
    build_dir = cut.PHASE/f'build{build}'
    bind(name,previous,build,workspace,build_dir)
    cut.qualify(step,build,workspace,build_dir)
    seal(name,step,build,workspace,build_dir)
    print(f'{name} scoped identity gates passed on build{build}.',flush=True)


if __name__=='__main__':
    main()
