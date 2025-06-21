#include "data_manager.h"
#include "soltrace_system.h"
#include "utils/util_check.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <chrono>
#include <iostream>
#include <iomanip>

using namespace OptixCSP;

dataManager::dataManager() : launch_params_D(nullptr) {
	
    // Initialize launch parameters with default values
	launch_params_H.width = 10;
	launch_params_H.height = 1;
	launch_params_H.max_depth = 5;

	launch_params_H.hit_point_buffer = nullptr;
	launch_params_H.sun_vector = make_float3(0.0f, 0.0f, 10.0f);
	launch_params_H.max_sun_angle = 0.0f;

	launch_params_H.sun_v0 = make_float3(0.0f, 0.0f, 0.0f);
	launch_params_H.sun_v1 = make_float3(0.0f, 0.0f, 0.0f);
	launch_params_H.sun_v2 = make_float3(0.0f, 0.0f, 0.0f);
	launch_params_H.sun_v3 = make_float3(0.0f, 0.0f, 0.0f);
}

dataManager::~dataManager() {
	cleanup(); 
}


OptixCSP::LaunchParams* dataManager::getDeviceLaunchParams() const { return launch_params_D; }


void dataManager::allocateLaunchParams() {
    CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&launch_params_D), sizeof(LaunchParams)));
}

void dataManager::updateLaunchParams() {
    CUDA_CHECK(cudaMemcpy(launch_params_D, &launch_params_H, sizeof(LaunchParams), cudaMemcpyHostToDevice));
}

void dataManager::allocateGeometryDataArray(std::vector<GeometryDataST> geometry_data_array_H) {


    CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&geometry_data_array_D),
        geometry_data_array_H.size() * sizeof(GeometryDataST)));

	CUDA_CHECK(cudaMemcpy(geometry_data_array_D, geometry_data_array_H.data(),
		geometry_data_array_H.size() * sizeof(GeometryDataST), cudaMemcpyHostToDevice));
	// make sure launch_params_H is updated with the new geometry data array
	launch_params_H.geometry_data_array = geometry_data_array_D;

}

void dataManager::updateGeometryDataArray(std::vector<GeometryDataST> geometry_data_array_H) {

	if (geometry_data_array_D == nullptr) {
		throw std::runtime_error("Geometry data array is not allocated.");
	}

	CUDA_CHECK(cudaMemcpy(geometry_data_array_D, geometry_data_array_H.data(),
		geometry_data_array_H.size() * sizeof(GeometryDataST), cudaMemcpyHostToDevice));

	//launch_params_H.geometry_data_array = geometry_data_array_D;

}

void dataManager::cleanup() {
	CUDA_CHECK(cudaFree(launch_params_D));
	launch_params_D = nullptr;

	CUDA_CHECK(cudaFree(geometry_data_array_D));
	geometry_data_array_D = nullptr;
}
