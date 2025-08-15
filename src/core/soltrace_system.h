#pragma once

#include <string>
#include <vector>
#include <memory>                 
#include <cstddef>                
#include <cstdio>                 



#include "core/soltrace_state.h" // SoltraceState
#include "core/vec3d.h"      // Vec3d
#include "core/timer.h"
#include "core/CspElement.h" // CspElement
#include "core/Surface.h"    // Surface and derived classes

namespace OptixCSP {

    class GeometryManager;
    class pipelineManager;
    class dataManager;
    class CspElement;
    class Vec3d;
    class Surface;

    class SolTraceSystem {
    public:
        SolTraceSystem(int numSunPoints);
        ~SolTraceSystem();

        /// Call to this function mark the completion of the simulation setup
        void initialize();

        /// Execute the ray tracing simulation
        void run();

        /// Update launch params
        void update();

        // Read a stinput file for the simulation setup.
        bool read_st_input(const char* filename);

        // Write the output to a file.
        void write_sun_output(const std::string& filename);
        void write_hp_output(const std::string& filename);

        /// Explicit cleanup
        void clean_up();

        void set_verbose(bool verbose) { m_verbose = verbose; } // Set verbosity for debugging
        /// <summary>
        /// set the number of rays launched
        /// </summary>
        /// <param name="numSunPoints"></param>
        void set_sun_points(int num) { m_num_sunpoints = num; }

        /// <summary>
        /// set normalized sun vector
        /// </summary>
        /// <param name="sunVector"></param>
        void set_sun_vector(OptixCSP::Vec3d vect);

        void set_sun_angle(double angle) { m_sun_angle = angle; } // Set the sun angle


        /// <summary>
        /// compute number of heliostat CspElements added to the system 
        /// </summary>
        /// <returns></returns>
        size_t get_num_heliostats() const
        {
            return m_element_list.size() - 1; // Return the number of heliostats (elements) added
        }

        /// <summary>
        /// compute number of receiver elements added to the system
        /// </summary>
        /// <returns></returns>
        int get_num_receivers() const {
            return 1; // Assuming one receiver for now, can be modified later
        }

        /// <summary>
        /// add element
        /// /// </summary>
        void add_element(std::shared_ptr<CspElement> element);

        double get_time_trace();
        double get_time_setup();

        void print_launch_params();


    private:

        std::shared_ptr<GeometryManager> geometry_manager;
        std::shared_ptr<pipelineManager> pipeline_manager;
        std::shared_ptr<dataManager>     data_manager;

        int m_num_sunpoints;
        bool m_verbose;

        OptixCSP::Vec3d m_sun_vector;
        double m_sun_angle;
        OptixCSP::SoltraceState m_state;

        std::vector<std::shared_ptr<CspElement>> m_element_list;
        void create_shader_binding_table();

        // Helper functions to read a stinput file
        bool read_system(FILE* fp);
        bool read_stage(FILE* fp);
        bool read_element(FILE* fp);
        bool read_optic(FILE* fp);
        bool read_optic_surface(FILE* fp);
        bool read_sun(FILE* fp);
        void read_line(char* buf, int len, FILE* fp);
        std::vector<std::string> split(const std::string& str, const std::string& delim, bool ret_empty, bool ret_delim);

        Timer m_timer_setup;
        Timer m_timer_trace;

        // memory usage
        size_t m_mem_free_before;
        size_t m_mem_free_after;


    };
}