"""Prepare serial gradation inputs without changing production files."""
import struct
import numpy as np
import metric_floor_diagnostic as d

definitions={}
exec((d.OUT/'combined_metric_tests.py').read_text().split('\ncache =')[0],definitions)
cache=np.load(d.OUT/'independent_recovery.npz');H,dual=cache['hessians'],cache['dual']
mesh=d.capcheck.read_su2(d.RUN/'m6.su2')
P,fields,_=d.capcheck.read_restart(d.RUN/'restart_flow_adap_00000.dat');N=len(P)
production=np.zeros((N,3,3))
for a,b,key in [(0,0,'XX'),(0,1,'XY'),(0,2,'XZ'),(1,1,'YY'),(1,2,'YZ'),(2,2,'ZZ')]:
    production[:,a,b]=production[:,b,a]=fields['Metric_'+key]
eig=np.linalg.eigvalsh(production);floor=eig.min()
plateau=np.max(abs(eig-floor),axis=1)<floor*1e-12
pairs=np.concatenate([mesh.E[:,[a,b]] for a in range(4) for b in range(4) if a!=b])
keys=np.unique(pairs[:,0]*N+pairs[:,1])
offsets=np.searchsorted(keys//N,np.arange(N+1)).astype('<u8')
neighbors=(keys%N).astype('<u8')
old=definitions['build'](H,dual,False,1)
# Calibrate ONLY the observed isotropic plateau, not weak ungraded directions elsewhere.
old*=floor/np.linalg.eigvalsh(old[plateau]).min()
assert np.allclose(np.linalg.eigvalsh(old[plateau]),floor,rtol=1e-12)
local=np.load(d.OUT/'local_floor_ungraded_candidate.npz')['metric']
for name,M in [('original',old),('local',local)]:
    with (d.OUT/(name+'_gradation_input.bin')).open('wb') as stream:
        stream.write(struct.pack('<QQ',N,len(neighbors)))
        for a in [P.astype('<f8'),dual.astype('<f8'),offsets,neighbors,M.astype('<f8')]:
            stream.write(a.tobytes())
print('Prepared',N,'points and',len(neighbors),'directed edges; production unchanged.')
