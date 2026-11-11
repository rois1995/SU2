"""Cheap analytic BL-law controls only; no substitute for the composed M6 metric."""
from pathlib import Path
import json,math,hashlib
nodes=(.1488743389816312109,.4333953941292471908,.6794095682990244062,.8650633666889845107,.9739065285171717201)
weights=(.2955242247147528702,.2692667193099963551,.2190863625159820440,.1494513491505805931,.0666713443086881376)
def gl10(f,a,b):
    mid=(a+b)/2;half=(b-a)/2
    return half*sum(w*(f(mid-half*n)+f(mid+half*n)) for n,w in zip(nodes,weights))
assert abs(gl10(lambda t:1,0,1)-1)<1e-15
assert abs(gl10(lambda t:t**18,0,1)-1/19)<1e-15
h0,g=2e-6,1.2
rows=[]
inverse=lambda length:(h0*(g+1)/2*math.exp((length-.5)*2*(g-1)/(g+1))-h0)/(g-1)
for D in (h0*.4,h0,1e-5,1e-4,1e-3,*(inverse(length) for length in (.899995,.900005,1.799995,1.800005))):
    cut=h0/2
    exact=D/h0 if D<=cut else .5+(g+1)/(2*(g-1))*math.log((h0+(g-1)*D)/(h0+(g-1)*cut))
    f=lambda d:1/max(h0,2*(h0+(g-1)*d)/(g+1))
    plain=gl10(f,0,D)
    parts=gl10(f,0,min(D,cut))+(gl10(f,cut,D) if D>cut else 0)
    rows.append(dict(normal_displacement=D,analytic=exact,fixed10_whole=plain,fixed10_per_profile_piece=parts,
                     whole_relative_error=abs(plain-exact)/exact,piece_relative_error=abs(parts-exact)/exact,
                     whole_split_difference=(plain>1.8)!=(exact>1.8),whole_coarsen_difference=(plain<.9)!=(exact<.9)))
out=dict(scope='Analytic planar pure-normal continuous hn law only, no flow/intersections/fade/curvature. Not an actual mesh or M6 composed-target result.',first_height=h0,growth=g,rows=rows,
         lesson='Known clamp cuts alone do not guarantee a fixed small rule resolves strong but physically prescribed variation. No production tolerance or decision changed.',script_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
Path(__file__).with_name('edge_accuracy382_profile_controls.json').write_text(json.dumps(out,indent=2)+'\n')
print(json.dumps(out,indent=2))
