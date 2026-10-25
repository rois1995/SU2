#!/usr/bin/env python3
"""Check native publication receipts; process success alone is not mesh acceptance."""
import json
import pathlib
import sys


def check(prefix):
    prefix = pathlib.Path(prefix)
    # Older C++ diagnostics write lower-case inf for unavailable uncertainty.
    data = json.loads(pathlib.Path(str(prefix) + '_receipt.json').read_text().replace(':inf', ':Infinity'))
    construction = data.get('construction') or {}
    validity = construction.get('validity') or {}
    audit = construction.get('first_row_audit') or {}
    for label, value in [('complete', data.get('complete')), ('written', data.get('written')),
                         ('accepted', construction.get('accepted')), ('valid', validity.get('valid'))]:
        if value != 1:
            raise ValueError(f'{prefix.name}: {label} is not true; event is not published/accepted')
    for label in ('nonpositive', 'height_failures', 'overshared', 'same_orientation', 'interior_physical',
                  'open_unbound', 'unknown_marker', 'inconsistent_points', 'nonmanifold_skin_edges'):
        if validity.get(label) != 0:
            raise ValueError(f'{prefix.name}: missing or failing validity check {label}')
    if audit.get('violations') != 0:
        raise ValueError(f'{prefix.name}: missing or failing first-row audit')
    for suffix in ('.su2', '.su2.native_ref', '.su2.native_rows'):
        path = pathlib.Path(str(prefix) + suffix)
        if not path.is_file() or path.stat().st_size == 0:
            raise ValueError(f'{prefix.name}: missing or empty publication {path.name}')
    return data


if __name__ == '__main__':
    try:
        check(sys.argv[1])
    except (OSError, ValueError, IndexError) as error:
        print(error, file=sys.stderr)
        sys.exit(1)
