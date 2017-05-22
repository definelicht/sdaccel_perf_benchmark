About
-----

This project measures floating point performance on Xilinx FPGAs using SDAccel with kernels compiled in Vivado HLS.
Tuning the performance to reach maximum is a slow and manual process, and must be done individually for each SDAccel DSA.

The kernel with in instantiates a deep pipeline of operations, with a number of individually pipelined stages writing to the next in a systolic array fashion. In order to maximize performance, the parameters to tweak are:

- Data type, configured by the option `BENCHMARK_DATA_TYPE`.
- Number of stages/processing elements, each of which will be a dataflow function. This is configured using the option `BENCHMARK_COMPUTE_DEPTH`.
- SIMD width of each stage, replicating the computations horizontally, configured using the option `BENCHMARK_COMPUTE_WIDTH`.
- Number of adds and mults per stage, which can be used to vary the ratio between the two types of operations, configured using `BENCHMARK_ADDS_PER_STAGE` and `BENCHMARK_MULTS_PER_STAGE`.
- Which IP core is used to implement adds and mults respectively, e.g. `FAddSub_nodsp`, configured with the options `BENCHMARK_ADD_CORE` and `BENCHMARK_MULT_CORE`.
- Target frequency of the kernel, configured by the option `BENCHMARK_TARGET_CLOCK`.

Building
--------

To build with dummy parameters:

```sh
mkdir "<path to build dir>"
cd "<path to build dir>"
cmake "<path to source dir>" -DBENCHMARK_DSA="<DSA string to target, e.g. 'xilinx:tul-pcie3-ku115:2ddr:3.1'>"
make           # Builds host-side software
make synthesis # Runs HLS
make kernel    # Builds hardware kernel
```

Running
-------

After building with `make`, run the binary `ExecuteKernel.exe`.  
This executable will look for the kernel file `peak_benchmark.xclbin`, which should be located in the same directory. The file will print the result of the benchmark to standard output.

Theoretical peak
----------------

Along with the SDAccel kernel, this repository includes a Python scripts `peak/maximize.py` to compute the theoretical peak performance of a given FPGA chip, based on the resources available and the resource consumption of the floating point implementations on the architecture in question. The script takes as input a JSON file containing the board specifications and a JSON file containing the specification of which operations to optimize for. Examples of both are
included in the `peak/board` and `peak/ops` subdirectories, respectively.

The additional parameters `maxlut` and `maxdsp` can be set to limit the number of resources that can be used. This is useful for taking into account routing congestion at very high resource utilization, as well as the constant overhead introduced by components such as PCIe and memory controllers. SDAccel DSAs that do not have "XPR" in the name only allow a fixed subset of the chip to be used for user logic, as the rest is reserved. The AlphaData 7V3 DSA, for example, statically
reserves 30% of both LUT and DSP resources, so this should be considered when computing the theoretical maximum performance.

Example usage:

```sh
% ./maximize.py boards/TUL-KU115.board ops/Jacobi2D_SP.ops -maxlut=0.7 
Operation counts:
  float/add/FAddSub_fulldsp: 1566 instances
  float/mult/FMul_fulldsp: 522 instances
Total: 2088 ops/cycle
Peak: 626.4 Gops/s at 300.0 MHz
Utilization:
  4176 / 5520 DSP (75.7%, 75.7% of available)
  464058 / 663360 LUT (70.0%, 99.9% of available)
```

Maximization procedure
----------------------

When aiming to achieve peak performance on a given board, the recommend procedure is as follows:

1. Run the script in `peak/maximize.py` with varying ratios of addition to multiplications, and identify the ratio with the highest performance potential. (Note: It is recommended to set `maxlut` to a fraction less than 1, both to account for constant overhead from memory controllers and to allow some slack when routing the design.)
2. Set `BENCHMARK_COMPUTE_DEPTH` and `BENCHMARK_COMPUTE_WIDTH` so that their product is less than or equal to the number of compute units reported by the maximization script, and set `BENCHMARK_ADD_CORE` and `BENCHMARK_MULT_CORE` to the reported values. 
3. If the build fails, inspect the output:
  - If placement of the design failed, check with resource has been exceeded and by how much, and adjust the `maxlut` or `maxdsp` parameters of the maximization script accordingly. Repeat from step 1.
  - If routing or timing failed, slightly reduce the number of compute units by adjusting `BENCHMARK_COMPUTE_DEPTH` and/or `BENCHMARK_COMPUTE_WIDTH`, and repeat step 3.
4. Once a build succeeds, final tweaking can be done by adjusting `BENCHMARK_TARGET_CLOCK` and the ratio between `BENCHMARK_COMPUTE_DEPTH` and `BENCHMARK_COMPUTE_WIDTH`.

The costs file
--------------

When running the script `peak/maximize.py`, the default resource costs for floating point operations used are ones extracted from performing high-level synthesis when targeting Virtex~7, included in `peak/ops/Virtex7.ops`. To get the most accurate numbers, an operation costs file can be constructed from either datasheet or experimental numbers, then passed to the maximization script via the `-ops=<path>` option. 


Bugs
----

The code included in this repository has _only_ been tested with SDx 2016.3.
Please report bugs to the issue tracker, or email `johannes.definelicht@inf.ethz.ch`.

