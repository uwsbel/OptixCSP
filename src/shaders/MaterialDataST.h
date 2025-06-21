#pragma once

#include <cuda_runtime.h>

namespace OptixCSP
{

    struct MaterialData
    {
        enum Type
        {
            MIRROR = 0,
            GLASS = 1,
            RECEIVER = 2
        };

        struct Mirror
        {
            float reflectivity;
            float transmissivity;
            float slope_error;
            float specularity_error;
        };

        struct Glass
        {
            // Not used currently
            float  importance_cutoff;
            float3 cutoff_color;
            float  fresnel_exponent;
            float  fresnel_minimum;
            float  fresnel_maximum;
            float  refraction_index;
            float3 refraction_color;
            float3 reflection_color;
            float3 extinction_constant;
            float3 shadow_attenuation;
            int    refraction_maxdepth;
            int    reflection_maxdepth;
        };

        struct Receiver
        {
            // dummy properties for now
            float reflectivity;
            float transmissivity;
            float slope_error;
            float specularity_error;
        };

        union
        {
            Mirror      mirror;
            Glass       glass;
            Receiver    receiver;
        };
    };

}   