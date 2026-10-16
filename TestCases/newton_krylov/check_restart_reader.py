"""Check binary restart coordinates and conserved-field offsets without launching a solver."""
from pathlib import Path
import struct
from tempfile import TemporaryDirectory

from compare_manufactured_flow import state


def check_reader(directory, dimensions, coordinates, conserved):
    """Write two known SU2 records and retain their complete coordinate and field identities."""
    fields = ['x', 'y', 'z'][:dimensions] + ['Density', 'Momentum_x', 'Momentum_y']
    if dimensions == 3:
        fields.append('Momentum_z')
    fields.append('Energy')
    path = directory / f'restart_{dimensions}d.dat'
    with path.open('wb') as stream:
        stream.write(struct.pack('5i', 535532, len(fields), len(coordinates), 0, 0))
        for field in fields:
            stream.write(field.encode().ljust(33, b'\0'))
        for point, values in zip(coordinates, conserved):
            row = point + values
            stream.write(struct.pack(f'{len(row)}d', *row))
    actual_fields, actual = state(path, len(fields), len(coordinates), dimensions)
    assert actual_fields == fields and actual == dict(zip(coordinates, conserved))
    if dimensions == 3:
        # Shared x,y coordinates must not silently merge distinct spanwise copies.
        try:
            state(path, len(fields), len(coordinates), 2)
        except AssertionError:
            pass
        else:
            raise AssertionError('Incomplete coordinate keys silently discarded a spanwise copy')


if __name__ == '__main__':
    with TemporaryDirectory() as temporary:
        directory = Path(temporary)
        check_reader(directory, 2, [(0.0, 0.0), (0.0, 1.0)],
                     [(1.0, 0.2, 0.3, 10.0), (2.0, 0.4, 0.6, 20.0)])
        check_reader(directory, 3, [(0.0, 0.0, 0.0), (0.0, 0.0, 0.2)],
                     [(1.0, 0.2, 0.3, 0.4, 10.0), (2.0, 0.4, 0.6, 0.8, 20.0)])
    print('Restart reader: 2D compatibility, 3D offsets and spanwise-copy checks passed.')
