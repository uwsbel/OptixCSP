#include "geometry_manager.h"
#include "shaders/GeometryDataST.h"
#include "sun_utils.h"
#include "soltrace_state.h"
#include "utils/util_check.hpp"
#include "data_manager.h"
#include <vector>
#include <optix_stubs.h>


using namespace OptixCSP;

void GeometryManager::collect_geometry_info(const std::vector<std::shared_ptr<CspElement>>& element_list,
                                            LaunchParams& params) {    
    m_aabb_list_H.clear(); // Clear the existing AABB list
    m_sbt_index_H.clear(); // Clear the existing SBT index list
	m_geometry_data_array_H.clear(); // Clear the existing geometry data array

	m_obj_counts = static_cast<uint32_t>(element_list.size()); // Number of objects in the scene

	// Resize
	m_aabb_list_H.resize(m_obj_counts);
	m_geometry_data_array_H.resize(m_obj_counts);
    m_sbt_index_H.resize(m_obj_counts);


    for (uint32_t i = 0; i < m_obj_counts; i++) {

		std::shared_ptr<CspElement> element = element_list[i];

        // Create an OptixAabb from the geometry data
        OptixAabb aabb;
        float3 m_min;
        float3 m_max;
        uint32_t sbt_offset = 0;

        if (element->get_aperture_type() == ApertureType::CIRCLE) {
            m_min = make_float3(0.0f, 0.0f, 0.0f); // Initialize min to a large value
            m_max = make_float3(0.0f, 0.0f, 0.0f); // Initialize max to a small value
        }


        if (element->get_aperture_type() == ApertureType::RECTANGLE) {
            element->compute_bounding_box();
            m_min.x = (float)(element->get_lower_bounding_box()[0]);
            m_min.y = (float)(element->get_lower_bounding_box()[1]);
			m_min.z = (float)(element->get_lower_bounding_box()[2]);

			m_max.x = (float)(element->get_upper_bounding_box()[0]);
			m_max.y = (float)(element->get_upper_bounding_box()[1]);
			m_max.z = (float)(element->get_upper_bounding_box()[2]);

            if (element->get_surface_type() == SurfaceType::PARABOLIC) {
                sbt_offset = static_cast<uint32_t>(OpticalEntityType::RECTANGLE_PARABOLIC_MIRROR);
                // no receiver only mirrors
            }
            else if (element->get_surface_type() == SurfaceType::FLAT) {
                if (element->is_receiver())
                    sbt_offset = static_cast<uint32_t>(OpticalEntityType::RECTANGLE_FLAT_RECEIVER);
                else 
					sbt_offset = static_cast<uint32_t>(OpticalEntityType::RECTANGLE_FLAT_MIRROR);
            }
            else if (element->get_surface_type() == SurfaceType::CYLINDER){
                sbt_offset = static_cast<uint32_t>(OpticalEntityType::CYLINDRICAL_RECEIVER);
            }
			else {
            }
        }

        if (element->get_aperture_type() == ApertureType::TRIANGLE) {
            element->compute_bounding_box();
            m_min.x = (float)(element->get_lower_bounding_box()[0]);
            m_min.y = (float)(element->get_lower_bounding_box()[1]);
			m_min.z = (float)(element->get_lower_bounding_box()[2]);
			m_max.x = (float)(element->get_upper_bounding_box()[0]);
			m_max.y = (float)(element->get_upper_bounding_box()[1]);
			m_max.z = (float)(element->get_upper_bounding_box()[2]);

            if (element->is_receiver())
                sbt_offset = static_cast<uint32_t>(OpticalEntityType::TRIANGLE_FLAT_RECEIVER);

        }


        aabb.minX = m_min.x;
        aabb.minY = m_min.y;
        aabb.minZ = m_min.z;

        aabb.maxX = m_max.x;
        aabb.maxY = m_max.y;
        aabb.maxZ = m_max.z;


		m_aabb_list_H[i] = aabb; // Store the AABB in the list
        m_sbt_index_H[i] = sbt_offset; // Store the SBT index
        m_geometry_data_array_H[i] = element_list[i]->toDeviceGeometryData();
    }

    // print out computed minimum distance 
	std::cout << "Minimum distance to sun plane: " << m_sun_plane_distance << std::endl;
}


void GeometryManager::compute_sun_plane_H(LaunchParams& params) {

    m_sun_plane_distance = -1;
    float3 sun_vector = params.sun_vector;

    float* sun_plane_dist_D;
    CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&sun_plane_dist_D), sizeof(float)));
    CUDA_CHECK(cudaMemset(sun_plane_dist_D, 0, sizeof(float)));

    compute_d_on_gpu(reinterpret_cast<const OptixAabb*>(m_aabb_list_D), m_obj_counts, sun_vector, sun_plane_dist_D);

    CUDA_CHECK(cudaMemcpy(reinterpret_cast<void*>(&m_sun_plane_distance),
                         reinterpret_cast<void*>(sun_plane_dist_D),
                         sizeof(float),
                         cudaMemcpyDeviceToHost));

    float3 sun_u, sun_v;
    float3 axis = (abs(sun_vector.x) < 0.9f) ? make_float3(1.0f, 0.0f, 0.0f) : make_float3(0.0f, 1.0f, 0.0f);
    sun_u = normalize(cross(axis, sun_vector));
    sun_v = normalize(cross(sun_vector, sun_u));
    // allocate uv bounds on device, float array of size four
    float sun_uv_bounds[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    // allocate memory for the sun_u_bounds on device
    float* sun_uv_bounds_D;
    CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&sun_uv_bounds_D), 4 * sizeof(float)));

    compute_uv_bounds_on_gpu(reinterpret_cast<const OptixAabb*>(m_aabb_list_D),
        m_obj_counts,
        m_sun_plane_distance,
        sun_vector,
        sun_u,
        sun_v,
        tan(params.max_sun_angle),
        sun_uv_bounds_D);

    // Copy the computed bounds back to the host
    cudaMemcpy(sun_uv_bounds, sun_uv_bounds_D, 4 * sizeof(float), cudaMemcpyDeviceToHost);

    float u_min = sun_uv_bounds[0];
    float u_max = sun_uv_bounds[1];
    float v_min = sun_uv_bounds[2];
    float v_max = sun_uv_bounds[3];

    params.sun_v0 = u_min * sun_u + v_min * sun_v + m_sun_plane_distance * sun_vector;
    params.sun_v1 = u_max * sun_u + v_min * sun_v + m_sun_plane_distance * sun_vector;
    params.sun_v2 = u_max * sun_u + v_max * sun_v + m_sun_plane_distance * sun_vector;
    params.sun_v3 = u_min * sun_u + v_max * sun_v + m_sun_plane_distance * sun_vector;

    CUDA_CHECK(cudaFree(reinterpret_cast<void*>(sun_plane_dist_D)));
    CUDA_CHECK(cudaFree(reinterpret_cast<void*>(sun_uv_bounds_D)));

}

void GeometryManager::create_geometries(LaunchParams& params) {

    // Allocate memory on the device for the AABB array.
    CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&m_aabb_list_D), m_obj_counts * sizeof(OptixAabb)));
    CUDA_CHECK(cudaMemcpy(reinterpret_cast<void*>(m_aabb_list_D),
               m_aabb_list_H.data(), 
		       m_obj_counts * sizeof(OptixAabb),
               cudaMemcpyHostToDevice));

	compute_sun_plane_H(params);

    // populate aabb_input_flags vector, size of types, no rebuild
    std::vector<uint32_t> aabb_input_flags(NUM_OPTICAL_ENTITY_TYPES);
    for (int i = 0; i < NUM_OPTICAL_ENTITY_TYPES; i++) {
        aabb_input_flags[i] = OPTIX_GEOMETRY_FLAG_DISABLE_ANYHIT;
    }

	// device vector for SBT index, no need to rebuild
    CUdeviceptr d_sbt_index;
    CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&d_sbt_index), m_obj_counts * sizeof(uint32_t)));
    CUDA_CHECK(cudaMemcpy(reinterpret_cast<void*>(d_sbt_index),
                          m_sbt_index_H.data(),
        m_obj_counts * sizeof(uint32_t),
                          cudaMemcpyHostToDevice));

    // Configure the input for the GAS build process.
    m_aabb_input.type = OPTIX_BUILD_INPUT_TYPE_CUSTOM_PRIMITIVES;
    m_aabb_input.customPrimitiveArray.aabbBuffers = &m_aabb_list_D;
    m_aabb_input.customPrimitiveArray.flags = aabb_input_flags.data();
    m_aabb_input.customPrimitiveArray.numSbtRecords = NUM_OPTICAL_ENTITY_TYPES;
    m_aabb_input.customPrimitiveArray.numPrimitives = m_obj_counts;
    m_aabb_input.customPrimitiveArray.sbtIndexOffsetBuffer = d_sbt_index;
    m_aabb_input.customPrimitiveArray.sbtIndexOffsetSizeInBytes = sizeof(uint32_t);
    m_aabb_input.customPrimitiveArray.primitiveIndexOffset = 0;

    // Set up acceleration structure (AS) build options.
    m_accel_build_options = {
        OPTIX_BUILD_FLAG_ALLOW_UPDATE,        // allow update
        OPTIX_BUILD_OPERATION_BUILD           // operation type, build a new aceleration structure
    };

    OptixAccelBufferSizes gas_buffer_sizes;     // sizes for temp and output buffers.

    // Query the memory usage required for building the GAS.
	OPTIX_CHECK(optixAccelComputeMemoryUsage(m_state.context, 
                                             &m_accel_build_options, 
                                             &m_aabb_input,
                                             1,
                                             &gas_buffer_sizes));

    m_temp_buffer_size   = gas_buffer_sizes.tempSizeInBytes;
    m_output_buffer_size = gas_buffer_sizes.outputSizeInBytes;

    CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&m_temp_buffer),   m_temp_buffer_size));
    CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&m_output_buffer), m_output_buffer_size));

    // Build the GAS.
	OPTIX_CHECK(optixAccelBuild(m_state.context,								  // OptiX context
		m_state.stream,                                  // CUDA stream (default is 0)
        &m_accel_build_options,
        &m_aabb_input,
        1,
        m_temp_buffer,
        m_temp_buffer_size,
        m_output_buffer,
        m_output_buffer_size,
		&m_state.gas_handle,                             // Output handle for the GAS
		nullptr,                                        // Emitted properties (not used here)
		0));                                           // Number of emitted properties
    
}


void GeometryManager::update_geometry_info(const std::vector<std::shared_ptr<CspElement>>& element_list,
	LaunchParams& params) {
	// Recollect geometry info
	collect_geometry_info(element_list, params);

    // update device aabb list
	CUDA_CHECK(cudaMemcpyAsync(
		reinterpret_cast<void*>(m_aabb_list_D),
		m_aabb_list_H.data(),
		m_aabb_list_H.size() * sizeof(OptixAabb),
		cudaMemcpyHostToDevice, m_state.stream));

    std::vector<uint32_t> aabb_input_flags(NUM_OPTICAL_ENTITY_TYPES);
    for (int i = 0; i < NUM_OPTICAL_ENTITY_TYPES; i++) {
        aabb_input_flags[i] = OPTIX_GEOMETRY_FLAG_DISABLE_ANYHIT;
    }

	m_aabb_input.customPrimitiveArray.flags = aabb_input_flags.data();


    CUDA_CHECK(cudaMemcpyAsync(
        reinterpret_cast<void*>(m_aabb_input.customPrimitiveArray.aabbBuffers[0]),
        m_aabb_list_H.data(),
        m_aabb_list_H.size() * sizeof(OptixAabb),
        cudaMemcpyHostToDevice, m_state.stream));

    m_accel_build_options.operation = OPTIX_BUILD_OPERATION_UPDATE; // set to update 

    OPTIX_CHECK(optixAccelBuild(m_state.context,								  // OptiX context
        m_state.stream,                                  // CUDA stream (default is 0)
        &m_accel_build_options,
        &m_aabb_input,
        1,
        m_temp_buffer,
        m_temp_buffer_size,
        m_output_buffer,
        m_output_buffer_size,
        &m_state.gas_handle,                             // Output handle for the GAS
        nullptr,                                        // Emitted properties (not used here)
        0));                                           // Number of emitted properties


	compute_sun_plane_H(params);
}