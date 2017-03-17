#include "PeakBenchmark.h"
#include <hls_stream.h>

void ReadIn(DataPack const *in, hls::stream<DataPack> &out, long iterations) {
  DataPack value;
ReadIn:
  for (long i = 0; i < iterations; ++i) {
    #pragma HLS PIPELINE
    if (i == 0) {
      value = *in;
    }
    out.write(value);
  }
}

void Compute(hls::stream<DataPack> &in, hls::stream<DataPack> &out,
             long iterations) {
ComputeMain:
  for (long i = 0; i < iterations; ++i) {
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
        value[j] = value[j] + kAddVal;
      }
    ComputeMults:
      for (int k = 0; k < kMultsPerStage; ++k) {
        #pragma HLS UNROLL
        static constexpr Data_t kMultVal = 0.5;
        value[j] = value[j] * kMultVal;
      }
    }
    out.write(value);
  }
}

void WriteOut(hls::stream<DataPack> &in, DataPack *out, long iterations) {
WriteOut:
  for (long i = 0; i < iterations; ++i) {
    #pragma HLS PIPELINE
    const auto read = in.read();
    if (i == iterations - 1) {
      *out = read;
    }
  }
}

void PeakBenchmark(DataPack const *in, DataPack *out, long iterations) {

  #pragma HLS INTERFACE m_axi port=in  offset=slave bundle=gmem0
  #pragma HLS INTERFACE m_axi port=out offset=slave bundle=gmem1
  #pragma HLS INTERFACE s_axilite port=in         bundle=control
  #pragma HLS INTERFACE s_axilite port=out        bundle=control
  #pragma HLS INTERFACE s_axilite port=iterations bundle=control
  #pragma HLS INTERFACE s_axilite port=return     bundle=control

  #pragma HLS DATAFLOW

  hls::stream<DataPack> pipes[kDepth + 1];

  ReadIn(in, pipes[0], iterations);

UnrollCompute:
  for (int d = 0; d < kDepth; ++d) {
    #pragma HLS UNROLL
    Compute(pipes[d], pipes[d+1], iterations);
  }

  WriteOut(pipes[kDepth], out, iterations);

}
