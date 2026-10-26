"""Materialize hash-checked local records for serial construction replay; never rerun MPI capture."""
import hashlib,json,struct
from pathlib import Path
from analyze import read
from run import ROOT,save

def main():
    capture=ROOT/'capture';out=ROOT/'replay_input';out.mkdir(exist_ok=True)
    receipts=json.loads((ROOT/'capture_archive_receipts.json').read_text())
    for name,row in receipts.items():
        data=read(capture/name);assert hashlib.sha256(data).hexdigest()==row['raw_sha256'],name
        (out/name).write_bytes(data)
    report=json.loads((capture/'transaction_rank2.json').read_text())
    data=bytearray()
    def pack(fmt,*values):data.extend(struct.pack('='+fmt,*values))
    def text(value):
        b=value.encode();pack('Q',len(b))
        for c in b:pack('Q',c)
    pack('Q',len(report['events']))
    for e in report['events']:
        text(e['stage']);text(e['reason']);pack('Q',e['passed'])
        pack('Qddd',*e['point']);pack('QQQdd',e['first_cell'],e['first_node'],e['metric_failure'],
            float('nan') if e.get('q') is None else e['q'],float('nan') if e.get('J') is None else e['J'])
    (out/'events.bin').write_bytes(data)
    save(out/'pins.json',{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in out.iterdir() if p.is_file() and p.name!='pins.json'})
    print('Prepared',len(receipts),'verified records; no input mesh or metric rebuilt.')

if __name__=='__main__':main()
