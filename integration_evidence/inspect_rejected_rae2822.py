"""Locate saved native failure cells and export a ParaView mesh with a failure mask."""
import argparse,csv,hashlib,json,sys,xml.etree.ElementTree as ET
from pathlib import Path
import numpy as np


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('case',type=Path)
    p.add_argument('--cycle',type=int,choices=(1,2),default=1)
    p.add_argument('--compare-case',type=Path)
    p.add_argument('--metric-audit',type=Path)
    p.add_argument('--native-mask-only',action='store_true',help='Export reported failure locations without claiming an independent metric audit')
    args=p.parse_args()
    root=Path(__file__).resolve().parent.parent;sys.path.insert(0,str(root/'TestCases/adaptation/capability'));import capcheck
    wd=args.case.resolve();base=wd/f'mesh_adap_{args.cycle:05d}_rejected';meshpath=Path(str(base)+'.su2');csvpath=Path(str(base)+'_failures.csv')
    m=capcheck.read_su2(meshpath);gates=capcheck.check_validity(m)
    with csvpath.open() as f:failures=list(csv.DictReader(f))
    cellkeys={tuple(sorted(tuple(m.P[i]) for i in tri)):i for i,tri in enumerate(m.E)}
    pointkeys={tuple(point):i for i,point in enumerate(m.P)}
    mask=np.zeros(len(m.E),dtype=int);ids=np.full(len(m.E),-1,dtype=np.int64);locations=[]
    for f in failures:
        xyz=np.array([(float(f[f'x{k}']),float(f[f'y{k}'])) for k in range(3)])
        cell=cellkeys[tuple(sorted(map(tuple,xyz)))];assert not mask[cell];mask[cell]=1;ids[cell]=int(f['native_cell_id'])
        center=xyz.mean(0);nearest=None
        for marker,edges in m.M.items():
            a=m.P[edges[:,0]];d=m.P[edges[:,1]]-a
            u=np.clip(np.einsum('ni,ni->n',center-a,d)/np.einsum('ni,ni->n',d,d),0,1)
            distances=np.linalg.norm(center-a-u[:,None]*d,axis=1);i=int(np.argmin(distances))
            item=(float(distances[i]),marker)
            if nearest is None or item<nearest:nearest=item
        locations.append(dict(exported_cell_row=int(cell),native_cell_id=int(f['native_cell_id']),rank=int(f['rank']),
                              quality=float(f['quality']),max_metric_edge=float(f['max_metric_edge']),
                              centroid=center.tolist(),nearest_boundary=nearest[1],distance_to_boundary=nearest[0],
                              coordinates=xyz.tolist(),exported_point_ids=[pointkeys[tuple(a)] for a in xyz]))
    auditpath=args.metric_audit.resolve() if args.metric_audit else wd/'independent_rejected_metric_audit.json'
    if args.native_mask_only:
        audit=dict(min_quality=min(r['quality'] for r in locations),max_simpson_length=None,
                   all_exact_positive=None)
        metric_scope='Native CSV failure values; no independent target audit. Minimum is over reported cells only.'
    else:
        audit=json.loads(auditpath.read_text())
        assert audit['all_exact_positive'] and audit['manifold_oriented_edges'] and audit['physical_equals_exposed']
        assert {r['exported_cell_row'] for r in locations}=={r['cell'] for r in audit['bad_cells']}
        assert np.isclose(min(r['quality'] for r in locations),audit['min_quality'],rtol=1e-5)
        metric_scope='Independent frozen-target audit matches the native failure mask.'
    vtk=ET.Element('VTKFile',type='UnstructuredGrid',version='0.1',byte_order='LittleEndian')
    grid=ET.SubElement(vtk,'UnstructuredGrid');piece=ET.SubElement(grid,'Piece',NumberOfPoints=str(len(m.P)),NumberOfCells=str(len(m.E)))
    def array(parent,name,kind,values,components=1):
        el=ET.SubElement(parent,'DataArray',type=kind,Name=name,NumberOfComponents=str(components),format='ascii')
        el.text=' '.join(format(float(v),'.17g') if kind=='Float64' else str(int(v)) for v in np.asarray(values).ravel())
    points=ET.SubElement(piece,'Points');array(points,'Points','Float64',np.column_stack([m.P,np.zeros(len(m.P))]),3)
    cells=ET.SubElement(piece,'Cells');array(cells,'connectivity','Int64',m.E);array(cells,'offsets','Int64',3*np.arange(1,len(m.E)+1));array(cells,'types','UInt8',np.full(len(m.E),5))
    fields=ET.SubElement(piece,'CellData',Scalars='MetricQualityFailure');array(fields,'MetricQualityFailure','UInt8',mask);array(fields,'NativeFailureCellID','Int64',ids)
    output=Path(str(base)+'.vtu');assert not output.exists();ET.ElementTree(vtk).write(output,encoding='utf-8',xml_declaration=True)
    # Read the artifact independently through the standard XML parser.
    tree=ET.parse(output)
    decoded=np.fromstring(tree.find('.//Points/DataArray').text,sep=' ').reshape(-1,3)
    assert np.array_equal(decoded[:,:2],m.P)
    encoded={node.attrib['Name']:np.fromstring(node.text,sep=' ',dtype=np.int64) for node in tree.findall('.//Cells/DataArray')+tree.findall('.//CellData/DataArray')}
    assert np.array_equal(encoded['connectivity'].reshape(-1,3),m.E) and np.array_equal(encoded['MetricQualityFailure'],mask)
    sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
    pairing={}
    if args.compare_case:
        old=args.compare_case.resolve()
        pairing={name:sha(wd/name)==sha(old/name) for name in ('input.su2','run.cfg','history.csv','solution_adap_00000.dat','flow_adap_00000.vtu')}
    record=dict(mesh=str(meshpath),paraview_mesh=str(output),points=len(m.P),triangles=len(m.E),failure_count=len(locations),
                minimum_metric_quality=audit['min_quality'],maximum_metric_length=audit['max_simpson_length'],validity=gates,
                exact_positive_orientation=audit['all_exact_positive'],prior_run_byte_pairing=pairing,
                near_trailing_edge_failures=sum(r['centroid'][0]>.99 for r in locations),locations=locations,
                input_files_sha256={str(f):sha(f) for f in (meshpath,csvpath,*(() if args.native_mask_only else (auditpath,)))},
                vtu_sha256=sha(output),checker_sha256=sha(Path(__file__)),
                metric_validation=metric_scope,scope='Rejected geometry only; no solution transferred. Native IDs and exported cell rows differ. Near-duplicate gate is reported without suppressing it.')
    Path(str(base)+'_locations.json').write_text(json.dumps(record,indent=2)+'\n')
    import matplotlib
    matplotlib.use('Agg');import matplotlib.pyplot as plt
    fig,axes=plt.subplots(1,2,figsize=(13,5),constrained_layout=True)
    centers=np.array([r['centroid'] for r in locations])
    for ax in axes:
        ax.triplot(m.P[:,0],m.P[:,1],m.E,color='#345075',lw=.2)
        for edge in m.M['AIRFOIL']:ax.plot(m.P[edge,0],m.P[edge,1],'k-',lw=.7)
        ax.scatter(centers[:,0],centers[:,1],color='red',s=14,zorder=4);ax.set(xlabel='x/c',ylabel='y/c')
    axes[0].set(xlim=(-.02,1.03),ylim=(-.1,.11),title=f'{len(locations)} reported failures');axes[0].set_aspect('equal')
    worst=min(locations,key=lambda r:r['quality']);xyz=np.array(worst['coordinates']);lo=xyz.min(0);hi=xyz.max(0);pad=np.maximum((hi-lo)*.3,1e-7)
    axes[1].set(xlim=(lo[0]-pad[0],hi[0]+pad[0]),ylim=(lo[1]-pad[1],hi[1]+pad[1]),title=f"Worst reported cell, q={worst['quality']:.3g}");axes[1].set_aspect('equal')
    fig.savefig(Path(str(base)+'_locations.png'),dpi=180);plt.close(fig)
    print('Rejected candidate inspected:',len(locations),'failure cells; source artifacts byte pairing:',pairing)
    print('Independent topology gates:',gates)

if __name__=='__main__':main()
