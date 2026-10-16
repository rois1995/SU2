"""Compare converged conventional and Newton flow on a generated manufactured problem."""
import argparse
import csv
import json
import math
import os
from pathlib import Path
import struct
import subprocess
import sys
import time

from report_state import sha


CONFIG = '''SOLVER= NAVIER_STOKES
KIND_VERIFICATION_SOLUTION= MMS_NS_UNIT_QUAD
MESH_FORMAT= RECTANGLE
MESH_BOX_SIZE= 9,9,5
MESH_BOX_LENGTH= 1,1,1
MARKER_CUSTOM= (x_minus,x_plus,y_minus,y_plus)
REF_DIMENSIONALIZATION= DIMENSIONAL
INIT_OPTION= TD_CONDITIONS
VISCOSITY_MODEL= CONSTANT_VISCOSITY
MU_CONSTANT= 1.716e-5
CONDUCTIVITY_MODEL= CONSTANT_PRANDTL
MUSCL_FLOW= NO
CONV_NUM_METHOD_FLOW= ROE
MGLEVEL= 0
MG_PRE_SMOOTH= 1
CFL_NUMBER= 10
CFL_ADAPT= NO
NEWTON_STEP_TOL= 1e-14
NEWTON_ADAPT_CFL= NO
NEWTON_ADAPTIVE_FORCING= NO
LINEAR_SOLVER_PREC= ILU
LINEAR_SOLVER_ERROR= 1e-8
LINEAR_SOLVER_ITER= 80
ITER= 2000
CONV_STARTITER= 5
CONV_FIELD= (RMS_DENSITY,RMS_MOMENTUM-X,RMS_MOMENTUM-Y,RMS_ENERGY)
CONV_RESIDUAL_MINVAL= -6
OUTPUT_FILES= RESTART
OUTPUT_WRT_FREQ= 2000
RESTART_FILENAME= restart_flow
SCREEN_OUTPUT= (INNER_ITER,RMS_DENSITY,RMS_ENERGY)
HISTORY_OUTPUT= (ITER,RMS_RES,LINSOL)
'''

MMS_CASE = {'config': CONFIG, 'name': '81-point dimensional Navier-Stokes MMS, first-order Roe',
            'fields': {'flow': ['rms[Rho]', 'rms[RhoU]', 'rms[RhoV]', 'rms[RhoE]']},
            'raw_rms': 1e-6, 'normalized_rms': 1e-9, 'projection': 0, 'state_tolerance': 2e-8,
            'quantity_tolerance': 2e-8, 'max_updates': 2000,
            'limits': 'FLOW alone: segregated and coupled flags describe the same single equation group. '
                      'Launch-inclusive timings on a tiny case do not establish speedup.'}


def rows(path):
    with path.open() as stream:
        return [{key.strip().strip('"'): value.strip() for key, value in row.items()}
                for row in csv.DictReader(stream)]


def state(path, expected_variables=6):
    with path.open('rb') as stream:
        magic, variables, points, _, _ = struct.unpack('5i', stream.read(20))
        assert magic == 535532 and variables == expected_variables and points == 81
        fields = [stream.read(33).split(b'\0')[0].decode() for _ in range(variables)]
        values = struct.unpack(str(points * variables) + 'd', stream.read(points * variables * 8))
    assert all(math.isfinite(value) for value in values)
    return fields, {values[p:p + 2]: values[p + 2:p + variables] for p in range(0, len(values), variables)}


def main(case=MMS_CASE):
    parser = argparse.ArgumentParser(description=case['name'])
    parser.add_argument('--binary', required=True, type=Path)
    parser.add_argument('--reporter', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--ranks', type=int, default=1)
    parser.add_argument('--threads', type=int, default=1)
    args = parser.parse_args()
    if args.ranks < 1 or args.threads < 1:
        parser.error('ranks and threads must be positive')
    binary, reporter = args.binary.resolve(strict=True), args.reporter.resolve(strict=True)
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    result = {'binary_sha256': sha(binary), 'reporter_sha256': sha(reporter),
              'runner_sha256': sha(Path(sys.argv[0]).resolve()), 'comparison_driver_sha256': sha(Path(__file__)),
              'report_script_sha256': sha(Path(__file__).with_name('report_state.py')),
              'ranks': args.ranks, 'threads': args.threads, 'case': case['name'], 'limits': case['limits'], 'runs': []}
    tags = {'flow': 'FLOW', 'turbulence': 'TURB', 'transition': 'TRANS', 'species': 'SPECIES'}
    selection = 'NEWTON_EQUATIONS= (' + ','.join(tags[system] for system in case['fields']) + ')\n'
    lower = 'NEWTON_BLOCK_LOWER= YES\n' if len(case['fields']) > 1 else ''
    variables = sum(len(names) for names in case['fields'].values())
    reference = None
    reference_quantities = None
    reference_activity = None
    for mode, settings in [('native', ''), ('segregated', selection),
                            ('coupled', selection + 'NEWTON_COUPLED= YES\n' + lower)]:
        directory = output / mode
        directory.mkdir()
        config = directory / 'run.cfg'
        config.write_text(case['config'] + settings)
        # Measure each solver rank rather than the MPI launcher's memory.
        measured = 'exec /usr/bin/time -f \'{"max_rss_kib":%M,"user_seconds":%U,"system_seconds":%S}\' ' \
                   '-o "resource_rank_${OMPI_COMM_WORLD_RANK}.json" "$@"'
        command = ['mpirun', '--bind-to', 'none', '-n', str(args.ranks), '/bin/sh', '-c', measured,
                   'measure', str(binary), config.name]
        started = time.perf_counter()
        with (directory / 'log.txt').open('w') as log:
            process = subprocess.run(command, cwd=directory, env=dict(os.environ, OMP_NUM_THREADS=str(args.threads)),
                                     stdout=log, stderr=subprocess.STDOUT, timeout=180)
        run = {'mode': mode, 'command': command, 'exit_code': process.returncode,
               'wall_seconds': time.perf_counter() - started, 'config_sha256': sha(config)}
        result['runs'].append(run)
        (output / 'results.json').write_text(json.dumps(result, indent=2) + '\n')
        assert process.returncode == 0, directory
        resources = [json.loads((directory / ('resource_rank_%d.json' % rank)).read_text())
                     for rank in range(args.ranks)]
        run['rank_resources'] = resources
        run['max_rank_rss_kib'] = max(row['max_rss_kib'] for row in resources)
        history = rows(directory / 'history.csv')
        run['updates'] = len(history)
        assert len(history) < case['max_updates'], 'Iteration cap reached'
        run['last_history_log10_rms'] = [float(history[-1][key]) for names in case['fields'].values() for key in names]
        assert max(run['last_history_log10_rms']) <= math.log10(case['raw_rms'])
        run['outer_krylov_iterations'] = sum(int(value) for row in history for key, value in row.items()
                                           if key == 'Linear_Solver_Iterations' or key.startswith('LinSolIter'))
        run['inner_krylov_iterations'] = 0
        run['residual_assemblies'] = {system: len(history) for system in case['fields']}
        if settings:
            attempts, groups = [], set()
            # The final accepted logs precede the last all-active physical refresh.
            run['residual_assemblies'] = {system: 1 for system in case['fields']}
            for path in directory.glob('newton_*.csv'):
                if path.stem.endswith('_trials'):
                    continue
                groups.add(path.stem)
                group = rows(path)
                accepted = [row for row in group if row['decision'] == 'ACCEPTED']
                assert len(accepted) == len(history), path
                for row in accepted:
                    assert math.isfinite(float(row['true_linear_defect'])) and float(row['true_linear_defect']) <= 0.1
                    assert float(row['merit']) <= float(row['base_merit']) + 1e-4 * float(row['step']) * float(row['slope'])
                assert all(float(row['rollback_state_error']) == 0 for row in group)
                for system in case['fields']:
                    run['residual_assemblies'][system] += int(group[-1][system + '_residual_evaluations'])
                attempts.extend(group)
            assert groups == ({'newton_coupled'} if mode == 'coupled' else
                              {'newton_' + tags[system].lower() for system in case['fields']})
            run['attempts'] = len(attempts)
            run['outer_krylov_iterations'] = sum(int(row['linear_iterations']) for row in attempts)
            run['inner_krylov_iterations'] = sum(int(row['native_linear_iterations']) for row in attempts)
        run['flow_residual_assemblies'] = run['residual_assemblies']['flow']
        destination = output / (mode + '_physical')
        restart = directory / 'restart_flow.dat'
        subprocess.run([sys.executable, str(Path(__file__).with_name('report_state.py')), '--reporter', str(reporter),
                        '--config', str(config), '--restart', str(restart), '--output', str(destination),
                        '--ranks', str(args.ranks), '--threads', str(args.threads)], check=True, timeout=150)
        report = json.loads((destination / 'report.json').read_text())
        assert set(report['systems']) == set(case['fields'])
        for physical in report['systems'].values():
            assert max(physical['rms']) <= case['raw_rms']
            assert max(physical['normalized_rms']) <= case['normalized_rms']
            assert max(physical['projection_max']) <= (case['projection'] if mode == 'native' else 0)
        run['physical'] = report
        if case.get('transition_model'):
            activity = report['transition_activity']
            assert activity['model'] == case['transition_model']
            assert activity['sampled_points'] > 0
            active_band = 'gamma_below_0_1' if case['transition_model'] == 'SA_BC' else 'gamma_0_1_to_0_9'
            assert activity[active_band] > 0 and activity['gamma_above_0_9'] > 0, activity
            assert 0 <= activity['gamma_min'] <= activity['gamma_mean'] <= activity['gamma_max']
            if case['transition_model'] == 'SA_BC':
                assert activity['gamma_max'] <= 1
            if reference_activity is None:
                reference_activity = activity
            run['transition_activity_difference_from_native'] = {
                key: abs(activity[key] - reference_activity[key]) for key in ['gamma_min', 'gamma_max', 'gamma_mean']}
            assert max(run['transition_activity_difference_from_native'].values()) <= case['quantity_tolerance']
        fields, solution = state(restart, 2 + variables)
        if case.get('minimum_species_range'):
            indices = [j for j, name in enumerate(fields[2:]) if name.startswith('Species_')]
            assert len(indices) == len(case['fields']['species'])
            interior = [values for point, values in solution.items() if all(0 < x < 1 for x in point)]
            assert interior
            run['interior_species_range'] = [[min(row[j] for row in interior), max(row[j] for row in interior)]
                                             for j in indices]
            assert all(0 <= low < high <= 1 and high - low >= case['minimum_species_range']
                       for low, high in run['interior_species_range'])
            assert all(sum(row[j] for j in indices) <= 1 for row in interior)
        if reference is None:
            reference = fields, solution
            reference_quantities = report['quantities']
        assert fields == reference[0] and solution.keys() == reference[1].keys()
        run['state_difference_from_native'] = [max(abs(solution[p][v] - reference[1][p][v]) for p in solution) /
            max(abs(reference[1][p][v]) for p in solution) for v in range(variables)]
        assert max(run['state_difference_from_native']) <= case['state_tolerance']
        run['quantity_difference_from_native'] = {key: abs(value - reference_quantities[key]) /
            max(abs(reference_quantities[key]), 1e-12) for key, value in report['quantities'].items()}
        assert max(run['quantity_difference_from_native'].values()) <= case['quantity_tolerance']
        run['evidence_sha256'] = {str(p.relative_to(output)): sha(p) for p in sorted(directory.iterdir()) if p.is_file()}
        run['report_manifest_sha256'] = sha(destination / 'manifest.json')
        (output / 'results.json').write_text(json.dumps(result, indent=2) + '\n')
        print(mode, 'converged:', run['updates'], 'updates, physical normalized RMS',
              {system: values['normalized_rms'] for system, values in report['systems'].items()}, flush=True)


if __name__ == '__main__':
    main()
