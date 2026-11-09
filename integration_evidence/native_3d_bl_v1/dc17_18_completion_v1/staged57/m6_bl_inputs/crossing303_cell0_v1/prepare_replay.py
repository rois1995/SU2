"""Convert the captured cell JSON to the strict 50-double hidden-probe input."""
from pathlib import Path
import json, math
root = Path(__file__).resolve().parent
cell = json.loads((root / 'cell.json').read_text())
values = [cell['core'], cell['log_scale']]
values += [x for point in cell['coordinates'] for x in point]
values += [x for tensor in cell['sensor'] for row in tensor for x in row]
assert len(values) == 50 and all(math.isfinite(x) for x in values)
(root / 'cell.txt').write_text('\n'.join(format(x, '.17g') for x in values) + '\n')
