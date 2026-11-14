set debuginfod enabled off
set pagination off
set confirm off
set auto-load off
set disable-randomization off
set print thread-events off
handle SIGPIPE nostop noprint pass
python
import gdb,json,time,os
from pathlib import Path
profile=Path.cwd()/'profile';rank=int(os.environ.get('OMPI_COMM_WORLD_RANK','0'));stem='rank'+str(rank);event=0
def clock(name,**extra):
    with (profile/(stem+'.engine_clocks.jsonl')).open('a') as out:
        out.write(json.dumps(dict(event=name,adaptation=event,pid=gdb.selected_inferior().pid,monotonic_ns=time.monotonic_ns(),wall_ns=time.time_ns(),**extra))+'\n')
class EngineEnd(gdb.FinishBreakpoint):
    def stop(self):
        clock('AdaptMetric_return')
        return False
class EngineStart(gdb.Breakpoint):
    def stop(self):
        global event
        event+=1;clock('AdaptMetric_entry');EngineEnd(gdb.newest_frame(),internal=True)
        if rank==0:(profile/(stem+'.lbr_request.json')).write_text(json.dumps({'phase':'event'+str(event)+'_construction'})+'\n')
        return False
class TimerEpoch(gdb.Breakpoint):
    def stop(self):
        try:clock('Adapt_timer_epoch',epoch_ns=int(gdb.parse_and_eval('started._M_d._M_rep')))
        except Exception as error:clock('Adapt_timer_epoch_unavailable',error=str(error))
        return False
EngineStart('SU2Native3D::AdaptMetric',internal=True)
TimerEpoch('CNativeEngine3D.cpp:1176',internal=True)
gdb.execute('starti')
pid=gdb.selected_inferior().pid;ticks=int(Path('/proc',str(pid),'stat').read_text().rsplit(')',1)[1].split()[19])
(profile/(stem+'.exec.json')).write_text(json.dumps(dict(pid=pid,start_ticks=ticks,monotonic_ns=time.monotonic_ns(),wall_ns=time.time_ns(),program=gdb.current_progspace().filename,clock_method='GDB starti, AdaptMetric entry/return, existing started steady_clock variable'))+'\n')
clock('process_starti')
gdb.execute('continue')
clock('inferior_terminal')
end
