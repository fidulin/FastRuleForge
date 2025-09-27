// FastRuleForge source code

#pragma once


#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstring>

#include <limits>
#include <cmath>
#include <algorithm>

class GPU_executor{
public:
  MTL::Device* _mDevice;
  MTL::CommandQueue* _mCommandQueue;
  MTL::Library* _mLibrary;
  MTL::Function* _mFunction;
  MTL::ComputePipelineState* _mPipelineState;

  MTL::Buffer* _mBufferStrings;
  MTL::Buffer* _mBufferLengths;
  MTL::Buffer* _mBufferPointers;
  MTL::Buffer* _mBufferResult;

  MTL::Buffer* _mBufferSimilarity;
  MTL::Buffer* _mBufferResponsibility;
  MTL::Buffer* _mBufferAvailability;

  int PASSWORDS_COUNT = 0;
  int total_length = 0;
    
  std::vector<unsigned char> lengths_vec;
  std::vector<int> pointers_vec;
  std::vector<std::string> passwords;

  char* concatenated_string = nullptr;

  std::vector<std::vector<int>> clusters;

  std::string kernelSource;
  
  size_t preferred_multiple;

  GPU_executor() {
    load_kernel("src/kernel_source.metal");
  }

  ~GPU_executor() {
    if (concatenated_string) {
        delete[] concatenated_string;
    }
  }

  void load_kernel(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) {
      std::cerr << "Failed to open kernel file, check if \"" << filename << "\" is not missing" << std::endl;
      exit(1);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    kernelSource = buffer.str();
  }

  int process_input(std::string filename, bool verbose = false);
  
  int setup(std::string kernel_main_function, bool verbose = false);

  int* HAC_calculate(unsigned char threshold, size_t local_work_size, size_t global_work_size);
  
  int* calculate_distances_to(int index, unsigned char threshold, size_t global_work_size);

  void AP_compute_matrix(float damping, int option);

  int* AP_calculate(int iter, float lambda);

  int clean();

  void handle_error(NS::Error* error, int callerLine);
};