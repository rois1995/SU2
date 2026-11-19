"""Enforce the A9 wall limit after the shared mesh load."""

import json
import os
import selectors
import signal
import subprocess
import sys
import time


def run(argv):
    with subprocess.Popen(argv, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                          start_new_session=True) as process, selectors.DefaultSelector() as selector:
        selector.register(process.stdout, selectors.EVENT_READ)
        pending, deadline, setup = b'', None, None
        header = b'FP0 A9_setup '
        while True:
            interval = .25 if deadline is None else max(0., min(.25, deadline - time.monotonic()))
            if selector.select(interval):
                chunk = os.read(process.stdout.fileno(), 16384)
                if not chunk:
                    return process.wait()
                sys.stdout.buffer.write(chunk)
                sys.stdout.buffer.flush()
                lines = (pending + chunk).split(b'\n')
                pending = lines.pop()
                for line in lines:
                    if line.startswith(b'FP0 A9_begin '):
                        setup = json.loads(line.removeprefix(b'FP0 A9_begin '))
                        deadline = time.monotonic() + 900
                    elif line.startswith(header):
                        setup = json.loads(line[len(header):])
                        if deadline is None:
                            deadline = time.monotonic() + max(0., 900 - setup['seconds'])
            if deadline is not None and time.monotonic() >= deadline and process.poll() is None:
                os.killpg(process.pid, signal.SIGTERM)
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    os.killpg(process.pid, signal.SIGKILL)
                    process.wait()
                # A larger earlier buffer can contain an unterminated reporter line.
                print('\nFP0 A9 ' + json.dumps({'desired': setup['desired'], 'cells': setup.get('cells'),
                      'seconds': 900, 'stop': 'outer_time_limit', 'error': '900 s optimizer wall limit'}), flush=True)
                return 124


if __name__ == '__main__':
    sys.exit(run(sys.argv[1:]))
