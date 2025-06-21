#ifndef SOLTRACE_VECTOR3D_H
#define SOLTRACE_VECTOR3D_H


#include <iostream>
#include <cmath>

namespace OptixCSP {

    class Vector3d {
    public:
        // Constructors.
        Vector3d() : data{ 0.0, 0.0, 0.0 } {}
        Vector3d(double x, double y, double z) : data{ x, y, z } {}

        // Access operator (const and non-const).
        double operator[](int i) const { return data[i]; }
        double& operator[](int i) { return data[i]; }

        // Addition.
        Vector3d operator+(const Vector3d& rhs) const {
            return Vector3d(data[0] + rhs.data[0],
                data[1] + rhs.data[1],
                data[2] + rhs.data[2]);
        }

        // Subtraction.
        Vector3d operator-(const Vector3d& rhs) const {
            return Vector3d(data[0] - rhs.data[0],
                data[1] - rhs.data[1],
                data[2] - rhs.data[2]);
        }

        // Scalar multiplication.
        Vector3d operator*(double scalar) const {
            return Vector3d(data[0] * scalar,
                data[1] * scalar,
                data[2] * scalar);
        }

        // Scalar division.
        Vector3d operator/(double scalar) const {
            return Vector3d(data[0] / scalar,
                data[1] / scalar,
                data[2] / scalar);
        }

        // Dot product.
        double dot(const Vector3d& rhs) const {
            return data[0] * rhs.data[0] +
                data[1] * rhs.data[1] +
                data[2] * rhs.data[2];
        }

        // Cross product.
        Vector3d cross(const Vector3d& rhs) const {
            return Vector3d(data[1] * rhs.data[2] - data[2] * rhs.data[1],
                data[2] * rhs.data[0] - data[0] * rhs.data[2],
                data[0] * rhs.data[1] - data[1] * rhs.data[0]);
        }

        // Norm (magnitude).
        double norm() const {
            return std::sqrt(dot(*this));
        }

        // Normalized vector.
        Vector3d normalized() const {
            double n = norm();
            return (n > 0) ? (*this) / n : Vector3d();
        }

        // Output operator for printing.
        friend std::ostream& operator<<(std::ostream& os, const Vector3d& v) {
            os << "(" << v.data[0] << ", " << v.data[1] << ", " << v.data[2] << ")";
            return os;
        }

    private:
        double data[3];
    };

    // TODO: move this to a separate file 
    class Matrix33d
    {
    public:
        // constructors
        // Default constructor: identity matrix.
        Matrix33d() : data{ 1.0, 0.0, 0.0,
                            0.0, 1.0, 0.0,
                            0.0, 0.0, 1.0 } {
        }

        Matrix33d(const double arr[9]) {
            for (int i = 0; i < 9; ++i) {
                data[i] = arr[i];
            }
        }

        // constructor from nine individual values 
        Matrix33d(double a11, double a12, double a13,
            double a21, double a22, double a23,
            double a31, double a32, double a33) {
            data[0] = a11; data[1] = a12; data[2] = a13;
            data[3] = a21; data[4] = a22; data[5] = a23;
            data[6] = a31; data[7] = a32; data[8] = a33;
        }

        Matrix33d(std::initializer_list<double> init) {
            if (init.size() != 9)
                throw std::runtime_error("must have exactly 9 elements.");
            int i = 0;
            for (double val : init) {
                data[i++] = val;
            }
        }

        // A * A
        Matrix33d operator*(const Matrix33d& other) const {
            Matrix33d result;
            for (int row = 0; row < 3; ++row) {
                for (int col = 0; col < 3; ++col) {
                    result.data[row * 3 + col] = 0;
                    for (int k = 0; k < 3; ++k) {
                        result.data[row * 3 + col] += data[row * 3 + k] * other.data[k * 3 + col];
                    }
                }
            }
            return result;
        }

        // A * v
        Vector3d operator*(const Vector3d& v) const {
            return Vector3d(
                data[0] * v[0] + data[1] * v[1] + data[2] * v[2],
                data[3] * v[0] + data[4] * v[1] + data[5] * v[2],
                data[6] * v[0] + data[7] * v[1] + data[8] * v[2]
            );
        }

        // Access operator (const) 
        double operator()(int i, int j) const { return data[i * 3 + j]; } // row-major order

        // transpose 
        Matrix33d transpose() const {
            return Matrix33d(data[0], data[3], data[6],
                data[1], data[4], data[7],
                data[2], data[5], data[8]);
        }

        // for a rotation matrix, we care about basis vectors
        Vector3d get_x_basis() const {
            return Vector3d(data[0], data[3], data[6]);
        }
        Vector3d get_y_basis() const {
            return Vector3d(data[1], data[4], data[7]);
        }
        Vector3d get_z_basis() const {
            return Vector3d(data[2], data[5], data[8]);
        }


    private:
        double data[9];
    };

    // class Point3d
    // {
    // public:
    // private:
    // };
}
#endif
