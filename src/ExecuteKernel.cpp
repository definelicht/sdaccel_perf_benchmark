/// @author    Johannes de Fine Licht (johannes.definelicht@inf.ethz.ch)
/// @date      May 2017
/// @copyright This software is copyrighted under the BSD 3-Clause License.

#include "PerformanceBenchmark.h"
#include "hlslib/SDAccel.h"

int main(int argc, char **) {

  if (argc != 1) {
    std::cerr << "Usage: ./ExecuteKernel.exe" << std::endl;
    return 1;
  }

  constexpr int kOpsPerCycle =
      kDepth * kWidth * (kAddsPerStage + kMultsPerStage);
  constexpr double kOpsTotal = static_cast<double>(kIterationsKernel) * kOpsPerCycle;
  constexpr double kOpsPerSecond = kOpsPerCycle * (1e-3 * kFrequency);

  std::cout << "Expected performance at " << kFrequency
            << " MHz: " << kOpsPerSecond << " GOp/s\n";

  try {

    hlslib::ocl::Context context;

    auto readDevice = context.MakeBuffer<DataPack, hlslib::ocl::Access::read>(
        hlslib::ocl::MemoryBank::bank0, 1);
    auto writeDevice = context.MakeBuffer<DataPack, hlslib::ocl::Access::write>(
        hlslib::ocl::MemoryBank::bank0, 1);

    DataPack initial(kInputVal);
    readDevice.CopyFromHost(&initial);

    std::cout << "Creating kernel..." << std::flush;
    auto kernel =
        context.MakeKernel("perf_benchmark.xclbin", "PerformanceBenchmark",
                           readDevice, writeDevice);
    std::cout << " Done.\n";

    std::cout << "Executing kernel..." << std::flush;
    const auto elapsed = kernel.ExecuteTask();
    std::cout << " Done.\nPerformed " << std::setprecision(2) << kOpsTotal
              << " operations in " << elapsed.first
              << " seconds, corresponding to "
              << (1e-9 * kOpsTotal / elapsed.first) << " GOp/s" << std::endl;

  } catch (std::runtime_error const &err) {
    std::cerr << "Execution failed with error: \"" << err.what() << "\"."
              << std::endl;
    return 1;
  }
}
