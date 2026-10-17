"""Finalize unchanged executed cases; preserve the report-only helper correction explicitly."""
import hashlib,json
from pathlib import Path
import numpy as np
import campaign as c
path=Path(c.__file__);before=path.read_bytes();archive=path.with_name('executed_campaign.py')
assert not archive.exists();archive.write_bytes(before)
def serializable(value):
 if isinstance(value,np.generic):return value.item()
 raise TypeError(type(value).__name__)
c.save=lambda p,v:p.write_text(json.dumps(v,indent=2,allow_nan=False,default=serializable)+'\n')
c.main() # Original runtime/helper hashes still match; invoke must reuse every completed case.
old=b"  mass=json.loads((folder/'reassessment_policy_m2_stage0.json').read_text())"
assert before.count(old)==1
after=before.replace(old,b'  reference=float(reference)\n'+old);path.write_bytes(after)
transition={'scope':'Report serialization only. Completed runtime cases reused with their unchanged executed helper, source, binaries, inputs and logs.',
 'executed_helper_sha256':hashlib.sha256(before).hexdigest(),'corrected_helper_sha256':hashlib.sha256(after).hexdigest(),
 'correction':'Convert NumPy quadrature reference to Python float before deriving a JSON Boolean.',
 'finalizer_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest()}
path.with_name('report_helper_transition.json').write_text(json.dumps(transition,indent=2)+'\n')
