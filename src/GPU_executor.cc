// FastRuleForge source code

#include "GPU_executor.hh"
#include <CL/cl.h>
#include <exception>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <cmath>
#include <map>

int GPU_executor::process_input(std::string filename, bool verbose){
  total_length = 0;
  PASSWORDS_COUNT = 0;
  lengths_vec.clear();
  pointers_vec.clear();
  passwords.clear();

  std::ifstream inFile(filename);
  if (!inFile.is_open()){
    std::cerr << "ERROR: opening file failed" << std::endl;
    exit(-1);
  }
  int skipped_lenght = 0;
  int skipped_chars = 0;

  std::string password;
  unsigned int running_offset = 0;
  while (std::getline(inFile, password)){
    unsigned int pass_length = static_cast<unsigned int>(password.size());
    if (pass_length > 29) {
      //std::cerr << "WARNING: maximal password length is 29" << std::endl;
      skipped_lenght++;
      continue;
    }

    // Validate that all characters are printable ASCII.
    bool valid = std::all_of(password.begin(), password.end(), [](char c) {
      return c >= 32 && c <= 126;
    });

    if (!valid) {
      //std::cerr << "WARNING: skipping non-printable password: " << password << std::endl;
      skipped_chars++;
      continue;
    }

    lengths_vec.push_back(pass_length);
    pointers_vec.push_back(running_offset);
    passwords.push_back(password);

    running_offset += pass_length;
    PASSWORDS_COUNT++;
  }
  inFile.close();
  
  total_length = running_offset;
  
  concatenated_string = new char[total_length + 1];

  unsigned int offset = 0;
  for (const auto &pwd : passwords) {
    size_t len = pwd.size();
    if (offset + len > total_length) {
      std::cerr << "Internal error" << std::endl;
      delete[] concatenated_string;
      exit(-1);
    }
    std::memcpy(concatenated_string + offset, pwd.data(), len);
    offset += static_cast<unsigned int>(len);
  }
  concatenated_string[total_length] = '\0';

  if(verbose){std::cout << "skipped " << skipped_lenght+skipped_chars << " passwords" << std::endl;}
  return 0;
}

int GPU_executor::setup(std::string kernel_main_function, bool verbose){
  ret = clGetPlatformIDs(10, platforms, &platforms_num);
  handle_error(ret, __LINE__);
  
  bool found = false;
  for(int i=0; i<platforms_num; i++){
    ret = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_GPU, 1, &device, &devices_num);
    if(ret == CL_SUCCESS){
      found = true;
      char name[256];
      clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(name), name, NULL);
      if(verbose){
        std::cout << "Using GPU device: " << name << std::endl;
      }
      break;
    }
  }
  if(!found){
    if(verbose){
      std::cout << "Warning: GPU not found" << std::endl;
    }
    for(int i=0; i<platforms_num; i++){
      ret = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_CPU, 1, &device, &devices_num);
      if(ret == CL_SUCCESS){
        found = true;
        char name[256];
        clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(name), name, NULL);
        if(1){
          std::cout << "Warning: using CPU device: |" << name << "|, this may affect performance" << std::endl;
        }
        
        break;
      }
    }
  }
  if(!found){
    std::cout << "NO DEVICE TO RUN OPENCL FOUND, SHUTTING DOWN" << std::endl;
    return -1;
  }
  handle_error(ret, __LINE__);
  
  context = clCreateContext(NULL, 1, &device, NULL, NULL, &ret);
  handle_error(ret, __LINE__);
  queue = clCreateCommandQueueWithProperties(context, device, 0, &ret);
  handle_error(ret, __LINE__);
  
  bufferStrings = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, total_length * sizeof(char), concatenated_string, &ret);
  handle_error(ret, __LINE__);
  bufferLengths = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, PASSWORDS_COUNT * sizeof(unsigned char), lengths_vec.data(), &ret);
  handle_error(ret, __LINE__);
  bufferPointers = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, PASSWORDS_COUNT * sizeof(int), pointers_vec.data(), &ret);
  handle_error(ret, __LINE__);
  bufferResult = clCreateBuffer(context, CL_MEM_READ_WRITE, PASSWORDS_COUNT * sizeof(int), NULL, &ret);
  handle_error(ret, __LINE__);

  const char* kernel_src = kernelSource.c_str();
  program = clCreateProgramWithSource(context, 1, &kernel_src, NULL, &ret);
  ret = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
  

  if(false){ //for debugging
    size_t log_size;
    clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
    char *log = (char *)malloc(log_size);
    clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
    printf("Build Log:\n%s\n", log);
    free(log);
  }
  handle_error(ret, __LINE__);
  
  const char* kernel_main_function_cstr = kernel_main_function.c_str();
  kernel = clCreateKernel(program, kernel_main_function_cstr, &ret);

  ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), &bufferStrings);
  handle_error(ret, __LINE__);
  ret = clSetKernelArg(kernel, 1, sizeof(int), &PASSWORDS_COUNT);
  handle_error(ret, __LINE__);
  ret = clSetKernelArg(kernel, 2, sizeof(cl_mem), &bufferLengths);
  handle_error(ret, __LINE__);
  ret = clSetKernelArg(kernel, 3, sizeof(cl_mem), &bufferPointers);
  handle_error(ret, __LINE__);
  ret = clSetKernelArg(kernel, 4, sizeof(cl_mem), &bufferResult);
  handle_error(ret, __LINE__);

  //trz to find optimal work group size
  clGetKernelWorkGroupInfo(
    kernel, device,
    CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE,
    sizeof(size_t), &preferred_multiple, NULL);

  return 0;
}

int* GPU_executor::HAC_calculate(unsigned char threshold, size_t local_work_size, size_t global_work_size){
  ret = clSetKernelArg(kernel, 5, sizeof(unsigned char), &threshold);
  handle_error(ret, __LINE__);

  int* result = new int[PASSWORDS_COUNT];
  for(int i = 0; i < PASSWORDS_COUNT; i++){
    result[i] = 0;
  }
  ret = clEnqueueWriteBuffer(queue, bufferResult, CL_TRUE, 0, PASSWORDS_COUNT * sizeof(int), result, 0, NULL, NULL);
  handle_error(ret, __LINE__);

  local_work_size = 512;
  global_work_size = ((PASSWORDS_COUNT + preferred_multiple - 1) / preferred_multiple) * preferred_multiple;
  ret = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_work_size, NULL, 0, NULL, NULL);
  handle_error(ret, __LINE__);
  clFinish(queue);
  
  ret = clEnqueueReadBuffer(queue, bufferResult, CL_TRUE, 0, PASSWORDS_COUNT * sizeof(int), result, 0, NULL, NULL);
  handle_error(ret, __LINE__);

  return result;
}

int* GPU_executor::calculate_distances_to(int index, unsigned char threshold, size_t global_work_size){
  ret = clSetKernelArg(kernel, 5, sizeof(int), &index);
  handle_error(ret, __LINE__);

  ret = clSetKernelArg(kernel, 6, sizeof(unsigned char), &threshold);
  handle_error(ret, __LINE__);

  global_work_size = ((PASSWORDS_COUNT + preferred_multiple - 1) / preferred_multiple) * preferred_multiple;
  ret = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_work_size, NULL, 0, NULL, NULL);
  handle_error(ret, __LINE__);

  int* distances_array = new int[PASSWORDS_COUNT];
  ret = clEnqueueReadBuffer(queue, bufferResult, CL_TRUE, 0, PASSWORDS_COUNT * sizeof(int), distances_array, 0, NULL, NULL);
  handle_error(ret, __LINE__);
  clFinish(queue);

  return distances_array;
}


void GPU_executor::AP_compute_matrix(float damping, int option){
  size_t local_work_size[2] = {32, 32};
  size_t global_work_size[2] = {
    ((PASSWORDS_COUNT + local_work_size[0] - 1) / local_work_size[0]) * local_work_size[0], 
    ((PASSWORDS_COUNT + local_work_size[1] - 1) / local_work_size[1]) * local_work_size[1]
  };

  ret = clSetKernelArg(kernel, 4, sizeof(cl_mem), &bufferSimilarity);
  handle_error(ret, __LINE__);
  ret = clSetKernelArg(kernel, 5, sizeof(cl_mem), &bufferResponsibility);
  handle_error(ret, __LINE__);
  ret = clSetKernelArg(kernel, 6, sizeof(cl_mem), &bufferAvailability);
  handle_error(ret, __LINE__);
  ret = clSetKernelArg(kernel, 7, sizeof(float), &damping);
  handle_error(ret, __LINE__);
  ret = clSetKernelArg(kernel, 8, sizeof(int), &option);

  ret = clEnqueueNDRangeKernel(queue, kernel, 2, NULL, global_work_size, local_work_size, 0, NULL, NULL);
  handle_error(ret, __LINE__);
}

int* GPU_executor::AP_calculate(int iter, float lambda) {
  int N = PASSWORDS_COUNT;
  float* S = new float[N*N];
  float* R = new float[N*N];
  float* A = new float[N*N];
  for(int i = 0; i < N*N; i++) {
    S[i] = 0.0f;
    R[i] = 0.0f;
    A[i] = 0.0f;
  }

  int option = 0;

  int size = N * (N - 1) / 2;
  std::vector<float> tmpS;

  bufferSimilarity = clCreateBuffer(context, CL_MEM_READ_WRITE, PASSWORDS_COUNT * PASSWORDS_COUNT * sizeof(float), NULL, &ret);
  handle_error(ret, __LINE__);

  bufferAvailability = clCreateBuffer(context, CL_MEM_READ_WRITE, PASSWORDS_COUNT * PASSWORDS_COUNT * sizeof(float), NULL, &ret);
  handle_error(ret, __LINE__);

  bufferResponsibility = clCreateBuffer(context, CL_MEM_READ_WRITE, PASSWORDS_COUNT * PASSWORDS_COUNT * sizeof(float), NULL, &ret);
  handle_error(ret, __LINE__);

  AP_compute_matrix(lambda, 0); //option 0 for computing similarity matrix

  ret = clEnqueueReadBuffer(queue, bufferSimilarity, CL_TRUE, 0, PASSWORDS_COUNT * PASSWORDS_COUNT * sizeof(float), S, 0, NULL, NULL);
  handle_error(ret, __LINE__);
  
  //compute similarity between data point i and j
  for (int i = 0; i < N - 1; i++) {
    for (int j = i + 1; j < N; j++) {
      tmpS.push_back(S[i*N + j]);
    }
  }
  std::sort(tmpS.begin(), tmpS.end());
  
  //set diagonal of simmilarity matrix to median
  float median;
  if(size % 2 == 0){
    float mid1 = tmpS[size / 2];
    float mid2 = tmpS[size / 2 - 1];
    median = (mid1 + mid2) / 2.0f;
  }
  else{
    median = tmpS[size / 2];
  }

  for (int i = 0; i < N; i++) {
    S[i * PASSWORDS_COUNT + i] = median;
  }


  //std::cout << "Simmilarity computed" << std::endl;

  ret = clEnqueueWriteBuffer(queue, bufferSimilarity, CL_TRUE, 0, PASSWORDS_COUNT * PASSWORDS_COUNT * sizeof(float), S, 0, NULL, NULL);
  handle_error(ret, __LINE__);
  
  // Run Affinity Propagation
  for (int m = 0; m < iter; m++) {
    AP_compute_matrix(lambda, 1);
    clFinish(queue);

    ret = clEnqueueReadBuffer(queue, bufferResponsibility, CL_TRUE, 0, PASSWORDS_COUNT * PASSWORDS_COUNT * sizeof(float), R, 0, NULL, NULL);
    handle_error(ret, __LINE__);
    clFinish(queue);

    ret = clEnqueueReadBuffer(queue, bufferAvailability, CL_TRUE, 0, PASSWORDS_COUNT * PASSWORDS_COUNT * sizeof(float), A, 0, NULL, NULL);
    handle_error(ret, __LINE__);
    clFinish(queue);
  }
  
  std::vector<int> exemplars;
  for (int i = 0; i < N; i++) {
    if (R[i*PASSWORDS_COUNT+i] + A[i*PASSWORDS_COUNT+i] > 0) {
      exemplars.push_back(i);
    }
  }
  
  int* clusterAssignment = new int[N];
  int exemplarToClusterID[PASSWORDS_COUNT];
  for (int i = 0; i < PASSWORDS_COUNT; i++) {
    exemplarToClusterID[i] = -1;
  }
  int nextClusterID = 0;

  for (int i = 0; i < N; i++) {
    float max = -1e100;
    int bestExemplar = -1;

    for (int j = 0; j < exemplars.size(); j++) {
      int ex = exemplars[j];
      float sim = S[i * PASSWORDS_COUNT + ex];

      if (sim > max) {
        max = sim;
        bestExemplar = ex;
      }
    }

    if (exemplarToClusterID[bestExemplar] == -1) {
      exemplarToClusterID[bestExemplar] = nextClusterID;
      nextClusterID++;
    }

    clusterAssignment[i] = exemplarToClusterID[bestExemplar];
  }

  delete[] S;
  delete[] R;
  delete[] A;
  clReleaseMemObject(bufferSimilarity);
  clReleaseMemObject(bufferResponsibility);
  clReleaseMemObject(bufferAvailability);
  return clusterAssignment;
}

int GPU_executor::clean(){
  clReleaseMemObject(bufferStrings);
  clReleaseMemObject(bufferLengths);
  clReleaseMemObject(bufferPointers);
  clReleaseMemObject(bufferResult);
  clReleaseKernel(kernel);
  clReleaseProgram(program);
  clReleaseCommandQueue(queue);
  clReleaseContext(context);

  return 0;
}

void GPU_executor::handle_error(cl_int ret, int callerLine){
  if(ret != CL_SUCCESS){
    std::cout << getCLErrorString(ret) << " at line " << callerLine << std::endl;
    exit(-1);
  }
}
