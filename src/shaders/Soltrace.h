#pragma once

#include "GeometryDataST.h"
#include "MaterialDataST.h"

#include <vector_types.h>
#include <optix.h>

namespace OptixCSP{

    const unsigned int NUM_ATTRIBUTE_VALUES = 4u;
    const unsigned int NUM_PAYLOAD_VALUES   = 2u;
    const unsigned int MAX_TRACE_DEPTH      = 5u;
    
    struct HitGroupData
    {
        MaterialData material_data;
    };

    enum RayType
    {
        RAY_TYPE_RADIANCE = 0,
        RAY_TYPE_COUNT = 1         // not using occlusion/shadow rays atm
    };

    enum OpticalEntityType : unsigned int {
        RECTANGLE_FLAT_MIRROR         = 0,
        RECTANGLE_PARABOLIC_MIRROR    = 1,
        RECTANGLE_FLAT_RECEIVER       = 2,
        CYLINDRICAL_RECEIVER          = 3,
		TRIANGLE_FLAT_RECEIVER        = 4,
	    NUM_OPTICAL_ENTITY_TYPES
    };

    struct LaunchParams
    {
        unsigned int                width;   // essentially number of rays launched and sun points 
        unsigned int                height;
        int                         max_depth;

        float4*                     hit_point_buffer;
        float3*                     sun_dir_buffer;
        OptixTraversableHandle      handle;

        float3                      sun_vector;
        float                       max_sun_angle;
		unsigned long long          sun_dir_seed; // seed for the sun direction randomization

        float3                      sun_v0;
        float3                      sun_v1;
        float3                      sun_v2;
        float3                      sun_v3;

	    GeometryDataST*             geometry_data_array;
    };

    struct PerRayData
    {
        unsigned int ray_path_index;  // Index of the ray in the ray path buffer
        unsigned int depth;           // Trace depth
    };

} // end namespace OptixCSP