/// @author    Johannes de Fine Licht (johannes.definelicht@inf.ethz.ch)
/// @date      May 2017 
/// @copyright This software is copyrighted under the BSD 3-Clause License. 

#include "PerformanceBenchmark.h"
#include <hls_stream.h>

void ReadIn(DataPack const *in, hls::stream<DataPack> &out) {
  DataPack value;
ReadIn:
  for (long i = 0; i < kIterationsKernel; ++i) {
    #pragma HLS PIPELINE
    // Only read a single value to minimize the influence of memory effects on
    // performance
    if (i == 0) {
      value = *in;
    }
    out.write(value);
  }
}

void Compute(hls::stream<DataPack> &in, hls::stream<DataPack> &out) {
ComputeMain:
  for (long i = 0; i < kIterationsKernel; ++i) {
    #pragma HLS PIPELINE
    auto value = in.read();
  ComputeWidth:
    for (int j = 0; j < kWidth; ++j) {
      #pragma HLS UNROLL
    ComputeAdds:
      for (int k = 0; k < kAddsPerStage; ++k) {
        #pragma HLS UNROLL
        static constexpr Data_t kAddVal =
            kInputVal * ((1 << kMultsPerStage) - 1) / kAddsPerStage;
        const auto eval = value[j] + kAddVal;
        BENCHMARK_RESOURCE_PRAGMA(eval, BENCHMARK_ADD_CORE)
        value[j] = eval;
      }
    ComputeMults:
      for (int k = 0; k < kMultsPerStage; ++k) {
        #pragma HLS UNROLL
        static constexpr Data_t kMultVal = 0.5;
        const auto eval = value[j] * kMultVal;
        BENCHMARK_RESOURCE_PRAGMA(eval, BENCHMARK_MULT_CORE)
        value[j] = eval;
      }
    }
    out.write(value);
  }
}

void WriteOut(hls::stream<DataPack> &in, DataPack *out) {
WriteOut:
  for (long i = 0; i < kIterationsKernel; ++i) {
    #pragma HLS PIPELINE
    const auto read = in.read();
    // Only write out a single value to minimize the influence of memory effects
    // on performance
    if (i == kIterationsKernel - 1) {
      *out = read;
    }
  }
}

void PerformanceBenchmark(DataPack const *in, DataPack *out) {

  #pragma HLS INTERFACE m_axi port=in  offset=slave bundle=gmem0
  #pragma HLS INTERFACE m_axi port=out offset=slave bundle=gmem1
  #pragma HLS INTERFACE s_axilite port=in         bundle=control
  #pragma HLS INTERFACE s_axilite port=out        bundle=control
  #pragma HLS INTERFACE s_axilite port=return     bundle=control

  #pragma HLS DATAFLOW

  hls::stream<DataPack> pipes[kDepth + 1];

  ReadIn(in, pipes[0]);


UnrollCompute:
  for (int d = 0; d < kDepth; ++d) {
    #pragma HLS UNROLL
    Compute(pipes[d], pipes[d+1]);
  }

  WriteOut(pipes[kDepth], out);

}
