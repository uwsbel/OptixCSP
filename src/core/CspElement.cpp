#include <cstdint>
#include <string>

#include "vec3d.h"
#include "soltrace_type.h"
#include "Surface.h"
#include "Aperture.h"
#include "utils/math_util.h"
#include "shaders/GeometryDataST.h"
#include "CspElement.h"
#include <vector>


using namespace OptixCSP;

CspElementBase::CspElementBase() {}

CspElement::CspElement() {
    m_origin = Vec3d(0.0, 0.0, 0.0);
    m_aim_point = Vec3d(0.0, 0.0, 1.0); // Default aim direction
    m_euler_angles = Vec3d(0.0, 0.0, 0.0); // Default orientation
    m_zrot = 0.0;
    m_surface = nullptr;
    m_aperture = nullptr;
    m_receiver = false;
}

// set and get origin 
const Vec3d& CspElement::get_origin() const{
    return m_origin;
}

void CspElement::set_origin(const Vec3d& o) {
    m_origin = o;
}

void CspElement::set_aim_point(const Vec3d& a) {
    m_aim_point = a;
}

const Vec3d& CspElement::get_aim_point() const {
    return m_aim_point;
}

void CspElement::set_zrot(double zrot) {
    m_zrot = zrot;
}

double CspElement::get_zrot() const {
    return m_zrot;
}


std::shared_ptr<Aperture> CspElement::get_aperture() const {
    return m_aperture;
}

std::shared_ptr<Surface> CspElement::get_surface() const {
    return m_surface;
}

ApertureType CspElement::get_aperture_type() const {
    return m_aperture->get_aperture_type();
}

SurfaceType CspElement::get_surface_type() const {
    return m_surface->get_surface_type();
}

// Optical CspElements setters.
void CspElement::set_aperture(const std::shared_ptr<Aperture>& aperture)
{
    m_aperture = aperture;
}
void CspElement::set_surface(const std::shared_ptr<Surface>& surface)
{
    m_surface = surface;
}

// set orientation based on aimpoint and zrot
void CspElement::update_euler_angles(const Vec3d& aim_point, const double zrot) {
    Vec3d normal = aim_point - m_origin;
    normal.normalized();
    m_euler_angles = OptixCSP::normal_to_euler(normal, zrot);
}

void CspElement::update_euler_angles() {
    Vec3d normal = m_aim_point - m_origin;
    normal.normalized();
    m_euler_angles = OptixCSP::normal_to_euler(normal, m_zrot);
}

void CspElement::update_element(const Vec3d& aim_point, const double zrot) {
    m_aim_point = aim_point;
    m_zrot = zrot;
    update_euler_angles();
}
// return L2G rotation matrix
Matrix33d CspElement::get_rotation_matrix() const {
    // get G2L rotation matrix from euler angles 
    // TODO: need to think about if we store this or not
    Matrix33d mat_G2L = OptixCSP::get_rotation_matrix_G2L(m_euler_angles);
    return mat_G2L.transpose();
}


// return upper bounding box
Vec3d CspElement::get_upper_bounding_box() const {
    return m_upper_box_bound;
}

// return lower bounding box
Vec3d CspElement::get_lower_bounding_box() const {
    return m_lower_box_bound;
}


GeometryDataST CspElement::toDeviceGeometryData() const {

    GeometryDataST geometry_data;

    SurfaceType surface_type = m_surface->get_surface_type();
    ApertureType aperture_type = m_aperture->get_aperture_type();

    if (aperture_type == ApertureType::RECTANGLE) {

        double width = m_aperture->get_width();
        double height = m_aperture->get_height();

        Matrix33d rotation_matrix = get_rotation_matrix();  // L2G rotation matrix

        Vec3d v1 = rotation_matrix.get_x_basis();
        Vec3d v2 = rotation_matrix.get_y_basis();

        if (surface_type == SurfaceType::FLAT) {
            GeometryDataST::Rectangle_Flat heliostat(OptixCSP::toFloat3(m_origin), OptixCSP::toFloat3(v1), OptixCSP::toFloat3(v2), (float)width, (float)height);
            geometry_data.setRectangle_Flat(heliostat);
        }

        if (surface_type == SurfaceType::PARABOLIC) {
			v1 = v1 * (float)(-width);
			v2 = v2 * (float)height;
			float3 anchor = OptixCSP::toFloat3(m_origin - v1 * 0.5 - v2 * 0.5);
            GeometryDataST::Rectangle_Parabolic heliostat(OptixCSP::toFloat3(v1), OptixCSP::toFloat3(v2),  anchor,
                (float)m_surface->get_curvature_1(),
                (float)m_surface->get_curvature_2());
            geometry_data.setRectangleParabolic(heliostat);
        }

		if (surface_type == SurfaceType::CYLINDER) {
            float radius = static_cast<float>(width) / 2.0f;
            float half_height = static_cast<float>(height) / 2.0f;

			float3 center = OptixCSP::toFloat3(m_origin);
			Matrix33d rotation_matrix = get_rotation_matrix();  // L2G rotation matrix

			float3 base_x = OptixCSP::toFloat3(rotation_matrix.get_x_basis());

			float3 base_z = OptixCSP::toFloat3(rotation_matrix.get_z_basis());

			GeometryDataST::Cylinder_Y heliostat(center, radius, half_height, base_x, base_z);

			geometry_data.setCylinder_Y(heliostat);
		}
    }

    if (aperture_type == ApertureType::TRIANGLE) {

		Vec3d v1, v2, v3;
		// first cast to ApertureTriangle type
		ApertureTriangle tri = static_cast<ApertureTriangle&>(*m_aperture);

		v1 = tri.get_v0();
		v2 = tri.get_v1();
		v3 = tri.get_v2();

		// given the origin and rotation, compute global coordinates of the triangle vertices
		Matrix33d rotation_matrix = get_rotation_matrix();  // L2G rotation matrix
		Vec3d v1_global = rotation_matrix * v1 + m_origin;
		Vec3d v2_global = rotation_matrix * v2 + m_origin;
		Vec3d v3_global = rotation_matrix * v3 + m_origin;

		GeometryDataST::Triangle_Flat heliostat(OptixCSP::toFloat3(v1_global), OptixCSP::toFloat3(v2_global), OptixCSP::toFloat3(v3_global));
		geometry_data.setTriangle_Flat(heliostat);        
    }

    return geometry_data;
}


// we also need to implement the bounding box computation
// for a case like a rectangle aperture,
// once we have the origin, euler angles, rotatioin matrix
// and the aperture size, we can compute the bounding box
// this can be called when adding an element to the system
void CspElement::compute_bounding_box() {
    // this can also be called while "initializing" the element
    // get the rotation matrix first
    Matrix33d rotation_matrix = get_rotation_matrix();  // L2G rotation matrix

    // now check the type of the aperture
    ApertureType aperture_type = m_aperture->get_aperture_type();
	SurfaceType surface_type = m_surface->get_surface_type();

    if (aperture_type == ApertureType::RECTANGLE && surface_type != SurfaceType::CYLINDER) {
        // get the width and height of the aperture
        double width = m_aperture->get_width();
        double height = m_aperture->get_height();

        // compute the four corners of the rectangle locally
        Vec3d corner1 = Vec3d(-width / 2, -height / 2, 0.0);
        Vec3d corner2 = Vec3d( width / 2, -height / 2, 0.0);
        Vec3d corner3 = Vec3d( width / 2,  height / 2, 0.0);
        Vec3d corner4 = Vec3d(-width / 2,  height / 2, 0.0);

        // transform the corners to the global frame
        Vec3d corner1_global = rotation_matrix * corner1 + m_origin;
        Vec3d corner2_global = rotation_matrix * corner2 + m_origin;
        Vec3d corner3_global = rotation_matrix * corner3 + m_origin;
        Vec3d corner4_global = rotation_matrix * corner4 + m_origin;

        // now update the bounding box, need to find the min and max x, y, z
        m_lower_box_bound[0] = fmin(fmin(corner1_global[0], corner2_global[0]), fmin(corner3_global[0], corner4_global[0]));
        m_lower_box_bound[1] = fmin(fmin(corner1_global[1], corner2_global[1]), fmin(corner3_global[1], corner4_global[1]));
        m_lower_box_bound[2] = fmin(fmin(corner1_global[2], corner2_global[2]), fmin(corner3_global[2], corner4_global[2]));

        m_upper_box_bound[0] = fmax(fmax(corner1_global[0], corner2_global[0]), fmax(corner3_global[0], corner4_global[0]));
        m_upper_box_bound[1] = fmax(fmax(corner1_global[1], corner2_global[1]), fmax(corner3_global[1], corner4_global[1]));
        m_upper_box_bound[2] = fmax(fmax(corner1_global[2], corner2_global[2]), fmax(corner3_global[2], corner4_global[2]));
    }

    // slightly different for the cylinder, we want to know the radius and half height
	if (surface_type == SurfaceType::CYLINDER) {
		// get the radius and full height of the cylinder
        double width  = m_aperture->get_width();
        double height = m_aperture->get_height();

        // compute 8 corners of the cyliinder box locally
		Vec3d corner1 = Vec3d(-width / 2, -height / 2, -width / 2);
		Vec3d corner2 = Vec3d( width / 2, -height / 2, -width / 2);
		Vec3d corner3 = Vec3d(-width / 2,  height / 2, -width / 2);
		Vec3d corner4 = Vec3d( width / 2,  height / 2, -width / 2);
        Vec3d corner5 = Vec3d(-width / 2, -height / 2,  width / 2);
        Vec3d corner6 = Vec3d( width / 2, -height / 2,  width / 2);
        Vec3d corner7 = Vec3d(-width / 2,  height / 2,  width / 2);
        Vec3d corner8 = Vec3d( width / 2,  height / 2,  width / 2);

		// get the rotation matrix
		Matrix33d rotation_matrix = get_rotation_matrix();  // L2G rotation matrix

		// transform the corners to the global frame
		Vec3d corner1_global = rotation_matrix * corner1 + m_origin;
		Vec3d corner2_global = rotation_matrix * corner2 + m_origin;
		Vec3d corner3_global = rotation_matrix * corner3 + m_origin;
		Vec3d corner4_global = rotation_matrix * corner4 + m_origin;
		Vec3d corner5_global = rotation_matrix * corner5 + m_origin;
		Vec3d corner6_global = rotation_matrix * corner6 + m_origin;
		Vec3d corner7_global = rotation_matrix * corner7 + m_origin;
		Vec3d corner8_global = rotation_matrix * corner8 + m_origin;

		// go through the corners and find the min and max x, y, z
		std::vector<Vec3d> corners = { corner1_global, corner2_global, corner3_global, corner4_global,
					corner5_global, corner6_global, corner7_global, corner8_global };

		double min_x = std::numeric_limits<double>::max();
		double min_y = std::numeric_limits<double>::max();
		double min_z = std::numeric_limits<double>::max();

		double max_x = std::numeric_limits<double>::lowest();
		double max_y = std::numeric_limits<double>::lowest();
		double max_z = std::numeric_limits<double>::lowest();

		for (auto& corner : corners) {
			min_x = fmin(min_x, corner[0]);
			min_y = fmin(min_y, corner[1]);
			min_z = fmin(min_z, corner[2]);

			max_x = fmax(max_x, corner[0]);
			max_y = fmax(max_y, corner[1]);
			max_z = fmax(max_z, corner[2]);
		}

		// set the lower and upper bounds
		m_lower_box_bound[0] = min_x;
		m_lower_box_bound[1] = min_y;
		m_lower_box_bound[2] = min_z;

		m_upper_box_bound[0] = max_x;
		m_upper_box_bound[1] = max_y;
		m_upper_box_bound[2] = max_z;
	}


    // bounding box for triangle aperture
    if (aperture_type == ApertureType::TRIANGLE) {
        // get the three vertices of the triangle aperture in local coordinates
		// first cast to ApertureTriangle type
		ApertureTriangle tri = static_cast<ApertureTriangle&>(*m_aperture);

        Vec3d v1 = tri.get_v0();
        Vec3d v2 = tri.get_v1();
        Vec3d v3 = tri.get_v2();
        // transform the vertices to the global frame
        Vec3d v1_global = rotation_matrix * v1 + m_origin;
        Vec3d v2_global = rotation_matrix * v2 + m_origin;
        Vec3d v3_global = rotation_matrix * v3 + m_origin;
        // now update the bounding box, need to find the min and max x, y, z
        m_lower_box_bound[0] = fmin(fmin(v1_global[0], v2_global[0]), v3_global[0]);
        m_lower_box_bound[1] = fmin(fmin(v1_global[1], v2_global[1]), v3_global[1]);
        m_lower_box_bound[2] = fmin(fmin(v1_global[2], v2_global[2]), v3_global[2]);
        m_upper_box_bound[0] = fmax(fmax(v1_global[0], v2_global[0]), v3_global[0]);
        m_upper_box_bound[1] = fmax(fmax(v1_global[1], v2_global[1]), v3_global[1]);
        m_upper_box_bound[2] = fmax(fmax(v1_global[2], v2_global[2]), v3_global[2]);
	}




}