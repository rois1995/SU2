"""Reuse the frozen case runner with the B clock recorder and hashed dependencies."""
from pathlib import Path
import sys
base=Path(__file__).with_name('run_measurement.py');source=base.read_text()
assert source.count("str(ROOT/'rank_perf.py')")==1
source=source.replace("str(ROOT/'rank_perf.py')","str(ROOT/'rank_perf_B.py')")
source=source.replace("for p in (ROOT/'rank_perf.py',ROOT/'engine_clock.gdb',Path(__file__).resolve(),ROOT/'brief.md')", "for p in (ROOT/'rank_perf.py',ROOT/'rank_perf_B.py',ROOT/'engine_clock_B.gdb',ROOT/'run_measurement.py',Path(__file__).resolve(),ROOT/'brief.md')")
assert sys.argv[1:]==['B']
exec(compile(source,str(base),'exec'))
