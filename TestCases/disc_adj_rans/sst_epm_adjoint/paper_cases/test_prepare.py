#!/usr/bin/env python3
"""Small preparation tests; optional SU2 --dryrun parsing, never a CFD iteration."""

import argparse
import gzip
import hashlib
import math
import json
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch
import zipfile

from convert_hump import convert
from fetch_hump import convert_archive, main as fetch_main
from prepare import CASES, MODES, STRENGTHS, main as prepare_main, mesh_metadata, prepare, read_config, strength_label


class PreparationTest(unittest.TestCase):
    binary = None

    def test_archive_pipeline(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            archive = root / "fixture.zip"
            name = "hump2newtop_noplenumZ3x2.p2dfmt.gz"
            grid = gzip.compress(b"1\n3 2\n-3 0.5 2 -3 0.5 2\n0 .1 0 1 1 1\n")
            with zipfile.ZipFile(archive, "w") as bundle:
                bundle.writestr("nested/" + name, grid)
                bundle.writestr("../../not_a_mesh", "must not extract")
            with patch("fetch_hump.LEVELS", ((3, 2),)):
                records = convert_archive(archive, root)
            self.assertEqual((records[0]["points"], records[0]["cells"], records[0]["jobs"]), (6, 2, 3 + len(MODES) * len(STRENGTHS)))
            self.assertEqual(records[0]["source_sha256"], hashlib.sha256(grid).hexdigest())
            self.assertFalse((root / "not_a_mesh").exists())
            with patch("fetch_hump.LEVELS", ((5, 3),)):
                with self.assertRaises(ValueError):
                    convert_archive(archive, root)
            destination = root / "offline"
            with patch("fetch_hump.LEVELS", ((3, 2),)), \
                    patch("sys.argv", ["fetch_hump.py", str(destination), "--archive", str(archive)]), \
                    patch("urllib.request.urlopen", side_effect=AssertionError("Offline mode used the network")):
                fetch_main()
            evidence = json.loads((destination / "assets.json").read_text())
            self.assertEqual(evidence["archive_sha256"], hashlib.sha256(archive.read_bytes()).hexdigest())
            self.assertEqual(evidence["archive_bytes"], archive.stat().st_size)
            self.assertEqual(evidence["levels"][0]["jobs"], 3 + len(MODES) * len(STRENGTHS))
            self.assertFalse((destination / archive.name).exists())

    def test_campaigns(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            for case, (dimension, markers) in CASES.items():
                # --dryrun only reads dimension; this is deliberately not a physical mesh.
                mesh = root / f"{case}.su2"
                mesh.write_text(f"NDIME= {dimension}\nNELEM= 1\nNPOIN= 4\nNMARK= {len(markers)}\n" +
                                "".join(f"MARKER_TAG= {m}\nMARKER_ELEMS= 1\n" for m in sorted(markers)))
                destination = root / case
                manifest = prepare(case, destination, mesh)
                self.assertEqual(len(manifest["jobs"]), 3 + len(MODES) * len(STRENGTHS))
                self.assertEqual(manifest["mesh"]["sha256"], hashlib.sha256(mesh.read_bytes()).hexdigest())
                self.assertEqual(manifest["nonlinear_amplitudes_available"], list(STRENGTHS))
                with self.assertRaises(FileExistsError):
                    prepare(case, destination, mesh)
                for job in manifest["jobs"]:
                    directory = destination / job["directory"]
                    cfg = read_config(directory / "run.cfg")
                    self.assertTrue((directory / cfg["MESH_FILENAME"]).is_file())
                    self.assertEqual(cfg["UQ_DELTA_B"], "1")
                    self.assertEqual(cfg["UQ_URLX"], strength_label(job["urlx"]) if job["urlx"] else "1")
                    if job["urlx"]:
                        self.assertEqual(job["directory"], f"nonlinear_urlx{strength_label(job['urlx'])}/{job['epm_mode']}")
                    self.assertEqual(len(cfg["CONV_FIELD"].strip("()").split(",")), dimension + 4)
                    if job["epm_mode"]:
                        component, permute = MODES[job["epm_mode"]]
                        self.assertEqual((cfg["UQ_COMPONENT"], cfg["UQ_PERMUTE"]), (str(component), permute))
                        self.assertEqual(cfg["SST_OPTIONS"], "(V2003m, UQ)")
                        self.assertEqual(cfg["RESTART_SOL"], "YES")
                    if job["directory"] != "baseline":
                        self.assertEqual((directory / cfg["SOLUTION_FILENAME"]).resolve(), destination / "baseline/restart_flow")
                    if job["directory"].startswith("adjoint"):
                        self.assertEqual(cfg["SST_OPTIONS"], "V2003m")
                        self.assertEqual(cfg["SST_EPM_POSTPROCESS"], "YES")
                        self.assertEqual(cfg["RESTART_SOL"], "NO")
                    if case == "nasa_hump":
                        inlet = [s.strip() for s in cfg["MARKER_INLET"].strip("()").split(",")]
                        outlet = float(cfg["MARKER_OUTLET"].strip("()").split(",")[1])
                        self.assertTrue(math.isclose(float(inlet[1]), 298.93))
                        self.assertTrue(math.isclose(float(inlet[2]) / outlet, 1.007 / 0.99962))
                    if self.binary:
                        result = subprocess.run([str(self.binary), "--dryrun", "run.cfg"], cwd=directory,
                                                text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30)
                        if job["directory"] == "baseline":
                            self.assertEqual(result.returncode, 0, result.stdout[-8000:])
                            self.assertIn("No solver started", result.stdout)
                        else:
                            # SU2's dry run still loads primal restarts during preprocessing.
                            self.assertEqual(result.returncode, 1, result.stdout[-8000:])
                            self.assertIn("Unable to open SU2 restart file " + cfg["SOLUTION_FILENAME"] + ".dat",
                                          result.stdout[-1500:])
                        self.assertNotIn("Ignoring the following", result.stdout)
                mesh.write_text(mesh.read_text().replace("NDIME=", "NDIME= 9\n%"))
                with self.assertRaises(ValueError):
                    mesh_metadata(mesh, case)
            for bad in ([], [0.0], [1.5], [0.1, 0.1]):
                with self.assertRaises(ValueError):
                    prepare("naca0012", root / "bad_strengths", strengths=bad)
            full = prepare("naca0012", root / "full_strength", strengths=[1])
            self.assertEqual([j["directory"] for j in full["jobs"] if j["urlx"]][0], "nonlinear_urlx1/1C_KEEP")
            blocked = prepare("naca0012", root / "missing")
            self.assertEqual(blocked["status"], "mesh_required")
            self.assertIsNone(blocked["mesh"])
            self.assertEqual(blocked["direct_amplitudes_available"], [])
            self.assertFalse([j for j in blocked["jobs"] if j["direct_amplitude"] is not None])

    def test_direct_spec(self):
        """SST_EPM_DIRECT configs: the extraction path at fixed amplitudes, next to the classical sweep."""
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            for case, (dimension, markers) in CASES.items():
                mesh = root / f"{case}.su2"
                mesh.write_text(f"NDIME= {dimension}\nNELEM= 1\nNPOIN= 4\nNMARK= {len(markers)}\n" +
                                "".join(f"MARKER_TAG= {m}\nMARKER_ELEMS= 1\n" for m in sorted(markers)))
                destination = root / case
                amplitudes = [0.01, -0.01, 0.1]
                manifest = prepare(case, destination, mesh, direct_amplitudes=amplitudes)
                self.assertEqual(manifest["direct_amplitudes_available"], sorted(amplitudes))
                self.assertEqual(manifest["nonlinear_amplitudes_available"], list(STRENGTHS))
                self.assertIn("SST_EPM_DIRECT", manifest["direct_amplitude_definition"])
                direct = [job for job in manifest["jobs"] if job["direct_amplitude"] is not None]
                self.assertEqual(len(manifest["jobs"]), 3 + len(MODES) * (len(STRENGTHS) + len(amplitudes)))
                self.assertEqual(sorted((j["direct_amplitude"], j["epm_mode"]) for j in direct),
                                 sorted((a, m) for a in amplitudes for m in MODES))
                for job in direct:
                    self.assertIsNone(job["urlx"])
                    label = strength_label(job["direct_amplitude"])
                    self.assertEqual(job["directory"], f"direct_spec/A{label}/{job['epm_mode']}")
                    self.assertEqual(job["requires"], ["baseline:converged_restart"])
                    directory = destination / job["directory"]
                    cfg = read_config(directory / "run.cfg")
                    component, permute = MODES[job["epm_mode"]]
                    self.assertEqual((cfg["UQ_COMPONENT"], cfg["UQ_PERMUTE"]), (str(component), permute))
                    self.assertEqual(cfg["SST_EPM_DIRECT"], "YES")
                    self.assertEqual(cfg["SST_EPM_DIRECT_AMPLITUDE"], label)
                    self.assertEqual(float(cfg["SST_EPM_DIRECT_AMPLITUDE"]), job["direct_amplitude"])
                    # Plain SST with the full classical target controls: the amplitude is the direct option.
                    self.assertEqual(cfg["SST_OPTIONS"], "V2003m")
                    self.assertEqual((cfg["UQ_URLX"], cfg["UQ_DELTA_B"]), ("1", "1"))
                    self.assertEqual(cfg["MATH_PROBLEM"], "DIRECT")
                    self.assertEqual(cfg["SST_EPM_POSTPROCESS"], "NO")
                    self.assertEqual(cfg["RESTART_SOL"], "YES")
                    self.assertEqual(cfg["CFL_ADAPT"], "NO")
                    self.assertEqual(len(cfg["CONV_FIELD"].strip("()").split(",")), dimension + 4)
                    self.assertTrue((directory / cfg["MESH_FILENAME"]).is_file())
                    self.assertEqual((directory / cfg["SOLUTION_FILENAME"]).resolve(), destination / "baseline/restart_flow")
                    if self.binary:
                        result = subprocess.run([str(self.binary), "--dryrun", "run.cfg"], cwd=directory,
                                                text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30)
                        # The direct guards pass; the dry run then stops at the missing baseline restart.
                        self.assertEqual(result.returncode, 1, result.stdout[-8000:])
                        self.assertIn("EPM direct mode: case", result.stdout)
                        self.assertIn("Unable to open SU2 restart file " + cfg["SOLUTION_FILENAME"] + ".dat",
                                      result.stdout[-1500:])
                        self.assertNotIn("Ignoring the following", result.stdout)
            for bad in ([1.5], [-1.01], [0.1, 0.1], [float("nan")], [float("inf")]):
                with self.assertRaises(ValueError):
                    prepare("naca0012", root / "bad_direct", direct_amplitudes=bad)
                self.assertFalse((root / "bad_direct").exists())
            with patch("sys.argv", ["prepare.py", "naca0012", str(root / "cli"), "--strengths", "0.5",
                                    "--direct-amplitudes", "0.02", "-0.02"]):
                prepare_main()
            cli = json.loads((root / "cli" / "manifest.json").read_text())
            self.assertEqual(cli["direct_amplitudes_available"], [-0.02, 0.02])
            self.assertEqual(cli["nonlinear_amplitudes_available"], [0.5])
            self.assertTrue((root / "cli" / "direct_spec/A-0.02/2C_SWAP/run.cfg").is_file())

    def test_hump_conversion(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            source, target = root / "synthetic.p2dfmt.gz", root / "mesh.su2"
            # Synthetic geometry only: exercise the format, orientation and boundary mapping.
            source.write_bytes(gzip.compress(b"1\n3 2\n-3 0.5 2 -3 0.5 2\n0 .1 0 1 1 1\n"))
            convert(source, target)
            info = mesh_metadata(target, "nasa_hump")
            self.assertEqual((info["points"], info["elements"]), (6, 2))
            self.assertIn("9 0 1 4 3", target.read_text())
            with self.assertRaises(FileExistsError):
                convert(source, target)
            source.write_bytes(gzip.compress(b"1\n3 2\n-3 .5 2 -3 .5 2\n0 .1 0 -1 -1 -1\n"))
            with self.assertRaises(ValueError):
                convert(source, root / "invalid.su2")

    def test_existing_mesh_markers(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            for case, markers, wall in [
                    ("naca0012", ["Wall", "Farfield"], "Wall"),
                    ("onera_m6", ["WING3D", "FARFIELD", "SYMMETRY"], "WING3D")]:
                mesh = root / f"{case}.su2"
                mesh.write_text(f"NDIME= {CASES[case][0]}\nNELEM= 1\nNPOIN= 4\nNMARK= {len(markers)}\n" +
                                "".join(f"MARKER_TAG= {m}\nMARKER_ELEMS= 1\n" for m in markers))
                original = mesh.read_bytes()
                manifest = prepare(case, root / case, mesh)
                self.assertTrue(manifest["mesh"]["marker_mapping"])
                self.assertEqual(mesh.read_bytes(), original)
                self.assertEqual((root / case / "mesh/mesh.su2").read_bytes(), original)
                for job in manifest["jobs"]:
                    config = read_config(root / case / job["config"])
                    self.assertEqual(config["MARKER_MONITORING"], f"({wall})")
                    self.assertEqual(config["MARKER_HEATFLUX"], f"({wall}, 0)")
                    self.assertEqual(config["MARKER_FAR"], "(Farfield)" if case == "naca0012" else "(FARFIELD)")
                mesh.write_text(mesh.read_text().replace(wall, "UnknownWall"))
                with self.assertRaises(ValueError):
                    mesh_metadata(mesh, case)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, help="optional local SU2_CFD_AD without OpenMP; --dryrun only")
    args = parser.parse_args()
    PreparationTest.binary = args.binary.resolve(strict=True) if args.binary else None
    unittest.main(argv=[__file__])
