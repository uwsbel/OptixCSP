#pragma once
#include "device_util.h"
#ifndef __CUDACC_RTC__
#include <cassert>
#else
#define assert(x) /*nop*/
#endif

namespace OptixCSP {
   struct GeometryDataST
    {
        enum Type
        {
            PARALLELOGRAM = 0,
            CYLINDER_Y = 1,
            RECTANGLE_PARABOLIC = 2,
            UNKNOWN_TYPE = 3,
            RECTANGLE_FLAT = 4,
			TRIANGLE_FLAT = 5
        };

        struct Parallelogram
        {
            Parallelogram() = default;
            Parallelogram(float3 v1, float3 v2, float3 anchor)
                : v1(v1)
                , v2(v2)
                , anchor(anchor)
            {
                float3 normal = normalize(cross(v1, v2));
                float  d = dot(normal, anchor);
                this->v1 *= 1.0f / dot(v1, v1);
                this->v2 *= 1.0f / dot(v2, v2);
                plane = make_float4(normal, d);
            }
            float4 plane;
            float3 v1;
            float3 v2;
            float3 anchor;
        };

        // same as parallelogram, however defined with different attributes
        struct Rectangle_Flat
        {
            Rectangle_Flat() = default;
            Rectangle_Flat(float3 center, float3 x, float3 y, float width, float height)
                : center(center), x(x), y(y), width(width), height(height)
            {
                float3 normal = normalize(cross(x, y));
                float  d = dot(normal, center);
                plane = make_float4(normal, d);
            }

            float4 plane;
            float3 center;
            float3 x;
            float3 y;
            float width;
            float height;
        };

        struct Cylinder_Y {
            Cylinder_Y() = default;
            Cylinder_Y(float3 center, float radius, float half_height, float3 base_x, float3 base_z)
                : center(center)
                , radius(radius)
                , half_height(half_height)
                , base_x(base_x)
                , base_z(base_z) {
                assert(dot(base_x, base_z) < 1e-3f);
            }


            float3 center;
            float radius;
            float half_height;
            float3 base_x;   // x axis of the cylinder
            float3 base_z;   // z axis of the cylinder
        };

        struct Rectangle_Parabolic {

            Rectangle_Parabolic() = default;
            Rectangle_Parabolic(float3 v1, float3 v2, float3 anchor, float curv_x, float curv_y)
                : v1(v1)
                , v2(v2)
                , anchor(anchor)
                , curv_x(curv_x)
                , curv_y(curv_y)
            {
                float3 normal = normalize(cross(v1, v2));
                float d = dot(normal, anchor);
                this->v1 *= 1.0f / dot(v1, v1);
                this->v2 *= 1.0f / dot(v2, v2);
                plane = make_float4(normal, d);
            }

            float4 plane;
            float3 v1;
            float3 v2;
            float3 anchor;
            //float3 focus;
            float curv_x;
            float curv_y;
        };

        struct Triangle_Flat {
            Triangle_Flat() = default;
            Triangle_Flat(const float3& a, const float3& b, const float3& c)
                : v0(a), e1(b - a), e2(c - a)
            {
                normal = normalize(cross(e1, e2));
                d = dot(normal, v0);
            }
            float3 v0;     // base vertex
            float3 e1, e2; // edges
            float3 normal;
            float  d;      // plane distance
        };

        GeometryDataST() = default;

        void setParallelogram(const Parallelogram& p)
        {
            assert(type == UNKNOWN_TYPE);
            type = PARALLELOGRAM;
            parallelogram = p;
        }

        __host__ __device__ const Parallelogram& getParallelogram() const
        {
            assert(type == PARALLELOGRAM);
            return parallelogram;
        }

        void setRectangle_Flat(const Rectangle_Flat& r)
        {
            assert(type == UNKNOWN_TYPE);
            type = RECTANGLE_FLAT;
            rectangle_flat = r;
        }

        __host__ __device__ const Rectangle_Flat& getRectangle_Flat() const
        {
            assert(type == RECTANGLE_FLAT);
            return rectangle_flat;
        }

        void setCylinder_Y(const Cylinder_Y& c)
        {
            assert(type == UNKNOWN_TYPE);
            type = CYLINDER_Y;
            cylinder_y = c;
        }

        __host__ __device__ const Cylinder_Y& getCylinder_Y() const
        {
            assert(type == CYLINDER_Y);
            return cylinder_y;
        }

        void setRectangleParabolic(const Rectangle_Parabolic& r)
        {
            assert(type == UNKNOWN_TYPE);
            type = RECTANGLE_PARABOLIC;
            rectangle_parabolic = r;
        }

        __host__ __device__ const Rectangle_Parabolic& getRectangleParabolic() const
        {
            assert(type == RECTANGLE_PARABOLIC);
            return rectangle_parabolic;
        }

        void setTriangle_Flat(const Triangle_Flat& t)
        {
            assert(type == UNKNOWN_TYPE);
            type = TRIANGLE_FLAT;
            triangle_flat = t;
		}

        __host__ __device__ const Triangle_Flat& getTriangle_Flat() const
        {
            assert(type == TRIANGLE_FLAT);
            return triangle_flat;
		}


        Type type = UNKNOWN_TYPE;

    private:
        union
        {
            Parallelogram parallelogram;
            Cylinder_Y cylinder_y;
            Rectangle_Parabolic rectangle_parabolic;
            Rectangle_Flat rectangle_flat;
			Triangle_Flat triangle_flat;
        };
    };
}