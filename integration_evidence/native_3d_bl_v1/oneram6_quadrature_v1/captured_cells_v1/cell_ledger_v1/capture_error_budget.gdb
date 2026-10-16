set pagination off
set confirm off
set print elements 2
set print max-depth 2
catch throw
run
bt 6
python
import gdb,json
from pathlib import Path
out=[];frame=gdb.selected_frame()
expressions=['totalValue','totalComparison','totalAngularError','totalRadialError','unresolved','retained','visited','nextPanel','panel.cone','panel.fraction','panel.depth','panel.value','panel.comparison','panel.angularError','panel.radialError','panel.count','panel.unresolved','ray.value','ray.error','ray.count','interval.value','interval.error','interval.depth','interval.left','interval.right']
while frame:
 name=frame.name() or ''
 if 'CBoundaryLayerMetric' in name:
  frame.select();row={'frame':name,'values':{}}
  for expression in expressions:
   try:row['values'][expression]=float(gdb.parse_and_eval('(long double)('+expression+')'))
   except (gdb.error,TypeError):pass
  try:row['triangle']=[[float(gdb.parse_and_eval('panel.triangle._M_elems[%d]._M_elems[%d]'%(i,j))) for j in range(3)]for i in range(3)]
  except (gdb.error,TypeError):pass
  if row['values']:out.append(row)
 frame=frame.older()
Path('error_budget.json').write_text(json.dumps(out,indent=2)+'\n');print(json.dumps(out,indent=2))
end
quit
