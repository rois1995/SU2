"""Build only the standalone spike, with Ninja in the existing build directory."""
import os, pathlib, shlex, subprocess, sys
wt = pathlib.Path(__file__).resolve().parent.parent
build = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else wt.parent / 'build_b0')
out = pathlib.Path(sys.argv[2] if len(sys.argv) > 2 else wt.parent / 'runs/b0/bin')
q = lambda p: shlex.quote(str(p))
flags = '-std=c++17 -O2 -g -Wall -Wno-unused-variable ' + ' '.join('-I' + q(p) for p in [wt / 'externals/eigen', wt / 'externals/metis/include', '/usr/local/include'])
files = ['b0_core', 'b0_engine', 'b0_analysis', 'b0_driver', 'b0_e0', 'b0_main']
out.mkdir(parents=True, exist_ok=True)
lines = ['rule compile', f'  command = mpicxx {flags} -MMD -MF $out.d -c $in -o $out', '  depfile = $out.d', '  deps = gcc', 'rule link', '  command = mpicxx $in -o $out ' + ' '.join(q(p) for p in [build / 'externals/metis/libmetis.a', '/usr/local/lib/libmmg.a', '/home/rausa/Software/scotch/lib/libscotch.a', '/home/rausa/Software/scotch/lib/libscotcherrexit.a']) + ' -lm -lrt -pthread']
for f in files:
    lines.append(f'build {f}.e0.o: compile {wt / "B0Spike" / (f + ".cpp")}')
lines.append(f'build {out / "b0spike"}: link ' + ' '.join(f + '.e0.o' for f in files))
lines.append(f'default {out / "b0spike"}')
(build / 'b0.ninja').write_text('\n'.join(lines) + '\n')
if os.getloadavg()[0] >= 7:
    raise SystemExit('load gate 7: build deferred; rerun when load falls below 7')
subprocess.run(['ninja', '-C', str(build), '-f', 'b0.ninja', '-j2'], check=True)
