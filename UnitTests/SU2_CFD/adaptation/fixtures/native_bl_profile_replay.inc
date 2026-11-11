TEST_CASE("Native BL profile replay preserves composed target and monotone support", "[NativeBLProfileReplay]") {
  CBoundaryLayerMetric::Wall wall;
  wall.layer = {"wall", .01, 1.2, .1};
  wall.coord = {0,0,0, 1,0,0, 0,1,0};
  wall.conn = {0,1,2,2};
  CBoundaryLayerMetric source(3, {wall});

  source.ConfigureOuterNormal(.03, 2.);
  source.SelectOuterNormal(.75);
  source.SelectOuterNormal(.75);  // No-op selections are not history entries.
  source.FreezeOuterNormal();
  source.FreezeOuterNormal();
  source.SelectTangentialScale(.5);
  source.SelectTangentialScale(.25);
  REQUIRE(source.ConfigureOuterTransition(.01, 1.3));
  CHECK_FALSE(source.ConfigureOuterTransition(.1, 1.3));  // Coarsening keeps the finer support.

  const auto& history = source.GetProfileHistory();
  REQUIRE(history.size() == 6);
  CHECK(history[0].operation == 1);
  CHECK(history[1].operation == 2);
  CHECK(history[2].operation == 3);
  CHECK(history[3].operation == 4);
  CHECK(history[4].operation == 4);
  CHECK(history[5].operation == 5);

  CBoundaryLayerMetric replay(3, {wall});
  replay.ReplayProfile(history);
  CHECK(replay.GetProfileHistory().size() == history.size());
  CHECK(replay.GetOuterNormal().protected_distance == source.GetOuterNormal().protected_distance);
  CHECK(replay.GetOuterNormal().maximum_growth == source.GetOuterNormal().maximum_growth);
  CHECK(replay.GetOuterNormal().parameter == source.GetOuterNormal().parameter);
  CHECK(replay.GetOuterNormal().version == source.GetOuterNormal().version);
  CHECK(replay.GetOuterNormal().frozen);
  CHECK(replay.GetTangentialScale() == source.GetTangentialScale());
  CHECK(replay.GetOuterTransitionCore() == source.GetOuterTransitionCore());
  CHECK(replay.GetSampleVersion() == source.GetSampleVersion());
  CHECK(replay.GetGeometrySampleVersion() == source.GetGeometrySampleVersion());
  CHECK(replay.GetSupportVersion() == source.GetSupportVersion());

  for (double z : {0., .01, .05, .09, .11, .3}) {
    const su2double p[] = {.2, .2, z};
    const auto a = source.Evaluate(0, p, .01), b = replay.Evaluate(0, p, .01);
    CHECK(a.profile_version == b.profile_version);
    CHECK(a.distance == b.distance);
    CHECK(a.hn == b.hn);
    CHECK(a.weight == b.weight);
    CheckTensor(3, a.full, b.full);
    CheckTensor(3, a.metric, b.metric);
  }

  const su2double p[] = {.2, .2, .05};
  CBoundaryLayerMetric frozenCore(3, {wall});
  frozenCore.ConfigureOuterNormal(.03, 2.);
  frozenCore.SelectOuterNormal(.75);
  frozenCore.FreezeOuterNormal();
  frozenCore.SelectTangentialScale(.25);
  frozenCore.ConfigureOuterTransition(.1, 1.3);
  const auto detailed = replay.Evaluate(0, p, .01), coarse = frozenCore.Evaluate(0, p, .01);
  CHECK(detailed.hn == coarse.hn);
  CheckTensor(3, detailed.full, coarse.full);
  CHECK(replay.GetSupportVersion() == source.GetSupportVersion());
}

TEST_CASE("Native BL profile replay rejects malformed, redundant and nonfresh history", "[NativeBLProfileReplay]") {
  CBoundaryLayerMetric::Wall wall;
  wall.layer = {"wall", .01, 1.2, .1};
  wall.coord = {0,0,0, 1,0,0, 0,1,0};
  wall.conn = {0,1,2,2};
  const std::vector<CBoundaryLayerMetric::ProfileChange> configure{{1,.03,2.}};
  CBoundaryLayerMetric replay(3, {wall});
  CHECK_THROWS(replay.ReplayProfile({{99,0,0}}));
  CHECK_THROWS(replay.ReplayProfile({{1,.001,2.}}));
  CBoundaryLayerMetric redundant(3, {wall});
  CHECK_THROWS(redundant.ReplayProfile({{1,.03,2.},{2,0,0}}));  // Discard the private failed reconstruction.
  replay.ReplayProfile(configure);
  CHECK_THROWS(replay.ReplayProfile(configure));
  CHECK(replay.GetProfileHistory().size() == 1);
  replay.FreezeOuterNormal();
  CHECK_THROWS(replay.SelectOuterNormal(.5));
}

TEST_CASE("Native BL profile replay refuses tangent changes without publishing them", "[NativeBLProfileReplay]") {
  CBoundaryLayerMetric::FeatureOptions options;
  options.hgrad = 1.3;
  CBoundaryLayerMetric layers(3, Ridge(), 45., options);
  layers.SelectTangentialScale(.25);
  const auto version = layers.GetSampleVersion();
  const auto historySize = layers.GetProfileHistory().size();
  SU2NativeGeometry::AtlasBudget budget;
  budget.maximum_bytes = 1;
  budget.maximum_work = 10000;
  CBoundaryLayerMetric::GeometryReservationScope reservation(layers, budget);
  CHECK_THROWS(layers.SelectTangentialScale(.5));
  CHECK(layers.GetTangentialScale() == .25);
  CHECK(layers.GetSampleVersion() == version);
  CHECK(layers.GetProfileHistory().size() == historySize);
}
