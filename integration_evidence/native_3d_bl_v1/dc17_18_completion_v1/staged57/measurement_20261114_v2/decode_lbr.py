"""Decode optional LBR without inline expansion; failures are evidence, never run stops."""
import collections,hashlib,json,re,subprocess,sys
from pathlib import Path
FRAME=re.compile(r'^\s+([0-9a-f]+)\s+(.+)\s+\((.*)\)\s*$')
def fold(text):
    stacks=collections.Counter();frames=[]
    for line in text.splitlines()+['']:
        if not line.strip():
            if frames:stacks[tuple(frames)]+=1;frames=[]
        elif line.startswith('\t') or line.startswith('        '):
            m=FRAME.match(line)
            if m:frames.append(m.group(2))
    return stacks
assert fold('\t 123 foo+0x1 (binary)\n\t 456 bar+0x2 (binary)\n\n')=={('foo+0x1','bar+0x2'):1}
for name in sys.argv[1:]:
    raw=Path(name);base=raw.with_suffix('');status=raw.with_name(raw.name.replace('.lbr.data','.lbr_status.json'))
    assert status.exists(),status
    capture=json.loads(status.read_text());meta=dict(capture_status=capture,raw_bytes=raw.stat().st_size,raw_sha256=hashlib.file_digest(raw.open('rb'),'sha256').hexdigest())
    argv=['nice','-n','19','/usr/bin/perf','script','--no-inline','--ns','-i',str(raw)]
    result=subprocess.run(argv,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
    base.with_suffix('.decode.stderr').write_text(result.stderr)
    stacks=fold(result.stdout) if result.returncode==0 else collections.Counter()
    base.with_suffix('.folded.txt').write_text(''.join(';'.join(reversed(frames))+f' {n}\n' for frames,n in stacks.most_common()))
    meta.update(argv=argv,decode_exit=result.returncode,sample_stacks=sum(stacks.values()),unique_stacks=len(stacks),raw_removed=True,stacks=[dict(leaf_to_root=list(frames),samples=n) for frames,n in stacks.most_common()])
    base.with_suffix('.summary.json').write_text(json.dumps(meta,indent=2)+'\n');raw.unlink()
    print(raw.name,'decode',result.returncode,'stacks',sum(stacks.values()),'stderr bytes',len(result.stderr))
