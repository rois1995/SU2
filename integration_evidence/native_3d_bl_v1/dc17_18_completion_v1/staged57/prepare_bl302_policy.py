"""Prepare a bounded geometry-policy probe; no CFD, normalization or mesh acceptance."""
from pathlib import Path
import hashlib,json,re
R=Path(__file__).resolve().parent;REPO=R.parents[3]
source=REPO/'integration_evidence/native_3d_bl_v1/oneram6_coarse_v1/run.cfg'
reference=R.parent/'validation54/m6_euler/m6.su2.native_ref'
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
assert sha(reference)=='2e99fb7870e2c718f110da8509e4c0f7e746ce138e7f27ab57aa0dedbb6c42de'
folder=R/'m6_bl_inputs/curved_policy302_v1';folder.mkdir(parents=True,exist_ok=False)
changes={'ADAP_SENSOR':'( MACH )','ADAP_SIZES':'( 500000 )','ADAP_COMPLEXITY':'500000','ADAP_NORM':'1',
         'ADAP_NATIVE_MEMORY_LIMIT':'12288','ADAP_NATIVE_RANKS':'1','ADAP_BL_METHOD':'FIRST_ROW','ADAP_ANGLE':'60'}
text=source.read_text()
for key,value in changes.items():
    line=key+'= '+value
    if re.search(r'^'+key+r'\s*=',text,re.M):text=re.sub(r'^'+key+r'\s*=.*$',line,text,flags=re.M)
    else:text+='\n'+line+'\n'
assert 'ADAP_NATIVE_MAX_TIME= 300' in text
config=folder/'run.cfg';config.write_text('% Geometry-policy probe only; no CFD is invoked by NativeActualCubicBLPolicy3D.\n'+text)
receipt=dict(scope='Actual original M6 no-CAD curved geometry policy only. No CFD seed is read, no normalized flow target or accepted BL mesh is claimed.',
    source_config=str(source),source_config_sha256=sha(source),config=str(config),config_sha256=sha(config),
    reference=str(reference),reference_sha256=sha(reference),changes=changes,
    preserved_constraints=dict(first_height=2e-6,growth=1.2,thickness=.01,hausd=1e-4,native_time=300),
    preparation_sha256=sha(__file__))
(folder/'provenance.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt,indent=2))
