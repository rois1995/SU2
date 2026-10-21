#!/usr/bin/env python3
"""WP-0(b) test-only input preparation for the exact oblique annulus (no C++ build, no mesh mutation).

Writes plain-text blocks read by [NativeJunctionReplay3D]:
  annulus_case.txt  -- original sensor donors (initial.json), initial input cells, trial before/after
                       patches, WP-0(a) variant coordinates; the C++ side loads the saved v6 reference,
                       rejected binary state and parameter records directly from annulus_n1.
"""
import hashlib, json, sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
SRC = (HERE / '../../mapped_component_geometry_v1/annulus_n1').resolve()
STEM = 'affine_annulus_bl_budget_20_m1'
WP0A = (HERE / '../wp0a').resolve()

def sha(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()

def faces(c):
    v = c
    return [(v[1], v[2], v[3]), (v[0], v[3], v[2]), (v[0], v[1], v[3]), (v[0], v[2], v[1])]

def patch_block(tag, record):
    coords = {n[0]: n[1:4] for n in record['nodes']}
    surface = {tuple(sorted(s['nodes'])): s['facet'] for s in record['surface']}
    out = [f"{tag} {len(record['cells'])}"]
    for c in record['cells']:
        ids = c['nodes']
        row = [str(c['id'])]
        for n in ids:
            x = coords[n]
            row += [str(n), repr(float(x[0])), repr(float(x[1])), repr(float(x[2]))]
        for f in faces(ids):
            key = tuple(sorted(f))
            row += (['1', str(surface[key])] if key in surface else ['0', '0'])
        out.append(' '.join(row))
    return out

def main():
    inputs = {
        'initial': SRC / f'{STEM}_initial.json',
        'before': SRC / f'{STEM}_rejected_rank0_trial_before.json',
        'after': SRC / f'{STEM}_rejected_rank0_trial_after.json',
        'trial': SRC / f'{STEM}_rejected_rank0_trial.json',
        'profile': SRC / f'{STEM}_frozen_profile.json',
        'reference': SRC / f'{STEM}_rejected_rank0.native_ref',
        'state': SRC / f'{STEM}_rejected_rank0.bin',
        'cfg': SRC / f'{STEM}.cfg',
        'before_parameters': SRC / f'{STEM}_rejected_rank0_trial_before.parameters.bin',
        'after_parameters': SRC / f'{STEM}_rejected_rank0_trial_after.parameters.bin',
        'wp0a_result': WP0A / 'result.json',
        'wp0a_progress': WP0A / 'progress_result.json',
    }
    initial = json.loads(inputs['initial'].read_text())
    profile = json.loads(inputs['profile'].read_text())
    trial = json.loads(inputs['trial'].read_text())
    lines = ['SU2_JUNCTION_ANNULUS 1', f"core {profile['core']!r}",
             f"recorded {trial['weakest_cell']} {trial['minimum_quality']!r} {trial['minimum_scaled_jacobian']!r}"]
    donors = initial['original']
    # SaveRecords omits extension metadata: donors that are not input cells are the generated exterior
    # extension records of the declared plane (uniform sensor; component id is assigned on the C++ side).
    cells = {tuple(sorted(c['nodes'])) for c in initial['cells']}
    lines.append(f'donors {len(donors)}')
    for i, d in enumerate(donors):
        extension = tuple(sorted(v[0] for v in d['vertices'])) not in cells
        row = [str(i), str(int(extension))]
        for n in d['vertices']:
            row += [str(n[0])] + [repr(float(x)) for x in n[1:4]] + [repr(float(x)) for x in n[4]]
        lines.append(' '.join(row))
    lines += patch_block('input', initial)
    lines += patch_block('before', json.loads(inputs['before'].read_text()))
    lines += patch_block('after', json.loads(inputs['after'].read_text()))
    # WP-0(a) variant coordinates (approximate Python optimum/witness), re-evaluated natively.
    result = json.loads(inputs['wp0a_result'].read_text())
    progress = json.loads(inputs['wp0a_progress'].read_text())
    variants = []
    for v in result['variants']:
        variants.append((v['variant'], v['coords'], v.get('swap_shell_old_new_ring') is not None))
    for v in progress['variants']:
        variants.append((v['variant'], v['coords'], 'swap' in v['variant']))
    lines.append(f'variants {len(variants)}')
    for name, coords, swap in variants:
        token = name.replace(' ', '_')
        lines.append(f"variant {token} {int(swap)} {len(coords)}")
        for k, x in sorted(coords.items(), key=lambda r: int(r[0])):
            lines.append(f"{k} {float(x[0])!r} {float(x[1])!r} {float(x[2])!r}")
    lines.append('end')
    (HERE / 'annulus_case.txt').write_text('\n'.join(lines) + '\n')
    prov = {'script_sha256': sha(__file__), 'inputs': {k: {'path': str(p.relative_to(HERE.parents[3])) if str(p).startswith(str(HERE.parents[3])) else str(p), 'sha256': sha(p)} for k, p in inputs.items()},
            'output': {'annulus_case.txt': sha(HERE / 'annulus_case.txt')}}
    (HERE / 'prepare_annulus_provenance.json').write_text(json.dumps(prov, indent=1) + '\n')

if __name__ == '__main__':
    main()
