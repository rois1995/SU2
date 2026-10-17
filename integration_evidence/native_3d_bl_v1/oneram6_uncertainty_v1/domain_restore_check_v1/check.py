import json,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
import support_correction as experiment
from run import ROOT,sha
folder=Path(__file__).resolve().parent
original=experiment.SHARED/'manifest.json'
manifest=folder/'manifest.json'
manifest.write_bytes(original.read_bytes());before=sha(manifest)
experiment.SHARED=folder
experiment.domain()
source=folder/'original.bin';expected=json.loads(manifest.read_text())
assert sha(source)==expected['original_sha256'] and sha(manifest)==before
experiment.domain()
source.unlink();source.write_bytes(b'intentional invalid export')
try:experiment.domain()
except AssertionError:pass
else:raise AssertionError('Changed original export was silently overwritten')
assert source.read_bytes()==b'intentional invalid export'
source.unlink()
result=dict(status='PASS_DOMAIN_RESTORATION_AND_NO_OVERWRITE',restored_bytes=expected['bytes'],restored_sha256=expected['original_sha256'],manifest_sha256=before,manifest_preserved=True,changed_existing_payload_rejected=True,temporary_restored_payload_removed=True,helper_sha256=sha(ROOT/'integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/support_correction.py'),check_sha256=sha(Path(__file__)))
(folder/'result.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result),flush=True)
