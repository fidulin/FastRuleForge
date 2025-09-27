// FastRuleForge source code

#include "GPU_executor.hh"

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
    _mDevice = MTL::CreateSystemDefaultDevice();
    if(!_mDevice){
        std::cerr << "Failed to create Metal device." << std::endl;
        return -1;
    }

    if(verbose){
        std::cout << "Using GPU device: " << _mDevice->name()->utf8String() << std::endl;
    }

    _mCommandQueue = _mDevice->newCommandQueue();
    if(!_mCommandQueue){
        std::cerr << "Failed to create command queue." << std::endl;
        return -1;
    }

    NS::Error* pError = nullptr;
    _mLibrary = _mDevice->newLibrary(NS::String::string(kernelSource.c_str(), NS::UTF8StringEncoding), nullptr, &pError);
    handle_error(pError, __LINE__);

    _mFunction = _mLibrary->newFunction(NS::String::string(kernel_main_function.c_str(), NS::UTF8StringEncoding));
    if(!_mFunction){
        std::cerr << "Failed to create function state." << std::endl;
        return -1;
    }

    _mPipelineState = _mDevice->newComputePipelineState(_mFunction, &pError);
    handle_error(pError, __LINE__);

    _mBufferStrings = _mDevice->newBuffer(concatenated_string, total_length * sizeof(char), MTL::ResourceStorageModeShared);
    _mBufferLengths = _mDevice->newBuffer(lengths_vec.data(), PASSWORDS_COUNT * sizeof(unsigned char), MTL::ResourceStorageModeShared);
    _mBufferPointers = _mDevice->newBuffer(pointers_vec.data(), PASSWORDS_COUNT * sizeof(int), MTL::ResourceStorageModeShared);
    _mBufferResult = _mDevice->newBuffer(PASSWORDS_COUNT * sizeof(int), MTL::ResourceStorageModeShared);

    preferred_multiple = _mPipelineState->threadExecutionWidth();

    return 0;
}

int* GPU_executor::HAC_calculate(unsigned char threshold, size_t local_work_size, size_t global_work_size){
    int* result = new int[PASSWORDS_COUNT];
    memset(result, 0, PASSWORDS_COUNT * sizeof(int));
    memcpy(_mBufferResult->contents(), result, PASSWORDS_COUNT * sizeof(int));

    MTL::CommandBuffer* pCommandBuffer = _mCommandQueue->commandBuffer();
    MTL::ComputeCommandEncoder* pComputeEncoder = pCommandBuffer->computeCommandEncoder();

    pComputeEncoder->setComputePipelineState(_mPipelineState);
    pComputeEncoder->setBuffer(_mBufferStrings, 0, 0);
    pComputeEncoder->setBytes(&PASSWORDS_COUNT, sizeof(int), 1);
    pComputeEncoder->setBuffer(_mBufferLengths, 0, 2);
    pComputeEncoder->setBuffer(_mBufferPointers, 0, 3);
    pComputeEncoder->setBuffer(_mBufferResult, 0, 4);
    pComputeEncoder->setBytes(&threshold, sizeof(unsigned char), 5);

    MTL::Size gridSize = MTL::Size::Make(PASSWORDS_COUNT, 1, 1);
    NS::UInteger threadGroupSize = _mPipelineState->maxTotalThreadsPerThreadgroup();
    if (threadGroupSize > PASSWORDS_COUNT) {
        threadGroupSize = PASSWORDS_COUNT;
    }
    MTL::Size threadgroupSize = MTL::Size::Make(threadGroupSize, 1, 1);

    pComputeEncoder->dispatchThreads(gridSize, threadgroupSize);
    pComputeEncoder->endEncoding();
    pCommandBuffer->commit();
    pCommandBuffer->waitUntilCompleted();

    memcpy(result, _mBufferResult->contents(), PASSWORDS_COUNT * sizeof(int));

    return result;
}

int* GPU_executor::calculate_distances_to(int index, unsigned char threshold, size_t global_work_size){
    MTL::CommandBuffer* pCommandBuffer = _mCommandQueue->commandBuffer();
    MTL::ComputeCommandEncoder* pComputeEncoder = pCommandBuffer->computeCommandEncoder();

    pComputeEncoder->setComputePipelineState(_mPipelineState);
    pComputeEncoder->setBuffer(_mBufferStrings, 0, 0);
    pComputeEncoder->setBytes(&PASSWORDS_COUNT, sizeof(int), 1);
    pComputeEncoder->setBuffer(_mBufferLengths, 0, 2);
    pComputeEncoder->setBuffer(_mBufferPointers, 0, 3);
    pComputeEncoder->setBuffer(_mBufferResult, 0, 4);
    pComputeEncoder->setBytes(&index, sizeof(int), 5);
    pComputeEncoder->setBytes(&threshold, sizeof(unsigned char), 6);

    MTL::Size gridSize = MTL::Size::Make(PASSWORDS_COUNT, 1, 1);
    NS::UInteger threadGroupSize = _mPipelineState->maxTotalThreadsPerThreadgroup();
    if (threadGroupSize > PASSWORDS_COUNT) {
        threadGroupSize = PASSWORDS_COUNT;
    }
    MTL::Size threadgroupSize = MTL::Size::Make(threadGroupSize, 1, 1);

    pComputeEncoder->dispatchThreads(gridSize, threadgroupSize);
    pComputeEncoder->endEncoding();
    pCommandBuffer->commit();
    pCommandBuffer->waitUntilCompleted();

    int* distances_array = new int[PASSWORDS_COUNT];
    memcpy(distances_array, _mBufferResult->contents(), PASSWORDS_COUNT * sizeof(int));

    return distances_array;
}


void GPU_executor::AP_compute_matrix(float damping, int option){
    MTL::CommandBuffer* pCommandBuffer = _mCommandQueue->commandBuffer();
    MTL::ComputeCommandEncoder* pComputeEncoder = pCommandBuffer->computeCommandEncoder();

    pComputeEncoder->setComputePipelineState(_mPipelineState);
    pComputeEncoder->setBuffer(_mBufferStrings, 0, 0);
    pComputeEncoder->setBytes(&PASSWORDS_COUNT, sizeof(int), 1);
    pComputeEncoder->setBuffer(_mBufferLengths, 0, 2);
    pComputeEncoder->setBuffer(_mBufferPointers, 0, 3);
    pComputeEncoder->setBuffer(_mBufferSimilarity, 0, 4);
    pComputeEncoder->setBuffer(_mBufferResponsibility, 0, 5);
    pComputeEncoder->setBuffer(_mBufferAvailability, 0, 6);
    pComputeEncoder->setBytes(&damping, sizeof(float), 7);
    pComputeEncoder->setBytes(&option, sizeof(int), 8);

    MTL::Size gridSize = MTL::Size::Make(PASSWORDS_COUNT, PASSWORDS_COUNT, 1);
    NS::UInteger w = _mPipelineState->threadExecutionWidth();
    NS::UInteger h = _mPipelineState->maxTotalThreadsPerThreadgroup() / w;
    MTL::Size threadgroupSize = MTL::Size::Make(w, h, 1);

    pComputeEncoder->dispatchThreads(gridSize, threadgroupSize);
    pComputeEncoder->endEncoding();
    pCommandBuffer->commit();
    pCommandBuffer->waitUntilCompleted();
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

  _mBufferSimilarity = _mDevice->newBuffer(N * N * sizeof(float), MTL::ResourceStorageModeShared);
  _mBufferResponsibility = _mDevice->newBuffer(N * N * sizeof(float), MTL::ResourceStorageModeShared);
  _mBufferAvailability = _mDevice->newBuffer(N * N * sizeof(float), MTL::ResourceStorageModeShared);

  AP_compute_matrix(lambda, 0); //option 0 for computing similarity matrix

  memcpy(S, _mBufferSimilarity->contents(), N * N * sizeof(float));
  
  //compute similarity between data point i and j
  std::vector<float> tmpS;
  for (int i = 0; i < N - 1; i++) {
    for (int j = i + 1; j < N; j++) {
      tmpS.push_back(S[i*N + j]);
    }
  }
  std::sort(tmpS.begin(), tmpS.end());
  
  //set diagonal of simmilarity matrix to median
  float median;
  int size = tmpS.size();
  if(size > 0) {
    if(size % 2 == 0){
      float mid1 = tmpS[size / 2];
      float mid2 = tmpS[size / 2 - 1];
      median = (mid1 + mid2) / 2.0f;
    }
    else{
      median = tmpS[size / 2];
    }
  } else {
    median = 0;
  }


  for (int i = 0; i < N; i++) {
    S[i * PASSWORDS_COUNT + i] = median;
  }

  memcpy(_mBufferSimilarity->contents(), S, N * N * sizeof(float));
  
  // Run Affinity Propagation
  for (int m = 0; m < iter; m++) {
    AP_compute_matrix(lambda, 1);
    memcpy(R, _mBufferResponsibility->contents(), N * N * sizeof(float));
    memcpy(A, _mBufferAvailability->contents(), N * N * sizeof(float));
  }
  
  std::vector<int> exemplars;
  for (int i = 0; i < N; i++) {
    if (R[i*PASSWORDS_COUNT+i] + A[i*PASSWORDS_COUNT+i] > 0) {
      exemplars.push_back(i);
    }
  }
  
  int* clusterAssignment = new int[N];
  std::map<int, int> exemplarToClusterID;
  int nextClusterID = 0;

  for (int i = 0; i < N; i++) {
    float max_val = -std::numeric_limits<float>::infinity();
    int bestExemplar = -1;

    for (int ex : exemplars) {
      float sim = S[i * PASSWORDS_COUNT + ex];
      if (sim > max_val) {
        max_val = sim;
        bestExemplar = ex;
      }
    }
    
    if (bestExemplar != -1) {
        if (exemplarToClusterID.find(bestExemplar) == exemplarToClusterID.end()) {
            exemplarToClusterID[bestExemplar] = nextClusterID++;
        }
        clusterAssignment[i] = exemplarToClusterID[bestExemplar];
    } else {
        clusterAssignment[i] = -1; // No suitable exemplar found
    }
  }

  delete[] S;
  delete[] R;
  delete[] A;
  _mBufferSimilarity->release();
  _mBufferResponsibility->release();
  _mBufferAvailability->release();
  return clusterAssignment;
}

int GPU_executor::clean(){
    _mBufferStrings->release();
    _mBufferLengths->release();
    _mBufferPointers->release();
    _mBufferResult->release();

    _mPipelineState->release();
    _mFunction->release();
    _mLibrary->release();
    _mCommandQueue->release();
    _mDevice->release();

    return 0;
}

void GPU_executor::handle_error(NS::Error* error, int callerLine){
  if(error){
    std::cerr << "Metal Error: " << error->localizedDescription()->utf8String() << " at line " << callerLine << std::endl;
    exit(-1);
  }
}