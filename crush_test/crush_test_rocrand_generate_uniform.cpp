// Copyright (c) 2017 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <array>
#include <iterator>
#include <random>
#include <algorithm>
#include <climits>

#include <boost/program_options.hpp>

#include <hip/hip_runtime.h>
#include <rocrand.h>
#include <file_common.hpp>

extern "C" {
#include "bbattery.h"
}

#define HIP_CHECK(condition)         \
  {                                  \
    hipError_t error = condition;    \
    if(error != hipSuccess){         \
        std::cout << error << std::endl; \
        exit(error); \
    } \
  }

#define ROCRAND_CHECK(condition)                 \
  {                                              \
    rocrand_status status = condition;           \
    if(status != ROCRAND_STATUS_SUCCESS) {       \
        std::cout << status << std::endl; \
        exit(status); \
    } \
  }

#ifndef DEFAULT_RAND_N
const size_t DEFAULT_RAND_N = 1024 * 1024 * 128;
#endif

void run_crush_test(const size_t size, const rocrand_rng_type rng_type)
{
    float * data;
    float * h_data = new float[size];
    HIP_CHECK(hipMalloc((void **)&data, size * sizeof(float)));

    rocrand_generator generator;
    ROCRAND_CHECK(rocrand_create_generator(&generator, rng_type));
    // Make sure memory is allocated
    HIP_CHECK(hipDeviceSynchronize());

    ROCRAND_CHECK(rocrand_generate_uniform(generator, (float *) data, size));
    HIP_CHECK(hipDeviceSynchronize());
    
    HIP_CHECK(hipMemcpy(h_data, data, size * sizeof(float), hipMemcpyDeviceToHost));
    HIP_CHECK(hipDeviceSynchronize());
    
    rocrand_file_write_results("rocrand_generate.txt", h_data, size);
    
    rocrand_destroy_generator(generator);
    HIP_CHECK(hipFree(data));
    delete[] h_data;
    
    bbattery_SmallCrushFile("rocrand_generate.txt");
}

int main(int argc, char *argv[])
{
    namespace po = boost::program_options;
    po::options_description options("options");
    options.add_options()
        ("help", "show usage instructions")
        ("size", po::value<size_t>()->default_value(DEFAULT_RAND_N), "number of values")
        ("engine", po::value<std::string>()->default_value("philox"), "random number engine")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, options), vm);
    po::notify(vm);

    if(vm.count("help")) {
        std::cout << options << std::endl;
        return 0;
    }

    const size_t size = vm["size"].as<size_t>();
    const std::string& engine = vm["engine"].as<std::string>();

    if(engine == "philox" || engine == "all"){
        std::cout << "philox4x32_10:" << std::endl;
        run_crush_test(size, ROCRAND_RNG_PSEUDO_PHILOX4_32_10);
        return 0;
    }
    
    std::cerr << "Error: unknown random number engine '" << engine << "'" << std::endl;
    return -1;
}