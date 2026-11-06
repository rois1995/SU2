// Parse/identity checks only: no mesh, solver or time stepping is constructed.
#include "../../../Common/include/CConfig.hpp"
#include <fstream>
#include <iostream>

int main(int argc, char** argv) {
  if (argc != 4) {
    std::cerr << "usage: stress_config_probe config PARSE|WRITE|CHECK checkpoint_stem\n";
    return 2;
  }
  std::ifstream input(argv[1]);
  if (!input) return 2;
  CConfig config(input, SU2_COMPONENT::SU2_CFD, false);
  config.SetLength_Ref(1);
  config.SetVelocity_Ref(1);
  config.SetDensity_Ref(1);
  const std::string action = argv[2];
  if (action == "WRITE")
    config.WriteStressRestart(argv[3]);
  else if (action == "CHECK")
    config.CheckStressRestart(argv[3]);
  else if (action != "PARSE")
    return 2;
  std::cout << config.StressDiscrepancyIdentity(true) << '\n';
}
