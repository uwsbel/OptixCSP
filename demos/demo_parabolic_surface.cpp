// main.cpp
#include "core/soltrace_system.h"
#include <iostream>
#include <filesystem>
enum TestCase {PARABOLIC, FLAT};

using namespace std;
using namespace OptixCSP;

int main(int argc, char* argv[]) {
    // Default number of rays launched for the simulation
    int num_rays = 100000;
    if (argc >= 2)
        num_rays = std::stoi(argv[1]);

    std::cout << "Starting ST_System simulation with " << num_rays << " sun points..." << std::endl;
	SolTraceSystem system(num_rays);


    Vec3d sun_vector(0, 0, 1.); // sun vector
	system.set_sun_vector(sun_vector); // Set the sun vector

    double curv_x = 0.05;
    double curv_y = 0.05;

    double focal_length = 1. / (2 * curv_x);

    double dim_x = 1.0;
    double dim_y = 1.0;

	//Vector3d origin(0, 5, 0); // origin of the element
	//Vector3d aim_point(0, 4.551982, 1.897836); // aim point of the element

    Vec3d origin(0, 0, 0); // origin of the element
    Vec3d aim_point(0, 0, 10); // aim point of the element


    auto e1 = std::make_shared<CspElement>();
    e1->set_origin(origin);
    e1->set_aim_point(aim_point); // Aim direction

    TestCase test_case = TestCase::PARABOLIC;

	std::shared_ptr<Surface> surface;

    switch (test_case) {
		case TestCase::PARABOLIC:
    		std::cout << "Using parabolic surface" << std::endl;
            surface = std::make_shared<SurfaceParabolic>();
            std::dynamic_pointer_cast<SurfaceParabolic>(surface)->set_curvature(curv_x, curv_y);
            e1->set_surface(surface);
            break;
		case TestCase::FLAT:
            std::cout << "Using flat surface" << std::endl;
            surface = std::make_shared<SurfaceFlat>();
            e1->set_surface(surface);
            break;
	}


    auto aperture = std::make_shared<ApertureRectangle>(dim_x, dim_y);
    e1->set_aperture(aperture);
	system.add_element(e1); // Add the element to the system

    ///////////////////////////////////
    // Add receiver the last for now //
    // TODO: this needs to be changed, sequence should not matter //
    ///////////////////////////////////////////////////////////////
    Vec3d receiver_origin(0, 0, 8.0); // origin of the receiver
    Vec3d receiver_aim_point(0, 0, -1); // aim point of the receiver
    double receiver_dim_x = 0.4;
    double receiver_dim_y = 0.4;


	auto e2 = std::make_shared<CspElement>();
	e2->set_origin(receiver_origin);
	e2->set_aim_point(receiver_aim_point); // Aim direction
	auto receiver_aperture = std::make_shared<ApertureRectangle>(receiver_dim_x, receiver_dim_y);
	e2->set_aperture(receiver_aperture);

	auto receiver_surface = std::make_shared<SurfaceFlat>();
	e2->set_surface(receiver_surface);

	system.add_element(e2); // Add the receiver to the system

    //Initialize the simulation
    system.initialize();

	// run ray tracing simulation
    system.run();

    std::string filename = "output";
    // write the result to a file 
    switch (test_case) {
    	case TestCase::PARABOLIC :
		    std::cout << "Writing parabolic surface results..." << std::endl;
			filename += "_parabolic.csv";
		    break;
	    case TestCase::FLAT :
		    std::cout << "Writing flat surface results..." << std::endl;
			filename += "_flat.csv";
            break;

    }


    std::string out_dir = "out_parabolic_surface/";
    if (!std::filesystem::exists(std::filesystem::path(out_dir))) {
        std::cout << "Creating output directory: " << out_dir << std::endl;
        if (!std::filesystem::create_directory(std::filesystem::path(out_dir))) {
            std::cerr << "Error creating directory " << out_dir << std::endl;
            return 1;
        }

    }

	system.write_hp_output(out_dir + filename);
	system.write_simulation_json(out_dir + "summary.json");

    // Clean up all allocated resources.
    system.clean_up();

    std::cout << "Simulation completed successfully." << std::endl;
    
    return 0;
}
