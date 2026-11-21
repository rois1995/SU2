"""Run the pinned FP2-A workflow with the Correction 4 rank evaluation."""

from pathlib import Path
import sys

sys.dont_write_bytecode = True
import reevaluate_rank_check as correction4

HERE = Path(__file__).resolve().parent
BASE = HERE / 'run_g5.py'
BASE_SHA = '9eed0bf393054aa5826cd61994cc0ac4f2d4eab8af80c931f156ae2ead4fbf18'


def main():
    source = BASE.read_text()
    assert correction4.a.sha(BASE) == BASE_SHA
    rank_line = "    rank_check = HERE / 'rank_check' / f'build{label}' / 'rank_check_summary.json'"
    assert source.count(rank_line) == 1
    source = source.replace(rank_line, '''    assert label == 627
    rank_check = correction4.EVALUATION
    correction4.verify()
    sources += [correction4.OLD_SUMMARY, Path(correction4.__file__).resolve(), correction4_driver_base]''')
    marker = 'CORE_CONFIG_SHA = '
    assert source.count(marker) == 1
    source = source.replace(marker, '''ORDERS.update({
    HERE / 'inputs/FP2_A_CORRECTION_3.md': '01d871b7e2999e820d18529271b7a7050f3471f50983c552acd88e775406d758',
    correction4.ORDER: correction4.ORDER_SHA,
})
''' + marker)
    # Preserve the original script's digest in the sealed OFF provenance and reuse its workflow checks.
    namespace = {'__file__': str(Path(__file__).resolve()), '__name__': '__main__',
                 'correction4': correction4, 'correction4_driver_base': BASE}
    exec(compile(source, str(BASE), 'exec'), namespace)


if __name__ == '__main__':
    main()
