"""Compare complete private stage shards; this does not certify mesh acceptance."""
import argparse
import hashlib
import json
import math
import struct
from pathlib import Path


RECORD_BYTES = 208  # RecordStream: cell Q + 4*(Qddd) + version Q + 4*(present Q, facet Q).


def sha(data):
    return hashlib.sha256(data).hexdigest()


def decode_owned(data, state):
    cells, nodes, coordinates, physical = state
    if len(data) < 8:
        raise ValueError("Missing owned-shard count")
    count, = struct.unpack_from("=Q", data)
    if len(data) != 8 + count * RECORD_BYTES:
        raise ValueError("Owned-shard count/length mismatch")
    for at in range(8, len(data), RECORD_BYTES):
        cell, = struct.unpack_from("=Q", data, at)
        if cell in cells:
            raise ValueError("Duplicate owned cell ID")
        ids = []
        for slot in range(4):
            offset = at + 8 + 32 * slot
            node, x, y, z = struct.unpack_from("=Qddd", data, offset)
            point = data[offset + 8:offset + 32]
            key = (x, y, z)
            if not all(map(math.isfinite, key)):
                raise ValueError("Nonfinite owned coordinate")
            if node in nodes and nodes[node] != point:
                raise ValueError("One node ID has different coordinate bits")
            if key in coordinates and coordinates[key] != node:
                raise ValueError("Different node IDs share a coordinate")
            nodes[node] = point
            coordinates[key] = node
            ids.append(node)
        if len(set(ids)) != 4:
            raise ValueError("Cell repeats a node ID")
        cells.add(cell)
        for slot in range(4):
            present, _ = struct.unpack_from("=QQ", data, at + 144 + 16 * slot)
            if present not in (0, 1):
                raise ValueError("Invalid physical-face flag")
            if present:
                physical.update(ids[k] for k in range(4) if k != slot)
    return count


def read_stage(root, label, ranks, pins):
    state = (set(), {}, {}, set())
    parameters = []
    for rank in range(ranks):
        name = f"stage_{label}_rank{rank}_manifest.json"
        raw = (root / name).read_bytes()
        pins[name] = sha(raw)
        manifest = json.loads(raw)
        expected_stage = "window" if label == "observation_stop" else label
        if (manifest.get("rank") != rank or manifest.get("stage") != expected_stage or
                manifest.get("phase") != "improvement" or
                not manifest.get("complete_shard") or not manifest.get("private_capture") or
                manifest.get("record_format") != 1 or manifest.get("owned_record_bytes") != RECORD_BYTES or
                manifest.get("final_mesh_accepted")):
            raise ValueError("Unsupported/incomplete private stage manifest")
        if label == "observation_stop" and (not manifest.get("diagnostic_observation_stop") or
                manifest.get("quality_windows_observed") != 4):
            raise ValueError("Missing bounded quality observation stop")
        name = manifest["files"]["owned"]
        raw = (root / name).read_bytes()
        pins[name] = sha(raw)
        if decode_owned(raw, state) != manifest["working_rows"]:
            raise ValueError("Owned-shard count differs from manifest")
        name = manifest["files"]["parameters"]
        raw = (root / name).read_bytes()
        pins[name] = sha(raw)
        if not manifest["parameters_present"] or len(raw) != manifest["parameters_payload_bytes"] or len(raw) < 40:
            raise ValueError("Missing/truncated parameter payload")
        parameters.append(raw)
    return state, parameters


def compare(root, ranks):
    pins = {}
    entry, entry_parameters = read_stage(root, "quality_entry", ranks, pins)
    stop, stop_parameters = read_stage(root, "observation_stop", ranks, pins)
    ec, en, _, ep = entry
    sc, sn, _, sp = stop
    old_ids, new_ids = en.keys(), sn.keys()
    return {
        "private_capture": True, "final_mesh_accepted": False, "ranks": ranks,
        "entry": {"cells": len(ec), "vertices": len(en), "physical_vertices": len(ep)},
        "stop": {"cells": len(sc), "vertices": len(sn), "physical_vertices": len(sp)},
        "difference": {
            "cells_added": len(sc - ec), "cells_removed": len(ec - sc),
            "vertices_added": len(new_ids - old_ids), "vertices_removed": len(old_ids - new_ids),
            "retained_vertices_changed_coordinate_bits": sum(en[k] != sn[k] for k in old_ids & new_ids),
            "physical_vertices_added": len(sp - ep), "physical_vertices_removed": len(ep - sp),
        },
        "parameters": [{
            "rank": rank, "entry_header": list(struct.unpack_from("=5Q", a)),
            "stop_header": list(struct.unpack_from("=5Q", b)),
            "identical_except_revision": a[:8] + a[16:] == b[:8] + b[16:],
        } for rank, (a, b) in enumerate(zip(entry_parameters, stop_parameters))],
        "record_layout_source": "CNativeDistributed2D.hpp RecordStream; CNativeMesh3D.hpp Cell/Node; CNativeDistributed3D.hpp OwnedCell",
        "sha256": {"analyzer": sha(Path(__file__).read_bytes()), **pins},
    }


def self_check():
    def state():
        return set(), {}, {}, set()
    row = struct.pack("=Q", 7)
    for node, point in enumerate(((0., 0., 0.), (1., 0., 0.), (0., 1., 0.), (0., 0., 1.))):
        row += struct.pack("=Qddd", node, *point)
    row += struct.pack("=9Q", 1, 1, 10, 0, 0, 0, 0, 0, 0)
    data = struct.pack("=Q", 1) + row
    decoded = state()
    assert decode_owned(data, decoded) == 1
    assert len(decoded[1]) == 4 and decoded[3] == {1, 2, 3}
    bad_node = bytearray(data)
    struct.pack_into("=Q", bad_node, 48, 0)
    duplicate_coordinate = bytearray(data)
    duplicate_coordinate[56:80] = duplicate_coordinate[24:48]
    for bad in (data[:-1], struct.pack("=Q", 2) + row + row, bad_node, duplicate_coordinate):
        try:
            decode_owned(bad, state())
        except ValueError:
            continue
        raise AssertionError("Malformed owned records were accepted")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("attempt", type=Path, nargs="?")
    parser.add_argument("--ranks", type=int, default=6)
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()
    if args.self_check:
        self_check()
    if args.attempt:
        if args.ranks < 1:
            parser.error("--ranks must be positive")
        output = args.attempt / "topology_comparison.json"
        output.write_text(json.dumps(compare(args.attempt, args.ranks), indent=2) + "\n")
        print(output)
    elif not args.self_check:
        parser.error("provide an attempt or --self-check")
