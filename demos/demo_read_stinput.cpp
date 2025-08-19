#include "core/soltrace_system.h"
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <filesystem>

// TODO: should setup sun models from stinput file

using namespace std;
using namespace OptixCSP;

int main(int argc, char* argv[]) {

    

    if (argc != 6) {
		std::cout << "Usage: " << argv[0] << "<stinput_name>" << " <num_of_rays>" << " sun_error <0/1>" << "<output_dir>" << " <write hitpoints?>" << std::endl;
        return 0;
    }

    // number of rays launched for the simulation
    std::string stinput_name = argv[1]; // stinput file name
	int num_rays = stoi(argv[2]);
    bool use_sun_error = std::stoi(argv[3]);
	std::string output_dir = argv[4]; // output directory
	bool write_hitpoints = std::stoi(argv[5]); // write hitpoints to file

    // Create the simulation system.
    SolTraceSystem system(num_rays);

	const char* stinput_file = stinput_name.c_str(); // Default stinput file name

	system.read_st_input(stinput_file);
	double sun_angle = 0.00465; // Default sun angle

    if (use_sun_error) {
        std::cout << "Using sun error model." << std::endl;
        system.set_sun_angle(sun_angle);
    }
    else {
        std::cout << "Not using sun error model." << std::endl;
        system.set_sun_angle(0.0);
	}

    system.initialize();
    system.run();

    int num_hits = system.get_num_hits_receiver();
	std::cout << "Number of rays hitting the receiver: " << num_hits << std::endl;

    if (!std::filesystem::exists(std::filesystem::path(output_dir))) {
        std::cout << "Creating output directory: " << output_dir << std::endl;
        if (!std::filesystem::create_directory(std::filesystem::path(output_dir))) {
            std::cerr << "Error creating directory " << output_dir << std::endl;
            return 1;
        }

	}

    if (use_sun_error == true) {
        std::cout << "Using sun error model, ";
        if (write_hitpoints)
            system.write_hp_output(output_dir + "sun_error_1_hit_points_" + std::to_string(num_rays) + "_rays.csv");
        system.write_simulation_json(output_dir + "sun_error_1_summary_" + std::to_string(num_rays) + "_rays.json");

    }
    else {
        std::cout << "Not using sun error model, ";
		if (write_hitpoints)
            system.write_hp_output(output_dir + "sun_error_0_hit_points_" + std::to_string(num_rays) + "_rays.csv");
        system.write_simulation_json(output_dir + "sun_error_0_summary_" + std::to_string(num_rays) + "_rays.json");

    }

    /////////////////////////////////////////
    // STEP 6  Be a good citizen, clean up //
    /////////////////////////////////////////
    system.clean_up();



    return 0;
}