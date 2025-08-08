#include "core/soltrace_system.h"
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <filesystem>


using namespace std;
using namespace OptixCSP;

int main(int argc, char* argv[]) {
    int num_rays = 1000000;
    // Create the simulation system.
    SolTraceSystem system(num_rays);

    // Element 1
    Vec3d origin_e1(0, 0, 0); // origin of the element
    Vec3d aim_point_e1(0, 0, 1);  // aim point of the element
    auto e1 = std::make_shared<CspElement>();
    e1->set_origin(origin_e1);
    e1->set_aim_point(aim_point_e1); // Aim direction
    e1->set_zrot(0); // Set the rotation around the Z-axis

    auto surface_e1 = std::make_shared<SurfaceFlat>();
    e1->set_surface(surface_e1);


    ////////////////////////////////////////
    // STEP 1.3 create rectangle aperture //
    ////////////////////////////////////////
    double dim_x = 1.0;
    double dim_y = 1.0;
    auto aperture_e1 = std::make_shared<ApertureRectangle>(dim_x, dim_y);
    e1->set_aperture(aperture_e1);

    system.add_element(e1); // Add the receiver to the system


    double receiver_dim_x;
    double receiver_dim_y;
    receiver_dim_x = 2.0; // width of the receiver
    receiver_dim_y = 2.0; // height of the receiver
    Vec3d receiver_origin(0, 0, 10.0); // origin of the receiver
    Vec3d receiver_aim_point(0, 0, -1); // aim point of the receiver

    auto e4 = std::make_shared<CspElement>();
    e4->set_origin(receiver_origin);
    e4->set_aim_point(receiver_aim_point); // Aim direction
    e4->set_zrot(0.0); // No rotation for the receiver


    auto receiver_aperture = std::make_shared<ApertureRectangle>(receiver_dim_x, receiver_dim_y);
    e4->set_aperture(receiver_aperture);

    ///////////////////////////////////
    // STEP 2.3 create flat surface //
    ///////////////////////////////////
    auto receiver_surface = std::make_shared<SurfaceFlat>();
    e4->set_surface(receiver_surface);

    ////////////////////////////////////////////
    // STEP 2.4 Add the element to the system //
    ///////////////////////////////////////////
    system.add_element(e4); // Add the receiver to the system


    
    Vec3d sun_vector(0.0, 0.0, 100.0); // sun vector
    // set up sun angle 
    double sun_angle = 0.465; // 0.00465; // sun angle
    system.set_sun_angle(sun_angle);
	system.set_sun_vector(sun_vector);

    ///////////////////////////////////
    // STEP 3  Initialize the system //
    ///////////////////////////////////
    system.initialize();

    ////////////////////////////
    // STEP 4  Run Ray Trace //
    ///////////////////////////
    // TODO: set up different sun position trace // 
    system.run();

    //////////////////////////
    // STEP 5  Post process //
    //////////////////////////
	std::string out_dir = "out_sun_modeling/";
	// check if out_dir exists, if not create it 
    if (std::filesystem::exists(out_dir)) {
        std::cout << "Output directory already exists " << out_dir << ", rewriting results\n" << std::endl;
    } else if (!std::filesystem::create_directory(std::filesystem::path(out_dir))) {
        std::cerr << "Error creating directory " << out_dir << std::endl;
        return 1;
    }

    system.write_hp_output(out_dir + "hit_points.csv");
	system.write_sun_output(out_dir + "sun_direction_gaussian_old.csv");

    /////////////////////////////////////////
    // STEP 6  Be a good citizen, clean up //
    /////////////////////////////////////////
    system.clean_up();



    return 0;
}