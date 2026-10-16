"""Check default relative comparisons and explicit absolute bounds for symmetry-zero quantities."""
from compare_manufactured_flow import quantity_differences


if __name__ == '__main__':
    reference = {'mass': 2.0, 'moment_z': 0.0}
    actual = {'mass': 2.000001, 'moment_z': 5e-15}
    tolerance = 1e-6
    expected = {key: abs(value - reference[key]) / max(abs(reference[key]), 1e-12)
                for key, value in actual.items()}
    assert quantity_differences(actual, reference, tolerance) == expected
    assert expected['moment_z'] > tolerance
    bounded = quantity_differences(actual, reference, tolerance, {'moment_z': 1e-12})
    assert max(bounded.values()) <= tolerance
    assert bounded['mass'] == expected['mass']
    assert quantity_differences({'mass': 2.0, 'moment_z': 2e-12}, reference,
                                tolerance, {'moment_z': 1e-12})['moment_z'] > tolerance
    for invalid in [-1, float('nan'), float('inf')]:
        try:
            quantity_differences(actual, reference, tolerance, {'moment_z': invalid})
        except AssertionError:
            pass
        else:
            raise AssertionError('An invalid absolute tolerance was accepted')
    print('Quantity comparison: unchanged defaults and strict symmetry-zero absolute bounds passed.')
