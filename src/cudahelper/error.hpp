//
// Configuration
//

// Include guard
#ifndef _TRACETRANSFORM_CUDAHELPER_ERROR_
#define _TRACETRANSFORM_CUDAHELPER_ERROR_

// Standard library
#include <stdexcept>

// CUDA
#include <cuda.h>
#include <cuda_runtime_api.h>


//
// Module definitions
//

namespace CUDAHelper {
// Exception wrapper class for Cuda error codes
class Error : public std::runtime_error {
  public:
    explicit Error(cudaError_t errorCode)
        : std::runtime_error(cudaGetErrorString(errorCode)), _code(errorCode) {}

  private:
    cudaError_t _code;
};
}

#endif
