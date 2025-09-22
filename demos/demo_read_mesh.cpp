// this is for reading a triangular obj mesh file and using each mesh triangle as an aperture for the receiver

#include "core/soltrace_system.h"
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <sstream>

using namespace std;
using namespace OptixCSP;


class mesh_receiver {
public: 
    mesh_receiver(std::string& m_mesh_file) {
        
		std::ifstream file(m_mesh_file);

        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file: " + m_mesh_file);
        }

        std::vector<Vec3d> vertices;
        std::vector<int> vertex_id;

        std::string line;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string prefix;
            iss >> prefix;

            if (prefix == "v") {
                // Vertex line
                double x, y, z;
                iss >> x >> y >> z;
                vertices.emplace_back(x, y, z);
            }
            else if (prefix == "f") {
                // Face line
				int id;
                char slash; // To handle "v/vt/vn" format
                for (int i = 0; i < 3; ++i) {
                    std::string token;
                    iss >> token; // Read the entire token (e.g., "1//1")

                    // Find the position of the first '/'
                    size_t slash_pos = token.find('/');
                    if (slash_pos != std::string::npos) {
                        // Extract the part before the first '/'
                        id = std::stoi(token.substr(0, slash_pos));
                    }
                    else {
                        // If no '/' is found, the token is just the vertex index
                        id = std::stoi(token);
                    }

                    // OBJ indices are 1-based, convert to 0-based
                    vertex_id.push_back(id - 1);
                }
            }
        }

        file.close();

		num_triangles = vertex_id.size() / 3;

        // Compute normals for each face
        for (int i = 0; i < num_triangles; i++) {
            const Vec3d& v0 = vertices[vertex_id[3 * i]];
			const Vec3d& v1 = vertices[vertex_id[3 * i + 1]];
			const Vec3d& v2 = vertices[vertex_id[3 * i + 2]];

            Vec3d edge1 = v1 - v0;
            Vec3d edge2 = v2 - v0;
            Vec3d normal = edge1.cross(edge2).normalized();

			vertices_v0.push_back(v0);
			vertices_v1.push_back(v1);
            vertices_v2.push_back(v2);
			normals.push_back(normal);

        }
        
	}

	Vec3d get_v0(int i) { return vertices_v0[i]; }
	Vec3d get_v1(int i) { return vertices_v1[i]; }
	Vec3d get_v2(int i) { return vertices_v2[i]; }
	Vec3d get_normal(int i) { return normals[i]; }
	int get_num_triangles() { return num_triangles; }

    void scale(double scale) {
        for (auto& v : vertices_v0) v = v * scale;
        for (auto& v : vertices_v1) v = v * scale;
		for (auto& v : vertices_v2) v = v * scale;

    }

    private:
        std::vector<Vec3d> vertices_v0;
		std::vector<Vec3d> vertices_v1;
		std::vector<Vec3d> vertices_v2;
		std::vector<Vec3d> normals;
		int num_triangles;


};



int main(int argc, char* argv[]) {
    int num_rays = 1000000;


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
        // Create a flat surface if not parabolic
        auto surface_e1 = std::make_shared<SurfaceFlat>();
        e1->set_surface(surface_e1);


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

        // Create a flat surface if not parabolic
        auto surface_e2 = std::make_shared<SurfaceFlat>();
        e2->set_surface(surface_e2);

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

        // Create a flat surface if not parabolic
        auto surface_e3 = std::make_shared<SurfaceFlat>();
        e3->set_surface(surface_e3);

        auto aperture_e3 = std::make_shared<ApertureRectangle>(dim_x, dim_y);
        e3->set_aperture(aperture_e3);

        system.add_element(e3);


        //////////////////////////////////////////////
        // STEP 2.1 Create receiver, flat rectangle //
        //////////////////////////////////////////////
        Vec3d receiver_origin(0, 0, 10.0); // origin of the receiver


		std::string mesh_file = "../data/sphere.obj";
		mesh_receiver receiver(mesh_file);
		int num_triangles = receiver.get_num_triangles();
		std::cout << "Number of triangles in the mesh: " << num_triangles << std::endl;

		for (int i = 0; i < num_triangles; i++) {
            Vec3d p1 = receiver.get_v0(i);
            Vec3d p2 = receiver.get_v1(i);
            Vec3d p3 = receiver.get_v2(i);
            Vec3d normal = receiver.get_normal(i);
            Vec3d receiver_aim_point = receiver_origin + normal; // aim point of the receiver
            auto e4 = std::make_shared<CspElement>();
            e4->set_origin(receiver_origin);
            e4->set_aim_point(receiver_aim_point); // Aim direction
            e4->set_zrot(0.0); // No rotation for the receiver
            // Create a triangular aperture
            e4->set_aperture(std::make_shared<ApertureTriangle>(p1, p2, p3));
            // Create a flat surface
            auto receiver_surface = std::make_shared<SurfaceFlat>();
            e4->set_surface(receiver_surface);
            e4->set_receiver(true); // Mark this element as a receiver
            system.add_element(e4); // Add the receiver to the system
		}


    Vec3d sun_vector(0.0, 0.0, 100.0); // sun vector

    // set up sun angle 
    //double sun_angle = 0.00465; // 0.00465; // sun angle
    double sun_angle = 0; // 0.00465; // sun angle

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
    int num_hits = system.get_num_hits_receiver();
    std::cout << "Number of rays hitting the receiver: " << num_hits << std::endl;

	std::string out_dir = "out_mesh_receiver/";
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