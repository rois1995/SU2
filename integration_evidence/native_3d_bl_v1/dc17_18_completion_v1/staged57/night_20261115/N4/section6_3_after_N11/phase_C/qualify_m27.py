"""Bind the accepted C11 validation adapters to the print-only M27 build."""
from pathlib import Path
import subprocess

import run_cut_step as cut


def main():
    phase = cut.PHASE
    assert Path('/proc/1/comm').read_text().strip() == 'systemd'
    previous = phase / 'C11/validation'
    end = cut.read(previous / 'acceptance_summary.json')
    assert end['build'] == 569 and all(end[gate]['passed'] for gate in ('G3', 'G4', 'GQ'))
    workspace = phase / 'M27/validation'
    build = phase / 'build570'
    workspace.mkdir(parents=True, exist_ok=False)
    build.mkdir(exist_ok=False)
    source_paths = (
        'Common/include/adaptation/CNativeD9Profile3D.hpp',
        'Common/src/adaptation/CNativeAffineReference.cpp',
        'Common/src/adaptation/CNativeEngine3D.cpp',
        'Common/src/adaptation/native_engine3d/EngineTrial.cpp',
    )
    pins = cut.read(phase / 'build569/source_pins.json')
    for source in source_paths:
        path = str(cut.REPO / source)
        assert path in pins and cut.sha(path) != pins[path], path
        pins[path] = cut.sha(path)
    assert all(cut.sha(path) == digest for path, digest in pins.items())
    cut.save(build / 'source_pins.json', pins)
    cut.save(workspace / 'candidate_source.json', {
        'item': 'N17 M27', 'base_commit': subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip(),
        'source_paths':list(source_paths), 'build':570,
    })
    names = ('run_g1_569.py', 'run_g2_gq_prefix_569.py', 'validate_g2_gq.py',
             'run_g3_569.py', 'validate_end_b.py', 'launch_g3.py', 'run_g4_569.py')
    for name in names:
        source = (previous / name).read_text().replace('C11/validation', 'M27/validation').replace('569', '570')
        source = source.replace('C11_G3', 'N17_G3')
        target = workspace / name.replace('569', '570')
        compile(source, str(target), 'exec')
        target.write_text(source)
    (workspace / 'run_g2_gq_prefix.py').symlink_to('run_g2_gq_prefix_570.py')
    (build / 'build.py').write_text((phase / 'build569/build.py').read_text().replace('569','570'))
    alias = (phase / 'build569_aliases.py').read_text().replace('569', '570').replace('C11/validation','M27/validation')
    (phase / 'build570_aliases.py').write_text(alias)
    for name in ('run_g1_570.py','run_g2_gq_prefix_570.py','run_g3_570.py'):
        subprocess.run([cut.PYTHON,str(workspace / name),'--self-check'],check=True)
    cut.qualify(11, 570, workspace, build)
    build_log = (build / 'build.log').read_text(errors='replace')
    touched_names = tuple(Path(name).name for name in source_paths)
    warnings = [line for line in build_log.splitlines() if 'warning:' in line and any(name in line for name in touched_names)]
    assert not warnings, warnings
    summary = {'state':'PASSED', 'build':570, 'source_pins':len(pins), 'new_warnings_in_touched_files':warnings}
    summary['G1'] = {
        cohort: cut.read(workspace / f'G1_candidate_{cohort}_summary.json')
        for cohort in ('serial','mpi2','mpi4')
    }
    for gate in ('G2','GQ'):
        summary[gate], _ = cut.seal_prefix(workspace,11,570,gate)
    case, = (workspace / 'cases').glob('N17_G3_*')
    summary['G3'] = cut.read(case / 'g3_end_b_gate.json')
    summary['G4'] = cut.read(workspace / 'g4_case_paths.json')
    assert all(summary[gate]['passed'] for gate in ('G2','G3','G4','GQ'))
    cut.save(workspace / 'acceptance_summary.json', summary)
    print('M27 scoped gates passed on build570.',flush=True)


if __name__ == '__main__':
    main()
