"""Read-only directional diagnostics from validated checkpoint witnesses; never restore a production target."""
from pathlib import Path
import hashlib,json,struct,sys
import numpy as np
R=Path(__file__).resolve().parent
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
def inspect(path,expected):
    data=path.read_bytes();assert hashlib.sha256(data).hexdigest()==expected
    assert sys.byteorder=='little'
    integer=lambda offset:struct.unpack_from('<Q',data,offset)[0]
    scalar=lambda word:struct.unpack_from('<d',data,word*8)[0]
    assert integer(0)==0x4e424c5441524731 and integer(8)==2
    reference_bytes,nodes,owned,span,layers,roles=struct.unpack_from('<6Q',data,16)
    assert owned<=nodes<=61330 and layers==1 and roles<1024
    # Version2: original79scalars + four exact stage scalars; chars occupy one scalar.
    assert struct.unpack_from('<3Q',data,79*8)==(5,0,7)
    offset=83*8
    for k in range(layers):
        count=integer(offset);offset+=8;assert count<1024
        name=''.join(chr(integer(offset+8*j)) for j in range(count));offset+=8*count
        h0,growth,thickness=struct.unpack_from('<3d',data,offset);offset+=24
        assert h0==6.4e-5 and growth==1.2 and thickness==.01
    offset+=16*roles
    for k in range(2):
        count=integer(offset);offset+=8;assert count<100000
        offset+=24*count
    result={}
    for k in range(nodes):
        row=struct.unpack_from('<QQ21d',data,offset);offset+=184
        identity,local=row[:2];assert identity<span and local<nodes
        if local<owned:
            assert identity not in result
            result[identity]=(np.array(row[2:5]),np.array(row[5:14]).reshape(3,3),np.array(row[14:23]).reshape(3,3))
            offset+=96  # Original frame9 + original eigenvalues3, not a target witness.
    assert len(result)==owned and offset+reference_bytes+8==len(data)
    reference_sha=hashlib.sha256(data[offset:offset+reference_bytes]).hexdigest()
    return result,dict(core=scalar(20),tangent_scale=scalar(21),owned=owned,reference_sha256=reference_sha)
def main():
    receipt=R/'target_checkpoint441_producer.json';saved=json.loads(receipt.read_text());directory=Path(saved['producer'])
    geometry_path=R/'geometry369_assessment.json';geometry=json.loads(geometry_path.read_text())
    all_nodes={};headers=[];pins={str(receipt):sha(receipt),str(geometry_path):sha(geometry_path)}
    for rank in range(6):
        path=directory/f'target_rank{rank}.bin';nodes,header=inspect(path,saved['files'][path.name]['sha256']);headers.append(header);pins[str(path)]=sha(path)
        json_path=directory/f'saved_cfd_metric_rank{rank}.jsonl';pins[str(json_path)]=sha(json_path)
        rows=[json.loads(line) for line in json_path.read_text().splitlines()];assert header['core']==rows[0]['core'] and header['tangent_scale']==rows[0]['tangential_scale']
        checked=set()
        for row in rows[1:]:
            if 'id' not in row:continue
            identity=row['id'];point,core,witness=nodes[identity];assert np.array_equal(point,row['x'])
            assert np.array_equal(core[np.triu_indices(3)],row['core_tensor'])
            assert np.array_equal(witness,witness.T) and np.all(np.isfinite(witness))
            assert np.linalg.eigvalsh(witness)[0]>0
            checked.add(identity)
        assert len(checked)==len(nodes)
        for identity,value in nodes.items():assert identity not in all_nodes;all_nodes[identity]=value
    assert len(all_nodes)==61330 and len({x['reference_sha256'] for x in headers})==1
    output=[]
    for row in geometry['points']:
        point,flow,combined=all_nodes[row['vertex']];assert np.linalg.norm(point-np.array(row['point']))<1e-12
        directions=np.array(row['directions']);normal=np.cross(*directions);normal/=np.linalg.norm(normal)
        assert np.max(np.abs(directions@directions.T-np.eye(2)))<1e-8
        density=lambda tensor,v:float(np.sqrt(v@tensor@v))
        curvature=np.abs(row['curvatures']);lam=1.8**2*curvature/(8e-4)
        qflow=[density(flow,v) for v in directions];qcombined=[density(combined,v) for v in directions]
        output.append(dict(context=row['context'],facet=row['facet'],vertex=row['vertex'],point=point.tolist(),principal_curvatures=row['curvatures'],directions=row['directions'],flow_directional_density=qflow,combined_directional_density=qcombined,combined_normal_density=density(combined,normal),local_osculating_geometry_density=np.sqrt(lam).tolist(),combined_over_local_geometry_density=[float(qcombined[k]/np.sqrt(lam[k])) if lam[k]>0 else None for k in range(2)]))
    report=dict(scope=__doc__,nodes_crosschecked_against_producer_JSON=61330,points=output,reference_sha256=headers[0]['reference_sha256'],core=headers[0]['core'],inherited_tangent_scale=headers[0]['tangent_scale'],limits=['Read-only version2 checkpoint inspection; official C++decoder/fresh-process fidelity remains a separate requirement.','Witnesses only at original fixed coordinates, never interpolation/transport authority.','Curvatures/directions are369local differential samples on the same immutable authority, not global bounds or certification.','Osculating proxy uses lambda=1.8^2*abs(curvature)/(8*1e-4); not a conservative geometry metric or a proven required mesh spacing.','Flow/full density comparisons are not an additive flow/geometry/height decomposition.','No movement/topology candidate, complete-patch admissibility, generated BL or target reasonableness acceptance.'],input_sha256=pins,helper_sha256=sha(__file__))
    path=R/'target_directions441_assessment.json';assert not path.exists();path.write_text(json.dumps(report,indent=2,allow_nan=False)+'\n')
    print(json.dumps(dict(samples=len(output),by_context={name:dict(samples=sum(x['context']==name for x in output),median_combined_density=np.median([x['combined_directional_density'] for x in output if x['context']==name],axis=0).tolist(),median_local_geometry_density=np.median([x['local_osculating_geometry_density'] for x in output if x['context']==name],axis=0).tolist()) for name in sorted({x['context'] for x in output})})))
if __name__=='__main__':main()
