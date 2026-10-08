#define __global__ __attribute__((global))

struct dim3 {
  unsigned int x, y, z;
};

extern "C" int cudaConfigureCall(dim3, dim3, unsigned long = 0,
                                 void * = nullptr);
extern "C" int cudaSetupArgument(const void *, unsigned long, unsigned long);
extern "C" int cudaLaunch(const void *);

__global__ void kernel() {}

void launch_kernel() { kernel<<<dim3{1, 1, 1}, dim3{1, 1, 1}>>>(); }
