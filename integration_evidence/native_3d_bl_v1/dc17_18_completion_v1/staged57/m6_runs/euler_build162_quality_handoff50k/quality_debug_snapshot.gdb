set pagination off
set confirm off
set print elements 0
set may-call-functions off
python
import gdb
f=gdb.newest_frame()
seen=set()
while f:
    n=f.name() or ''
    kind='optimizer' if ('MeshOptimizer::SmoothBoundary' in n or 'MeshOptimizer::Run' in n) else ('adapt' if ('::Adapt(' in n and 'operator()' not in n) else '')
    if kind and kind not in seen:
        seen.add(kind);f.select();sal=f.find_sal();gdb.write('SNAPSHOT_FRAME '+kind+' '+n[:180]+' line='+str(sal.line)+'\n')
        exprs=['this->report.trials','this->report.moves','this->report.rejected_quality','this->report.rejected_interface','this->report.rejected_edge','this->options.maximum_work','p'] if kind=='optimizer' else ['stats.rounds','stats.attempted','stats.accepted','stats.quality_progress','stats.geometry_progress','stats.optimization_trials','stats.optimization_validated','repair_work','repair_budget']
        for e in exprs:
            gdb.write('EXPR '+e+'\n')
            try:gdb.execute('print '+e)
            except Exception as x:gdb.write('UNAVAILABLE '+str(x)+'\n')
    f=f.older()
end
detach
quit
