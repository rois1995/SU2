"""Check independent final-state reporting, restart formats and hybrid reproducibility."""
import argparse
import csv
import json
import math
import os
from pathlib import Path
import struct
import subprocess
import sys

from report_state import option, sha


def close(left, right):
    if isinstance(left, dict):
        assert left.keys() == right.keys()
        for key in left:
            close(left[key], right[key])
    elif isinstance(left, list):
        assert len(left) == len(right)
        for a, b in zip(left, right):
            close(a, b)
    else:
        assert math.isclose(left, right, rel_tol=2e-12, abs_tol=1e-15), (left, right)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', required=True, type=Path)
    parser.add_argument('--reporter', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    binary, reporter = args.binary.resolve(strict=True), args.reporter.resolve(strict=True)
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    assert option('NEWTON_EQUATIONS= (FLOW, \\\n TURB)\nITER= 4\n', 'NEWTON_EQUATIONS', 'NONE') == 'NEWTON_EQUATIONS= NONE\nITER= 4\n'
    assert option('ITER= 4\n', 'OUTPUT_FILES', 'NONE') == 'ITER= 4\nOUTPUT_FILES= NONE\n'
    common = '''SOLVER= RANS
KIND_TURB_MODEL= SST
MESH_FORMAT= RECTANGLE
MESH_BOX_SIZE= 5,5,5
MESH_BOX_LENGTH= 1,1,1
MARKER_HEATFLUX= (y_minus,0)
MARKER_FAR= (x_minus,x_plus,y_plus)
MARKER_WALL_FUNCTIONS= (y_minus,STANDARD_WALL_FUNCTION)
MACH_NUMBER= 0.2
REYNOLDS_NUMBER= 1e4
MUSCL_FLOW= NO
MUSCL_TURB= NO
CONV_NUM_METHOD_FLOW= ROE
MGLEVEL= 0
CFL_NUMBER= 0.05
LINEAR_SOLVER_PREC= ILU
NEWTON_ADAPT_CFL= NO
NEWTON_ADAPTIVE_FORCING= NO
ITER= 4
CONV_STARTITER= 1000
OUTPUT_FILES= RESTART
OUTPUT_WRT_FREQ= 4
RESTART_FILENAME= restart_flow
SCREEN_OUTPUT= (INNER_ITER)
HISTORY_OUTPUT= (ITER,RMS_RES)
'''
    records = []
    for mode in ['native', 'coupled', 'mixture_lm']:
        directory = output / (mode + '_restart_' + 'long_path_' * 6)
        directory.mkdir()
        config = directory / 'run.cfg'
        physics = common
        if mode == 'mixture_lm':
            for key, value in {'SOLVER': 'INC_RANS', 'CONV_NUM_METHOD_FLOW': 'FDS',
                    'MARKER_WALL_FUNCTIONS': 'NONE', 'INC_DENSITY_MODEL': 'VARIABLE', 'INC_ENERGY_EQUATION': 'YES',
                    'FLUID_MODEL': 'FLUID_MIXTURE', 'KIND_SCALAR_MODEL': 'SPECIES_TRANSPORT', 'SPECIES_INIT': '0.4',
                    'MOLECULAR_WEIGHT': '28.96,16.043', 'SPECIFIC_HEAT_CP': '1009.39,2225',
                    'VISCOSITY_MODEL': 'CONSTANT_VISCOSITY', 'MU_CONSTANT': '1.8551e-5,1.1e-5',
                    'PRANDTL_LAM': '0.72,0.72', 'PRANDTL_TURB': '0.9,0.9', 'DIFFUSIVITY_CONSTANT': '1e-5',
                    'KIND_TRANS_MODEL': 'LM', 'MUSCL_SPECIES': 'NO'}.items():
                physics = option(physics, key, value)
        if mode != 'native':
            physics += ('NEWTON_EQUATIONS= (FLOW,TURB,TRANS,SPECIES)\n' if mode == 'mixture_lm' else
                        'NEWTON_EQUATIONS= (FLOW,TURB)\n') + 'NEWTON_COUPLED= YES\nNEWTON_BLOCK_LOWER= YES\n'
        config.write_text(physics)
        command = ['mpirun', '--bind-to', 'none', '-n', '2', str(binary), config.name]
        with (directory / 'log.txt').open('w') as log:
            subprocess.run(command, cwd=directory, env=dict(os.environ, OMP_NUM_THREADS='2'),
                           stdout=log, stderr=subprocess.STDOUT, timeout=60, check=True)
        restart = directory / 'restart_flow.dat'
        assert len(str(restart)) > 100
        protected = {p: sha(p) for p in directory.iterdir() if p.is_file()}
        reports = []
        layouts = [(2, 2)] if mode == 'coupled' else [(2, 2), (1, 1)]
        for ranks, threads in layouts:
            destination = output / (mode + '_%dx%d' % (ranks, threads))
            subprocess.run([sys.executable, str(Path(__file__).with_name('report_state.py')),
                            '--reporter', str(reporter), '--config', str(config), '--restart', str(restart),
                            '--output', str(destination), '--ranks', str(ranks), '--threads', str(threads)],
                           check=True, timeout=150)
            reports.append(json.loads((destination / 'report.json').read_text()))
        report = reports[0]
        if mode != 'coupled':
            close(report, reports[1])
            with restart.open('rb') as stream:
                magic, variables, points, _, _ = struct.unpack('5i', stream.read(20))
                assert magic == 535532
                fields = [stream.read(33).split(b'\0')[0].decode() for _ in range(variables)]
                values = struct.unpack(str(points * variables) + 'd', stream.read(points * variables * 8))
            ascii_restart = output / (directory.name + '.csv')
            with ascii_restart.open('w') as stream:
                stream.write(','.join('"' + name + '"' for name in ['PointID'] + fields) + '\n')
                for point in range(points):
                    stream.write(str(point) + ',' + ','.join(format(v, '.17g') for v in
                                 values[point * variables:(point + 1) * variables]) + '\n')
            destination = output / (mode + '_ascii')
            subprocess.run([sys.executable, str(Path(__file__).with_name('report_state.py')),
                            '--reporter', str(reporter), '--config', str(config), '--restart', str(ascii_restart),
                            '--output', str(destination), '--ranks', '2', '--threads', '2'], check=True, timeout=150)
            close(report, json.loads((destination / 'report.json').read_text()))
            if mode == 'mixture_lm':
                data = list(csv.reader(ascii_restart.read_text().splitlines()))
                for missing, count in [('species', 1), ('transition', 3)]:
                    incomplete = output / ('missing_' + missing + '.csv')
                    with incomplete.open('w') as stream:
                        csv.writer(stream).writerows(row[:-count] for row in data)
                    destination = output / ('missing_' + missing)
                    with (output / ('missing_' + missing + '_runner.log')).open('w') as log:
                        failed = subprocess.run([sys.executable, str(Path(__file__).with_name('report_state.py')),
                            '--reporter', str(reporter), '--config', str(config), '--restart', str(incomplete),
                            '--output', str(destination)], stdout=log, stderr=subprocess.STDOUT, timeout=150)
                    assert failed.returncode != 0
                    assert 'insufficient ' + missing + ' fields' in (destination / 'log.txt').read_text()
                    assert not (destination / 'report.json').exists()
        assert all(sha(path) == digest for path, digest in protected.items()), 'Reporting changed solver output'
        with (directory / 'history.csv').open() as stream:
            final = list(csv.DictReader(stream))[-1]
        final = {key.strip().strip('"'): float(value) for key, value in final.items()}
        names = ['rms[Rho]', 'rms[RhoU]', 'rms[RhoV]', 'rms[RhoE]', 'rms[k]', 'rms[w]']
        if mode == 'mixture_lm':
            names = ['rms[P]', 'rms[U]', 'rms[V]', 'rms[h]', 'rms[k]', 'rms[w]',
                     'rms[LM_1]', 'rms[LM_2]', 'rms[rho*Y_0]']
            assert set(report['systems']) == {'flow', 'turbulence', 'transition', 'species'}
        physical = [value for system in report['systems'].values() for value in system['rms']]
        assert len(physical) == len(names)
        history = [final[key] for key in names]
        difference = max(abs(math.log10(value) - old) for value, old in zip(physical, history))
        projection = max(v for system in report['systems'].values() for v in system['projection_max'])
        if mode != 'native':
            assert difference < 6e-9, difference
            assert projection == 0, projection
        else:
            assert difference > 1e-4, difference
            assert projection > 0, projection
        records.append({'mode': mode, 'physical_vs_history_log10_max': difference,
                        'projection_max': projection, 'source_files_unchanged': True})
        print(mode, 'passed', records[-1], flush=True)
    (output / 'results.json').write_text(json.dumps({'binary_sha256': sha(binary),
        'reporter_sha256': sha(reporter), 'checks': records}, indent=2) + '\n')


if __name__ == '__main__':
    main()
