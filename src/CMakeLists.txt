# ──────────────────────────────────────────────────────────────
#  src/CMakeLists.txt   – builds the OptixCSP core library
# ──────────────────────────────────────────────────────────────

# ---------------------------------------------------------------------------
# core target
# ---------------------------------------------------------------------------
if(WIN32)
  add_library(OptixCSP_core STATIC)
else()
  add_library(OptixCSP_core SHARED)
endif()

target_link_libraries(OptixCSP_core PRIVATE CUDA::cuda_driver CUDA::cudart)  # or cudart_static   
find_package(CUDAToolkit REQUIRED)
find_package(OptiX REQUIRED)
message(STATUS "OptiX include directory: ${OptiX_INCLUDE}")

# detect compute capabilities
execute_process(
    COMMAND nvidia-smi --query-gpu=compute_cap --format=csv,noheader
    OUTPUT_VARIABLE DETECTED_COMPUTE_CAP
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
string(REPLACE "." "" DETECTED_COMPUTE_CAP ${DETECTED_COMPUTE_CAP})
message(STATUS "Detected gpu compute capability: ${DETECTED_COMPUTE_CAP}")


file(GLOB_RECURSE CORE_SRC  CONFIGURE_DEPENDS  core/*.cpp core/*.cu)
file(GLOB_RECURSE CORE_HDR  CONFIGURE_DEPENDS  core/*.hpp core/*.h utils/*.hpp utils/*.h shaders/*.h)

target_sources(OptixCSP_core PRIVATE ${CORE_SRC} ${CORE_HDR})

target_include_directories(OptixCSP_core
   PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}
           ${OptiX_INCLUDE}
           ${CUDAToolkit_INCLUDE_DIRS}
   PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(OptixCSP_core
    PRIVATE CUDA::cuda_driver
            CUDA::cudart)

# have to specify the Optix_INCLUDE directory for the CUDA compiler 
target_compile_options(OptixCSP_core PRIVATE
    $<$<COMPILE_LANGUAGE:CUDA>:--use_fast_math -lineinfo -I"${OptiX_INCLUDE}">
)
# ---------------------------------------------------------------------------
# shaders target
# ---------------------------------------------------------------------------

set(SHADERS
    intersection
    materials
    sun
)

set(PTX_OUTPUT_DIR "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/ptx")
file(MAKE_DIRECTORY "${PTX_OUTPUT_DIR}")

message(STATUS "PTX output directory: ${PTX_OUTPUT_DIR}")

foreach(shader ${SHADERS})
    set(SHADER_SRC ${CMAKE_CURRENT_SOURCE_DIR}/shaders/${shader}.cu)
    set(SHADER_PTX ${PTX_OUTPUT_DIR}/${shader}.ptx)

    add_custom_command(
        OUTPUT ${SHADER_PTX}
        COMMAND ${CMAKE_CUDA_COMPILER}
            --use_fast_math
            -lineinfo
            -std=c++17
#            -arch=sm_${DETECTED_COMPUTE_CAP}
            -arch=sm_80
            -I${OptiX_INCLUDE}      
            -DOPTIX_INCLUDE_INTERNAL_FUNCTIONS
            -ptx
            -o ${SHADER_PTX}
            ${SHADER_SRC}
        DEPENDS ${SHADER_SRC}
        COMMENT "Generating PTX: ${shader}.ptx"
    )

    list(APPEND PTX_OUTPUTS ${SHADER_PTX})
    list(APPEND SHADER_FILES ${SHADER_SRC})

endforeach()

add_custom_target(OptixCSP_shaders DEPENDS ${PTX_OUTPUTS} ${SHADER_FILES}
                                   SOURCES ${SHADER_FILES}
)
# ---------------------------------------------------------------------------
# core depends on shaders target
# ---------------------------------------------------------------------------
add_dependencies(OptixCSP_core OptixCSP_shaders)


# ---------------------------------------------------------------------------
# generate sampleConfig.h
# ---------------------------------------------------------------------------
configure_file(
    ../sampleConfig.h.in               # contains:  #define SAMPLES_PTX_DIR "@PTX_OUTPUT_DIR@/"
    ${CMAKE_CURRENT_BINARY_DIR}/sampleConfig.h
    @ONLY)


# ---------------------------------------------------------------------------
# export targets for linking to third-party projects
# ---------------------------------------------------------------------------
set(OptixCSP_EXPORT_CONFIG_DIR "${CMAKE_BINARY_DIR}/cmake/")

export(TARGETS OptixCSP_core
       FILE ${OptixCSP_EXPORT_CONFIG_DIR}/OptixCSPTargets.cmake
       NAMESPACE OptixCSP::
)

# Generate Config file
include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/OptixCSPConfig.cmake.in
    ${OptixCSP_EXPORT_CONFIG_DIR}/OptixCSPConfig.cmake
    INSTALL_DESTINATION lib/cmake/OptixCSP
)
