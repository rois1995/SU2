"""Check input provenance on a saved integrated BL mesh, including a mid-audit edit.
Run: python3 integration_evidence/check_native_audit_inputs.py
"""
import hashlib
from pathlib import Path
import shutil
import tempfile
import audit_native_bl

if __name__ == '__main__':
    root = Path(__file__).resolve().parent
    prefix = root/'integrated_primal_controls_v6/audit_np4/native_bl_opposing_conservative_cycle_7'
    paths = [Path(str(prefix)+suffix) for suffix in ('_donor.su2', '_metric.csv', '_adapted.su2')]
    row = audit_native_bl.audit(*paths, .0045, wall_tags=('lower_a', 'lower_b', 'upper'))
    assert all(row[k] for k in ('all_exact_positive', 'manifold_oriented_edges', 'physical_equals_exposed'))
    assert row['disk_euler'] == 1 and row['min_quality'] >= .18 and row['max_simpson_length'] <= 1.8
    assert row['max_relative_height_error'] <= 1e-8 and not row['bad_cells']
    assert row['input_files_sha256'] == {str(p.resolve()): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}
    # Change only a temporary copy; the numeric input is unchanged by a trailing newline.
    with tempfile.TemporaryDirectory(prefix='audit_input_check_', dir=root) as name:
        copies = [Path(name)/p.name for p in paths]
        for source, target in zip(paths, copies):
            shutil.copy2(source, target)
        original_mesh = audit_native_bl.mesh
        def mesh_with_edit(path):
            result = original_mesh(path)
            if Path(path) == copies[0]:
                with copies[0].open('a') as stream:
                    stream.write('\n')
            return result
        audit_native_bl.mesh = mesh_with_edit
        try:
            try:
                audit_native_bl.audit(*copies, .0045, wall_tags=('lower_a', 'lower_b', 'upper'))
            except RuntimeError as error:
                assert str(error) == 'Audit inputs changed during evaluation'
            else:
                raise AssertionError('Changed input was silently accepted')
        finally:
            audit_native_bl.mesh = original_mesh
    print('PASS: valid saved BL target/input hashes and rejection of a mid-audit edit')
