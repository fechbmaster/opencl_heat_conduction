#include <stdio.h>
#include <stdlib.h>
#include <CL\cl.h>

char* readFile(const char* filename) {
	FILE *fp;
	char *fileData;
	long fileSize;

	// open
	fp = fopen(filename, "rb");
	if (!fp) {
		printf("Could not open file \n");
		//exit(-1);
	}

	// file size

	if (fseek(fp, 0, SEEK_END)) {
		printf("Error reading file 1 \n");
		//exit(-1);
	}

	fileSize = ftell(fp);

	if (fileSize < 0) {
		printf("Error reading file 2 \n");
		//exit(-1);
	}

	if (fseek(fp, 0, SEEK_SET)) {
		printf("Error reading file 3 \n");
		//exit(-1);
	}

	// read

	fileData = (char*)malloc(fileSize + 1);
	if (!fileData) {
		//exit(-1);
	}

	fread(fileData, fileSize, 1, fp);

	// terminate string

	fileData[fileSize] = '\0';

	// close file
	if (fclose(fp)) {
		printf("Error closing file \n");
		exit(-1);
	}

	//for (int i = 0; i < fileSize; i++) {
	//	printf("%c", fileData[i]);
	//}

	return fileData;
}

void printOpenCLInfos() {
	int i, j;
	char* value;
	size_t valueSize;
	cl_uint platformCount;
	cl_platform_id* platforms;
	cl_uint deviceCount;
	cl_device_id* devices;
	cl_uint maxComputeUnits;

	// get all platforms
	clGetPlatformIDs(0, NULL, &platformCount);
	platforms = (cl_platform_id*)malloc(sizeof(cl_platform_id) * platformCount);
	clGetPlatformIDs(platformCount, platforms, NULL);

	for (i = 0; i < platformCount; i++) {

		// get all devices
		clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, 0, NULL, &deviceCount);
		devices = (cl_device_id*)malloc(sizeof(cl_device_id) * deviceCount);
		clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, deviceCount, devices, NULL);

		// for each device print critical attributes
		for (j = 0; j < deviceCount; j++) {

			// print device name
			clGetDeviceInfo(devices[j], CL_DEVICE_NAME, 0, NULL, &valueSize);
			value = (char*)malloc(valueSize);
			clGetDeviceInfo(devices[j], CL_DEVICE_NAME, valueSize, value, NULL);
			printf("%d. Device: %s\n", j + 1, value);
			free(value);

			// print hardware device version
			clGetDeviceInfo(devices[j], CL_DEVICE_VERSION, 0, NULL, &valueSize);
			value = (char*)malloc(valueSize);
			clGetDeviceInfo(devices[j], CL_DEVICE_VERSION, valueSize, value, NULL);
			printf(" %d.%d Hardware version: %s\n", j + 1, 1, value);
			free(value);

			// print software driver version
			clGetDeviceInfo(devices[j], CL_DRIVER_VERSION, 0, NULL, &valueSize);
			value = (char*)malloc(valueSize);
			clGetDeviceInfo(devices[j], CL_DRIVER_VERSION, valueSize, value, NULL);
			printf(" %d.%d Software version: %s\n", j + 1, 2, value);
			free(value);

			// print c version supported by compiler for device
			clGetDeviceInfo(devices[j], CL_DEVICE_OPENCL_C_VERSION, 0, NULL, &valueSize);
			value = (char*)malloc(valueSize);
			clGetDeviceInfo(devices[j], CL_DEVICE_OPENCL_C_VERSION, valueSize, value, NULL);
			printf(" %d.%d OpenCL C version: %s\n", j + 1, 3, value);
			free(value);

			// print parallel compute units
			clGetDeviceInfo(devices[j], CL_DEVICE_MAX_COMPUTE_UNITS,
				sizeof(maxComputeUnits), &maxComputeUnits, NULL);
			printf(" %d.%d Parallel compute units: %d\n", j + 1, 4, maxComputeUnits);

		}

		free(devices);

	}

	free(platforms);
}

void printOpenCLInfos2() {
	int i, j;
	char* info;
	size_t infoSize;
	cl_uint platformCount;
	cl_platform_id *platforms;
	const char* attributeNames[5] = { "Name", "Vendor",
		"Version", "Profile", "Extensions" };
	const cl_platform_info attributeTypes[5] = { CL_PLATFORM_NAME, CL_PLATFORM_VENDOR,
		CL_PLATFORM_VERSION, CL_PLATFORM_PROFILE, CL_PLATFORM_EXTENSIONS };
	const int attributeCount = sizeof(attributeNames) / sizeof(char*);

	// get platform count
	clGetPlatformIDs(5, NULL, &platformCount);

	// get all platforms
	platforms = (cl_platform_id*)malloc(sizeof(cl_platform_id) * platformCount);
	clGetPlatformIDs(platformCount, platforms, NULL);

	// for each platform print all attributes
	for (i = 0; i < platformCount; i++) {

		printf("\n %d. Platform \n", i + 1);

		for (j = 0; j < attributeCount; j++) {

			// get platform attribute value size
			clGetPlatformInfo(platforms[i], attributeTypes[j], 0, NULL, &infoSize);
			info = (char*)malloc(infoSize);

			// get platform attribute value
			clGetPlatformInfo(platforms[i], attributeTypes[j], infoSize, info, NULL);

			printf("  %d.%d %-11s: %s\n", i + 1, j + 1, attributeNames[j], info);
			free(info);

		}

		printf("\n");

	}

	free(platforms);
}

int main() {
	const bool debug = false;
	const int width = 4098;
	const int height = 2050;
	const float deltaX = 0.5;
	const float deltaT = 0.01f;
	const float alpha = 1.0f;
	const int steps = 10000;
	size_t datasize = sizeof(float) * width * height;

	const float heatParams[3] = { deltaX, deltaT, alpha };
	const int heatDimensions[2] = { width, height };
	float* heatPrevious = (float*)malloc(datasize);
	float* heatValues = (float*)malloc(datasize);
	float* finalResult = (float*)malloc(datasize);
	if (debug) {
		printOpenCLInfos();
		printOpenCLInfos2();
	}

	for (int i = 0; i < width * height; i++) {
		if (i % width == 0)
			heatPrevious[i] = heatValues[i] = finalResult[i] = 250;
		else
			heatPrevious[i] = heatValues[i] = finalResult[i] = 0;
	}

	// 1. Discovering platform and devices

	cl_int status;
	cl_uint numPlatforms = 0;
	status = clGetPlatformIDs(0, NULL, &numPlatforms);
	if (debug)
		printf("%zd: ", status);

	cl_platform_id *platforms = NULL;
	platforms = (cl_platform_id*)malloc(numPlatforms * sizeof(cl_platform_id));

	status = clGetPlatformIDs(numPlatforms, platforms, NULL);
	if (debug)
		printf("%zd: ", status);

	cl_uint numDevices = 0;
	status = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_ALL, 0, NULL, &numDevices);
	if (debug)
		printf("%zd: ", status);

	cl_device_id *devices;
	devices = (cl_device_id*)malloc(numDevices * sizeof(cl_device_id));

	status = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_ALL, numDevices, devices, NULL);
	if (debug)
		printf("%zd: ", status);

	// 2. Creating a context

	cl_context context = clCreateContext(NULL, numDevices, devices, NULL, NULL, &status);
	if (debug)
		printf("%zd: ", status);

	// 3. Creating a command-queue for the first device

	cl_command_queue cmdQueue = clCreateCommandQueue(context, devices[0], CL_QUEUE_PROFILING_ENABLE, &status);
	if (debug)
		printf("%zd: ", status);

	// 4. Creating buffers to hold data

	cl_mem bufPrevious = clCreateBuffer(context, CL_MEM_READ_WRITE, datasize, NULL, &status);
	if (debug)
		printf("%zd: ", status);
	cl_mem bufValues = clCreateBuffer(context, CL_MEM_READ_WRITE, datasize, NULL, &status);
	if (debug)
		printf("%zd: ", status);
	cl_mem bufResult = clCreateBuffer(context, CL_MEM_WRITE_ONLY, datasize, NULL, &status);
	if (debug)
		printf("%zd: ", status);
	cl_mem bufParams = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(heatParams), NULL, &status);
	if (debug)
		printf("%zd: ", status);
	cl_mem bufDimensions = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(heatDimensions), NULL, &status);
	if (debug)
		printf("%zd: ", status);

	// 5. Copying the input data onto the device

	status = clEnqueueWriteBuffer(cmdQueue, bufPrevious, CL_TRUE, 0, datasize, heatPrevious, 0, NULL, NULL);
	if (debug)
		printf("%zd: ", status);
	status = clEnqueueWriteBuffer(cmdQueue, bufValues, CL_TRUE, 0, datasize, heatValues, 0, NULL, NULL);
	if (debug)
		printf("%zd: ", status);
	status = clEnqueueWriteBuffer(cmdQueue, bufParams, CL_TRUE, 0, sizeof(heatParams), heatParams, 0, NULL, NULL);
	if (debug)
		printf("%zd: ", status);
	status = clEnqueueWriteBuffer(cmdQueue, bufDimensions, CL_TRUE, 0, sizeof(heatDimensions), heatDimensions, 0, NULL, NULL);
	if (debug)
		printf("%zd: ", status);

	// 6. Creating the kernel and creating + building the prorgam

	char* programSource = readFile("HeatConductionKernel2.txt");

	/*const char* programSource =
	"__kernel \n"
	"void calcHeatConduction(__global float* A, __global float* B, __global float* C) \n"
	"{ \n"
	"\n"
	"int idx = get_global_id(0); \n"
	"C[idx] = A[idx] + B[idx]; \n"
	"} \n";*/

	cl_program program = clCreateProgramWithSource(context, 1, (const char**)&programSource, NULL, &status);
	if (debug)
		printf("%zd: ", status);
	status = clBuildProgram(program, numDevices, devices, NULL, NULL, NULL);
	if (debug)
		printf("%zd: ", status);

	// 7. Extracting the kernel from the program

	cl_kernel kernel = clCreateKernel(program, "calcHeatConduction", &status);
	if (debug)
		printf("%zd: ", status);

	// 8. Executing the kernel

	//status = clSetKernelArg(kernel, 0, sizeof(cl_mem), &bufPrevious);
	//printf("%zd: ", status);
	//status = clSetKernelArg(kernel, 1, sizeof(cl_mem), &bufValues);
	//printf("%zd: ", status);
	//status = clSetKernelArg(kernel, 2, sizeof(cl_mem), &bufResult);
	//printf("%zd: ", status);

	//size_t indexSpaceSize[2], workGroupSize[2];
	//indexSpaceSize[0] = width - 2;
	//indexSpaceSize[1] = height - 2;
	//workGroupSize[0] = width - 2;
	//workGroupSize[1] = height - 2;

	//status = clEnqueueNDRangeKernel(cmdQueue, kernel, 2, NULL, indexSpaceSize, workGroupSize, 0, NULL, NULL);
	//printf("%zd: ", status);

	size_t indexSpaceSize[2], workGroupSize[2];
	indexSpaceSize[0] = width - 2;
	indexSpaceSize[1] = height - 2;
	workGroupSize[0] = 64;
	workGroupSize[1] = 1;

	cl_event event;
	cl_ulong startTime, endTime;
	double totalTime = 0.0;
	double time;

	for (int step = 1; step <= steps; step++) {
		if (step % 1000 == 0)
			printf("Calculated steps %d ...\n", step);
		if (step % 2 == 0) {
			status = clSetKernelArg(kernel, 2, sizeof(cl_mem), &bufPrevious);
			if (debug)
				printf("%zd: ", status);
			status = clSetKernelArg(kernel, 3, sizeof(cl_mem), &bufValues);
			if (debug)
				printf("%zd: ", status);
		}
		else {
			status = clSetKernelArg(kernel, 2, sizeof(cl_mem), &bufValues);
			if (debug)
				printf("%zd: ", status);
			status = clSetKernelArg(kernel, 3, sizeof(cl_mem), &bufPrevious);
			if (debug)
				printf("%zd: ", status);
		}
		status = clSetKernelArg(kernel, 0, sizeof(cl_mem), &bufParams);
		if (debug)
			printf("%zd: ", status);
		status = clSetKernelArg(kernel, 1, sizeof(cl_mem), &bufDimensions);
		if (debug)
			printf("%zd: ", status);
		status = clSetKernelArg(kernel, 4, sizeof(cl_mem), &bufResult);
		if (debug)
			printf("%zd: ", status);
		status = clEnqueueNDRangeKernel(cmdQueue, kernel, 2, NULL, indexSpaceSize, workGroupSize, 0, NULL, &event);
		if (debug)
			printf("%zd: ", status);

		clFinish(cmdQueue);

		clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(startTime), &startTime, NULL);
		clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(endTime), &endTime, NULL);
		time = (endTime - startTime) * 10e-6f;
		totalTime += time;
	}

	printf("Total time: %0.3f ms\n", totalTime);
	printf("Average time per step: %0.3f ms\n", totalTime / steps);

	// 9. Copying output data back to the host

	status = clEnqueueReadBuffer(cmdQueue, bufResult, CL_TRUE, 0, datasize, finalResult, 0, NULL, NULL);
	if (debug)
		printf("%zd: ", status);

	// 10. Releasing resources

	clReleaseKernel(kernel);
	clReleaseProgram(program);
	clReleaseCommandQueue(cmdQueue);
	clReleaseMemObject(bufPrevious);
	clReleaseMemObject(bufValues);
	clReleaseMemObject(bufResult);
	clReleaseContext(context);
/*
	for (int i = 0; i < width * height; i++) {
		if (i % width == 0)
			printf("\n");
		printf("%f ", finalResult[i]);
	}
*/
	if (debug)
		printf("%f", finalResult[width * 150 + 1]);

	free(heatPrevious);
	free(heatValues);
	free(finalResult);
	free(platforms);
	free(devices);
	free(programSource);

	system("pause");

	return 0;
}