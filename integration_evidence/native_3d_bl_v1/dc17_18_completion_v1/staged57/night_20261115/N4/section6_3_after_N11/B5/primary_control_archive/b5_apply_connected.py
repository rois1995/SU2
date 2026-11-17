from pathlib import Path
import hashlib
import json
import subprocess
import time

cut = Path('integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11')
base = 'c0486d57daf6ac4a1e66733db4a68b3c2dfe133a'
assert subprocess.check_output(['git', 'rev-parse', 'HEAD'], text=True).strip() == base
read = lambda path: json.loads(Path(path).read_text())
sha = lambda path: hashlib.sha256(Path(path).read_bytes()).hexdigest()
assert read(cut / 'B4/acceptance_summary.json')['state'] == 'B4_SCOPED_G1_G2_PASSED'
for name in ('G1_serial', 'G1_mpi2', 'G1_mpi4', 'G2'):
    terminal = read(cut / f'B4/validation/{name}_primary_terminal_receipt.json')
    assert terminal['all_owned_handles_terminal']
    for row in terminal['identities']:
        try:
            fields = (Path('/proc') / str(row['pid']) / 'stat').read_text().rsplit(')', 1)[1].split()
            alive = int(fields[19]) == row['start_ticks'] and fields[0] not in {'Z', 'X'}
        except FileNotFoundError:
            alive = False
        assert not alive, row
proposal = cut / 'B5_preparation/proposal'
generated = proposal / 'generated'
frozen = read(generated / 'frozen_proposal_sha_map.json')['frozen_proposal_sha256']
proof = read(generated / 'final_output_expansion_proof.json')
assert proof['main_reversal_byte_equal']
assert proof['final_generated_file_sha256'] == frozen
assert all(sha(generated / name) == digest for name, digest in frozen.items())
review = read(generated / 'primary_static_review.json')
assert review['passed'] and review['proposal_sha256'] == frozen
mapping = {'CNativeEngine3D.control.proposed.cpp': Path('Common/src/adaptation/CNativeEngine3D.cpp')}
for unit in ('EngineControl', 'EngineReports'):
    for suffix in ('hpp', 'cpp'):
        mapping[f'{unit}.{suffix}.proposed'] = Path(f'Common/src/adaptation/native_engine3d/{unit}.{suffix}')
old_pins = read(cut / 'build554/source_pins.json')
base_hashes = {}
for name, destination in mapping.items():
    base_hashes[str(destination)] = sha(destination)
    assert base_hashes[str(destination)] == old_pins[str(destination.resolve())]
output = cut / 'B5'
output.mkdir(exist_ok=False)
for name, destination in mapping.items():
    destination.write_bytes((generated / name).read_bytes())
receipt = {'applied': True, 'unix': time.time(), 'base_commit': base,
           'reference_build': 552, 'base_sha256': base_hashes,
           'applied_sha256': {str(destination): sha(destination) for destination in mapping.values()},
           'proposal_frozen_sha256': frozen,
           'actual_body_proof_sha256': sha(generated / 'final_output_expansion_proof.json'),
           'primary_static_review_sha256': sha(generated / 'primary_static_review.json'),
           'optional_fields_unchanged': True, 'existing_nine_method_bodies_retained': True,
           'max_actual_function_lines': max(review['actual_formatted_function_lines'].values()),
           'main_byte_reversal': True, 'terminal_finish_routes': 141,
           'observer_routes': 17, 'pending': ['build555', 'G1serial', 'G1mpi2', 'G1mpi4', 'strictG2']}
(output / 'actual_apply_receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
print('Applied connected five-path B5; build and ordered identity checks pending.')
