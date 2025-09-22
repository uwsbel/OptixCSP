#include "core/soltrace_system.h"
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <filesystem>


using namespace std;
using namespace OptixCSP;

enum RECEIVER_TYPE { FLAT, CYLINDRICAL, TRIANGLE };

int main(int argc, char* argv[]) {
    bool parabolic = false; // Set to true for parabolic mirrors, false for flat mirrors
    RECEIVER_TYPE receiver_type = TRIANGLE;
    int num_rays = 1000;

    if (argc == 2) {
		num_rays = atoi(argv[1]);
    }

    // Create the simulation system.
    SolTraceSystem system(num_rays);

        //////////////////////////////////////////////////////////////////
        // STEP 0: initialize the ray trace system with number of rays //
        /////////////////////////////////////////////////////////////////

        /////////////////////////////////////////////////////////////
        // STEP 1.1 Create heliostats, parabolic rectangle mirror  //
        ////////////////////////////////////////////////////////////
        // Element 1
        Vec3d origin_e1(-5, 0, 0); // origin of the element
        Vec3d aim_point_e1(17.360680, 0, 94.721360); // aim point of the element
        auto e1 = std::make_shared<CspElement>();
        e1->set_origin(origin_e1);
        e1->set_aim_point(aim_point_e1); // Aim direction
        e1->set_zrot(-90.0); // Set the rotation around the Z-axis

        ///////////////////////////////////////////
        // STEP 1.2 create the parabolic surface //
        ///////////////////////////////////////////
        double curv_x = 0.0170679f;
        double curv_y = 0.0370679f;
        if (parabolic) {
            auto surface_e1 = std::make_shared<SurfaceParabolic>();
            surface_e1->set_curvature(curv_x, curv_y);
            e1->set_surface(surface_e1);
        }
        else {
            // Create a flat surface if not parabolic
            auto surface_e1 = std::make_shared<SurfaceFlat>();
            e1->set_surface(surface_e1);
        }


        ////////////////////////////////////////
        // STEP 1.3 create rectangle aperture //
        ////////////////////////////////////////
        double dim_x = 1.0;
        double dim_y = 1.95;
        auto aperture_e1 = std::make_shared<ApertureRectangle>(dim_x, dim_y);
        e1->set_aperture(aperture_e1);

        ////////////////////////////////////////////
        // STEP 1.4 Add the element to the system //
        ///////////////////////////////////////////
        system.add_element(e1);

        // Element 2
        Vec3d origin_e2(0, 5, 0); // origin of the element
        Vec3d aim_point_e2(0, -17.360680, 94.721360); // aim point of the element
        auto e2 = std::make_shared<CspElement>();
        e2->set_origin(origin_e2);
        e2->set_aim_point(aim_point_e2); // Aim direction
        e2->set_zrot(0.0); // No rotation for the element

        if (parabolic) {
            auto surface_e2 = std::make_shared<SurfaceParabolic>();
            surface_e2->set_curvature(curv_x, curv_y);
            e2->set_surface(surface_e2);
        }
        else {
            // Create a flat surface if not parabolic
            auto surface_e2 = std::make_shared<SurfaceFlat>();
            e2->set_surface(surface_e2);
        }

        auto aperture_e2 = std::make_shared<ApertureRectangle>(dim_x, dim_y);
        e2->set_aperture(aperture_e2);

        system.add_element(e2);

        // Element 3
        Vec3d origin_e3(5, 0, 0); // origin of the element
        Vec3d aim_point_e3(-17.360680, 0, 94.721360); // aim point of the element
        auto e3 = std::make_shared<CspElement>();
        e3->set_origin(origin_e3);
        e3->set_aim_point(aim_point_e3); // Aim direction
        e3->set_zrot(90.0);

        if (parabolic) {
            auto surface_e3 = std::make_shared<SurfaceParabolic>();
            surface_e3->set_curvature(curv_x, curv_y);
            e3->set_surface(surface_e3);
        }
        else {
            // Create a flat surface if not parabolic
            auto surface_e3 = std::make_shared<SurfaceFlat>();
            e3->set_surface(surface_e3);
        }

        auto aperture_e3 = std::make_shared<ApertureRectangle>(dim_x, dim_y);
        e3->set_aperture(aperture_e3);

        system.add_element(e3);

        //////////////////////////////////////////////
        // STEP 2.1 Create receiver, flat rectangle //
        //////////////////////////////////////////////
        Vec3d receiver_origin(0, 0, 9.5); // origin of the receiver
        Vec3d receiver_aim_point(0, 5, 0); // aim point of the receiver



        if (receiver_type == RECEIVER_TYPE::CYLINDRICAL) {
            receiver_aim_point[2] = receiver_origin[2];
        }

        auto e4 = std::make_shared<CspElement>();
        e4->set_origin(receiver_origin);
        e4->set_aim_point(receiver_aim_point); // Aim direction
        e4->set_zrot(0.0); // No rotation for the receiver


        ///////////////////////////////////////////
        // STEP 2.2 create rectangle aperture    //
        ///////////////////////////////////////////

        double receiver_dim_x;
        double receiver_dim_y;

        Vec3d p1(-0.5, -0.4, 0.0); // Bottom-left vertex
        Vec3d p2(0.5, -0.3, 0.0);  // Bottom-right vertex
        Vec3d p3(0.0, 0.5, 0.0);   // Top vertex

        switch (receiver_type) {
            case RECEIVER_TYPE::CYLINDRICAL:
                std::cout << "Using cylindrical receiver" << std::endl;
                receiver_dim_x = 0.5;  // diameter of the receiver
                receiver_dim_y = 2.0;  // full height of the cylindrical receiver

                e4->set_aperture(std::make_shared<ApertureRectangle>(receiver_dim_x, receiver_dim_y));

                break;
            case RECEIVER_TYPE::FLAT:
                std::cout << "Using flat receiver" << std::endl;
                receiver_dim_x = 2.0; // width of the receiver
                receiver_dim_y = 2.0; // height of the receiver

                e4->set_aperture(std::make_shared<ApertureRectangle>(receiver_dim_x, receiver_dim_y));

                break;
            case RECEIVER_TYPE::TRIANGLE:
                std::cout << "Using triangular receiver" << std::endl;

                e4->set_aperture(std::make_shared<ApertureTriangle>(p1, p2, p3));

                break;
		}



        ///////////////////////////////////
        // STEP 2.3 create flat surface //
        //////////////////////////////////
        if (receiver_type == RECEIVER_TYPE::CYLINDRICAL) {
            // Create a cylindrical surface if use_cylindical is true
            auto receiver_surface = std::make_shared<SurfaceCylinder>();
            e4->set_surface(receiver_surface);
        }
        else {
            // Create a flat surface if not using cylindrical
            auto receiver_surface = std::make_shared<SurfaceFlat>();
            e4->set_surface(receiver_surface);
        }

        ////////////////////////////////////////////
        // STEP 2.4 Add the element to the system //
        ///////////////////////////////////////////
		e4->set_receiver(true); // Mark this element as a receiver
        system.add_element(e4); // Add the receiver to the system

    // create another element e5 same as e4
		CspElement e5 = *e4; // Copy e4 to e5
        Vec3d p4(1.0, 0.5, 0);
        e5.set_aperture(std::make_shared<ApertureTriangle>(p3, p2, p4));
		system.add_element(std::make_shared<CspElement>(e5)); // Add e5 to the system

    
    Vec3d sun_vector(0.0, 0.0, 100.0); // sun vector

    // set up sun angle 
    //double sun_angle = 0.00465; // 0.00465; // sun angle
    //double sun_angle = 0; // 0.00465; // sun angle

    //system.set_sun_angle(sun_angle);
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
    int num_hits = system.get_num_hits_receiver();
    std::cout << "Number of rays hitting the receiver: " << num_hits << std::endl;

	std::string out_dir = "out_three_heliostats/";
    if (!std::filesystem::exists(std::filesystem::path(out_dir))) {
        std::cout << "Creating output directory: " << out_dir << std::endl;
        if (!std::filesystem::create_directory(std::filesystem::path(out_dir))) {
            std::cerr << "Error creating directory " << out_dir << std::endl;
            return 1;
        }

	}

    system.write_hp_output(out_dir + "sun_error_hit_points_" + to_string(num_rays) + "_rays.csv");
    system.write_simulation_json(out_dir + "sun_error_summary_" + to_string(num_rays) + "_rays.json");

    /////////////////////////////////////////
    // STEP 6  Be a good citizen, clean up //
    /////////////////////////////////////////
    system.clean_up();



    return 0;
}