#include "Aperture.h"

using namespace OptixCSP;

class Element;

// Aperture base class implementations
Aperture::Aperture() = default;

double Aperture::get_width() const { return 0.0; }
double Aperture::get_height() const { return 0.0; }
double Aperture::get_radius() const { return 0.0; }


// ApertureCircle implementations
ApertureCircle::ApertureCircle() : radius(1.0) {}
ApertureCircle::ApertureCircle(double r) : radius(r) {}

ApertureType ApertureCircle::get_aperture_type() const {
    return ApertureType::CIRCLE;
}

void ApertureCircle::set_size(double r) { 
    radius = r; 
}

double ApertureCircle::get_radius() const { 
    return radius; 
}

double ApertureCircle::get_width() const { 
    return 2.0 * radius; 
}

double ApertureCircle::get_height() const { 
    return 2.0 * radius; 
}

// ApertureRectangleEasy implementations
ApertureRectangle::ApertureRectangle() : x_dim(1.0), y_dim(1.0) {}
ApertureRectangle::ApertureRectangle(double xDim, double yDim) : x_dim(xDim), y_dim(yDim) {}

ApertureType ApertureRectangle::get_aperture_type() const {
    return ApertureType::RECTANGLE;
}

//float3 ApertureRectangle::get_origin() { return m_origin; }
//float3 ApertureRectangle::get_v1() { return m_x_axis; }
//float3 ApertureRectangle::get_v2() { return m_y_axis; }
double ApertureRectangle::get_width() const { return x_dim; }
double ApertureRectangle::get_height() const { return y_dim; }
 

// Aperture Triangle
ApertureTriangle::ApertureTriangle(Vec3d v0, Vec3d v1, Vec3d v2) : m_v0(v0), m_v1(v1), m_v2(v2) {}
ApertureTriangle::ApertureTriangle() : m_v0(0, 0, 0), m_v1(1, 0, 0), m_v2(0, 1, 0) {}
ApertureType ApertureTriangle::get_aperture_type() const { 
    return ApertureType::TRIANGLE; 
}

Vec3d ApertureTriangle::get_v0() const { return m_v0; }
Vec3d ApertureTriangle::get_v1() const { return m_v1; }
Vec3d ApertureTriangle::get_v2() const { return m_v2; }


