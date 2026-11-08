#!/usr/bin/env python3
"""Download NASA's hump archive and prepare five SU2 grid levels; never launch CFD."""

import argparse
import gzip
import hashlib
import io
import json
from pathlib import Path
import urllib.request
import zipfile

from convert_hump import convert
from prepare import prepare

URL = "https://www.nasa.gov/wp-content/uploads/2026/02/nasahump-grids.zip"
LEVELS = ((103, 28), (205, 55), (409, 109), (817, 217), (1633, 433))


def convert_archive(archive, output):
    records = []
    with zipfile.ZipFile(archive) as bundle:
        for ni, nj in LEVELS:
            stem = f"hump2newtop_noplenumZ{ni}x{nj}.p2dfmt"
            matches = [entry for entry in bundle.infolist()
                       if Path(entry.filename).name in (stem, stem + ".gz")]
            if len(matches) != 1:
                raise ValueError(f"Expected exactly one {stem}[.gz] in the NASA archive")
            entry = matches[0]
            if entry.file_size > 64 * 1024**2:
                raise ValueError(f"Unexpectedly large mesh member: {entry.filename}")
            directory = output / f"{ni}x{nj}"
            directory.mkdir()
            source = directory / Path(entry.filename).name
            data = bundle.read(entry)
            if source.suffix == ".gz":
                with gzip.GzipFile(fileobj=io.BytesIO(data)) as stream:
                    text = stream.read(64 * 1024**2 + 1)
            else:
                text = data
            if len(text) > 64 * 1024**2 or text.split(maxsplit=3)[:3] != [b"1", str(ni).encode(), str(nj).encode()]:
                raise ValueError(f"Invalid size or dimensions for {entry.filename}")
            source.write_bytes(data)
            mesh = directory / "mesh.su2"
            convert(source, mesh)
            manifest = prepare("nasa_hump", directory / "campaign", mesh)
            if manifest["mesh"]["points"] != ni*nj or manifest["mesh"]["elements"] != (ni-1)*(nj-1):
                raise ValueError("Converted topology count mismatch")
            records.append({"level": f"{ni}x{nj}", "archive_member": entry.filename,
                            "source_sha256": hashlib.sha256(data).hexdigest(),
                            "su2_sha256": manifest["mesh"]["sha256"],
                            "points": ni*nj, "cells": (ni-1)*(nj-1), "jobs": len(manifest["jobs"])})
    return records


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path, help="new asset directory; on CFDHub place under the dedicated SU2 study")
    args = parser.parse_args()
    try:
        args.output.mkdir(parents=True, exist_ok=False)
        archive = args.output / "nasahump-grids.zip"
        partial = args.output / "nasahump-grids.zip.partial"
        digest, size = hashlib.sha256(), 0
        with urllib.request.urlopen(URL, timeout=60) as response, partial.open("xb") as target:
            if response.status != 200:
                raise ValueError(f"Unexpected download status: {response.status}")
            while block := response.read(1024**2):
                size += len(block)
                if size > 2 * 1024**3:
                    raise ValueError("Archive exceeds the 2 GiB download limit")
                digest.update(block)
                target.write(block)
        partial.rename(archive)
        records = convert_archive(archive, args.output)
        (args.output / "assets.json").write_text(json.dumps({
            "url": URL, "archive_sha256": digest.hexdigest(), "archive_bytes": size,
            "levels": records, "status": "converted_and_prepared_not_simulated",
        }, indent=2) + "\n")
        print(f"Converted {len(records)} NASA grids and prepared {8*len(records)} configurations. No solver launched.")
    except (OSError, ValueError, zipfile.BadZipFile) as error:
        parser.exit(1, f"NASA setup failed: {error}\nIncomplete artifacts are retained; no success manifest was written.\n")


if __name__ == "__main__":
    main()
