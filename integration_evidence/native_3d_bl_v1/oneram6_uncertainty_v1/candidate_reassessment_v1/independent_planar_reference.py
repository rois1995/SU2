"""Independent coarea: analytic plateau/growth integral, Gauss only across the fade."""
import hashlib,json,math
from pathlib import Path
import numpy as np
HERE=Path(__file__).resolve().parent
values=[]
for order in (32,64,96):
 t,w=np.polynomial.legendre.leggauss(order);z=.285+.015*t;s=(z-.27)/.03;fade=1-3*s*s+2*s*s*s
 hn=(.1+.2*z)/1.1
 integral=.05*math.sqrt(1.5)/.1+math.sqrt(1.5)*1.1/.2*math.log(.154/.11)+.7+.015*float(w@np.exp(fade*(.5*math.log(1.5)-np.log(hn))))
 values.append(integral)
reference=json.loads((HERE/'summary.json').read_text())['accepted_planar_RANS_mass']['reference']
assert max(abs(x-reference) for x in values)<1e-12
report={'method':'Constant-I sensor, square-wall tangent eigenvalues .5/1.5, strict intersection; analytic h0 plateau and logarithmic growth integral; Gauss only in z=.27..3 fade.',
 'orders':[32,64,96],'integrals':values,'existing_tensor_oracle_reference':reference,
 'maximum_difference':max(abs(x-reference) for x in values),'proven_continuum_bound':False,
 'helper_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest()}
(HERE/'independent_planar_reference.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
