"""Check export version isolation using tiny fake files; no solver or scheduler work."""
import hashlib
import json
from pathlib import Path
import tempfile
import collect_native_cluster_results as collector

with tempfile.TemporaryDirectory(prefix='native-collection-check-') as tmp:
    root = Path(tmp)
    collector.ROOT = root
    collector.TOOLS = ('integration_evidence/audit.py',)
    tool = root / collector.TOOLS[0]
    tool.parent.mkdir(parents=True)
    old = root / 'ClusterResults/tools' / collector.TOOLS[0]
    old.parent.mkdir(parents=True)
    old.write_text('old campaign audit')
    source = root / 'raw'
    source.mkdir()
    for name in ('run.cfg', 'input.su2', 'solver.log', 'run_evidence.json',
                 'frozen_sensor.csv', 'native_frozen_adapted.su2', 'frozen_sensor_source_flow.vtu'):
        (source / name).write_text('fake input')
    exports = []
    for version in ('new campaign audit', 'later campaign audit'):
        tool.write_text(version)
        destination = root / 'ClusterResults/cases' / str(len(exports))
        row = collector.collect(source, destination)
        assert row['status'] == 'COMPLETE_SELECTED_EVIDENCE' and not row['missing']
        exported = destination / row['audit_tools']['root'] / collector.TOOLS[0]
        assert exported.read_text() == version
        digest = hashlib.sha256(exported.read_bytes()).hexdigest()
        assert row['audit_tools']['files'][collector.TOOLS[0]]['sha256'] == digest
        assert json.loads((destination / 'collection_manifest.json').read_text()) == row
        exports.append((destination, version))
    assert old.read_text() == 'old campaign audit'
    for destination, version in exports:
        assert (destination / 'tools' / collector.TOOLS[0]).read_text() == version
    try:
        collector.collect(source, exports[0][0])
    except AssertionError:
        pass
    else:
        raise AssertionError('Existing exported evidence was overwritten')
    (source / 'frozen_sensor_source_flow.vtu').unlink()
    incomplete = collector.collect(source, root / 'ClusterResults/cases/incomplete')
    assert incomplete['status'] == 'INCOMPLETE_SELECTED_EVIDENCE'
    assert incomplete['missing'] == ['frozen_sensor_source_flow.vtu']
    tool.unlink()
    missing_tool = root / 'ClusterResults/cases/missing_tool'
    try:
        collector.collect(source, missing_tool)
    except FileNotFoundError:
        pass
    else:
        raise AssertionError('Missing audit tool was accepted')
    assert not (missing_tool / 'collection_manifest.json').exists()
print('PASS: independent case audit versions, unchanged older exports, tool hashes, duplicate rejection, missing-file evidence and no premature manifest on tool failure; no solver or scheduler run.')
