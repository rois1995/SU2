set pagination off
set confirm off
set print elements 2
set print max-depth 2
break CBoundaryLayerMetric::IntegrationRule3D
python
import gdb
assert len(gdb.breakpoints())==1 and not gdb.breakpoints()[0].pending, 'Missing pre-quadrature breakpoint'
end
run
python
import gdb,hashlib,json,os,sys,time
from pathlib import Path
rank=int(os.environ['OMPI_COMM_WORLD_RANK']);size=int(os.environ['OMPI_COMM_WORLD_SIZE'])
assert size==4 and 0<=rank<size
prefix='rank%d_'%rank
try:
    frame=gdb.newest_frame()
    assert 'CBoundaryLayerMetric::IntegrationRule3D' in (frame.name() or ''), 'Unexpected stop'
    values={};frames=[]
    while frame:
        name=frame.name() or ''
        if 'CSolver' in name:
            frame.select();frames.append(name)
            for key in ('blMetric','geometry','frozenCore','logScale','complexityTrials'):
                if key in values:continue
                try:
                    value=gdb.parse_and_eval(key)
                    if not value.is_optimized_out:values[key]=value
                except gdb.error:pass
        frame=frame.older()
    assert set(values)=={'blMetric','geometry','frozenCore','logScale','complexityTrials'}, list(values)
    field=values['blMetric'];geometry=values['geometry'].dereference();nodes=geometry['nodes'].dereference()
    start=field['_M_impl']['_M_start'];finish=field['_M_impl']['_M_finish']
    count=int(finish-start);owned=int(geometry['nPointDomain'])
    assert int(geometry['nDim'])==3 and int(geometry['nPoint'])==count and 0<owned<=count<=1000000
    assert int(gdb.parse_and_eval('sizeof(CBoundaryLayerMetric::Tensor)'))==72
    assert int(gdb.parse_and_eval('sizeof(unsigned long)'))==8
    assert int(gdb.parse_and_eval('sizeof(su2double)'))==8 and sys.byteorder=='little'
    assert int(nodes['GlobalIndex']['m_rows'])==count
    assert int(nodes['Coord']['m_rows'])==count and int(nodes['Coord']['m_cols'])==3
    trial=int(values['complexityTrials']);assert trial==1, trial
    core=float(values['frozenCore']);scale=float(values['logScale']);assert core>0
    arrays=[('sensor.f64',int(start),count*72),('global.u64',int(nodes['GlobalIndex']['m_data']),count*8),
            ('coordinates.f64',int(nodes['Coord']['m_data']),count*24),('domain.u8',int(nodes['Domain']['m_data']),count)]
    assert sum(n for _,_,n in arrays)<256*1024*1024
    files={};inferior=gdb.selected_inferior()
    for name,address,length in arrays:
        data=bytes(inferior.read_memory(address,length));path=Path(prefix+name)
        temporary=path.with_suffix(path.suffix+'.partial');temporary.write_bytes(data);temporary.replace(path)
        files[path.name]={'bytes':length,'sha256':hashlib.sha256(data).hexdigest()}
    report={'state':'COMPLETE_PRE_QUADRATURE_RAW_CAPTURE','rank':rank,'size':size,'pid':inferior.pid,
            'points':count,'owned_points':owned,'complexity_trial':trial,'core':core,'log_scale':scale,
            'byteorder':sys.byteorder,'tensor_bytes':72,'gdb':gdb.VERSION,'source_frames':frames,'files':files,
            'helper_sha256':hashlib.sha256(Path('capture.gdb').read_bytes()).hexdigest(),
            'provenance_sha256':hashlib.sha256(Path('provenance.json').read_bytes()).hexdigest(),
            'scope':'Read-only memory extraction at first IntegrationRule3D entry; no inferior function calls. Sensor-only after gradation, before BL integration/final normalization/remeshing. Not an accepted adaptation.'}
    temporary=Path(prefix+'capture.json.partial');temporary.write_text(json.dumps(report,indent=2)+'\n');temporary.replace(prefix+'capture.json')
    print('FROZEN_FIELD_CAPTURE',rank,count,owned,core,scale,flush=True)
    deadline=time.monotonic()+45
    while True:
        if any(Path('rank%d_error.json'%r).exists() for r in range(size)):raise RuntimeError('Another rank failed capture')
        if all(Path('rank%d_capture.json'%r).exists() for r in range(size)):break
        if time.monotonic()>deadline:raise RuntimeError('Other ranks did not complete the pre-quadrature capture')
        time.sleep(.1)
except BaseException as error:
    Path(prefix+'error.json').write_text(json.dumps({'error':repr(error),'rank':rank,'scope':'Capture failed; no accepted checkpoint.'},indent=2)+'\n')
    raise
end
quit
