"""Prepare Part C without editing the active build or time reference."""
from pathlib import Path
import hashlib
import json

from draft_b import HEADER
from run_scoped import TEST_SOURCE

ROOT = Path(__file__).resolve().parent / 'C_draft'
DRIVER = 'SU2_CFD/src/drivers/CSinglezoneDriver.cpp'


def render():
    ROOT.mkdir()
    changes = {
        HEADER: [
            ('struct SplitRecord {',
             '''inline EngineOptions ProductionEngineOptions() {
  EngineOptions options;
  // The directory serves 256 features, and curved repair can ask for four per candidate.
  options.repair_batch = MaximumRepairBatch;
  return options;
}

struct SplitRecord {''', 1),
        ],
        DRIVER: [
            ('        auto remesher=std::make_unique<CNativeRemesher3D>(nativeReference3D,SU2Native3D::EngineOptions{},'
             'std::move(firstRowTarget),std::move(normalization));',
             '        auto remesher = std::make_unique<CNativeRemesher3D>(\n'
             '            nativeReference3D, SU2Native3D::ProductionEngineOptions(), std::move(firstRowTarget),\n'
             '            std::move(normalization));', 1),
            ('      auto remesher=std::make_unique<CNativeRemesher3D>(nativeReference3D,SU2Native3D::EngineOptions{},'
             'std::move(constraints),std::move(normalization));',
             '      auto remesher = std::make_unique<CNativeRemesher3D>(\n'
             '          nativeReference3D, SU2Native3D::ProductionEngineOptions(), std::move(constraints),\n'
             '          std::move(normalization));', 1),
        ],
        TEST_SOURCE: [
            ('const Tensor tensor{1,0,0,1,0,1};EngineOptions options;options.maximum_rounds=128;\n  Id first_bad_vertex',
             '''const Tensor tensor{1, 0, 0, 1, 0, 1};
  EngineOptions options;
  options.maximum_rounds = 128;
  options.candidate_batch = MaximumCandidateBatch;
  Id first_bad_vertex''', 1),
            ('}\n\nTEST_CASE("Metric adaptation resolves opposing length demands before shape census"',
             '''
  if (world.size == 2 || world.size == 4) {
    options.repair_batch = MaximumRepairBatch;
    const auto first = Repair(world, input, Uniform(input, tensor), geometry, {}, options);
    const auto repeat = Repair(world, input, Uniform(input, tensor), geometry, {}, options);
    INFO(first.reason);
    INFO(repeat.reason);
    REQUIRE(first.valid);
    REQUIRE(repeat.valid);
    CHECK(first.stats.cached_invalid_cells == 0);
    CHECK(repeat.stats.cached_invalid_cells == 0);
    CHECK(Bytes(first.cells) == Bytes(repeat.cells));
    CHECK(Bytes(input) == saved);
  }
}

TEST_CASE("Metric adaptation resolves opposing length demands before shape census"''', 1),
        ],
    }
    manifest = {}
    for name, replacements in changes.items():
        base = Path(name).read_bytes()
        text = base.decode()
        for old, new, count in replacements:
            assert text.count(old) == count, (name, old)
            text = text.replace(old, new)
        target = ROOT / name
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(text)
        manifest[name] = {'base': hashlib.sha256(base).hexdigest(),
                          'draft': hashlib.sha256(target.read_bytes()).hexdigest()}
    (ROOT / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')


if __name__ == '__main__':
    render()
