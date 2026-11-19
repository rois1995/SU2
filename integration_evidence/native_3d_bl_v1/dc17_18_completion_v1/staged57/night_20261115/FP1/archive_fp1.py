"""List small sealed FP1 receipts and verified log archives for a checkpoint."""

from argparse import ArgumentParser
from pathlib import Path

from run_g1 import ITEM, cut
from seal_f import archive


def paths(root):
    records = {}
    artifacts = []
    for path in sorted(root.rglob('*')):
        if path.is_symlink() or not path.is_file():
            continue
        if path.suffix in {'.log', '.jsonl', '.csv'} or path.name in {'stdout', 'stderr'}:
            packed, receipt = archive(path)
            artifacts.append(packed)
            records[str(packed)] = receipt
        elif path.suffix in {'.json', '.md', '.cfg', '.time', '.py'} or path.name == 'time.txt':
            artifacts.append(path)
    manifest = root / 'log_archives.json'
    if manifest.is_file():
        assert cut.read(manifest) == records
    else:
        cut.save(manifest, records)
    paths_file = root / 'checkpoint_paths.json'
    if not paths_file.exists():
        cut.save(paths_file, sorted(str(path) for path in set(artifacts) if path not in (manifest, paths_file))
                 + [str(manifest), str(paths_file)])
    return paths_file


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('root', type=Path)
    args = parser.parse_args()
    root = args.root.resolve()
    if ITEM not in root.parents:
        raise RuntimeError('Archive one finished FP1 part or workflow.')
    print(paths(root))
