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
profile=Path.cwd()/'profile'
def clock(event):
    with (profile/'engine_clocks.jsonl').open('a') as out:
        out.write(json.dumps(dict(event=event,pid=gdb.selected_inferior().pid,monotonic_ns=time.monotonic_ns(),wall_ns=time.time_ns()))+'\n')
class EngineEnd(gdb.FinishBreakpoint):
    def stop(self):
        clock('PrepareSurface_return')
        return False
class EngineStart(gdb.Breakpoint):
    def stop(self):
        clock('PrepareSurface_entry')
        EngineEnd(gdb.newest_frame(),internal=True)
        (profile/'rank0.lbr_request.json').write_text(json.dumps({'phase':'native_surface'})+'\n')
        return False
EngineStart('SU2Native3D::PrepareSurface',internal=True)
gdb.execute('starti')
pid=gdb.selected_inferior().pid
ticks=int(Path('/proc',str(pid),'stat').read_text().rsplit(')',1)[1].split()[19])
(profile/'rank0.exec.json').write_text(json.dumps(dict(pid=pid,start_ticks=ticks,monotonic_ns=time.monotonic_ns(),wall_ns=time.time_ns(),program=gdb.current_progspace().filename,clock_method='GDB starti stop; public PrepareSurface entry/return breakpoints'))+'\n')
clock('process_starti')
(profile/'rank0.lbr_request.json').write_text(json.dumps({'phase':'preparation'})+'\n')
gdb.execute('continue')
clock('inferior_terminal')
end
