#!/usr/bin/env python3
"""Compare two DC-8 runs (before/after production binaries): final meshes byte-identical and every non-timing JSON
field equal. Usage: compare_identity.py <root_before> <root_after> case..."""
import sys,json,hashlib,os
TIMING={'seconds','ms_per_op','peak_rss_kB','rss','total_seconds','binary_sha256','perf','field','phase_field'}
def strip(x):
    if isinstance(x,dict): return {k:strip(v) for k,v in x.items() if k not in TIMING}
    if isinstance(x,list): return [strip(v) for v in x]
    return x
def diff(a,b,path=''):
    out=[]
    if isinstance(a,dict) and isinstance(b,dict):
        for k in sorted(set(a)|set(b)):
            if k not in a or k not in b: out.append(path+'/'+k+' missing'); continue
            out+=diff(a[k],b[k],path+'/'+k)
    elif a!=b: out.append('%s: %r != %r'%(path,a,b))
    return out
b0,b1=sys.argv[1],sys.argv[2];ok=True;report={}
for c in sys.argv[3:]:
    m0=os.path.join(b0,'dc8_%s_final.mesh'%c);m1=os.path.join(b1,'dc8_%s_final.mesh'%c)
    h0=hashlib.sha256(open(m0,'rb').read()).hexdigest();h1=hashlib.sha256(open(m1,'rb').read()).hexdigest()
    j0=strip(json.load(open(os.path.join(b0,'dc8_%s.json'%c))));j1=strip(json.load(open(os.path.join(b1,'dc8_%s.json'%c))))
    d=diff(j0,j1)
    report[c]={'mesh_identical':h0==h1,'mesh_sha256':h1,'json_differences':d[:20]}
    ok=ok and h0==h1 and not d
    print(c,'mesh identical' if h0==h1 else 'MESH DIFFERS',len(d),'json diffs'); [print('   ',x) for x in d[:10]]
report['all_identical']=ok;json.dump(report,open(os.path.join(b1,'identity.json'),'w'),indent=1)
print('ALL IDENTICAL' if ok else 'DIFFERENCES'); sys.exit(0 if ok else 1)
