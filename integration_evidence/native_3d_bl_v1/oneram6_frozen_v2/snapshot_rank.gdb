set pagination off
set print frame-arguments none
thread 1
bt 24
python
import gdb,json
from pathlib import Path
result={'pid':gdb.selected_inferior().pid,'scope':'Single passive owned-rank phase/failure snapshot; no inferior function calls or timing-profile claim.','failure':None,'metric_context':{}}
frame=gdb.newest_frame()
while frame:
 name=frame.name() or ''
 if 'ElectFailure' in name:
  frame.select();local=gdb.parse_and_eval('local');msg=local['message'];p=msg['_M_dataplus']['_M_p'];n=int(msg['_M_string_length'])
  result['failure']={'severity':int(local['severity']),'global_cell':int(local['gid']),'message':p.string(length=n)}
 if name.startswith('CSolver::ComputeMetric'):
  frame.select()
  for key in ['complexityTrials','quadratureBuilds','quadratureReused','localQuadraturePoints','frozenCore','logScale','gradationSweeps','totalGradationSweeps','quadratureSeconds','gradationSeconds']:
   try:result['metric_context'][key]=float(gdb.parse_and_eval(key))
   except (gdb.error,TypeError):result['metric_context'][key]=None
 frame=frame.older()
path=Path('integration_evidence/native_3d_bl_v1/oneram6_frozen_v2/rank_snapshot_'+str(result['pid'])+'.json')
path.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
end
detach
