#include <optix_device.h>
#include <curand_kernel.h>
#include <vector_types.h>

//#include <cuda/helpers.h>
//#include <cuda/random.h>
#include "Soltrace.h"

// Launch parameters for soltrace
extern "C" {
    __constant__ OptixCSP::LaunchParams params;
}

namespace OptixCSP {

    // Halton sequence generator, used for quasi-random sampling
    // Generates a Halton sequence value for a given index and base
    __device__ float halton(int index, int base) {
        float f = 1.0f, result = 0.0f;
        while (index > 0) {
            f = f / base;
            result = result + f * (index % base);
            index = index / base;
        }
        return result;
    }

    // Generate a sample point within a parallelogram defined by the AABB (Axis-Aligned Bounding Box)
    // Uses the Halton sequence for sampling
    __device__ float3 haltonSampleInParallelogram(unsigned int sample_index) {
        // Generate Halton sequence values
        float u = halton(sample_index, 2); // Base 2 for x
        float v = halton(sample_index, 3); // Base 3 for y

        // Compute the two edge vectors of the parallelogram
        float3 edge1 = params.sun_v1 - params.sun_v0; // First edge vector
        float3 edge2 = params.sun_v3 - params.sun_v0; // Second edge vector

        return params.sun_v0 + u * edge1 + v * edge2;
    }

    // Generate a random sample point within a parallelogram using a random number generator (RNG)
    // The parallelogram is defined by an AABB (Axis-Aligned Bounding Box)
    __device__ float3 samplePointInParallelogram(unsigned int seed) {
        curandState rng_state;
        curand_init(seed, 0, 0, &rng_state);

        // Generate random values between 0 and 1
        float u = curand_uniform(&rng_state);
        float v = curand_uniform(&rng_state);

        // Compute the two edge vectors of the parallelogram
        float3 edge1 = params.sun_v1 - params.sun_v0; // First edge vector
        float3 edge2 = params.sun_v3 - params.sun_v0; // Second edge vector

        return params.sun_v0 + u * edge1 + v * edge2;
    }

    // Generate a random point within a disk with a given radius
    // Uses polar coordinates (r, theta) for sampling
    __device__ float2 samplePointInDisk(float radius, unsigned int seed) {
        curandState rng_state;
        curand_init(seed, 0, 0, &rng_state);

        // Generate random radius and angle values
        float r = radius * sqrtf(curand_uniform(&rng_state));
        float theta = 2.0f * M_PIf * curand_uniform(&rng_state);

        // Convert to Cartesian coordinates
        return make_float2(r * cosf(theta), r * sinf(theta));
    }

    // Sample a random ray direction within a cone defined by a maximum angle
    __device__ float3 sampleRayDirectionInCone(float3 dir, float half_angle, unsigned int seed) {
        curandState rng_state;
        curand_init(seed, 0, 0, &rng_state);

        // Build an orthonormal basis
        float3 w = normalize(dir);
        float3 u = normalize(cross(fabs(w.x) > 0.99f ? make_float3(0, 1, 0) : make_float3(1, 0, 0), w));
        float3 v = cross(w, u);

        // Random angles
        float cosTheta = cosf(half_angle);
        float rand1 = curand_uniform(&rng_state);
        float rand2 = curand_uniform(&rng_state);
        float phi = 2.0f * M_PIf * rand1;
        float z = cosTheta + (1.0f - cosTheta) * rand2;
        float r = sqrtf(1.0f - z * z);

        // Transform to world space
        return normalize(r * (cosf(phi) * u + sinf(phi) * v) + z * w);
    }
}
// == Ray Generation Program - Sun Source (Parallelogram Sampling)
extern "C" __global__ void __raygen__sun_source()
{
    // Lookup location in launch grid
    const uint3 launch_idx = optixGetLaunchIndex();         // Index of the current launch thread
    const uint3 launch_dims = optixGetLaunchDimensions();   // Dimensions of the launch grid
    const unsigned int ray_number = launch_idx.y * launch_dims.x + launch_idx.x;  // Unique ray ID

    float3 sun_sample_pos = OptixCSP::haltonSampleInParallelogram(ray_number);

    // Sample emission angle here - capturing sun distribution
    const float3 ray_gen_pos = sun_sample_pos;

    float3 init_ray_dir = -normalize(params.sun_vector);
    float3 ray_dir = OptixCSP::sampleRayDirectionInCone(init_ray_dir, params.max_sun_angle, ray_number);

    // Create the PerRayData structure to track ray state (e.g., path index and recursion depth)
    OptixCSP::PerRayData prd;
    prd.ray_path_index = ray_number;
    prd.depth = 0;

    // TODO make this a launch parameter
    params.hit_point_buffer[params.max_depth * prd.ray_path_index] = make_float4(0.0f, ray_gen_pos);
    

    // Cast and trace the ray through the scene
    optixTrace(
        params.handle,               // Acceleration structure handle
        ray_gen_pos,                 // Ray origin
        ray_dir,                     // Ray direction
        0.001f,                      // Minimum ray distance (near hit distance)
        1e16f,                       // Maximum ray distance (far hit distance)
        0.0f,                        // Time parameter (static for now)
        OptixVisibilityMask(1),      // Visibility mask (e.g., to restrict ray interactions)
        OPTIX_RAY_FLAG_NONE,         // Ray flags (no special flags)
        OptixCSP::RAY_TYPE_RADIANCE, // Ray type (radiance for sunlight)
        OptixCSP::RAY_TYPE_COUNT,    // Number of ray types
        OptixCSP::RAY_TYPE_RADIANCE, // SBT offset (ray type to launch)
        reinterpret_cast<unsigned int&>(prd.ray_path_index),
        reinterpret_cast<unsigned int&>(prd.depth)  
    );
}