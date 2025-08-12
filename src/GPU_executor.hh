// FastRuleForge source code

#pragma once
#define CL_TARGET_OPENCL_VERSION 300

#include <CL/cl.h>
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
  cl_platform_id platforms[10];
  cl_device_id device;
  cl_uint devices_num, platforms_num;
  cl_int ret;
  cl_context context;
  cl_command_queue queue;
  cl_program program;
  cl_kernel kernel;
  cl_mem bufferStrings = NULL;
  cl_mem bufferLengths = NULL;
  cl_mem bufferPointers = NULL;
  cl_mem bufferCluster1 = NULL;
  cl_mem bufferCluster1_size = NULL;
  cl_mem bufferResult = NULL;

  cl_mem bufferSimilarity = NULL;
  cl_mem bufferResponsibility = NULL;
  cl_mem bufferAvailability = NULL;


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
    load_kernel("src/kernel_source.cl");
  }

  void load_kernel(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) {
      std::cerr << "Failed to open kernel file, check if \"kernel_source.cl\" is not missing" << std::endl;
      exit(1);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    kernelSource = buffer.str();
  }

  int process_input(std::string filename, bool verbose = false);
  
  int setup(std::string kernel_main_function, bool verbose = false);

  int* HAC_calculate(unsigned char threshold, size_t local_work_size, size_t global_work_size);
  
  float* calculate_distances_to(int index, unsigned char threshold, size_t global_work_size);

  void AP_compute_matrix(float damping, int option);

  int* AP_calculate(int iter, float lambda);

  int clean();

  void handle_error(cl_int ret, int callerLine);

  std::string getCLErrorString(cl_int error) {
    switch (error) {
        case CL_SUCCESS:                                   return "CL_SUCCESS";
        case CL_DEVICE_NOT_FOUND:                          return "CL_DEVICE_NOT_FOUND";
        case CL_DEVICE_NOT_AVAILABLE:                      return "CL_DEVICE_NOT_AVAILABLE";
        case CL_COMPILER_NOT_AVAILABLE:                    return "CL_COMPILER_NOT_AVAILABLE";
        case CL_MEM_OBJECT_ALLOCATION_FAILURE:             return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
        case CL_OUT_OF_RESOURCES:                          return "CL_OUT_OF_RESOURCES";
        case CL_OUT_OF_HOST_MEMORY:                        return "CL_OUT_OF_HOST_MEMORY";
        case CL_PROFILING_INFO_NOT_AVAILABLE:              return "CL_PROFILING_INFO_NOT_AVAILABLE";
        case CL_MEM_COPY_OVERLAP:                          return "CL_MEM_COPY_OVERLAP";
        case CL_IMAGE_FORMAT_MISMATCH:                     return "CL_IMAGE_FORMAT_MISMATCH";
        case CL_IMAGE_FORMAT_NOT_SUPPORTED:                return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
        case CL_BUILD_PROGRAM_FAILURE:                     return "CL_BUILD_PROGRAM_FAILURE";
        case CL_MAP_FAILURE:                               return "CL_MAP_FAILURE";
        case CL_MISALIGNED_SUB_BUFFER_OFFSET:              return "CL_MISALIGNED_SUB_BUFFER_OFFSET";
        case CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST: return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";
        case CL_INVALID_VALUE:                             return "CL_INVALID_VALUE";
        case CL_INVALID_DEVICE_TYPE:                       return "CL_INVALID_DEVICE_TYPE";
        case CL_INVALID_PLATFORM:                          return "CL_INVALID_PLATFORM";
        case CL_INVALID_DEVICE:                            return "CL_INVALID_DEVICE";
        case CL_INVALID_CONTEXT:                           return "CL_INVALID_CONTEXT";
        case CL_INVALID_QUEUE_PROPERTIES:                  return "CL_INVALID_QUEUE_PROPERTIES";
        case CL_INVALID_COMMAND_QUEUE:                     return "CL_INVALID_COMMAND_QUEUE";
        case CL_INVALID_HOST_PTR:                          return "CL_INVALID_HOST_PTR";
        case CL_INVALID_MEM_OBJECT:                        return "CL_INVALID_MEM_OBJECT";
        case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:           return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
        case CL_INVALID_IMAGE_SIZE:                        return "CL_INVALID_IMAGE_SIZE";
        case CL_INVALID_SAMPLER:                           return "CL_INVALID_SAMPLER";
        case CL_INVALID_BINARY:                            return "CL_INVALID_BINARY";
        case CL_INVALID_BUILD_OPTIONS:                     return "CL_INVALID_BUILD_OPTIONS";
        case CL_INVALID_PROGRAM:                           return "CL_INVALID_PROGRAM";
        case CL_INVALID_PROGRAM_EXECUTABLE:                return "CL_INVALID_PROGRAM_EXECUTABLE";
        case CL_INVALID_KERNEL_NAME:                       return "CL_INVALID_KERNEL_NAME";
        case CL_INVALID_KERNEL_DEFINITION:                 return "CL_INVALID_KERNEL_DEFINITION";
        case CL_INVALID_KERNEL:                            return "CL_INVALID_KERNEL";
        case CL_INVALID_ARG_INDEX:                         return "CL_INVALID_ARG_INDEX";
        case CL_INVALID_ARG_VALUE:                         return "CL_INVALID_ARG_VALUE";
        case CL_INVALID_ARG_SIZE:                          return "CL_INVALID_ARG_SIZE";
        case CL_INVALID_KERNEL_ARGS:                       return "CL_INVALID_KERNEL_ARGS";
        case CL_INVALID_WORK_DIMENSION:                    return "CL_INVALID_WORK_DIMENSION";
        case CL_INVALID_WORK_GROUP_SIZE:                   return "CL_INVALID_WORK_GROUP_SIZE";
        case CL_INVALID_WORK_ITEM_SIZE:                    return "CL_INVALID_WORK_ITEM_SIZE";
        case CL_INVALID_GLOBAL_OFFSET:                     return "CL_INVALID_GLOBAL_OFFSET";
        case CL_INVALID_EVENT_WAIT_LIST:                   return "CL_INVALID_EVENT_WAIT_LIST";
        case CL_INVALID_EVENT:                             return "CL_INVALID_EVENT";
        case CL_INVALID_OPERATION:                         return "CL_INVALID_OPERATION";
        case CL_INVALID_GL_OBJECT:                         return "CL_INVALID_GL_OBJECT";
        case CL_INVALID_BUFFER_SIZE:                       return "CL_INVALID_BUFFER_SIZE";
        case CL_INVALID_MIP_LEVEL:                         return "CL_INVALID_MIP_LEVEL";
        case CL_INVALID_GLOBAL_WORK_SIZE:                  return "CL_INVALID_GLOBAL_WORK_SIZE";
        case CL_INVALID_PROPERTY:                          return "CL_INVALID_PROPERTY";
        default:                                           return "Unknown OpenCL error code";
    }
  }
};