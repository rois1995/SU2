// Isolated check of the M27 accounting with no geometry or mesh dependencies.
#include "Common/include/adaptation/CNativeD9Profile3D.hpp"
#include <cassert>

int main() {
  using namespace SU2NativeD9;
  Data data;
  ObserveUnionMiss(1, Answer::Magnitude, Answer::CutOff, 2, 0.25);
  assert(data.seen_faces.empty());

  SU2NativeTiming3D::Frame frame;
  frame.phase = SU2NativeTiming3D::Phase::Quality;
  SU2NativeTiming3D::current_frame = &frame;
  active = &data;
  {
    PartScope site(SitePart(Site::Guidance));
    {
      PartScope guidance(Part::GuidanceFresh);
      ObserveUnionMiss(1, Answer::Magnitude, Answer::CutOff, 2, 0.25);
      ObserveUnionMiss(1, Answer::Magnitude, Answer::Magnitude, 3, 0.50);
      ObserveUnionMiss(1, Answer::Magnitude, Answer::Magnitude, 4, 0.75);
    }
    assert(current_part == Part::GuidanceOther);
    ObserveUnionBypass();
  }
  assert(current_part == Part::None);
  {
    PartScope census(SitePart(Site::Census));
    ObserveUnionHit(1, Answer::Magnitude);
    ObserveUnionHit(1, Answer::Magnitude);
    ObserveUnionHit(2, Answer::Classified);
    ObserveUnionMiss(3, Answer::Classified, Answer::Classified, 8, 0.5);
  }

  const auto& guidance = data.unions[1][size_t(Part::GuidanceFresh)];
  assert(guidance.queries == 3 && guidance.hits == 0);
  assert((guidance.misses == std::array<uint64_t, 3>{1, 1, 1}));
  assert((guidance.work == std::array<uint64_t, 3>{2, 4, 3}));
  assert((guidance.seconds == std::array<double, 3>{0.25, 0.75, 0.50}));
  assert(guidance.faces_reached_census == 1 && guidance.work_reached_census == 2);
  const auto& census = data.unions[1][size_t(Part::Census)];
  assert(census.queries == 4 && census.hits == 3 && census.faces_reached_census == 0);
  assert(data.unions[1][size_t(Part::GuidanceOther)].bypass == 1);
  assert(data.seen_faces.size() == 3 && data.faces_from_earlier_event == 1);
  assert(data.seen_faces.at(2).stage == StageCount && data.seen_faces.at(2).reached_census);
  {
    SmoothChain chain{8, 3, true};
  }
  assert(data.chains.calls == 1 && data.chains.moved == 1 && data.chains.checked == 8 && data.chains.passed == 3);
  assert(data.chains.checked_bins[4] == 1 && data.chains.passed_bins[3] == 1);
  data.union_observer_failed = true;
  ObserveUnionHit(4, Answer::Classified);
  assert(data.seen_faces.size() == 3);
}
