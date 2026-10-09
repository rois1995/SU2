"""Tiny fake exports check dispatch, hash guards, isolation and failure retention."""
import importlib.util
import json
from pathlib import Path
import tempfile

script = Path(__file__).with_name('audit_metric_comparison.py')
spec = importlib.util.spec_from_file_location('saved_audit', script)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
with tempfile.TemporaryDirectory(prefix='saved-audit-selfcheck-') as temporary:
    root = Path(temporary)
    baseline, candidate = root / 'baseline', root / 'candidate'
    rows = []
    for version, results, kind in [('baseline', baseline, 'frozen_euler_to_bl'),
                                   ('candidate', candidate, 'actual_bl_to_euler')]:
        case = results / 'cases' / version
        tools = results / 'tools' if version == 'baseline' else case / 'tools'
        tools.mkdir(parents=True)
        case.mkdir(parents=True, exist_ok=True)
        (case / 'input.su2').write_text(version)
        (case / 'collection_manifest.json').write_text('{}')
        tool = tools / 'integration_evidence' / ('audit_native_frozen_case.py' if version == 'baseline' else 'audit_rae_unsteady.py')
        tool.parent.mkdir(parents=True)
        report = 'independent_frozen_metric_audit.json' if version == 'baseline' else 'independent_rae_unsteady_audit.json'
        tool.write_text('import sys,json\nfrom pathlib import Path\n' +
                       ("assert sys.argv[2:]==['rae_euler_to_bl','--self-contained']\n" if version == 'baseline' else 'assert len(sys.argv)==2\n') +
                       f"(Path(sys.argv[1])/'{report}').write_text(json.dumps(dict(status='PASS',version='{version}')))\n")
        rows.append(dict(version=version, case=version, kind=kind,
                         data_sha256={'input.su2':module.sha(case / 'input.su2')},
                         tool_sha256={str(tool.relative_to(tools)):module.sha(tool)},
                         manifest_sha256=module.sha(case / 'collection_manifest.json')))
    assessment = root / 'assessment.json'
    assessment.write_text(json.dumps(dict(cases=rows)))
    output = root / 'report'
    assert module.run(baseline,candidate,assessment,output)==0
    report = json.loads((output / 'validation.json').read_text())
    assert report['status']=='PASS' and [r['audit']['version'] for r in report['cases']]==['baseline','candidate']
    assert not list(baseline.rglob('independent_*.json')) and not list(candidate.rglob('independent_*.json'))
    try:module.run(baseline,candidate,assessment,output)
    except FileExistsError:pass
    else:raise AssertionError('Existing report overwritten')
    (baseline / 'cases/baseline/input.su2').write_text('changed')
    try:module.run(baseline,candidate,assessment,root / 'bad')
    except ValueError:pass
    else:raise AssertionError('Changed data admitted')
    assert not (root / 'bad').exists()
    (baseline / 'cases/baseline/input.su2').write_text('baseline')
    tool.write_text('import sys\nsys.exit(3)\n')
    rows[-1]['tool_sha256'][str(tool.relative_to(tools))]=module.sha(tool)
    assessment.write_text(json.dumps(dict(cases=rows)))
    assert module.run(baseline,candidate,assessment,root / 'failed')==1
    report=json.loads((root / 'failed/validation.json').read_text())
    assert report['status']=='FAIL' and report['cases'][0]['status']=='PASS' and report['cases'][1]['exit_code']==3
print('Saved comparison audit selfcheck PASS')
