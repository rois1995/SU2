"""B0CELLS1: small JSON header + packed little-endian .bin; legacy JSON still reads.

Cell rows: class/ar/db u8, quality/distance f64, face i64, 3 (2D) or 6
(3D) edge lengths f32. Face rows: step/kind i32, piece i64, before/after/final
quality f64, retained u8. Exact qualities preserve the existing gate decisions.
Rows are decoded on demand so archived 3D populations need no dict per cell.
"""
import json
import mmap
import pathlib
import struct
from collections.abc import Sequence


class Rows(Sequence):
    def __init__(self, data, offset, count, dim=None):
        self.data, self.offset, self.count = data, offset, count
        self.dim = dim
        self.record = struct.Struct('<BBBddq' + ('3f' if dim == 2 else '6f')) if dim else struct.Struct('<iiqdddB')

    def __len__(self):
        return self.count

    def __getitem__(self, index):
        if isinstance(index, slice):
            return [self[i] for i in range(*index.indices(self.count))]
        if index < 0:
            index += self.count
        if not 0 <= index < self.count:
            raise IndexError(index)
        row = self.record.unpack_from(self.data, self.offset + index * self.record.size)
        if self.dim:
            return dict(zip(('class', 'ar', 'db', 'q', 'distance', 'face', 'edges'), (*row[:6], list(row[6:]))))
        return dict(zip(('step', 'kind', 'piece', 'beforeQ', 'afterQ', 'finalQ', 'retainedFace'),
                        (*row[:2], str(row[2]), *row[3:6], bool(row[6]))))


def read(path):
    path = pathlib.Path(path)
    artifact = json.loads(path.read_text())
    if 'format' not in artifact:
        return artifact
    if artifact['format'] != 'B0CELLS1' or artifact['dim'] not in (2, 3):
        raise ValueError('unsupported cell artifact format/dimension')
    with path.with_suffix('.bin').open('rb') as f:
        if f.seek(0, 2) != artifact['bytes'] or artifact['bytes'] <= 0:
            raise ValueError('truncated or trailing cell artifact payload')
        data = mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ)
    offset = 0
    for m in [artifact['candidate']] + artifact['serial']:
        if type(m['ne']) is not int or m['ne'] < 0 or m['offset'] != offset:
            raise ValueError('invalid cell artifact population/offset')
        m['cells'] = Rows(data, offset, m['ne'], artifact['dim'])
        offset += len(m['cells']) * m['cells'].record.size
    faces = artifact['faces']
    if type(faces['count']) is not int or faces['count'] < 0 or faces['offset'] != offset:
        raise ValueError('invalid face artifact population/offset')
    artifact['faces'] = Rows(data, offset, faces['count'])
    if offset + len(artifact['faces']) * artifact['faces'].record.size != artifact['bytes']:
        raise ValueError('cell artifact layout/size mismatch')
    return artifact
