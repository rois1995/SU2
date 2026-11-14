"""Reuse the frozen recorder, adding only external CFD clock observations for B."""
from pathlib import Path
base=Path(__file__).with_name('rank_perf.py')
source=base.read_text()
needle="os.execv(program,[program,*sys.argv[4:]])"
assert source.count(needle)==1
source=source.replace(needle,"os.execv('/usr/bin/gdb',['gdb','--batch','-nx','-x',str(HERE/'engine_clock_B.gdb'),'--args',program,*sys.argv[4:]])")
exec(compile(source,str(base),'exec'))
