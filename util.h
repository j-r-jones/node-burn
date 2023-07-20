#pragma once

// CUDA helpers

#include <cstdlib>

#include <cuda.h>
#include <cuda_runtime.h>
#include <curand.h>
#include <cublas_v2.h>

// aliases for types used in timing host code
using clock_type    = std::chrono::high_resolution_clock;
using duration_type = std::chrono::duration<double>;
using timestamp_type = decltype(clock_type::now());

static timestamp_type timestamp() {
    return clock_type::now();
}

// return the time in seconds since timestamp t
static double duration(timestamp_type t) {
    return duration_type(clock_type::now()-t).count();
}

// return the time in seconds between two timestamps
static double duration(timestamp_type begin, timestamp_type end) {
    return duration_type(end-begin).count();
}

// helper for initializing cublas
// not threadsafe: if we want to burn more than one GPU this will need to be reworked.
static cublasHandle_t get_blas_handle() {
    static bool is_initialized = false;
    static cublasHandle_t cublas_handle;

    if(!is_initialized) {
        cublasCreate(&cublas_handle);
    }
    return cublas_handle;
}

void device_synchronize() {
    cudaDeviceSynchronize();
}

// error checking
static void check_status(cudaError_t status) {
    if(status != cudaSuccess) {
        std::cerr << "error: CUDA API call : "
                  << cudaGetErrorString(status) << std::endl;
        exit(-1);
    }
}

static void check_status(curandStatus_t status) {
    if(status != CURAND_STATUS_SUCCESS) {
        std::cerr << "error: CURAND" << std::endl;
        exit(-1);
    }
}

// allocate space on GPU for n instances of type T
template <typename T>
T* malloc_device(size_t n) {
    void* p;
    auto status = cudaMalloc(&p, n*sizeof(T));
    check_status(status);
    return (T*)p;
}

template <typename T>
T* malloc_host(size_t N, T value=T()) {
    T* ptr = (T*)(malloc(N*sizeof(T)));
    std::fill(ptr, ptr+N, value);

    return ptr;
}

template <typename T>
void copy_to_host(T* from, T* to, size_t n) {
    auto status = cudaMemcpy(to, from, n*sizeof(T), cudaMemcpyDeviceToHost);
    check_status(status);
}

static void gemm(double* a, double*b, double*c,
          int m, int n, int k,
          double alpha, double beta)
{
    cublasDgemm(
            get_blas_handle(),
            CUBLAS_OP_N, CUBLAS_OP_N,
            m, n, k,
            &alpha,
            a, m,
            b, k,
            &beta,
            c, m
    );
}

static void gpu_rand(double* x, std::size_t n) {
    curandGenerator_t gen;

    check_status(
            curandCreateGenerator(&gen, CURAND_RNG_PSEUDO_DEFAULT));
    check_status(
            curandSetPseudoRandomGeneratorSeed(gen, 1234ULL));
    check_status(
            curandGenerateNormalDouble(gen, x, n, 0., 1.));
    check_status(
            curandDestroyGenerator(gen));
}
