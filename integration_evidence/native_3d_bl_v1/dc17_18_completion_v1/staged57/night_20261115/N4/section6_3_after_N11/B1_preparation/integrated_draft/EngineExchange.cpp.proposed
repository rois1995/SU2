// Owns only empty optional slots; local Adapt scopes govern payload lifetimes.
#include "EngineExchange.hpp"

EngineExchange::EngineExchange() = default;
EngineExchange::~EngineExchange() = default;

void EngineExchange::RetireDirectory(const DirectoryStats& old) {
  auto& retired_directory = *this->retired_directory;

  retired_directory.collision_seconds += old.collision_seconds;
  retired_directory.collision_builds += old.collision_builds;
  retired_directory.collision_candidates += old.collision_candidates;
  retired_directory.collision_checks += old.collision_checks;
  retired_directory.elections += old.elections;
  retired_directory.election_seconds += old.election_seconds;
  retired_directory.parameter_agreement_seconds += old.parameter_agreement_seconds;
  retired_directory.parameter_publication_seconds += old.parameter_publication_seconds;
  retired_directory.parameter_edits += old.parameter_edits;
  retired_directory.parameter_faces += old.parameter_faces;
  retired_directory.parameter_payload_bytes += old.parameter_payload_bytes;
  retired_directory.change_scanned += old.change_scanned;
  retired_directory.change_sent += old.change_sent;
  retired_directory.change_received += old.change_received;
  retired_directory.change_seconds += old.change_seconds;
  retired_directory.maximum_work_bytes = std::max(retired_directory.maximum_work_bytes, old.maximum_work_bytes);
}
