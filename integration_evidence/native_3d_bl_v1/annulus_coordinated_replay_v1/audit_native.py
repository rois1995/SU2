#!/usr/bin/env python3
"""Reuse the independent patch audit on the native result, accounting for reserved vertex IDs."""
import hashlib,json,sys
from pathlib import Path
import audit
HERE=Path(__file__).resolve().parent
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    folder=Path(sys.argv[1]).resolve();raw=folder/'native_joint_after.json';fixture=HERE/'patch.dat'
    tokens=fixture.read_text().splitlines();header=tokens[1].split();point=int(header[3]);movers=list(map(int,tokens[2].split()))
    assert movers.pop(0)==len(movers)<=8
    reverse={point+i+1:id for i,id in enumerate(movers)}
    native=json.loads(raw.read_text());old=json.loads(audit.OLD.read_text());data=dict(old)
    data['nodes']=[[reverse.get(n[0],n[0])]+n[1:] for n in native['nodes']]
    data['cells']=[dict(id=i,nodes=[reverse.get(n,n) for n in c]) for i,c in enumerate(native['cells'])]
    data['surface']=[dict(facet=f[3],nodes=[reverse.get(n,n) for n in f[:3]]) for f in native['surface']]
    canonical=folder/'native_joint_canonical.json';payload=json.dumps(data,separators=(',',':'))+'\n'
    if canonical.exists():assert canonical.read_text()==payload,'Native artifact changed'
    else:canonical.write_text(payload)
    output=folder/'independent_patch.json'
    audit.main(cases=[('native_joint',canonical)],output_path=output)
    result=json.loads(output.read_text());result['native_artifact_sha256']=sha(raw);result['fixture_sha256']=sha(fixture)
    result['source_sha256'][str(Path(__file__).resolve().relative_to(audit.ROOT))]=sha(Path(__file__))
    result['reserved_to_original_ids']=reverse
    result['scope']='Native-produced private patch: exact volume/finite geometry/complete stars/no overlap, independent shape and changed-edge progress. No whole-mesh or transfer acceptance.'
    assert result['results']['native_joint']['refine_progress']
    output.write_text(json.dumps(result,indent=2)+'\n')
    print('NATIVE_PRIVATE_PATCH_INDEPENDENT_PASS',flush=True)
if __name__=='__main__':main()
