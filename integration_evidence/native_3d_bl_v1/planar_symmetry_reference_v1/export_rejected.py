#!/usr/bin/env python3
"""Export saved worker-local rejected geometry for inspection; never certify or publish a solver mesh."""
import json,shlex,sys
from pathlib import Path

def main():
    folder=Path(sys.argv[1]).resolve();paths=sorted(folder.glob('*_rejected_rank[0-9]*.json'))
    paths=[p for p in paths if p.stem.rsplit('_rank',1)[-1].isdigit()];assert paths
    cells=[];nodes={};faces=[];seen=set();facets=None;names=None
    for p in paths:
        data=json.loads(p.read_text())
        for n,*q in data['nodes']:
            prior=nodes.setdefault(n,q);assert prior==q
        for c in data['cells']:assert c['id'] not in seen;seen.add(c['id']);cells.append(c)
        faces+=data['surface']
        if facets is None:
            facets={f['id']:f['marker'] for f in data['facets']}
            text=p.with_suffix('.native_ref').read_text().splitlines();count=int(text[1].split()[0]);names=[shlex.split(s)[0] for s in text[2:2+count]]
        else:assert facets=={f['id']:f['marker'] for f in data['facets']}
    ordered=sorted(nodes);ids={n:i for i,n in enumerate(ordered)}
    lines=['NDIME= 3','NELEM= '+str(len(cells))]+['10 '+' '.join(str(ids[n]) for n in c['nodes'])+' '+str(i) for i,c in enumerate(cells)]
    lines+=['NPOIN= '+str(len(nodes))]+[' '.join(format(x,'.17g') for x in nodes[n])+' '+str(ids[n]) for n in ordered]
    markers=sorted({facets[f['facet']] for f in faces});lines+=['NMARK= '+str(len(markers))]
    for marker in markers:
        selected=[f['nodes'] for f in faces if facets[f['facet']]==marker]
        lines+=['MARKER_TAG= '+names[marker],'MARKER_ELEMS= '+str(len(selected))]+['5 '+' '.join(str(ids[n]) for n in f) for f in selected]
    prefix=paths[0].name.rsplit('_rejected_rank',1)[0];output=folder/(prefix+'_rejected.su2');output.write_text('\n'.join(lines)+'\n')
    (folder/(prefix+'_rejected_export.json')).write_text(json.dumps({'accepted':False,'cells':len(cells),'nodes':len(nodes),'snapshots':[p.name for p in paths],
        'purpose':'Inspect terminal private state; no accepted solution or restart','output':output.name},indent=2)+'\n')
    print(output,'cells',len(cells),'nodes',len(nodes),'REJECTED_UNVALIDATED')
if __name__=='__main__':main()
