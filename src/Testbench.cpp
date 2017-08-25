/// @author    Johannes de Fine Licht (johannes.definelicht@inf.ethz.ch)
/// @date      May 2017 
/// @copyright This software is copyrighted under the BSD 3-Clause License. 

#include "PerformanceBenchmark.h"

// For testing high performance configurations, running this becomes unfeasible.
// Should only be used to sanity check a small configuration.
int main() {

  DataPack input(kInputVal);
  DataPack output;

  PerformanceBenchmark(&input, &output);

  for (int i = 0; i < kWidth; ++i) {
    if (Data_t(output[i]) != kInputVal) {
      std::cerr << "Mismatch: " << Data_t(output[i]) << " (should be " << kInputVal
                << ")." << std::endl;
      return 1;
    }
  }
  std::cout << "Test ran successfully." << std::endl;

  return 0;
}
