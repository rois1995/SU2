// Diagnostic-only extra cases reuse actual qualified fixture and MPI/transaction helpers.
#include "../../../UnitTests/Common/adaptation/CNativeDistributed3D_tests.cpp"

TEST_CASE("Rotating reservation repeated shared claims elect each rank without mutation", "[NativeRotatingProbe3D]") {
  World world;
  Directory directory(world);
  auto owned=Partition(Rows(Box().patch),world);
  std::string reason;
  REQUIRE(directory.Build(owned,reason));
  const auto saved=directory.Snapshot();const auto mesh=Bytes(owned);
  const auto dependencies=directory.Lookup(Query(world,{{0,6,0},2},true));
  const auto imported=directory.Fetch(dependencies,owned);
  REQUIRE(imported.valid);
  for (int epoch=0;epoch<2*world.size;++epoch) {
    const bool chosen=directory.Reserve(imported,true,reason);
    CHECK(chosen==(world.rank==epoch%world.size));
    CHECK(world.sum(chosen)==1);
    CHECK(directory.Snapshot()==saved);CHECK(Bytes(owned)==mesh);
  }
  const bool vetoed=directory.Reserve(imported,true,reason,world.rank==0 ? 1 : SIZE_MAX);
  CHECK_FALSE(vetoed);CHECK(directory.Snapshot()==saved);CHECK(Bytes(owned)==mesh);
}

TEST_CASE("Rotating reservation disjoint complete stars remain concurrently selectable", "[NativeRotatingProbe3D]") {
  World world;
  Directory directory(world);
  Owned owned;
  const auto offset=Id(world.rank)*32;
  for (auto entry:Rows(Box().patch)) {
    auto row=entry.second;row.cell.id+=offset;
    for (auto& node:row.cell.v) {node.id+=offset;node.p.x+=2*world.rank;}
    for (auto& binding:row.physical) if (binding.present) binding.facet+=offset;
    owned.emplace(row.cell.id,row);
  }
  std::string reason;REQUIRE(directory.Build(owned,reason));
  const auto saved=directory.Snapshot();const auto mesh=Bytes(owned);
  const auto dependencies=directory.Lookup({Feature{{offset,offset+6,0},2}});
  const auto imported=directory.Fetch(dependencies,owned);REQUIRE(imported.valid);
  for (int epoch=0;epoch<2*world.size;++epoch) {
    const bool chosen=directory.Reserve(imported,true,reason);
    CHECK(chosen);CHECK(world.sum(chosen)==uint64_t(world.size));
    CHECK(directory.Snapshot()==saved);CHECK(Bytes(owned)==mesh);
  }
}

TEST_CASE("Rotating reservation phase mismatch rejects collectively without store mutation", "[NativeRotatingProbe3D]") {
  World world;
  if (world.size<2) {SUCCEED("A serial communicator cannot have mismatched rank phases.");return;}
  Directory directory(world);
  auto owned=Partition(Rows(Box().patch),world);
  std::string reason;REQUIRE(directory.Build(owned,reason));
  const auto saved=directory.Snapshot();const auto mesh=Bytes(owned);
  const auto dependencies=directory.Lookup(Query(world,{{0,6,0},2},true));
  const auto imported=directory.Fetch(dependencies,owned);REQUIRE(imported.valid);
  // Test-only corruption of the otherwise matching call count; underlying object is nonconst.
  if (world.rank==0) ++const_cast<DirectoryStats&>(directory.Statistics()).reservations;
  CHECK_FALSE(directory.Reserve(imported,true,reason));
  CHECK(reason.find("epoch/rank mismatch")!=std::string::npos);
  CHECK(directory.Snapshot()==saved);CHECK(Bytes(owned)==mesh);
}
