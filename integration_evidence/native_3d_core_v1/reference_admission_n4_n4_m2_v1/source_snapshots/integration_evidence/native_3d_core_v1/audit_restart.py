"""Independent persisted BDF2 state audit using the repository's binary restart reader."""
from pathlib import Path
import copy,math,struct,sys
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'TestCases/adaptation/capability'))
from capcheck import read_restart
from audit_geometry_bridge import affine,reject

FIELDS=('Density','Momentum_x','Momentum_y','Momentum_z','Energy')

def state_rows(rows,points):
    assert len(rows)==len(points) and len({r['id'] for r in rows})==len(rows)
    result={tuple(r['point']):r['states'] for r in rows}
    assert set(result)==points and len(result)==len(rows)
    maximum=0.
    for point,values in result.items():
        assert len(values)==15 and all(math.isfinite(v) for v in values)
        assert values[:5]==values[5:10]
        expected=[factor*v for factor in (1.,1.,.96875) for v in affine(point)]
        maximum=max(maximum,max(abs(a-b)/max(abs(a),abs(b),1e-8) for a,b in zip(values,expected)))
    assert maximum<2e-10,'Restarted current/BDF2 manufactured-state residual'
    return result,maximum

def binary_state(path,wanted,offset):
    assert path.is_file(),'Required BDF2 history file missing'
    data=path.read_bytes();assert len(data)>=20
    magic,nvar,npoint,size,reserved=struct.unpack_from('<5i',data)
    assert magic==535532 and size==8 and reserved==0 and npoint==len(wanted) and nvar==8
    assert len(data)==20+33*nvar+size*npoint*nvar,'Restart truncation or trailing bytes'
    points,fields,precision=read_restart(path)
    assert precision=='Float64' and set(fields)==set(('x','y','z')+FIELDS)
    tuples=[tuple(float(v) for v in point) for point in points]
    assert len(set(tuples))==len(tuples) and set(tuples)==set(wanted)
    maximum=0.
    for index,point in enumerate(tuples):
        values=[float(fields[name][index]) for name in FIELDS]
        assert all(math.isfinite(v) for v in values)
        expected=wanted[point][offset:offset+5]
        maximum=max(maximum,max(abs(a-b)/max(abs(a),abs(b),1e-8) for a,b in zip(values,expected)))
    assert maximum<2e-10,'Persisted current/older BDF2 state residual'
    return dict(file=path.name,points=npoint,fields=list(fields),maximum_state_relative_error=maximum)

def audit(folder,state,checkpoint):
    points={tuple(n[1:]) for n in checkpoint['nodes']}
    donor,error0=state_rows(state['donor'],points);loaded,error1=state_rows(state['accepted'],points)
    maximum=max(abs(a-b)/max(abs(a),abs(b),1e-8) for point in points for a,b in zip(donor[point],loaded[point]))
    assert maximum<2e-10,'Actual reader changed the persisted state'
    assert any(values[:5]!=values[10:] for values in donor.values()),'History test must distinguish the two files'
    current=folder/'envelope_restart_00002.dat';older=folder/'envelope_restart_00001.dat'
    files=[binary_state(current,donor,0),binary_state(older,donor,10)]
    bad=copy.deepcopy(state['accepted']);bad[0]['states'][10]+=.1
    reject(lambda:state_rows(bad,points))
    reject(lambda:state_rows(state['accepted'][:-1],points))
    reject(lambda:binary_state(current,donor,10))
    reject(lambda:binary_state(folder/'missing_bdf_history.dat',donor,10))
    return dict(points=len(points),maximum_state_relative_error=max(error0,error1,maximum),files=files,adversaries_rejected=4,
                scope='Actual two-file binary BDF2 write and real driver construction/Preprocess reload, matched by physical coordinates. Frozen sensor source is freshly constructed at the next event; no tensor-source persistence or PDE iteration claim.')
