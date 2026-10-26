"""Check captured dependencies against every worker's exact current cells; no mesh acceptance claim."""
import gzip
import hashlib
import json
import struct
from pathlib import Path
from run import ROOT,save

class Reader:
    def __init__(self,data):self.data=data;self.pos=0
    def take(self,fmt):
        fmt='='+fmt;n=struct.calcsize(fmt)
        assert self.pos+n<=len(self.data),'truncated record'
        value=struct.unpack_from(fmt,self.data,self.pos);self.pos+=n
        return value
    def count(self,limit):
        n=self.take('Q')[0];assert n<=limit,'excessive record count';return n
    def cell(self):
        cell=self.take('Q')[0]
        nodes=[self.take('Qddd') for _ in range(4)]
        assert len(set(n[0] for n in nodes))==4
        return cell,nodes
    def end(self):assert self.pos==len(self.data),'trailing bytes'

def read(path):
    if path.exists():return path.read_bytes()
    archive=Path(str(path)+'.gz')
    if not archive.exists():
        receipts=json.loads((ROOT/'capture_archive_receipts.json').read_text())
        archive=path.parent/receipts[path.name]['stored_as']
    return gzip.decompress(archive.read_bytes())

def selfcheck():
    r=Reader(struct.pack('=QQ',1,7));assert r.count(2)==1 and r.take('Q')==(7,);r.end()
    for data in [b'',b'123']:
        try:Reader(data).take('Q')
        except AssertionError:pass
        else:raise AssertionError('truncated record accepted')
    try:Reader(struct.pack('=Q',9)).count(2)
    except AssertionError:pass
    else:raise AssertionError('oversized record accepted')

def main():
    selfcheck();folder=ROOT/'capture';world={};nodes={};records=[]
    for rank in range(6):
        p=folder/('transaction_rank'+str(rank))
        report=json.loads(Path(str(p)+'.json').read_text());records.append(report)
        assert report['begun'] and not report['truncated'],report
        r=Reader(read(Path(str(p)+'_working.bin')))
        n=r.count(1000000);assert n==report['working_cells']
        for _ in range(n):
            cid,vertices=r.cell();version=r.take('Q')[0];physical=[r.take('QQ') for _ in range(4)]
            assert cid not in world,'duplicate global cell owner'
            world[cid]=(rank,version,vertices,physical)
            for node in vertices:
                assert node[0] not in nodes or nodes[node[0]]==node[1:],'shared coordinate mismatch'
                nodes[node[0]]=node[1:]
        r.end()
    assert len(set(r['round'] for r in records))==1,'inconsistent capture epoch'
    selected=[r for r in records if r['selected']];assert len(selected)==1
    report=selected[0];assert report['ended'];assert report['feature']==[23754,24333,0]
    checks=0;initial_star=None
    for i,event in enumerate(report['events']):
        stem=folder/('transaction_rank%d_event%d'%(report['rank'],i))
        r=Reader(read(Path(str(stem)+'_dependencies.bin')));deps=[]
        for _ in range(r.count(64)):
            cid,version,rank=r.take('QQq');assert cid in world
            assert world[cid][0:2]==(rank,version),'owner/version mismatch'
            deps.append(cid);checks+=1
        r.end();assert len(deps)==len(set(deps))
        r=Reader(read(Path(str(stem)+'_old.bin')));old=[]
        for _ in range(r.count(128)):
            cid,vertices=r.cell();old.append(cid)
            assert cid in deps and world[cid][2]==vertices,'old patch differs from current mesh'
        if event['stage']=='SplitEdge' and initial_star is None:
            actual={cid for cid,(_,_,v,_) in world.items() if {23754,24333}.issubset({n[0] for n in v})}
            assert set(old)==actual,'initial edge star incomplete'
            initial_star=sorted(actual)
    assert initial_star is not None
    result=dict(global_cells=len(world),global_nodes=len(nodes),selected_rank=report['rank'],round=report['round'],
        events=len(report['events']),dependency_records_verified=checks,initial_star=initial_star,
        full_current_mesh_available=True,complete_growth_replay=False,external_contact_certified=False,accepted_repair=False)
    save(ROOT/'context_analysis.json',result);print(json.dumps(result))
    # Archive only newly generated diagnostic scratch after validating a lossless round trip.
    receipts={}
    for p in sorted(folder.glob('transaction*.bin')):
        raw=p.read_bytes();packed=gzip.compress(raw,compresslevel=6,mtime=0)
        assert gzip.decompress(packed)==raw
        target=Path(str(p)+'.gz');assert not target.exists()
        target.write_bytes(packed)
        receipts[p.name]=dict(raw_sha256=hashlib.sha256(raw).hexdigest(),raw_bytes=len(raw),gzip_bytes=len(packed),gzip_sha256=hashlib.sha256(packed).hexdigest())
        p.unlink()
    if receipts:save(ROOT/'capture_archive_receipts.json',receipts)
    receipts=json.loads((ROOT/'capture_archive_receipts.json').read_text())
    canonical={};duplicates=[]
    for name,row in sorted(receipts.items()):
        key=row['raw_sha256'];stored=row.get('stored_as',name+'.gz')
        if key not in canonical:canonical[key]=stored
        row['stored_as']=canonical[key]
        archive=folder/canonical[key]
        assert hashlib.sha256(archive.read_bytes()).hexdigest()==row['gzip_sha256']
        duplicate=folder/(name+'.gz')
        if duplicate.name!=canonical[key] and duplicate.exists():
            assert duplicate.read_bytes()==archive.read_bytes()
            duplicates.append(duplicate)
    # Publish the lossless logical-name mapping before removing duplicate diagnostic copies.
    save(ROOT/'capture_archive_receipts.json',receipts)
    for duplicate in duplicates:duplicate.unlink()
    archive_bytes=sum(p.stat().st_size for p in folder.glob('transaction*') if p.is_file())
    assert archive_bytes<=25*1024*1024,('witness output cap exceeded',archive_bytes)
    print('Compressed capture bytes:',archive_bytes)

if __name__=='__main__':main()
