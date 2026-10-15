// Recorded remaining blockage and the independently identified complete-star closure; test only.
Patch CurvedRemainingWallPatch(bool complete) {
  const std::map<Id,Node> nodes{
    {0, {0, {0.0,0.0,0.0}}},
    {4, {4, {-0.7071067811865475,0.7071067811865476,0.0}}},
    {5, {5, {-1.0,1.2246467991473532e-16,0.0}}},
    {9, {9, {0.0,0.0,1.0}}},
    {13, {13, {-0.7071067811865475,0.7071067811865476,1.0}}},
    {14, {14, {-1.0,1.2246467991473532e-16,1.0}}},
    {66, {66, {-0.6767766952966369,0.17677669529663695,0.25}}},
    {67, {67, {-0.8561749504366182,0.21656808877726144,0.3333333333333333}}},
    {71, {71, {-0.6035533905932737,0.35355339059327384,0.5}}},
    {72, {72, {-0.7585438774988006,0.45227034917277725,0.6666666666666666}}},
  };
  Patch out;
  out.cells.push_back({1072, {{nodes.at(66),nodes.at(0),nodes.at(4),nodes.at(14)}}});
  out.cells.push_back({1074, {{nodes.at(67),nodes.at(4),nodes.at(5),nodes.at(14)}}});
  out.cells.push_back({1075, {{nodes.at(67),nodes.at(4),nodes.at(66),nodes.at(5)}}});
  out.cells.push_back({1076, {{nodes.at(67),nodes.at(4),nodes.at(14),nodes.at(66)}}});
  out.cells.push_back({1077, {{nodes.at(67),nodes.at(5),nodes.at(66),nodes.at(14)}}});
  out.cells.push_back({1200, {{nodes.at(71),nodes.at(0),nodes.at(14),nodes.at(4)}}});
  out.cells.push_back({1202, {{nodes.at(72),nodes.at(4),nodes.at(14),nodes.at(13)}}});
  out.cells.push_back({1203, {{nodes.at(72),nodes.at(4),nodes.at(13),nodes.at(71)}}});
  out.cells.push_back({1204, {{nodes.at(72),nodes.at(4),nodes.at(71),nodes.at(14)}}});
  out.cells.push_back({1205, {{nodes.at(72),nodes.at(13),nodes.at(14),nodes.at(71)}}});
  out.cells.push_back({1071, {{nodes.at(66),nodes.at(0),nodes.at(5),nodes.at(4)}}});
  out.cells.push_back({1201, {{nodes.at(71),nodes.at(0),nodes.at(13),nodes.at(14)}}});
  out.cells.push_back({11, {{nodes.at(13),nodes.at(0),nodes.at(9),nodes.at(14)}}});
  out.cells.push_back({1073, {{nodes.at(66),nodes.at(0),nodes.at(14),nodes.at(5)}}});
  if(complete)out.cells.push_back({1199, {{nodes.at(71),nodes.at(0),nodes.at(4),nodes.at(13)}}});
  out.surface.push_back({{4,5,14},8});
  out.surface.push_back({{4,14,13},8});
  out.surface.push_back({{0,5,4},0});
  out.surface.push_back({{13,14,9},24});
  return out;
}
