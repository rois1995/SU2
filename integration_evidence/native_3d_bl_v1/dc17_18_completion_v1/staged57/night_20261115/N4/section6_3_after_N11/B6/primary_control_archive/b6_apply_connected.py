from pathlib import Path
import hashlib
import json
import subprocess
import time

cut = Path('integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11')
base = '2a05da05ca1f9642fc1847c5dff57e78d2eb50df'
read = lambda path: json.loads(Path(path).read_text())
sha = lambda path: hashlib.sha256(Path(path).read_bytes()).hexdigest()
assert subprocess.check_output(['git', 'rev-parse', 'HEAD'], text=True).strip() == base
assert read(cut / 'B5/acceptance_summary.json')['state'] == 'B5_SCOPED_G1_G2_PASSED'
for tag in ('G1_serial', 'G1_mpi2', 'G1_mpi4', 'G2'):
    terminal = read(cut / f'B5/validation/{tag}_primary_terminal_receipt.json')
    assert terminal['all_owned_handles_terminal']
    for row in terminal['identities']:
        try:
            fields = (Path('/proc') / str(row['pid']) / 'stat').read_text().rsplit(')', 1)[1].split()
            alive = int(fields[19]) == row['start_ticks'] and fields[0] not in {'Z', 'X'}
        except FileNotFoundError:
            alive = False
        assert not alive, row
proposal = cut / 'B6_preparation/proposal/accepted_B5_rebind'
frozen = read(proposal / 'FROZEN_SHA256.json')
assert frozen['base_commit'] == base
assert all(sha(proposal / name) == digest for name, digest in frozen['sha256'].items())
proof = read(proposal / 'DRAFT_BINDINGS.json')
identity = read(proposal / 'OUTPUT_IDENTITY_RECEIPT.json')
assert proof['base'] == base and proof['actual_generated_body_tokens_equal_after_recursive_inverse']
assert proof['whole_main_byte_reverse'] and proof['header_original_fields_and_special_members_token_equal']
assert proof['started_borrow_const_as_original'] and max(proof['formatted_method_lines'].values()) <= 80
assert identity['all_three_candidate_files_byte_identical']
mapping = {'CNativeEngine3D.cpp': Path('Common/src/adaptation/CNativeEngine3D.cpp')}
for suffix in ('hpp', 'cpp'):
    mapping['EngineOwnership.' + suffix] = Path('Common/src/adaptation/native_engine3d/EngineOwnership.' + suffix)
old_pins = read(cut / 'build555/source_pins.json')
old_hashes = {}
for name, destination in mapping.items():
    old_hashes[str(destination)] = sha(destination)
    assert old_hashes[str(destination)] == old_pins[str(destination.resolve())]
    assert sha(proposal / (name + '.proposed')) == identity['sha256'][name + '.proposed']
output = cut / 'B6'
output.mkdir(exist_ok=False)
for name, destination in mapping.items():
    destination.write_bytes((proposal / (name + '.proposed')).read_bytes())
receipt = {'applied': True, 'unix': time.time(), 'base_commit': base, 'reference_build': 552,
           'base_sha256': old_hashes,
           'applied_sha256': {str(destination): sha(destination) for destination in mapping.values()},
           'proposal_frozen_receipt_sha256': sha(proposal / 'FROZEN_SHA256.json'),
           'actual_body_proof_sha256': sha(proposal / 'DRAFT_BINDINGS.json'),
           'candidate_identity_receipt_sha256': sha(proposal / 'OUTPUT_IDENTITY_RECEIPT.json'),
           'primary_review_sha256': sha(cut / 'B6_preparation/proposal/PRIMARY_SOURCE_REVIEW_B4_DRAFT.md'),
           'canonical_fields_and_special_members_unchanged': True,
           'max_new_method_lines': max(proof['formatted_method_lines'].values()),
           'main_byte_reversal': True, 'pending': ['build556', 'G1serial', 'G1mpi2', 'G1mpi4', 'strictG2']}
(output / 'actual_apply_receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
print('Applied connected three-path B6 ownership extraction; validation pending.')
