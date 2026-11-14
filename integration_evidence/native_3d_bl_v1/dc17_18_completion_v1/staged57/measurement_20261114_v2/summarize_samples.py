"""Leaf sample counts, with exclusive groups and explicit unresolved attribution."""
import collections,json,re,sys,struct,subprocess
from pathlib import Path
ROW=re.compile(r'^\s*(\d+)/(\d+)\s+(\d+\.\d+):\s+([0-9a-f]+)\s+(.+)\s+\((.*)\)\s*$')
GROUPS=('wall_proof','exact_predicates','target_BL','tree','memory','MPI','pack_unpack','cavity_topology','other')

def group(symbol,dso):
    if re.search(r'libmpi|libopen-pal|libopen-rte|libpmix|/openmpi/|/openmpi[0-9]+/|/mca_',dso) or re.search(r'\b(?:MPI_|PMPI_|ompi_|opal_|mca_|PMIx_)',symbol):return 'MPI'
    if re.search(r'_Rb_tree|_Rb_tree_increment|_Rb_tree_decrement',symbol):return 'tree'
    if re.search(r'\b(?:malloc|calloc|realloc|free|memcpy|memmove|memset|cfree)\b|operator new|operator delete|_int_malloc|_int_free|__mem(?:cpy|move|set)|malloc_consolidate',symbol):return 'memory'
    if re.search(r'CNativePredicates|SurfaceNormalSign|Orient3|orient3|OrientationSign|InSphere|SU2Native3D::\(anonymous namespace\)::(?:Integer<|Coordinate\(|ParametricBounds\()',symbol):return 'exact_predicates'
    if re.search(r'EntityFaceError|Bernstein|ErrorMagnitude|ClosestCurved|BoundPhysicalSurface|QuerySurfaceUnion|SubdivisionBox',symbol):return 'wall_proof'
    if re.search(r'BoundaryGuidance|CBoundaryLayerMetric|CNativeMetric|MetricAt|EvaluateMetric|FrozenField|SensorField|MetricTensor|dc10::OptionA::InStrip',symbol):return 'target_BL'
    if re.search(r'packFields|unpackFields|RecordStream|World::exchange|Serialize|Deserialize|PackFields|UnpackFields',symbol):return 'pack_unpack'
    if re.search(r'Cavity|Collapse|Split|Swap|BuildStar|BuildLink|Topology|CNativeEngine3D::',symbol):return 'cavity_topology'
    return 'other'

def summarize(path,start=None,end=None):
    prefix=path.name.split('.')[0];meta=json.loads((path.parent/(prefix+'.exec.json')).read_text())
    groups=collections.Counter();symbols=collections.Counter();first=last=None;ignored=0
    with path.open() as src:
        for line in src:
            m=ROW.match(line)
            if not m:continue
            pid,tid,t,ip,symbol,dso=m.groups();t=float(t)
            if int(pid)!=meta['pid'] or t<meta['monotonic_ns']/1e9 or (start is not None and t<start) or (end is not None and t>=end):ignored+=1;continue
            groups[group(symbol,dso)]+=1;symbols[(symbol,dso)]+=1
            first=t if first is None else min(first,t);last=t if last is None else max(last,t)
    total=sum(groups.values())
    def share(n):return dict(samples=n,percent=100*n/total if total else None,resolution='resolved' if n>=100 else 'not resolved')
    return dict(rank=int(prefix[4:]),native_pid=meta['pid'],native_samples=total,ignored_other_pid_before_exec_or_outside_window=ignored,window_start=start,window_end=end,first_monotonic=first,last_monotonic=last,groups={g:share(groups[g]) for g in GROUPS},top20=[dict(symbol=s,dso=d,**share(n)) for (s,d),n in symbols.most_common(20)],all_symbols=[dict(symbol=s,dso=d,group=group(s,d),samples=n) for (s,d),n in symbols.most_common()])

assert group('std::_Rb_tree<int>::find()', 'test_driver')=='tree'
assert group('ompi_request_wait_completion', 'libmpi.so')=='MPI'
assert group('[unknown]', '/usr/lib/x86_64-linux-gnu/openmpi/lib/openmpi3/mca_btl_vader.so')=='MPI'
assert group('EntityFaceError()', 'test_driver')=='wall_proof'
assert group('memcpy', 'libc.so')=='memory'
assert ROW.match(' 123/123 12345.123456789: 5a83c7 hello (test_driver)').group(5)=='hello'
def snapshot(profile):
    # A pipe stream sorts samples at EOF; decode a complete-record snapshot for live inspection.
    for raw in profile.glob('rank*.perf.data'):
        data=raw.read_bytes();assert data[:16]==b'PERFILE2\x10\0\0\0\0\0\0\0'
        end=16
        while end+8<=len(data):
            _,_,size=struct.unpack_from('<IHH',data,end)
            assert size>=8
            if end+size>len(data):break
            end+=size
        snap=raw.with_name(raw.name.replace('.perf.data','.snapshot.data'));snap.write_bytes(data[:end])
        output=snap.with_name(snap.name.replace('.data','.samples.txt'))
        with output.open('w') as out,snap.with_suffix('.stderr').open('w') as err:
            result=subprocess.run(['/usr/bin/perf','script','-i',str(snap),'--ns','-F','pid,tid,time,ip,sym,dso'],stdout=out,stderr=err)
        assert result.returncode==0
        snap.unlink()

if __name__=='__main__':
    for case in sys.argv[1:]:
        live=case.startswith('--snapshot=');p=Path(case.split('=',1)[1] if live else case)
        if live:snapshot(p/'profile')
        pattern='rank*.snapshot.samples.txt' if live else 'rank[0-9].samples.txt'
        result=[summarize(f) for f in sorted((p/'profile').glob(pattern))]
        (p/('snapshot_summary.json' if live else 'samples_summary.json')).write_text(json.dumps(result,indent=2)+'\n')
        for row in result:
            print(p.name,row['rank'],row['native_samples'],{g:round(v['percent'] or 0,2) for g,v in row['groups'].items()})
            print([(s['samples'],s['symbol'][:180]) for s in row['top20'][:8]])
