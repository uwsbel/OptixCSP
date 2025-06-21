#pragma once
#include <optix.h>
#include <vector>
#include <string>

namespace OptixCSP
{

    // TODO
    // members of this struct should be moved to pipelineManager, and dataManager class
    /// eventually there's no SoltraceState struct, but data, pipeline and geometry manager classes each have its own members. 
    struct SoltraceState
    {
        OptixDeviceContext          context = 0;
        OptixTraversableHandle      gas_handle = {};
        CUdeviceptr                 d_gas_output_buffer = {};

        OptixModule                 geometry_module = 0;
        OptixModule                 shading_module = 0;
        OptixModule                 sun_module = 0;

        OptixProgramGroup           raygen_prog_group = 0;
        OptixProgramGroup           radiance_miss_prog_group = 0;
        OptixProgramGroup           radiance_receiver_prog_group = 0;

        OptixPipeline               pipeline = 0;
        OptixPipelineCompileOptions pipeline_compile_options = {};

        CUstream                    stream = 0;

        OptixShaderBindingTable     sbt = {};
    };
}