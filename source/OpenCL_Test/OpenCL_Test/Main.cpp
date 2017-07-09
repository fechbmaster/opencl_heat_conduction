#include <stdio.h>
#include <stdlib.h>
#include <CL\cl.h>

const char* programSource =
"__kernel \n"
"void vecadd(__global int* A, __global int* B, __global int* C) \n"
"{ \n"
"\n"
"int idx = get_global_id(0); \n"
"C[idx] = A[idx] + B[idx]; \n"
"} \n";

int main() {
	const int elements = 4096;
	size_t datasize = sizeof(int) * elements;

	int* A = (int*)malloc(datasize);
	int* B = (int*)malloc(datasize);
	int* C = (int*)malloc(datasize);

	for (int i = 0; i < elements; i++) {
		A[i] = i;
		B[i] = i;
		C[0] = 0;
	}

	cl_int status;
	cl_platform_id platform;
	status = clGetPlatformIDs(1, &platform, NULL);
	cl_device_id device;
	status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 1, &device, NULL);

	cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, &status);
	cl_command_queue cmdQueue = clCreateCommandQueue(context, device, NULL, &status);
	cl_mem bufA = clCreateBuffer(context, CL_MEM_READ_ONLY, datasize, NULL, &status);
	cl_mem bufB = clCreateBuffer(context, CL_MEM_READ_ONLY, datasize, NULL, &status);
	cl_mem bufC = clCreateBuffer(context, CL_MEM_WRITE_ONLY, datasize, NULL, &status);

	status = clEnqueueWriteBuffer(cmdQueue, bufA, CL_FALSE, 0, datasize, A, 0, NULL, NULL);
	status = clEnqueueWriteBuffer(cmdQueue, bufB, CL_FALSE, 0, datasize, B, 0, NULL, NULL);

	cl_program program = clCreateProgramWithSource(context, 1, (const char**)&programSource, NULL, &status);
	status = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
	cl_kernel kernel = clCreateKernel(program, "vecadd", &status);
	status = clSetKernelArg(kernel, 0, sizeof(cl_mem), &bufA);
	status = clSetKernelArg(kernel, 1, sizeof(cl_mem), &bufB);
	status = clSetKernelArg(kernel, 2, sizeof(cl_mem), &bufC);

	size_t indexSpaceSize[1], workGroupSize[1];
	indexSpaceSize[0] = elements;
	workGroupSize[0] = 1024;

	status = clEnqueueNDRangeKernel(cmdQueue, kernel, 1, NULL, indexSpaceSize, workGroupSize, 0, NULL, NULL);
	status = clEnqueueReadBuffer(cmdQueue, bufC, CL_TRUE, 0, datasize, C, 0, NULL, NULL);

	for  (int i = 0; i < elements; i++)	{
		printf("%d \n", C[i]);
	}

	clReleaseKernel(kernel);
	clReleaseProgram(program);
	clReleaseCommandQueue(cmdQueue);
	clReleaseMemObject(bufA);
	clReleaseMemObject(bufB);
	clReleaseMemObject(bufC);
	clReleaseContext(context);

	free(A);
	free(B);
	free(C);

	system("pause");

	return 0;
}