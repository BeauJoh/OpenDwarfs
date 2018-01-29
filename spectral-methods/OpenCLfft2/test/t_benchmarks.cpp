//
// OpenCL FFT benchmarks
// Copyright Eric Bainville Mar 2011.
// Modified by Beau Johnston Sep 2017.
// All rights reserved.
//

#ifdef WIN32
#define USE_FFTW 1
#endif

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <assert.h>
#if USE_CUDA
#include <cufft.h>
#include <cuda_runtime.h>
#endif
#if USE_MKL
#include <mkl_dfti.h>
#endif
#if USE_FFTW
#include <fftw3.h>
#endif
#include "CLFFT.h"
#include "TestFunctions.h"
#include "../../../include/common_args.h"
#include "../../../include/lsb.h"
#define BENCHMARK_IO 0

#define MIN_TIME_SEC 2

#if USE_MKL
// Run complex to complex X[2*N] to Y[2*N]. Return total time (s).
double runMKL(size_t n,const float * x,float * y,double maxBenchmarkTime)
{
  DFTI_DESCRIPTOR_HANDLE h;
  DftiCreateDescriptor(&h,DFTI_SINGLE,DFTI_COMPLEX,1,n);
  DftiSetValue(h,DFTI_PLACEMENT,DFTI_NOT_INPLACE);
  DftiCommitDescriptor(h);

  const int nops = 2;
  double t = getRealTime();
  for (int op = 0;op < nops;op++)
    {
      DftiComputeForward(h,(void *)x,y);
    }
  t = (getRealTime() - t)/(double)nops;

  DftiFreeDescriptor(&h);
  return t;
}
#endif

#if USE_FFTW
// Run complex to complex X[2*N] to Y[2*N]. Return total time (s).
double runFFTW(size_t n,const float * x,float * y,double maxBenchmarkTime)
{
  fftwf_plan p1 = fftwf_plan_dft_1d((int)n,(fftwf_complex *)x,(fftwf_complex *)y,
                                    FFTW_FORWARD,FFTW_ESTIMATE);
  double totalIT = 0;
  double t0 = getRealTime();
  double t1;
  for (int nit=1;nit<=1024;nit<<=1)
  {
    for (int it = 0;it < nit;it++)
      {
        fftwf_execute(p1);
      }
    totalIT += nit;
    t1 = getRealTime();
    if (t1 - t0 >= maxBenchmarkTime) break;
  }
  fftwf_destroy_plan(p1);
  return (t1 - t0)/totalIT;
}

bool benchmarkFFTW(size_t maxLog2N,double maxBenchmarkTime) // float only
{
  float * x = (float *) malloc((2 * sizeof(float)) << maxLog2N);
  float * y = (float *) malloc((2 * sizeof(float)) << maxLog2N);
  rand(2 << maxLog2N, x);
  for (size_t log2n = 8; log2n <= maxLog2N; log2n++)
  {
    size_t n = 1 << log2n;
    double t = runFFTW(n, x, y, maxBenchmarkTime);
    double flop = 5 * (double) log2n * (double) n;
    double perf = flop / t;
    printf("FFTW(float): N=2^%d=%d T=%.2fms  P=%.2f Gflop/s\n", (int) log2n, (int) n, t * 1.0e3, perf * 1.0e-9);
  }
  free(x);
  free(y);
  return true;
}

#endif

#if USE_CUDA
// Run complex to complex X[2*N] to Y[2*N].
double runCUFFT(size_t n,const float * x,float * y, double maxBenchmarkTime)
{
  cufftHandle plan;
  cufftComplex * inData = 0;
  cufftComplex * outData = 0;
  size_t dataSize = sizeof(cufftComplex) * n;
  cudaError_t status;
  cufftResult fftStatus;

  fftStatus = cufftPlan1d(&plan,(int)n,CUFFT_C2C,1); // 1 is BATCH size
  assert(fftStatus == CUFFT_SUCCESS);
  status = cudaMalloc((void **)(&inData),dataSize);
  assert(status == cudaSuccess);
  status = cudaMalloc((void **)(&outData),dataSize);
  assert(status == cudaSuccess);

  // Send X to device
  status = cudaMemcpy(inData,x,dataSize,cudaMemcpyHostToDevice);
  assert(status == cudaSuccess);

  double totalIT = 0;
  double t0 = getRealTime();
  double t1;
  for (int nit = 1; nit <= 1024; nit++)
  {
    for (int it = 0; it < nit; it++)
    {
      // Run the FFT
#if BENCHMARK_IO
      cudaMemcpy(inData,x,dataSize,cudaMemcpyHostToDevice);
#endif
      cufftExecC2C(plan, inData, outData, CUFFT_FORWARD);
#if BENCHMARK_IO
      cudaMemcpy(y,outData,dataSize,cudaMemcpyDeviceToHost);
#endif
    }
    cudaDeviceSynchronize();
    t1 = getRealTime();
    totalIT += nit;
    if (t1 - t0 >= maxBenchmarkTime) break;
  } // nit loop
  double t = (t1 - t0)/totalIT;

  // Get Y from device
  status = cudaMemcpy(y,outData,dataSize,cudaMemcpyDeviceToHost);
  assert(status == cudaSuccess);
  cudaDeviceSynchronize();

  cufftDestroy(plan);
  cudaFree(inData);
  cudaFree(outData);

  return t;
}

bool benchmarkCUFFT(size_t maxLog2N,double maxBenchmarkTime) // float only
{
  float * x = (float *) malloc((2 * sizeof(float)) << maxLog2N);
  float * y = (float *) malloc((2 * sizeof(float)) << maxLog2N);
  rand(2 << maxLog2N, x);
  for (size_t log2n = 8; log2n <= maxLog2N; log2n++)
  {
    size_t n = 1 << log2n;
    double t = runCUFFT(n, x, y, maxBenchmarkTime);
    double flop = 5 * (double) log2n * (double) n;
    double perf = flop / t;
    printf("CUFFT(float): N=2^%d=%d T=%.2fms  P=%.2f Gflop/s\n", (int) log2n, (int) n, t * 1.0e3, perf * 1.0e-9);
  }
  free(x);
  free(y);
  return true;
}

#endif // CUDA

clfft::Event simpleForward1D(clfft::Context * clfft,int device,size_t n,cl_mem in,cl_mem out,clfft::EventVector deps)
{
  cl_mem b[2];
  int current = 0;
  size_t p = (size_t)1;
  clfft::Event e;
  size_t bufferSize = n * 2 * clfft->getRealTypeSize();
  b[current] = in;
  b[1-current] = out;

  while (p<n)
  {
    size_t radix = (size_t)2;
    if ( (p<<4) <= n ) radix = 16;
    else if ( (p<<3) <= n ) radix = 8;
    else if ( (p<<2) <= n) radix = 4;
    else radix = 2;
    e = clfft->enqueueRadixRKernel(device,n,1,p,radix,clfft::FORWARD_DIRECTION,b[current],b[1-current],256,deps);
    if (!CLFFT_CHECK_EVENT(e)) return e;
    deps = clfft::EventVector(e);
    p *= radix;
    current = 1 - current;
  }
  if (current != 1)
  {
      e = clfft->enqueueCopy(device,b[current],b[1-current],0,0,bufferSize,deps);
  }
  return e;
}

int runCLFFT(clfft::Context * clfft,size_t n,void * x,void * y)
{
    int realType = clfft->getRealType();
    size_t realSize = (realType == clfft::FLOAT_REAL_TYPE)?sizeof(float):sizeof(double);
    size_t bufferSize = realSize * n * 2;
    cl_int status;
    cl_mem bIn = 0;
    cl_mem bOut = 0;
    bool ok = true;
    int deviceID = 0;
    clfft::Event e;

    LSB_Set_Rparam_string("region", "device_side_buffer_setup");
    LSB_Res();
    bIn = clCreateBuffer(clfft->getOpenCLContext(),CL_MEM_READ_WRITE,bufferSize,0,&status);
    if (!CLFFT_CHECK_STATUS(status)) { ok = false; goto END; }
    bOut = clCreateBuffer(clfft->getOpenCLContext(),CL_MEM_READ_WRITE,bufferSize,0,&status);
    if (!CLFFT_CHECK_STATUS(status)) { ok = false; goto END; }
    LSB_Rec(0);

    LSB_Set_Rparam_string("region","device_side_h2d_copy");
    LSB_Res();
    e = clfft->enqueueWrite(deviceID,bIn,true,0,bufferSize,x,clfft::EventVector()); // blocking
    if (!CLFFT_CHECK_EVENT(e)) { ok = false; goto END; }
    LSB_Rec(0);

    LSB_Set_Rparam_string("region", "fft_kernel");
    LSB_Res();
    e = simpleForward1D(clfft,deviceID,n,bIn,bOut,e);
    if (!CLFFT_CHECK_EVENT(e)) { ok = false; goto END; }
    status = clfft->enqueueBarrier(deviceID);
    if (!CLFFT_CHECK_STATUS(status)) { ok = false; goto END; }
    status = clfft->finish(deviceID);
    LSB_Rec(0);

    if (!CLFFT_CHECK_STATUS(status)) { ok = false; goto END; }

    LSB_Set_Rparam_string("region", "device_side_d2h_copy");
    LSB_Res();
    e = clfft->enqueueRead(deviceID,bOut,true,0,bufferSize,y,e); // blocking
    if (!CLFFT_CHECK_EVENT(e)) { ok = false; goto END; }
    LSB_Rec(0);

END:

    if (bIn != 0) clReleaseMemObject(bIn);
    if (bOut != 0) clReleaseMemObject(bOut);

    if (!ok) return -1; // Error
    else return 0;
}

double runCLFFT(clfft::Context * clfft,size_t n,void * x,void * y, double maxBenchmarkTime)
{
  int realType = clfft->getRealType();
  size_t realSize = (realType == clfft::FLOAT_REAL_TYPE)?sizeof(float):sizeof(double);
  size_t bufferSize = realSize * n * 2;
  cl_int status;
  cl_mem bIn = 0;
  cl_mem bOut = 0;
  bool ok = true;
  double t0,t1,t,totalIT;
  int deviceID = 0;
  t = -1;
  clfft::Event e;

  bIn = clCreateBuffer(clfft->getOpenCLContext(),CL_MEM_READ_WRITE,bufferSize,0,&status);
  if (!CLFFT_CHECK_STATUS(status)) { ok = false; goto END; }
  bOut = clCreateBuffer(clfft->getOpenCLContext(),CL_MEM_READ_WRITE,bufferSize,0,&status);
  if (!CLFFT_CHECK_STATUS(status)) { ok = false; goto END; }

  e = clfft->enqueueWrite(deviceID,bIn,true,0,bufferSize,x,clfft::EventVector()); // blocking
  if (!CLFFT_CHECK_EVENT(e)) { ok = false; goto END; }

  t0 = getRealTime();
  t1 = 0;
  totalIT = 0;
  for (int nit = 1; nit <= 1024; nit <<= 1)
  {
    for (int it = 0; it < nit; it++)
    {
      e = simpleForward1D(clfft,deviceID,n,bIn,bOut,e);
      if (!CLFFT_CHECK_EVENT(e)) { ok = false; goto END; }
      status = clfft->enqueueBarrier(deviceID);
      if (!CLFFT_CHECK_STATUS(status)) { ok = false; goto END; }
    }
    status = clfft->finish(deviceID);
    if (!CLFFT_CHECK_STATUS(status)) { ok = false; goto END; }

    totalIT += nit;
    t1 = getRealTime();
    if (t1 - t0 >= maxBenchmarkTime) break; // Run 3s max test
  } // nit loop
  t = (t1 - t0) / totalIT; // s per FFT

  e = clfft->enqueueRead(deviceID,bOut,true,0,bufferSize,y,e); // blocking
  if (!CLFFT_CHECK_EVENT(e)) { ok = false; goto END; }

END:

  if (bIn != 0) clReleaseMemObject(bIn);
  if (bOut != 0) clReleaseMemObject(bOut);

  if (!ok) return -1; // Error
  return t;
}

bool benchmarkCLFFT(size_t maxLog2N,clfft::RealType realType,double maxBenchmarkTime)
{
  std::string msg;
  clfft::Context * clfft = 0;
  //cl_context context = 0;
  bool ok = true;
  size_t realSize;
  size_t maxBufferSize;
  size_t maxN;
  void * x = 0;
  void * y = 0;

  ocd_initCL();
  //context = createGPUContext();
  if (context == 0)
  {
    fprintf(stderr,"Could not create OpenCL context\n");
    ok = false; goto END;
  }
  clfft = clfft::Context::create(context,realType,msg);
  if (clfft == 0)
  {
    fprintf(stderr,"Creation failed:\n%s\n",msg.c_str());
    ok = false; goto END;
  }
  //clReleaseContext(context); // clfft still references the context

  realSize = (realType == clfft::FLOAT_REAL_TYPE)?sizeof(float):sizeof(double);
  maxN = (size_t)1 << maxLog2N;
  maxBufferSize = realSize * (size_t)2 * maxN;
  x = malloc(maxBufferSize);
  y = malloc(maxBufferSize);
  if (realType == clfft::FLOAT_REAL_TYPE) rand<float>((size_t)2*maxN,(float *)x);
  else rand<double>(2*maxN,(double *)x);

  for (size_t log2n=8;log2n <= maxLog2N;log2n++)
  {
    rand<float>((size_t)2*maxN,(float *)x);
    size_t n = (size_t)1 << log2n;
    double t = runCLFFT(clfft,n,x,y,maxBenchmarkTime);
    double flop = 5 * (double)log2n * (double)n;
    double perf = flop / t; // flop/s per FFT
    printf("CLFFT(%s): N=2^%d=%d T=%.2fms  P=%.2f Gflop/s\n",(realType==clfft::FLOAT_REAL_TYPE)?"float":"double",
                (int)log2n,(int)n,t*1.0e3,perf*1.0e-9);
  } // log2n loop

END:
  if (x != 0) free(x);
  if (y != 0) free(y);
  delete clfft;
  return ok;
}

bool runBenchmarks(double maxBenchmarkTime)
{
  bool ok = true;

  printf("Running benchmarks...\n");

  // BENCHMARKS
  const size_t maxLog2N = 24;

  ok &= benchmarkCLFFT(maxLog2N,clfft::FLOAT_REAL_TYPE,maxBenchmarkTime);
  ok &= benchmarkCLFFT(maxLog2N,clfft::DOUBLE_REAL_TYPE,maxBenchmarkTime);
#if USE_CUDA
  ok &= benchmarkCUFFT(maxLog2N,maxBenchmarkTime); // float only
#endif
#if USE_FFTW
  ok &= benchmarkFFTW(maxLog2N,maxBenchmarkTime);
#endif

  return ok;
}

void printHelp(){
    printf("openclfft performs a one dimensional fast fourier transform over a given signal length N.\n");
    printf("Arguments are supported in the following form:\n");
    printf("\t./openclfft -p [platform id] -d [device id] -t [type id] -- [N]\n");
    printf("\twhere: [platform id] is the integer id for the OpenCL platform to use,\n");
    printf("\t       [device id] is the integer id for the OpenCL device,\n");
    printf("\t       [type id] is the integer id for the OpenCL platform to use, by default this determines type automatically according to the selected device characteristics,\n");
    printf("\t       [N] is an integer (and must be a power of 2) an indicates the length of signal on which to perform the FFT.\n");
    printf("Additionally -d or --default, runs the default (original Gflops benchmark)\n");
    printf("sample usage:\n");
    printf("\t./openclfft -p 0 -d 0 -t 0 -- 128\n");
}

bool isPowerOfTwo(unsigned int x)
{
    return (x != 0) && ((x & (x - 1)) == 0);
}

int benchmark(int N){
    ocd_initCL();
    LSB_Init("openclfft", 0);
    LSB_Set_Rparam_int("repeats_to_two_seconds", 0);
    LSB_Set_Rparam_int("signal_length",N);
    std::string msg;
    LSB_Set_Rparam_string("region", "host_side_setup");
    LSB_Res();
    clfft::Context* clfft = clfft::Context::create(context,clfft::FLOAT_REAL_TYPE,msg);
    //generate the data of length N
    void* x = malloc(sizeof(float)*N*2);//    new float[N*2];
    void* y = malloc(sizeof(float)*N*2);//    new float[N*2];
    //ones<float>((size_t)N,(float*)x);
    sine<float>((size_t)N,(float*)x);
    zeros<float>((size_t)N,(float*)y);
    printf("Working kernel memory: %fKiB\n",(sizeof(float)*N*2*2)/1024.0);
    LSB_Rec(0);

    int return_code = 0;
    int lsb_timing_repeats = 0;
    struct timeval startTime, currentTime, elapsedTime;
    gettimeofday(&startTime, NULL);
    do {
        LSB_Set_Rparam_int("repeats_to_two_seconds", lsb_timing_repeats);

        return_code &= runCLFFT(clfft,N,(void*)x,(void*)y);

        lsb_timing_repeats++;
        gettimeofday(&currentTime, NULL);
        timersub(&currentTime, &startTime, &elapsedTime);
    } while (elapsedTime.tv_sec < MIN_TIME_SEC);

    //dumpRealArray<float>((size_t)N,(float*)y);
    free(x);//delete x;
    free(y);//delete y;/
    LSB_Finalize();
    return(return_code);
}

int main(int argc, char**argv)
{
    bool ok = true;
    srand(0);
    ocd_init(&argc, &argv, NULL);
    if (argc == 2){
        if (strcmp(argv[1],"-h") == 0 || strcmp(argv[1],"--help") == 0){
            printHelp();
            return(EXIT_SUCCESS);
        }
        if (strcmp(argv[1],"-d") == 0 || strcmp(argv[1],"--default") == 0){
            ok &= runBenchmarks(0.5);
        }
        int signal_length = atoi(argv[1]);
        if(signal_length == 0){
            printf("Please enter a valid signal length of N elements\n");
            printHelp();
            return(EXIT_FAILURE);
        }
        if(!isPowerOfTwo(signal_length)){
            printf("N must be a power of 2, but instead is %i\n",signal_length);
            printHelp();
            return(EXIT_FAILURE);
        }
        return(benchmark(signal_length));
    }else{
        printHelp();
        return(EXIT_SUCCESS);
    }
    printf("%s\n",(ok)?"OK":"FAILED!");
#ifdef WIN32
    printf("Press a key.\n");
    getchar();
#endif
}
