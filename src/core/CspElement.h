#pragma once

#include <cstdint>
#include <string>
#include <memory>

#include "vec3d.h"
#include "soltrace_type.h"
#include "Surface.h"
#include "Aperture.h"
#include "utils/math_util.h"
#include "shaders/GeometryDataST.h"

namespace OptixCSP {

    class Aperture;
    class CspElementBase {
    public:
        CspElementBase();
        virtual ~CspElementBase() = default;

        // Positioning and orientation.
        virtual const Vec3d& get_origin() const = 0;
        virtual void set_origin(const OptixCSP::Vec3d&) = 0;
        virtual const Vec3d& get_aim_point() const = 0;
        virtual void set_aim_point(const Vec3d&) = 0;
        //virtual const Vec3d& get_euler_angles() const = 0;
        //virtual void set_euler_angles(const Vec3d&) = 0;

        // Bounding box accessors.
        //virtual const Vec3d& get_upper_bounding_box() const = 0;
        //virtual const Vec3d& get_lower_bounding_box() const = 0;


	    virtual GeometryDataST toDeviceGeometryData() const = 0;

        void set_receiver(bool val) { m_receiver = val; }
		bool is_receiver() const { return m_receiver; }
        bool m_receiver; // true if receiver, false if not, you can think of receiver as the last element in the optical path

    protected:
        // Derived classes must implement bounding box computation.
        //virtual int set_bounding_box() = 0;
    };

    // A concrete implementation of Element that stores data in member variables.
    class CspElement : public CspElementBase {
    public:
        CspElement();
        ~CspElement() = default;

        // set and get origin 
        const Vec3d& get_origin() const override;
        void set_origin(const Vec3d& o) override;
        void set_aim_point(const Vec3d& a) override;
        const Vec3d& get_aim_point() const override;

        // set zrot (in degrees)
        void set_zrot(double zrot);
        double get_zrot() const;
        std::shared_ptr<Aperture> get_aperture() const;
        std::shared_ptr<Surface> get_surface() const;
        ApertureType get_aperture_type() const;
        SurfaceType get_surface_type() const;

        // Optical CspElements setters.
        void set_aperture(const std::shared_ptr<Aperture>& aperture);
        void set_surface(const std::shared_ptr<Surface>& surface);

        // set orientation based on aimpoint and zrot
        void update_euler_angles(const Vec3d& aim_point, const double zrot);
	    // set orientation based on the CspElement's aim point and zrot
        void update_euler_angles();

        void update_element(const Vec3d& aim_point, const double zrot);

        // return L2G rotation matrix
        Matrix33d get_rotation_matrix() const;


        // return upper bounding box
        Vec3d get_upper_bounding_box() const;

	    // return lower bounding box
        Vec3d get_lower_bounding_box() const;

        // convert to device data available to GPU
        GeometryDataST toDeviceGeometryData() const override; 

        // we also need to implement the bounding box computation
        // for a case like a rectangle aperture,
        // once we have the origin, euler angles, rotatioin matrix
        // and the aperture size, we can compute the bounding box
        // this can be called when adding an element to the system
        void compute_bounding_box();


    private:
        Vec3d m_origin;
        Vec3d m_aim_point;
        Vec3d m_euler_angles;  // euler angles, need to be computed from aim point and zrot
        double m_zrot; // zrot from the stinput file, user provided value, in degrees

        Vec3d m_upper_box_bound;
        Vec3d m_lower_box_bound;

        std::shared_ptr<Surface> m_surface;
        std::shared_ptr<Aperture> m_aperture;

    };
}