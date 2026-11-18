"""Bind the qualified N14 workflow and isolated restart to the selected phase-C build."""
from argparse import ArgumentParser
from pathlib import Path
import hashlib
import json
import shutil

parser = ArgumentParser(description=__doc__)
parser.add_argument('build', type=int)
args = parser.parse_args()
phase = Path(__file__).resolve().parents[1]
night = phase.parents[1]
reference = night / 'N14/section6_12/validation'
build = phase / f'build{args.build}'
assert json.loads((build / 'build_status.json').read_text())['pins_verified']
end = json.loads((phase / 'C11/validation/acceptance_summary.json').read_text())
assert all(end[gate]['passed'] for gate in ('G3', 'G4', 'GQ'))
if args.build != end['build']:
    measurement = json.loads((phase / 'M27/validation/acceptance_summary.json').read_text())
    assert measurement['state'] == 'PASSED' and measurement['build'] == args.build
    assert all(measurement[gate]['passed'] for gate in ('G2', 'G3', 'G4', 'GQ'))
workspace = phase / 'phase_C/G5'
workspace.mkdir(exist_ok=False)
names = ('check_connected_events.py', 'check_complete_events.py', 'check_timer_tree.py',
         'unchanged_build499_source_pins.json')
for name in names:
    shutil.copyfile(reference / name, workspace / name)
bindings = {'check_connected_events.py': ('TIMER_PATH = HERE.parent / "f3b_20261115" / "check_timer_tree.py"', 'STAGED = next(parent for parent in HERE.parents if parent.name == "staged57")\nTIMER_PATH = STAGED / "f3b_20261115" / "check_timer_tree.py"'), 'check_timer_tree.py': ('sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "nobl_clean_check_20261114"))', 'STAGED = next(parent for parent in Path(__file__).resolve().parents if parent.name == "staged57")\nsys.path.insert(0, str(STAGED / "nobl_clean_check_20261114"))')}
for name, (old, new) in bindings.items():
    helper = workspace / name
    text = helper.read_text()
    assert text.count(old) == 1
    helper.write_text(text.replace(old, new, 1))
for stem in ('run_full', 'run_restart', 'validate_restart'):
    name = stem + '552.py'
    text = (reference / name).read_text().replace('N11 = HERE.parent', 'N11 = HERE.parents[1]')
    text = text.replace('N11.name == "section6_12"', 'N11.name == "section6_3_after_N11"')
    text = text.replace('HERE.name == "validation"', 'HERE.name == "G5"')
    text = text.replace('552', str(args.build))
    (workspace / f'{stem}{args.build}.py').write_text(text)
pins = {name.name: hashlib.sha256(name.read_bytes()).hexdigest() for name in workspace.iterdir()}
(workspace / 'retargeted_helper_pins.json').write_text(json.dumps(pins, indent=2) + '\n')
print(workspace)
