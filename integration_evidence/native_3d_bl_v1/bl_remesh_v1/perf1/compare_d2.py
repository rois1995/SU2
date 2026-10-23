# Compare before/after D2 dumps: bitwise long-double lengths, split success sets and certified split agreement.
import sys,json
def load(path):
    L,S={},{}
    for line in open(path):
        f=line.split()
        if f[0]=='L': L[f[1]]=f[2]
        elif f[0]=='S': S[f[1]]=None if f[2]=='ERR' else (f[2:5],float.fromhex(f[5].replace('L','')) if False else f[5],f[6])
    return L,S
def ld(h):  # x87 hex float "0x8.16e5p-2" -> float (comparison only)
    m,e=h.split('p');return float.fromhex(m+'p'+e)
out={}
for s in ('witnesses','controls'):
    Lb,Sb=load(f'd2_before/{s}.dump');La,Sa=load(f'd2_after/{s}.dump')
    both=[k for k in Sb if Sb[k] and Sa.get(k)]
    worst=0.0;identical=0
    for k in both:
        pb,qb,rb=Sb[k];pa,qa,ra=Sa[k]
        identical+=pb==pa
        worst=max(worst,abs(ld(qa)-ld(qb))/ld(rb))
    out[s]={'edges':len(Lb),'lengths_bitwise_equal':sum(Lb[k]==La.get(k) for k in Lb),
            'splits_before_ok':sum(1 for v in Sb.values() if v),'splits_after_ok':sum(1 for v in Sa.values() if v),
            'ok_before_fail_after':sum(1 for k in Sb if Sb[k] and not Sa.get(k)),
            'fail_before_ok_after':sum(1 for k in Sa if Sa[k] and not Sb.get(k)),
            'both_ok':len(both),'both_ok_identical_point':identical,
            'max_prefix_difference_relative_to_half_length':worst}
print(json.dumps(out,indent=1))
