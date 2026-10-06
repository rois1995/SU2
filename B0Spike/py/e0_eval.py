"""Score E0 artifacts, persist frozen calibration and report serial-excess damage."""
import argparse
import json
import pathlib
from e0_cells import read
from e0gates import attribution, calibrate, score, unscorable


def evaluate(path):
    a = read(path)
    if len(a['serial']) != 25:
        raise ValueError('identity + 19 calibration + 5 held-back serial meshes required')
    try:
        model = calibrate(a['serial'][0], a['serial'][1:20])
    except ValueError as error:
        reason = str(error)
        return dict(file=str(path), model=None, candidate=unscorable(reason),
                    controls=[unscorable(reason) for _ in a['serial'][20:25]], damage=[], calibrationError=reason)
    return dict(file=str(path), model=model, candidate=score(a['candidate'], model),
                controls=[score(m, model) for m in a['serial'][20:25]], damage=attribution(a, model), calibrationError=None)


if __name__ == '__main__':
    p = argparse.ArgumentParser()
    p.add_argument('artifacts', nargs='+')
    p.add_argument('--out', required=True)
    args = p.parse_args()
    results = [evaluate(f) for f in args.artifacts]
    pathlib.Path(args.out).write_text(json.dumps(results, indent=2) + '\n')
    for result in results:
        for name, scored in [('candidate', result['candidate'])] + [(f'control {i}', s) for i, s in enumerate(result['controls'])]:
            if scored['reason']:
                print(f"{result['file']}: {name} unscorable: {scored['reason']}")
    print(f'E0 evaluation: {len(results)} masks -> {args.out}')
