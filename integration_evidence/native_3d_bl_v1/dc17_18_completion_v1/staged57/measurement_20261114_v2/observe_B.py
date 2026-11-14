"""Timestamp complete emitted marker lines and request optional rank0 LBR windows."""
import json,re,sys,time
from pathlib import Path
case=Path(sys.argv[1]);profile=case/'profile';offsets={};pending={};previous={};seen=set();adaptation={}
assert case.name=='run_B'
with (case/'marker_receipts.jsonl').open('x') as out:
    while True:
        for path in profile.glob('rank*.program.log'):
            rank=int(re.search(r'rank(\d+)',path.name).group(1));before=time.monotonic_ns()
            with path.open('rb') as src:
                src.seek(offsets.get(rank,0));block=src.read();offsets[rank]=src.tell()
            after=time.monotonic_ns();text=pending.get(rank,b'')+block;parts=text.split(b'\n');pending[rank]=parts.pop()
            for line in parts:
                line=line.decode(errors='replace')
                if not line.startswith(('NATIVE3D_STAGE ','NATIVE3D_DETAIL ','NATIVE3D_QUALITY ','NATIVE3D_PHASE_OWNERSHIP ','NATIVE3D_LENGTH ')):continue
                fields=dict(re.findall(r'(\w+)=([^ ]+)',line));stage=fields.get('stage')
                if stage in ('construction_entry','surface_required_entry') and (rank not in adaptation or fields.get('rounds')=='0'):adaptation[rank]=adaptation.get(rank,0)+1
                out.write(json.dumps(dict(rank=rank,observed_after_ns=after,previous_read_ns=previous.get(rank,before),record=line.split()[0],fields=fields,line=line))+'\n');out.flush()
                if rank==0:
                    label='quality' if stage=='quality_entry' else 'interior' if stage=='split_interior_entry' else None
                    if stage=='quality_exit' and all(fields.get(k)=='0' for k in ('q_bad','j_bad','size_bad','geometry_bad')):label='finish'
                    if label:
                        phase='event'+str(adaptation.get(rank,1))+'_'+label
                        if phase not in seen:
                            (profile/'rank0.lbr_request.json').write_text(json.dumps({'phase':phase})+'\n');seen.add(phase)
            previous[rank]=after
        status=case/'status.json'
        if status.exists() and json.loads(status.read_text()).get('state')=='terminal':break
        time.sleep(.1)
