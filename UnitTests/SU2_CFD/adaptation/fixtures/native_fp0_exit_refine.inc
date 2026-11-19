namespace {
bool FP0SameLocalMesh(const Owned& before, const Owned& after) {
  if (before.size() != after.size()) return false;
  auto current = after.begin();
  for (const auto& row : before) {
    if (row.first != current->first) return false;
    for (size_t i = 0; i < row.second.cell.v.size(); ++i) {
      const auto& a = row.second.cell.v[i];
      const auto& b = current->second.cell.v[i];
      if (a.id != b.id || a.p.x != b.p.x || a.p.y != b.p.y || a.p.z != b.p.z) return false;
    }
    ++current;
  }
  return true;
}
}  // namespace

TEST_CASE("FP0 refines the saved valid frozen-volume exit", "[.][NativeFastPathExitRefine3D]") {
  World world;
  REQUIRE(world.size == 6);
  const auto* capture = std::getenv("SU2_NATIVE_FP0_EXIT_ROOT");
  const auto* authority = std::getenv("SU2_NATIVE_FP0_EXIT_REFERENCE");
  const auto* config_path = std::getenv("SU2_NATIVE_FROZEN_CONFIG");
  REQUIRE(capture);
  REQUIRE(authority);
  REQUIRE(config_path);

  const std::string shard = std::string(capture) + "/stage_exit_rank" + std::to_string(world.rank);
  auto options = ProductionEngineOptions();
  options.timings = true;
  options.report_rejected_neighborhood = true;
  size_t expected_rows = 0;
  CLocalFailure setup_failure;
  try {
    std::ifstream limits(shard + "_fp0_options.txt");
    limits >> expected_rows >> options.maximum_rounds >> options.maximum_work >> options.memory_ceiling
           >> options.maximum_seconds >> options.surface_tolerance >> options.wall_heights.tolerance;
    if (!limits || !expected_rows || !options.maximum_work || !options.memory_ceiling ||
        !(options.maximum_seconds > 0))
      throw std::runtime_error("Invalid FP0 exit options for " + shard);
    std::string extra;
    if (limits >> extra) throw std::runtime_error("Extra FP0 exit options for " + shard);

    std::ifstream config_stream(config_path);
    if (!config_stream) throw std::runtime_error("Cannot open pinned FP0 configuration.");
    const CConfig config(config_stream, SU2_COMPONENT::SU2_CFD, false);
    if (config.GetnAdap_BL() || options.surface_tolerance != SU2_TYPE::GetValue(config.GetAdap_Hausd()) ||
        options.wall_heights.tolerance != SU2_TYPE::GetValue(config.GetAdap_BL_FirstHeightTol()) ||
        options.maximum_seconds > SU2_TYPE::GetValue(config.GetAdap_Native_Max_Time()))
      throw std::runtime_error("FP0 exit options conflict with the pinned no-BL configuration.");
    options.job_memory_limit = config.GetAdap_Native_Job_MemoryLimit();
    if (!options.job_memory_limit) throw std::runtime_error("FP0 requires the configured job peak limit.");
    options.requirement_size = {SU2_TYPE::GetValue(config.GetAdap_Hmin()), SU2_TYPE::GetValue(config.GetAdap_Hmax()),
                                SU2_TYPE::GetValue(config.GetAdap_ARmax()), SU2_TYPE::GetValue(config.GetAdap_Hgrad()),
                                SU2_TYPE::GetValue(config.GetAdap_Complexity())};
  } catch (const std::exception& error) {
    setup_failure.Set(2, world.rank, error.what());
  }
  const auto setup_vote = world.elect(setup_failure);
  INFO(setup_vote.message);
  REQUIRE_FALSE(setup_vote.any);

  ReferenceState reference;
  bool loaded = false;
  size_t reference_bytes = 0;
  std::string load_reason;
  const bool admitted = LoadReference(authority, reference, loaded, load_reason, options.memory_ceiling, 0,
                                      reference_bytes);
  INFO(load_reason);
  REQUIRE(admitted);
  REQUIRE(loaded);
  REQUIRE(reference.original);
  REQUIRE(reference.original->Parameters());

  Owned exit_owned;
  std::vector<DonorCell> original_donors;
  CLocalFailure read_failure;
  try {
    {
      const auto bytes = TransactionRead(shard + "_owned.bin");
      RecordStream stream(bytes);
      uint64_t rows = 0;
      stream(rows);
      if (rows != expected_rows) throw std::runtime_error("FP0 exit owned row count differs from its manifest.");
      original_donors.reserve(expected_rows);
      for (uint64_t i = 0; i < rows; ++i) {
        OwnedCell row;
        stream(row);
        if (!row.sensor_present) throw std::runtime_error("FP0 exit cell lacks saved nodal tensors.");
        const auto id = row.cell.id;
        // The observer's original donors are the earlier target. Use the exit nodal tensors as the new target.
        original_donors.push_back({row.cell, row.sensor});
        if (!exit_owned.emplace(id, std::move(row)).second)
          throw std::runtime_error("FP0 exit shards have a repeated owned cell.");
      }
      if (!stream.End()) throw std::runtime_error("FP0 exit owned stream has trailing data.");
    }
    const auto bytes = TransactionRead(shard + "_parameters.bin");
    const auto boundary = BoundaryParameterState::FromPayload(
        reference.original->Parameters()->Authority(), bytes, options.memory_ceiling,
        transfer_memory::Add(reference.original->ResidentBytes(), OwnedStorageBytes(exit_owned),
                             transfer_memory::Bytes(original_donors), transfer_memory::Bytes(bytes)));
    reference.original = reference.original->WithParameterState(boundary);
  } catch (const std::exception& error) {
    read_failure.Set(2, world.rank, error.what());
  } catch (...) {
    read_failure.Set(2, world.rank, "Cannot read the FP0 exit shard " + shard);
  }
  const auto read_vote = world.elect(read_failure);
  INFO(read_vote.message);
  REQUIRE_FALSE(read_vote.any);

  const auto start = std::chrono::steady_clock::now();
  const auto result = Refine(world, exit_owned, original_donors, *reference.original, {}, options);
  const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
  const size_t peak = ProcessPeakRSS3D();
  const auto job_peak = CPassiveComm::Allreduce(uint64_t(peak), CPassiveComm::Op::SUM, world.comm);
  std::ostringstream row;
  row << std::setprecision(17) << "FP0 B_refine {\"rank\":" << world.rank
      << ",\"seconds\":" << seconds << ",\"engine_seconds\":" << result.stats.total_seconds
      << ",\"seed_seconds\":" << result.stats.seed_seconds << ",\"rounds\":" << result.stats.rounds
      << ",\"accepted\":" << result.stats.accepted << ",\"valid\":" << (result.valid ? "true" : "false")
      << ",\"same_local_mesh\":" << (FP0SameLocalMesh(exit_owned, result.cells) ? "true" : "false")
      << ",\"input_cells\":" << exit_owned.size() << ",\"result_cells\":" << result.cells.size()
      << ",\"repair_batch\":" << options.repair_batch << ",\"job_peak_rss_bytes\":" << job_peak
      << ",\"process_peak_rss_bytes\":" << peak << ",\"stop\":" << std::quoted(EngineStopName(result.stats.stop))
      << ",\"reason\":" << std::quoted(result.reason) << '}';
  std::cout << row.str() << std::endl;
}
