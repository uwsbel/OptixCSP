#pragma once

#include <cmath>
#include <vector_types.h>
#include <vector_functions.h>
#include "Vector3d.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace OptixCSP {

/**
 * Convert normal vector and z-rot to Euler angles (yaw-pitch-roll)
 * @param normal normal direction vector (dx, dy, dz)
 * @param zrot Z-axis rotation in degrees
 * @return Vector3d containing (yaw, pitch, roll) in degrees
 */
inline Vector3d normal_to_euler(const Vector3d& normal, double zrot) {
    // Ensure normal is normalized
    Vector3d n = normal.normalized();
    
    // Calculate Euler angles (SolTrace convention)
    double yaw = atan2(n[0], n[2]);    // Rotation about Y axis (alpha)
    double pitch = asin(n[1]);         // Rotation about X axis (beta)
    double roll = zrot * M_PI/180.0;   // Rotation about Z axis (gamma), convert to radians
    
    // Convert to degrees
    return Vector3d(yaw, pitch, roll);
}

/**
 * Build rotation matrix from Euler angles (Spencer-Murty convention)
 * Returns global-to-local transformation matrix
 * 
 * @param euler Euler angles (yaw, pitch, roll) in degrees
 * @return Matrix33d rotation matrix (global to local)
 */
inline Matrix33d get_rotation_matrix_G2L(const Vector3d& euler) {
    // Convert to radians
    double alpha = euler[0];  // yaw 
    double beta  = euler[1];  // pitch
    double gamma = euler[2];  // zrot
    
    // Precompute sines and cosines
    double ca = cos(alpha), sa = sin(alpha);
    double cb = cos(beta),  sb = sin(beta);
    double cg = cos(gamma), sg = sin(gamma);
    
    // Fill in elements of the transformation matrix (global to local) 
    // as per Spencer and Murty paper page 673 equation (2)
    return Matrix33d{
        ca*cg + sa*sb*sg, -cb*sg, -sa*cg + ca*sb*sg,
        ca*sg - sa*sb*cg,  cb*cg, -sa*sg - ca*sb*cg,
        sa*cb,             sb,     ca*cb
    };
}

/**
 * Transform a point from local to global coordinates
 * 
 * @param point Point in local coordinates
 * @param matrix Global-to-local rotation matrix
 * @param origin Origin of local coordinate system in global coordinates
 * @return Vector3d point in global coordinates
 */
inline Vector3d local_to_global(const Vector3d& point, const Matrix33d& matrix, const Vector3d& origin) {
    // Transpose is handled implicitly by how we access the matrix elements
    Vector3d rotated(
        matrix(0,0)*point[0] + matrix(1,0)*point[1] + matrix(2,0)*point[2],
        matrix(0,1)*point[0] + matrix(1,1)*point[1] + matrix(2,1)*point[2],
        matrix(0,2)*point[0] + matrix(1,2)*point[1] + matrix(2,2)*point[2]
    );
    
    // Translate to global position
    return rotated + origin;
}

/**
 * Transform a point from global to local coordinates
 * 
 * @param point Point in global coordinates
 * @param matrix Global-to-local rotation matrix
 * @param origin Origin of local coordinate system in global coordinates
 * @return Vector3d point in local coordinates
 */
inline Vector3d global_to_local(const Vector3d& point, const Matrix33d& matrix, const Vector3d& origin) {
    // Translate to origin
    Vector3d translated = point - origin;
    
    // Apply global-to-local rotation
    return Vector3d(
        matrix(0,0)*translated[0] + matrix(0,1)*translated[1] + matrix(0,2)*translated[2],
        matrix(1,0)*translated[0] + matrix(1,1)*translated[1] + matrix(1,2)*translated[2],
        matrix(2,0)*translated[0] + matrix(2,1)*translated[1] + matrix(2,2)*translated[2]
    );
}

/* 
 * convert Vector3d to float3
 * @param v Vector3d to convert
 * @return float3
 */
inline float3 toFloat3(const Vector3d& v) {
    return make_float3(static_cast<float>(v[0]), static_cast<float>(v[1]), static_cast<float>(v[2]));
}

}