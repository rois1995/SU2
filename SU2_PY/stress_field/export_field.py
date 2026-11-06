#!/usr/bin/env python3
"""Convert a saved v0.7 raw field to the SU2 format without renormalizing it."""
import argparse
import hashlib
import json
from pathlib import Path
import numpy as np
from field_io import write_field


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", required=True)
    parser.add_argument("--definition", help="SU2 field definition JSON")
    parser.add_argument("--v07-circle-manifest", help="original curved_field_manifest.json")
    parser.add_argument("--output", required=True)
    parser.add_argument("--text", action="store_true")
    args = parser.parse_args()
    if bool(args.definition) == bool(args.v07_circle_manifest):
        parser.error("choose exactly one definition source")
    if args.definition:
        definition = json.loads(Path(args.definition).read_text())
    else:
        source = json.loads(Path(args.v07_circle_manifest).read_text())
        if hashlib.sha256(Path(args.input).read_bytes()).hexdigest() != source["file_sha256"]:
            raise ValueError("v0.7 NPZ hash mismatch")
        definition = {"scenario": "v07_circle", "purpose": "SCENARIO",
                      "U_ref": source["reference_velocity_m_s"], "L_ref": source["reference_length_m"],
                      "epsilon": source["epsilon"], "ell": source["spatial_length_m"], "ell_w": source["wall_length_m"],
                      "geometry": {"type": "CIRCLE", "center": [0,0,0], "radius": source["geometry"]["inner_body_radius_m"]},
                      "support": {"type": "RADIAL", "start": source["support"]["one_to_radius_m"], "end": source["support"]["zero_from_radius_m"]},
                      "source_manifest": source}
    definition["source_npz_sha256"] = hashlib.sha256(Path(args.input).read_bytes()).hexdigest()
    with np.load(args.input, allow_pickle=False) as data:
        report = write_field(args.output, data["nodes"], data["cells"], data["raw_coefficients"], definition, binary=not args.text)
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
