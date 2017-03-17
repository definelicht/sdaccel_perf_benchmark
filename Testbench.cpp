#include "PeakBenchmark.h"

int main() {

  DataPack input(kInputVal);
  DataPack output;

  PeakBenchmark(&input, &output, 10);

  for (int i = 0; i < kWidth; ++i) {
    if (output[i] != kInputVal) {
      std::cerr << "Mismatch: " << output[i] << " (should be " << kInputVal
                << ")." << std::endl;
      return 1;
    }
  }

  return 0;
}
