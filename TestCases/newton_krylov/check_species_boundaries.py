"""Check species strong values in native, segregated and coupled production solves."""
import argparse
import csv
import hashlib
import json
import math
import os
from pathlib import Path
import struct
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', required=True, type=Path)
    parser.add_argument('--before', type=Path, help='Optional executable predating species boundary refresh')
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--ranks', type=int, default=2)
    parser.add_argument('--threads', type=int, default=2)
    args = parser.parse_args()
    if args.ranks < 1 or args.threads < 1:
        parser.error('ranks and threads must be positive')
    binaries = [('before', args.before.resolve())] if args.before else []
    binaries.append(('current', args.binary.resolve()))
    if any(not path.is_file() for _, path in binaries):
        parser.error('executable does not exist')
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    common = '''MESH_FORMAT= RECTANGLE
MESH_BOX_SIZE= 5,5,5
MESH_BOX_LENGTH= 1,1,1
MARKER_HEATFLUX= (y_minus,0)
MARKER_FAR= (y_plus)
INC_INLET_TYPE= VELOCITY_INLET
MARKER_INLET= (x_minus,300,1,1,0,0)
MARKER_INLET_SPECIES= (x_minus,0.75)
INC_OUTLET_TYPE= PRESSURE_OUTLET
MARKER_OUTLET= (x_plus,0)
MARKER_WALL_SPECIES= (y_minus,VALUE,0.65)
SOLVER= INC_NAVIER_STOKES
CONV_NUM_METHOD_FLOW= FDS
INC_DENSITY_MODEL= VARIABLE
INC_ENERGY_EQUATION= YES
FLUID_MODEL= FLUID_MIXTURE
KIND_SCALAR_MODEL= SPECIES_TRANSPORT
SPECIES_INIT= 0.4
MOLECULAR_WEIGHT= 28.96,16.043
SPECIFIC_HEAT_CP= 1009.39,2225
VISCOSITY_MODEL= CONSTANT_VISCOSITY
MU_CONSTANT= 1.8551e-5,1.1e-5
PRANDTL_LAM= 0.72,0.72
PRANDTL_TURB= 0.9,0.9
DIFFUSIVITY_CONSTANT= 1e-5
MUSCL_FLOW= NO
MUSCL_SPECIES= NO
MGLEVEL= 0
CFL_NUMBER= 0.05
NEWTON_ADAPT_CFL= NO
NEWTON_ADAPTIVE_FORCING= NO
ITER= 4
CONV_STARTITER= 1000
OUTPUT_FILES= RESTART
OUTPUT_WRT_FREQ= 4
SCREEN_OUTPUT= (INNER_ITER)
HISTORY_OUTPUT= (ITER,RMS_RES)
RESTART_FILENAME= restart_flow
'''
    strong = 'MARKER_SPECIES_STRONG_BC= (x_minus,x_plus)\n'
    profiles = {'weak_native': '', 'strong_native': strong,
                'species_only': strong + 'NEWTON_EQUATIONS= (SPECIES)\n',
                'segregated': strong + 'NEWTON_EQUATIONS= (FLOW,SPECIES)\n',
                'coupled': strong + 'NEWTON_EQUATIONS= (FLOW,SPECIES)\nNEWTON_COUPLED= YES\n',
                'explicit_flow': strong + 'NEWTON_EQUATIONS= (FLOW,SPECIES)\nNEWTON_COUPLED= YES\n'
                                          'TIME_DISCRE_FLOW= EULER_EXPLICIT\n'}
    results, histories = [], {}
    for name, physics in profiles.items():
        for label, executable in binaries:
            directory = output / name / label
            directory.mkdir(parents=True)
            config = directory / 'run.cfg'
            config.write_text(common + physics)
            command = ['mpirun', '--bind-to', 'none', '-n', str(args.ranks), str(executable), config.name]
            with (directory / 'log.txt').open('w') as log:
                process = subprocess.run(command, cwd=directory,
                    env=dict(os.environ, OMP_NUM_THREADS=str(args.threads)),
                    stdout=log, stderr=subprocess.STDOUT, timeout=120)
            assert process.returncode == 0, directory
            with (directory / 'history.csv').open() as stream:
                histories[name, label] = list(csv.DictReader(stream))
            assert len(histories[name, label]) == 4, directory
            with (directory / 'restart_flow.dat').open('rb') as stream:
                header = struct.unpack('5i', stream.read(20))
                assert header[:3] == (535532, 7, 25), header
                fields = [stream.read(33).split(b'\0')[0].decode() for _ in range(7)]
                values = struct.unpack('175d', stream.read(1400))
            assert all(math.isfinite(value) for value in values), directory
            points = {(values[i], values[i + 1]): values[i + fields.index('Species_0')]
                      for i in range(0, len(values), 7)}
            errors = {'inlet_error': max(abs(value - 0.75) for (x, y), value in points.items() if x == 0 and y > 0),
                      'wall_error': max(abs(value - 0.65) for (x, y), value in points.items() if y == 0),
                      'outlet_error': max(abs(value - points[0.75, y]) for (x, y), value in points.items()
                                          if x == 1 and 0 < y < 1)}
            groups = {}
            for path in directory.glob('newton_*.csv'):
                if path.stem.endswith('_trials'): continue
                with path.open() as stream:
                    attempts = list(csv.DictReader(stream))
                assert len(attempts) == 4 and all(row['decision'] == 'ACCEPTED' for row in attempts), path
                assert all(float(row['true_linear_defect']) <= 0.1 for row in attempts), path
                groups[path.stem] = len(attempts)
            expected = {'species_only': {'newton_species'}, 'segregated': {'newton_flow', 'newton_species'},
                        'coupled': {'newton_coupled'}, 'explicit_flow': {'newton_coupled'}}
            assert set(groups) == expected.get(name, set()), (directory, groups)
            if label == 'current':
                assert errors['wall_error'] <= 1e-14, (directory, errors)
                if groups:
                    assert errors['inlet_error'] == errors['outlet_error'] == 0, (directory, errors)
            row = {'profile': name, 'variant': label, 'command': command, 'threads': args.threads,
                   'binary_sha256': hashlib.sha256(executable.read_bytes()).hexdigest(),
                   'config_sha256': hashlib.sha256(config.read_bytes()).hexdigest(),
                   'restart_sha256': hashlib.sha256((directory / 'restart_flow.dat').read_bytes()).hexdigest(),
                   'accepted_updates': groups, **errors}
            results.append(row)
            (output / 'results.json').write_text(json.dumps(results, indent=2) + '\n')
            print(name, label, errors, flush=True)
    if args.before:
        assert histories['weak_native', 'before'] == histories['weak_native', 'current']
        assert any(row['inlet_error'] > 0.1 for row in results
                   if row['variant'] == 'before' and row['accepted_updates'])


if __name__ == '__main__':
    main()
