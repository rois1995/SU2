import sys,re,collections,subprocess
path=sys.argv[1]; anchor=sys.argv[2]; depth=int(sys.argv[3]) if len(sys.argv)>3 else 1
samples=[];cur=None
for line in open(path,errors='replace'):
    if not line.strip():
        if cur and cur[0].startswith('test_driver'): samples.append(cur[1])
        cur=None;continue
    if not line.startswith('\t'):
        cur=(line.split()[0],[]);continue
    parts=line.strip().split(' ',1)
    sym=parts[1] if len(parts)>1 else '?'
    sym=re.sub(r'\+0x[0-9a-f]+$','',sym)
    cur[1].append(sym)
if cur and cur[0].startswith('test_driver'): samples.append(cur[1])
def short(s):
    s=re.sub(r'\(.*','',s); return s[-90:]
tot=len(samples); inA=0; c=collections.Counter(); self_=collections.Counter()
for st in samples:
    idx=[i for i,f in enumerate(st) if anchor in f]
    if not idx: continue
    inA+=1; i=max(idx)  # outermost
    key=' > '.join(short(st[j]) for j in range(i-1,max(i-1-depth,-1),-1)) if i>0 else '(self)'
    c[key]+=1; self_[short(st[0])]+=1
print('samples',tot,'with anchor',inA, '%.1f%%'%(100*inA/tot))
for k,v in c.most_common(40): print('%6.2f%% %s'%(100*v/tot,k))
print('--- leaf self within anchor')
for k,v in self_.most_common(25): print('%6.2f%% %s'%(100*v/tot,k))
