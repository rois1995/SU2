def cgns_signature(path,markers=MARKERS):
    import h5py
    with h5py.File(path,'r') as file:
        def labelled(parent,label):
            return [g for g in parent.values() if isinstance(g,h5py.Group) and g.attrs.get('label')==label.encode()]
        bases=labelled(file,'CGNSBase_t');assert len(bases)==1
        base=bases[0];assert list(base[' data'][...])==[3,3]
        zones=labelled(base,'Zone_t');assert len(zones)==1;zone=zones[0]
        count,volume_count,boundary_count=map(int,zone[' data'][...].reshape(-1));assert count==783 and volume_count==3768 and boundary_count==0
        grids=labelled(zone,'GridCoordinates_t');assert len(grids)==1
        axes=[grids[0]['Coordinate'+axis][' data'][...].reshape(-1).tolist() for axis in 'XYZ'];assert all(len(a)==count for a in axes)
        points={i+1:tuple(p) for i,p in enumerate(zip(*axes))};assert all(math.isfinite(x) for p in points.values() for x in p)
        cells=Counter();boundary=Counter();ranges={};seen=set()
        for section in labelled(zone,'Elements_t'):
            kind=int(section[' data'][...].reshape(-1)[0]);assert kind in (5,10)
            first,last=map(int,section['ElementRange'][' data'][...].reshape(-1));assert first>0 and last>=first
            ids=set(range(first,last+1));assert not ids&seen;seen|=ids
            width=4 if kind==10 else 3;rows=section['ElementConnectivity'][' data'][...].reshape(-1).tolist();assert len(rows)==width*len(ids)
            name=section.name.rsplit('/',1)[-1]
            if kind==5:assert name in markers;ranges[name]=(first,last)
            for i in range(0,len(rows),width):
                nodes=rows[i:i+width];assert len(set(nodes))==width and all(n in points for n in nodes)
                if kind==10:
                    assert determinant([tuple(F(x) for x in points[n]) for n in nodes])>0,'Nonpositive actual CGNS tetrahedron'
                    cells[tuple(sorted(points[n] for n in nodes))]+=1
                else:boundary[name,tuple(sorted(points[n] for n in nodes))]+=1
        assert sum(cells.values())==volume_count
        skins=labelled(zone,'ZoneBC_t');assert len(skins)==1
        bcs=labelled(skins[0],'BC_t');assert {b.name.rsplit('/',1)[-1] for b in bcs}==set(ranges)==markers
        for bc in bcs:
            name=bc.name.rsplit('/',1)[-1];assert tuple(map(int,bc['PointRange'][' data'][...].reshape(-1)))==ranges[name]
        def closed(name,obj):
            if isinstance(obj,h5py.Group):
                assert all(isinstance(obj.get(k,getlink=True),h5py.HardLink) for k in obj),'External/soft HDF5 mesh dependency'
        closed('',file);file.visititems(closed)
        return cells,boundary
