#pragma once
#include <string>
#include <vector>
#include "optix.h"
#include "shaders/Soltrace.h"
#include "soltrace_type.h"
#include "soltrace_state.h"



namespace OptixCSP {
    /**
     * @class pipelineManager
     * @brief Manages the OptiX pipeline, including loading modules, creating programs, and handling cleanup.
     */
    class pipelineManager {
    public:
        /**
         * @brief Constructs a pipelineManager instance.
         * @param state Reference to SoltraceState for managing OptiX state.
         */
        pipelineManager(SoltraceState& state);

        /**
         * @brief Destroys the pipelineManager and cleans up resources.
         */
        ~pipelineManager();

        /**
         * @brief Cleans up all allocated resources including OptiX programs and modules.
         */
        void cleanup();

        /**
         * @brief Loads PTX code from a file.
         * @param kernelName The name of the PTX kernel file to load.
         * @return The loaded PTX code as a string.
         */
        std::string loadPtxFromFile(const std::string& kernelName);

        /**
         * @brief Loads all required OptiX modules.
         */
        void loadModules();

        /**
         * @brief Creates the OptiX pipeline with associated program groups.
         */
        void createPipeline();

        /**
         * @brief Retrieves the OptiX pipeline object.
         * @return The OptiX pipeline.
         */
        OptixPipeline getPipeline() const;

        /**
         * @brief Creates the ray generation program for the sun.
         *
         * Currently, there is only one ray generation source; this function can be extended for multiple sources.
         */
        void createSunProgram();

        /**
         * @brief Creates programs for mirror interactions.
         */
        void createMirrorPrograms();

        /**
         * @brief Creates programs for receiver interactions.
         */
        void createReceiverProgram();

        /**
         * @brief Creates the miss program, handling rays that do not hit any geometry.
         */
        void createMissProgram();

        /**
         * @brief Helper function to create a hit group program given intersection and closest hit functions.
         * @param group Reference to an OptixProgramGroup to be created.
         * @param intersectionModule OptiX module containing the intersection function.
         * @param intersectionFunc Name of the intersection function.
         * @param closestHitModule OptiX module containing the closest hit function.
         * @param closestHitFunc Name of the closest hit function.
         */
        void createHitGroupProgram(OptixProgramGroup& group,
            OptixModule intersectionModule,
            const char* intersectionFunc,
            OptixModule closestHitModule,
            const char* closestHitFunc);

        /**
         * @brief Retrieves the ray generation program group.
         * @return The OptixProgramGroup for ray generation.
         */
        OptixProgramGroup getRaygenProgram() const;

        /**
         * @brief Retrieves the miss program group.
         * @return The OptixProgramGroup for the miss shader.
         */
        OptixProgramGroup getMissProgram() const;

        /**
         * @brief Retrieves the receiver program group.
         * @return The OptixProgramGroup for receivers.
         */
        OptixProgramGroup getReceiverProgram() const;

        /**
         * @brief Retrieves the appropriate mirror program group based on surface and aperture type.
         * @param map SurfaceApertureMap specifying the surface and aperture combination.
         * @return The corresponding OptixProgramGroup.
         */
        OptixProgramGroup getMirrorProgram(SurfaceApertureMap map) const;

        /**
         * @brief Retrieves the appropriate receiver program group based on surface type.
         * @param surfaceType The type of surface (FLAT or CYLINDER).
         * @return The corresponding OptixProgramGroup.
         */
        OptixProgramGroup getReceiverProgram(SurfaceType surfaceType, ApertureType apertureType) const;

    private:
        SoltraceState& m_state;  ///< Reference to the simulation's OptiX state.
        std::vector<OptixProgramGroup> m_program_groups; ///< Stores all created OptiX program groups.

        // Number of program groups categorized by type.
        int num_raygen_programs = 1; ///< Number of ray generation programs.
        int num_heliostat_programs = 2; ///< Number of heliostat-related programs.
        int num_receiver_programs = 2; ///< Number of receiver-related programs.
        int num_miss_programs = 1; ///< Number of miss programs.
    };
}