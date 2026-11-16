"""Seal existing inputs for N5 fixture actions; does not launch computation."""
from pathlib import Path
import hashlib
import json

N5 = Path(__file__).resolve().parent
R = N5.parents[1]

def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def write(name, action, environment, files):
    tags = {'geometry': '[.][NativeD9Geometry]', 'serial_stack': '[.][NativeD9SerialStack]',
            'counter_check': '[.][NativeD9CounterCheck]'}
    value = {'schema': 'n5_d9_measurement_v1', 'action': action, 'ranks': 1,
             'case_name': 'n5_d9_' + name, 'test_filter': tags[action], 'environment': environment,
             'input_file_sha256': {str(p): sha(p) for p in files}}
    folder = N5 / 'descriptors'
    folder.mkdir(exist_ok=True)
    (folder / (name + '.json')).write_text(json.dumps(value, indent=2) + '\n')

def main():
    write('counter_check', 'counter_check', {}, [])
    for mode in ['M10_n1', 'M10_n2', 'M10_n4', 'M12_control', 'M12_meet']:
        write(mode.lower(), 'serial_stack', {'SU2_D9_STACK_MODE': mode,
              'SU2_D9_STACK_JSON': 'd9_stack_measurement.json'}, [])
    original = R / 'm6_inputs/euler_original_force_seed_v1'
    qualification = json.loads((original / 'qualification.json').read_text())
    reference = original / 'm6.su2.native_ref'
    mesh = original / 'm6.su2'
    for p in [reference, mesh]:
        assert qualification['output_sha256'][p.name] == sha(p)
    write('geometry_euler_input', 'geometry', {'SU2_D9_REFERENCE': str(reference),
          'SU2_D9_WALL_MARKERS': '2'}, [reference, mesh, original / 'qualification.json', original / 'run.cfg'])
    adapted = R / 'm6_inputs/euler_adapted293_event1_v1'
    checkpoint = json.loads((adapted / 'checkpoint.json').read_text())
    reference = adapted / 'adapted_mesh_adap_00001.su2.native_ref'
    mesh = adapted / 'adapted_mesh_adap_00001.su2'
    for p in [reference, mesh]:
        assert checkpoint['input_hashes'][p.name] == sha(p)
    write('geometry_euler293', 'geometry', {'SU2_D9_REFERENCE': str(reference),
          'SU2_D9_WALL_MARKERS': '2'}, [reference, mesh, adapted / 'checkpoint.json', adapted / 'run.cfg'])
    saved = R / 'saved_surface492_checkpoint.json'
    checkpoint = json.loads(saved.read_text())
    for key in ['prepared_reference', 'parameters', 'owned', 'composed_target']:
        value = checkpoint[key]
        assert sha(value['path']) == value['sha256']
    reference = Path(checkpoint['prepared_reference']['path'])
    parameters = Path(checkpoint['parameters']['path'])
    write('geometry_saved492', 'geometry', {'SU2_D9_REFERENCE': str(reference),
          'SU2_D9_PARAMETERS': str(parameters), 'SU2_D9_WALL_MARKERS': '2'},
          [reference, parameters, saved, Path(checkpoint['owned']['path']), Path(checkpoint['composed_target']['path'])])
    print('9 descriptors sealed; archived original, adapted293 and saved492 input hashes verified')

if __name__ == '__main__':
    main()
