#include <optix.h>
//#include <cuda/helpers.h>
#include "Soltrace.h"
#include <stdio.h>
#include "GeometryDataST.h"

extern "C" {
    __constant__ OptixCSP::LaunchParams params;
}


extern "C" __global__ void __intersection__parallelogram()
{
	int i = optixGetPrimitiveIndex();
    const OptixCSP::GeometryDataST::Parallelogram& parallelogram = params.geometry_data_array[i].getParallelogram();
        
    // Get ray information: origin, direction, and min/max distances over which ray should be tested
    const float3 ray_orig = optixGetWorldRayOrigin();
    const float3 ray_dir  = optixGetWorldRayDirection();
    const float  ray_tmin = optixGetRayTmin(), ray_tmax = optixGetRayTmax();

    // Compute ray intersection point
    float3 n  = make_float3( parallelogram.plane );
    float  dt = dot( ray_dir, n );
    // Compute distance t (point of intersection) along ray direction from ray origin
    float  t  = ( parallelogram.plane.w - dot( n, ray_orig ) ) / dt;

    // Verify intersection distance and Report ray intersection point
    if( t > ray_tmin && t < ray_tmax )
    {
        float3 p  = ray_orig + ray_dir * t;
        float3 vi = p - parallelogram.anchor;
        float  a1 = dot( parallelogram.v1, vi );
        if( a1 >= 0 && a1 <= 1 )
        {
            float a2 = dot( parallelogram.v2, vi );
            if( a2 >= 0 && a2 <= 1 )
            {
                optixReportIntersection( t, 
                    0, 
                    __float_as_uint( n.x ), 
                    __float_as_uint( n.y ), 
                    __float_as_uint( n.z ));
            }
        }
    }
}

extern "C" __global__ void __intersection__rectangle_flat()
{

	const OptixCSP::GeometryDataST::Rectangle_Flat& rectangle = params.geometry_data_array[optixGetPrimitiveIndex()].getRectangle_Flat();
        
    const float3 ray_orig = optixGetWorldRayOrigin();
    const float3 ray_dir = optixGetWorldRayDirection();
    const float ray_tmin = optixGetRayTmin();
    const float ray_tmax = optixGetRayTmax();

    // Get plane normal and distance
    float3 n = make_float3(rectangle.plane);
    float dt = dot(ray_dir, n);
    
    // Compute distance t (point of intersection) along ray direction from ray origin
    float t = (rectangle.plane.w - dot(n, ray_orig)) / dt;

    // Verify intersection distance
    if (t > ray_tmin && t < ray_tmax)
    {
        // Compute intersection point
        float3 p = ray_orig + ray_dir * t;
        
        // Compute vector from center to intersection point
        float3 v = p - rectangle.center;
        
        // Project onto x and y to get local coordinates
        float x = dot(rectangle.x, v);
        float y = dot(rectangle.y, v);
        
        // Check if point is within rectangle bounds
        if (x >= -rectangle.width/2 && x <= rectangle.width/2 &&
            y >= -rectangle.height/2 && y <= rectangle.height/2)
        {
            optixReportIntersection(t,
                0,
                __float_as_uint(n.x),
                __float_as_uint(n.y),
                __float_as_uint(n.z));
        }
    }
}

extern "C" __global__ void __intersection__cylinder_y()
{
	const OptixCSP::GeometryDataST::Cylinder_Y& cyl = params.geometry_data_array[optixGetPrimitiveIndex()].getCylinder_Y();

    // Get ray information: origin, direction, and min/max distances over which ray should be tested
    const float3 ray_orig = optixGetWorldRayOrigin();
    const float3 ray_dir = normalize(optixGetWorldRayDirection());
    const float  ray_tmin = optixGetRayTmin();
    const float  ray_tmax = optixGetRayTmax();

    // Transform ray to the cylinder's local coordinate system
    float3 local_ray_orig = ray_orig - cyl.center;
    float3 local_ray_dir = ray_dir;

	// TODO: check how to optimize this, there should be a way in optix to rotate coordinates 
    float3 local_x = cyl.base_x;
    float3 local_z = cyl.base_z;
    float3 local_y = cross(local_z, local_x);

    local_ray_orig = make_float3(
        dot(local_ray_orig, local_x),
        dot(local_ray_orig, local_y),
        dot(local_ray_orig, local_z)
    );
    local_ray_dir = make_float3(
        dot(local_ray_dir, local_x),
        dot(local_ray_dir, local_y),
        dot(local_ray_dir, local_z)
    );

	// solve quadratic equation for intersection
    float A = local_ray_dir.x * local_ray_dir.x + local_ray_dir.z * local_ray_dir.z;
    float B = 2.0f * (local_ray_orig.x * local_ray_dir.x + local_ray_orig.z * local_ray_dir.z);
    float C = local_ray_orig.x * local_ray_orig.x + local_ray_orig.z * local_ray_orig.z - cyl.radius * cyl.radius;

    float determinant = B * B - 4.0f * A * C;

    if (determinant < 0.0f)
    {
        // No intersection
        return;
    }

    // Compute intersection distances
    float t1 = (-B - sqrtf(determinant)) / (2.0f * A);
    float t2 = (-B + sqrtf(determinant)) / (2.0f * A);

    float t = t1 > 0.0f ? t1 : t2; // Use the closer valid intersection
    if (t < ray_tmin || t > ray_tmax)
    {
        // Intersection is out of bounds
        return;
    }

    // Compute intersection point in local space
    float3 local_hit_point = local_ray_orig + t * local_ray_dir;

    // Check if the hit point is within the cylinder's height bounds
    if (fabsf(local_hit_point.y) > cyl.half_height)
    {
        // If t1 is invalid, try t2
        t = t2;
        local_hit_point = local_ray_orig + t * local_ray_dir;
        if (t < ray_tmin || t > ray_tmax || fabsf(local_hit_point.y) > cyl.half_height)
        {
            return; // Both intersections are out of bounds
        }
    }

    // Compute normal in local coordinates
    float3 local_normal = normalize(make_float3(local_hit_point.x, 0.0f, local_hit_point.z));

    // Transform normal back to world coordinates
    float3 world_normal = local_normal.x * local_x + local_normal.y * local_y + local_normal.z * local_z;

    // Compute the hit point in world space
    float3 world_hit_point = ray_orig + t * ray_dir;

    // Report intersection to OptiX
    optixReportIntersection(t,
        0,
        __float_as_uint(world_normal.x),
        __float_as_uint(world_normal.y),
        __float_as_uint(world_normal.z));
}

// ray cylinder intersection with top and bottom caps 
// it can also be modeled as cylinder with two disks. 
extern "C" __global__ void __intersection__cylinder_y_capped()
{
	const OptixCSP::GeometryDataST::Cylinder_Y& cyl = params.geometry_data_array[optixGetPrimitiveIndex()].getCylinder_Y();

    // Get ray information: origin, direction, and min/max distances over which ray should be tested
    const float3 ray_orig = optixGetWorldRayOrigin();
    const float3 ray_dir = normalize(optixGetWorldRayDirection());
    const float  ray_tmin = optixGetRayTmin();
    const float  ray_tmax = optixGetRayTmax();

    // Transform ray to the cylinder's local coordinate system
    float3 local_ray_orig = ray_orig - cyl.center;
    float3 local_ray_dir = ray_dir;

    // Transform using the cylinder's local basis
    float3 local_x = cyl.base_x;
    float3 local_z = cyl.base_z;
    float3 local_y = cross(local_z, local_x);

    local_ray_orig = make_float3(
        dot(local_ray_orig, local_x),
        dot(local_ray_orig, local_y),
        dot(local_ray_orig, local_z)
    );
    local_ray_dir = make_float3(
        dot(local_ray_dir, local_x),
        dot(local_ray_dir, local_y),
        dot(local_ray_dir, local_z)
    );

    // Solve quadratic equation for intersection with curved surface
    float A = local_ray_dir.x * local_ray_dir.x + local_ray_dir.z * local_ray_dir.z;
    float B = 2.0f * (local_ray_orig.x * local_ray_dir.x + local_ray_orig.z * local_ray_dir.z);
    float C = local_ray_orig.x * local_ray_orig.x + local_ray_orig.z * local_ray_orig.z - cyl.radius * cyl.radius;

    float determinant = B * B - 4.0f * A * C;

    float t_curved = ray_tmax + 1.0f; // Initialize to invalid
    if (determinant >= 0.0f)
    {
        // Compute intersection distances
        float t1 = (-B - sqrtf(determinant)) / (2.0f * A);
        float t2 = (-B + sqrtf(determinant)) / (2.0f * A);

        // Select the closest valid intersection within bounds
        if (t1 > ray_tmin && t1 < ray_tmax && fabsf(local_ray_orig.y + t1 * local_ray_dir.y) <= cyl.half_height)
        {
            t_curved = t1;
        }
        else if (t2 > ray_tmin && t2 < ray_tmax && fabsf(local_ray_orig.y + t2 * local_ray_dir.y) <= cyl.half_height)
        {
            t_curved = t2;
        }
    }

    // Check intersection with top and bottom caps
    float t_caps = ray_tmax + 1.0f;
    {
        // Bottom cap: y = -half_height
        if (fabsf(local_ray_dir.y) > 1e-6f) // Avoid division by zero
        {
            float t = (-cyl.half_height - local_ray_orig.y) / local_ray_dir.y;
            float2 hit_point = make_float2(local_ray_orig.x + t * local_ray_dir.x,
                local_ray_orig.z + t * local_ray_dir.z);
            if (t > ray_tmin && t < ray_tmax && dot(hit_point, hit_point) <= cyl.radius * cyl.radius)
            {
                t_caps = t;
            }
        }

        // Top cap: y = +half_height
        if (fabsf(local_ray_dir.y) > 1e-6f)
        {
            float t = (cyl.half_height - local_ray_orig.y) / local_ray_dir.y;
            float2 hit_point = make_float2(local_ray_orig.x + t * local_ray_dir.x,
                local_ray_orig.z + t * local_ray_dir.z);
            if (t > ray_tmin && t < ray_tmax && dot(hit_point, hit_point) <= cyl.radius * cyl.radius)
            {
                t_caps = fminf(t_caps, t);
            }
        }
    }

    // Use the closest valid intersection
    float t = fminf(t_curved, t_caps);
    if (t >= ray_tmax || t <= ray_tmin)
    {
        return; // No valid intersection
    }

    // Compute intersection point and normal
    float3 local_hit_point = local_ray_orig + t * local_ray_dir;
    float3 local_normal;

    if (t == t_curved)
    {
        // Hit on the curved surface
        local_normal = normalize(make_float3(local_hit_point.x, 0.0f, local_hit_point.z));
    }
    else
    {
        // Hit on one of the caps
        local_normal = make_float3(0.0f, signbit(local_hit_point.y) ? -1.0f : 1.0f, 0.0f);
    }

    // Transform normal back to world coordinates
    float3 world_normal = local_normal.x * local_x + local_normal.y * local_y + local_normal.z * local_z;

    // Compute world-space hit point
    float3 world_hit_point = ray_orig + t * ray_dir;

    // Report intersection to OptiX
    optixReportIntersection(
        t,
        0, // User-defined instance ID or custom data
        __float_as_uint(world_normal.x),
        __float_as_uint(world_normal.y),
        __float_as_uint(world_normal.z)
    );
}


// For a parabolic surface rectangle aperture where 
// the base (normal projection) is defined by the center and its two unit edge vectors
// In a local coordinate system (with origin at the anchor) the flat rectangle covers:
//    x in [0, L1]  and  y in [0, L2],
// where L1 and L2 are the lengths of the original edge vectors.
// The parabolic surface is given by:
//    z = (curv_x/2)*x^2 + (curv_y/2)*y^2
// and the ray (in local coordinates) is:
//    (ox,oy,oz) + t*(dx,dy,dz)
// We solve for t such that:
//    oz + t*dz = (curv_x/2)*(ox+t*dx)^2 + (curv_y/2)*(oy+t*dy)^2
// which expands into a quadratic: A*t^2 + B*t + C = 0.
// After finding the valid t, we compute the local hit (x,y) and then check that
//   0 <= x <= L1   and   0 <= y <= L2.
// Finally, we compute the surface normal from the paraboloid derivative
//    f_x = curv_x * x    and    f_y = curv_y * y,
// so that the (unnormalized) local normal is (-f_x, -f_y, 1).
//
// The local hit point is then transformed back to world space for reporting.
extern "C" __global__ void __intersection__rectangle_parabolic()
{
    const OptixCSP::GeometryDataST::Rectangle_Parabolic& rect = params.geometry_data_array[optixGetPrimitiveIndex()].getRectangleParabolic();
    // Get ray information.
    const float3 ray_orig = optixGetWorldRayOrigin();
    const float3 ray_dir = optixGetWorldRayDirection();
    const float  ray_tmin = optixGetRayTmin();
    const float  ray_tmax = optixGetRayTmax();

    //
    // Build the local coordinate system.
    //
    // We assume that the rectangle was defined with an anchor at its corner and
    // two edge vectors. The stored rect.v1 and rect.v2 are the reciprocals:
    //     stored_v1 = original_v1 / dot(original_v1, original_v1)
    // Thus, the original edge lengths are:
    // Note rect.v1 had the size of 1/original_v1_length
    float L1 = 1.0f / length(rect.v1);
    float L2 = 1.0f / length(rect.v2);
    // And the unit edge directions are:
    float3 e1 = rect.v1 * L1; // recovers the original direction of edge 1, unit vector
    float3 e2 = rect.v2 * L2; // recovers the original direction of edge 2, unit ve
    // The flat (undeformed) rectangle's normal is:
    float3 n = normalize(cross(e2, e1));

    //
    // Transform ray into local coordinates.
    // The local coordinates (x,y,z) are defined such that:
    //   - The origin is at rect.anchor.
    //   - The x-axis is e1.
    //   - The y-axis is e2.
    //   - The z-axis is n.
    //
    // Compute the rectangle center (shifting from the lower-right corner)
    float3 rect_center = rect.anchor + (L1 / 2.0f) * e1 + (L2 / 2.0f) * e2;

    float3 d = ray_orig - rect_center;
    float ox = dot(d, e1);
    float oy = dot(d, e2);
    float oz = dot(d, n);

    float dx = dot(ray_dir, e1);
    float dy = dot(ray_dir, e2);
    float dz = dot(ray_dir, n);

    // Retrieve curvature parameters.
    const float curv_x = rect.curv_x;
    const float curv_y = rect.curv_y;

    float A = (curv_x * 0.5f) * (dx * dx) + (curv_y * 0.5f) * (dy * dy);
    float B = curv_x * (ox * dx) + curv_y * (oy * dy) - dz;
    float C = (curv_x * 0.5f) * (ox * ox) + (curv_y * 0.5f) * (oy * oy) - oz;

    float t = 0.0f;
    const float eps = 1e-12f;
    bool valid = false;

    if (fabsf(A) < eps) {
        // Degenerate (linear) case.
        t = -C / B;
        valid = (t > 0.0f);
    }
    else {
        float discr = B * B - 4.0f * A * C;
        if (discr >= 0.0f) {
            float sqrt_discr = sqrtf(discr);
            float t1 = (-B - sqrt_discr) / (2.0f * A);
            float t2 = (-B + sqrt_discr) / (2.0f * A);
            // Choose the smallest positive t.
            if (t1 > 0.0f && t1 < t2) {
                t = t1;
                valid = true;
            }
            else if (t2 > 0.0f) {
                t = t2;
                valid = true;
            }
        }
    }

    // Discard if no valid t or if t is not within the ray's bounds.
    if (!valid || t < ray_tmin || t > ray_tmax) {
        return;
    }

    //
    // Compute the local intersection coordinates.
    //
    float x_hit = ox + t * dx;
    float y_hit = oy + t * dy;
    // (Optionally, you could compute z_hit = oz + t*dz and verify it is near f(x,y).)

    //
    // Check if the hit is within the rectangle's flat bounds.
    // The parametric coordinates are:
    //    a1 = x_hit / (L1/2)   and   a2 = y_hit / (L2/2)
    //
    float a1 = x_hit / (L1/2.);
    float a2 = y_hit / (L2/2.);
    if (a1 < -1.0f || a1 > 1.0f || a2 < -1.0f || a2 > 1.0f) {
        return;
    }

    //
    // Compute the surface normal at the hit on the paraboloid.
    // The height function is:
    //    f(x,y) = (curv_x/2)*x^2 + (curv_y/2)*y^2
    // so its partial derivatives are:
    //    f_x = curv_x * x    and    f_y = curv_y * y.
    // Then the (unnormalized) local normal is:
    //    N_local = (-f_x, -f_y, 1) = ( -curv_x*x_hit, -curv_y*y_hit, 1 ).
    //
    float3 N_local = normalize(make_float3(-curv_x * x_hit,
        -curv_y * y_hit,
        1.0f));
    // Transform the normal back to world coordinates.
    float3 world_normal = normalize(N_local.x * e1 +
        N_local.y * e2 +
        N_local.z * n);

    // Compute the hit point in world space.
    float3 world_hit = ray_orig + t * ray_dir;

    // Report the intersection.
    // Here, the two reported extra attributes are the parametric coordinates (a1, a2),
    // encoded as unsigned integers.
    optixReportIntersection(t, 0,
        __float_as_uint(world_normal.x),
        __float_as_uint(world_normal.y),
        __float_as_uint(world_normal.z));    
}


// intersection algorithm for a flat triangle based on "Fast, Minimum Storage Ray/Triangle Intersection" by Möller and Trumbore (1997)
// code from here: https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm 
extern "C" __global__ void __intersection__triangle_flat()
{
	const OptixCSP::GeometryDataST::Triangle_Flat& tri = params.geometry_data_array[optixGetPrimitiveIndex()].getTriangle_Flat();

    const float3 ro = optixGetObjectRayOrigin();
    const float3 rd = optixGetObjectRayDirection();

	//printf("Ray origin: (%f,%f,%f), direction: (%f,%f,%f)\n", ro.x, ro.y, ro.z, rd.x, rd.y, rd.z);

    const float3 edge1 = tri.e1;
    const float3 edge2 = tri.e2;


    const float3 pvec = cross(rd, edge2);
    const float  det = dot(edge1, pvec);

    // Backface culling + parallel rejection
    // (det must be strictly positive and not tiny)
    const float eps = 1e-8f;
    if (det <= eps) return;

    const float inv_det = 1.0f / det;

    const float3 tvec = ro - tri.v0;
    const float  u = dot(tvec, pvec) * inv_det;
    if (u < 0.0f || u > 1.0f) return;

    const float3 qvec = cross(tvec, edge1);
    const float  v = dot(rd, qvec) * inv_det;
    if (v < 0.0f || (u + v) > 1.0f) 
        return;

    const float  t = dot(edge2, qvec) * inv_det;
    if (t < optixGetRayTmin() || t > optixGetRayTmax()) return;

    float3 world_normal = tri.normal;

    optixReportIntersection(t, 0,
        __float_as_uint(world_normal.x),
        __float_as_uint(world_normal.y),
        __float_as_uint(world_normal.z));

}
