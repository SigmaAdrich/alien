#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <helper_cuda.h>

#include <stdio.h>
#include <functional>

#include "CudaShared.cuh"
#include "CudaBase.cuh"
#include "CudaDeviceFunctions.cuh"

#define NUM_THREADS_PER_BLOCK 32
#define NUM_BLOCKS (32 * 5) /*160*/
#define NUM_CLUSTERS (NUM_BLOCKS * 50)

cudaStream_t cudaStream;
CudaData cudaData;

void init_Cuda(int2 size)
{
	cudaStreamCreate(&cudaStream);
	cudaSetDevice(0);

	cudaData.size = size;
	size_t mapSize = size.x * size.y * sizeof(CellCuda*) * LAYERS;
	cudaMallocManaged(&cudaData.map1, mapSize);
	cudaMallocManaged(&cudaData.map2, mapSize);
	for (int i = 0; i < size.x * size.y * LAYERS; ++i) {
		cudaData.map1[i] = nullptr;
		cudaData.map1[i] = nullptr;
		cudaData.map2[i] = nullptr;
		cudaData.map2[i] = nullptr;
	}
	int cellsPerCluster = 32;
	cudaData.clustersAC1 = ArrayController<ClusterCuda>(NUM_CLUSTERS * 2);
	cudaData.cellsAC1 = ArrayController<CellCuda>(NUM_CLUSTERS * cellsPerCluster * 2);
	cudaData.clustersAC2 = ArrayController<ClusterCuda>(NUM_CLUSTERS * 2);
	cudaData.cellsAC2 = ArrayController<CellCuda>(NUM_CLUSTERS * cellsPerCluster * 2);

	auto clusters = cudaData.clustersAC2.getArray(NUM_CLUSTERS);
	for (int i = 0; i < NUM_CLUSTERS; ++i) {
		clusters[i].pos = { random(size.x), random(size.y) };
		clusters[i].vel = { random(1.0f) - 0.5f, random(1.0) - 0.5f };
		clusters[i].angle = random(360.0f);
		clusters[i].angularVel = random(10.0f) - 5.0f;
		clusters[i].numCells = cellsPerCluster;

		clusters[i].cells = cudaData.cellsAC1.getArray(cellsPerCluster);
		for (int j = 0; j < cellsPerCluster; ++j) {
			clusters[i].cells[j].relPos = { j - 20.0f, j - 20.0f };
		}

	}
}

void calcNextTimestep_Cuda()
{
	movement_Kernel <<<NUM_BLOCKS, NUM_THREADS_PER_BLOCK, 0, cudaStream>>> (cudaData);
	cudaDeviceSynchronize();
	checkCudaErrors(cudaGetLastError());

	swap(cudaData.clustersAC1, cudaData.clustersAC2);
	cudaData.clustersAC2.reset();
}

void getDataRef_Cuda(int& numClusters, ClusterCuda*& clusters)
{
	numClusters = cudaData.clustersAC1.getNumEntries();
	clusters = cudaData.clustersAC1.getEntireArray();
}


void end_Cuda()
{
	cudaDeviceSynchronize();
	cudaDeviceReset();

	cudaData.cellsAC1.free();
	cudaData.clustersAC1.free();
	cudaData.cellsAC2.free();
	cudaData.clustersAC2.free();
	cudaFree(cudaData.map1);
	cudaFree(cudaData.map2);
}

