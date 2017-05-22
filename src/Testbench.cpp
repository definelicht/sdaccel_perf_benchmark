/// @author    Johannes de Fine Licht (johannes.definelicht@inf.ethz.ch)
/// @date      May 2017 
/// @copyright This software is copyrighted under the BSD 3-Clause License. 

#include "PerformanceBenchmark.h"

// For testing high performance configurations, running this becomes unfeasible.
// Should only be used to sanity check a small configuration.
int main() {

  constexpr int kOpsPerCycle =
      kDepth * kWidth * (kAddsPerStage + kMultsPerStage);
  constexpr double kOpsTotal = static_cast<double>(kIterations) * kOpsPerCycle;

  if (kOpsTotal > 1e9) {
    std::cerr << "Warning: running testbench for large configuration. This "
                 "could take a long time."
              << "\n";
  }

  DataPack input(kInputVal);
  DataPack output;

  PerformanceBenchmark(&input, &output);

  for (int i = 0; i < kWidth; ++i) {
    if (output[i] != kInputVal) {
      std::cerr << "Mismatch: " << output[i] << " (should be " << kInputVal
                << ")." << std::endl;
      return 1;
    }
  }
  std::cout << "Test ran successfully." << std::endl;

  return 0;
}
