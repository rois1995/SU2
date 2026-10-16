set pagination off
set confirm off
set print elements 2
set print max-depth 2
catch throw
run
up
bt 8
info locals
python
import gdb,json
from pathlib import Path
out=[];frame=gdb.selected_frame()
expressions=['totalValue','totalError','unresolved','rayValue','rayError','rayPoints','retained','visited','nextPanel','corner','fraction','fine','radialError','integral','panel.left','panel.right','panel.depth','panel.value','panel.error','panel.unresolved']
while frame:
 name=frame.name() or ''
 if 'CBoundaryLayerMetric' in name:
  frame.select();row={'frame':name,'values':{}}
  for expression in expressions:
   try:row['values'][expression]=float(gdb.parse_and_eval('(double)('+expression+')'))
   except (gdb.error,TypeError):pass
  if row['values']:out.append(row)
 frame=frame.older()
Path('parallel_error_budget.json').write_text(json.dumps(out,indent=2)+'\n');print(json.dumps(out,indent=2))
end
