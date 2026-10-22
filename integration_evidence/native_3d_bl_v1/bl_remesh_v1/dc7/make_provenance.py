#!/usr/bin/env python3
"""DC-7 provenance: test-only sources, binary, inputs, evidence files (sha256) and /usr/bin/time receipts."""
import hashlib, json, re, subprocess
from pathlib import Path
E = Path(__file__).resolve().parent
R = E.parents[3]
def sha(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def receipt(log):
    t = Path(log).read_text(errors='replace')
    g = lambda k: (re.search(k + r':\s*(.+)', t) or [None, None])[1]
    return {'elapsed': g(r'Elapsed \(wall clock\) time \(h:mm:ss or m:ss\)'), 'max_rss_kB': g('Maximum resident set size \(kbytes\)'),
            'exit': g('Exit status'), 'status': 'non-zero' if 'Command exited with non-zero status' in t else 'zero'}
out = {
    'scope': 'BL_FIRST_ROW_DESIGN rev 2.1 DC-7 (W -> R1 -> R2 T(V) -> V) test-only harness; no production C++ changed',
    'git_head': subprocess.run(['git', '-C', str(R), 'rev-parse', 'HEAD'], capture_output=True, text=True).stdout.strip(),
    'production_diff_empty': subprocess.run(['git', '-C', str(R), 'diff', '--quiet', 'HEAD', '--', 'Common', 'SU2_CFD'], capture_output=True).returncode == 0,
    'test_only_sources': {p: sha(R / p) for p in ['UnitTests/SU2_CFD/adaptation/fixtures/native_first_row_dc7.inc',
                                                  'UnitTests/SU2_CFD/adaptation/CNativeRemesher3D_tests.cpp',
                                                  'UnitTests/SU2_CFD/adaptation/fixtures/native_layer_2dway.inc',
                                                  'UnitTests/SU2_CFD/adaptation/fixtures/native_junction_replay.inc']},
    'binary': {'path': 'integration_evidence/build-integrated-v2/UnitTests/test_driver', 'sha256': sha(R / 'integration_evidence/build-integrated-v2/UnitTests/test_driver')},
    'campaign_log': (E / 'campaign.log').read_text().splitlines()[-12:],
    'receipts': {p.name: receipt(p) for p in sorted((E / 'runs').glob('*.final.log'))},
    'm6_field_export': {'receipt': receipt(E / 'm6_field/export.log'), 'status': (E / 'm6_field/export.status').read_text().split()},
    'files_sha256': {str(p.relative_to(E)): sha(p) for p in sorted(E.rglob('*')) if p.is_file() and p.name != 'provenance.json'
                     and '__pycache__' not in p.parts and not p.name.endswith('.bin')},
    'large_inputs_not_hashed_individually': 'm6_field/selected_rank*.bin are hashed below',
    'selected_bin_sha256': {p.name: sha(p) for p in sorted((E / 'm6_field').glob('selected_rank*.bin'))},
}
(E / 'provenance.json').write_text(json.dumps(out, indent=1) + '\n')
print('provenance written', len(out['files_sha256']), 'files')
