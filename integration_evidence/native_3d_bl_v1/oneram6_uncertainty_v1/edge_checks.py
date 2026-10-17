#!/usr/bin/env python3
"""Connected metric controls and saved-cell estimates after the closest-feature precision correction."""
import json, sys
from pathlib import Path
from run import HERE, OLD, invoke, sha


def main():
    tag = {'HELPER_SHA256': sha(Path(__file__).resolve())}
    fixture = OLD / 'cell329/replay.dat'
    label = sys.argv[1] if len(sys.argv) > 1 else 'normal_classification_v1'
    assert '/' not in label and label not in ('.', '..')
    campaign = HERE / label
    controls = campaign / 'controls'
    filters = '[NativeChordEnvelope],[NativeComplexityNormalization],[NativeComplexityEstimate3D],[NativeComposite3D],[NativeVolumeBL3D],[NativeParallelBL3D],[NativeCurvedBL3D],[NativePlanarTangentEdge3D],[NativeBalancedSplit3D],[NativeCurvedCoverage3D],[NativeLayeredField3D]'
    invoke(controls / '3d', tag, fixture, test_filter=filters)
    invoke(controls / '2d', tag, fixture, test_filter='[NativeComposite2D]')
    folder = campaign / 'scalar'
    report = {'scope': 'Bounded original P1 fields after closest-feature displacement precision correction; no complete-field mass or current independent volume reference claim.', 'cells': []}
    old = json.loads((HERE / 'distance_band_scalar_v1/summary.json').read_text())['original_cells']
    for cell in (88, 157, 329):
        case = folder / ('cell%d' % cell)
        result = invoke(case, dict(tag, SU2_NATIVE_BL_CORRECTION='1'), OLD / ('cell%d' % cell) / 'replay.dat')
        data = json.loads((case / 'correction.json').read_text())
        for offset in (0, 2):
            for key in ('value', 'indicator', 'evaluations'):
                assert data['runs'][offset][key] == data['runs'][offset + 1][key]
        baseline = next(row for row in old if row['cell'] == cell)
        data.update(cell=cell, provenance_sha256=result['provenance_sha256'])
        data['changes_from_prior_point_evaluation'] = [a['value'] - b['value'] for a, b in zip(data['runs'], baseline['runs'])]
        report['cells'].append(data)
    (folder / 'summary.json').write_text(json.dumps(report, indent=2) + '\n')


if __name__ == '__main__':
    main()
