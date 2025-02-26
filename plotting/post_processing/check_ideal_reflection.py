import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D

# Requires: pip install shapely
from shapely.geometry import Polygon
from shapely.ops import unary_union

# GPU heliostat geometry
# rect1_3d = np.array([
#     [-4.05108223553502, 0.5, -0.224009], 
#     [-4.05108223553502, -0.5, -0.224009],
#     [-5.94891776446498, -0.5, 0.2240091973214239],
#     [-5.94891776446498, 0.5, 0.2240091973214239],
# ])

# CPU heliostat geometry, computed by printing max/min x, y, z hit point values
# Minimum x: -5.958820461685046, Maximum x: -4.041069770525555
# Minimum y: -0.5208086614526061, Maximum y: 0.5206490038702324
# Minimum z: -0.2263727198354415, Maximum z: 0.22634680717540345

rect1_3d = np.array([
    [-4.041069770525555, 0.5206490038702324, -0.2263727198354415], 
    [-4.041069770525555, -0.5208086614526061, -0.2263727198354415],
    [-5.958820461685046, -0.5208086614526061, 0.22634680717540345],
    [-5.958820461685046, 0.5206490038702324, 0.22634680717540345],
])

# Receiver geometry
rect2_3d = np.array([
    [-1.0, -0.894427, 9.552786],
    [1.0, -0.894427, 9.552786],
    [1.0, 0.894427, 10.447214],
    [-1.0, 0.894427, 10.447214],
])

# --- Reflection code from earlier---
d = np.array([0, 0.09950373, -0.9950373])       # incident direction (unit)
# n = np.array([-0.4480182, 0, -1.8978355])       # unnormalized normal
# compute normal from heliostat corner data above instead
n = np.cross(rect1_3d[1] - rect1_3d[0], rect1_3d[2] - rect1_3d[1])
n /= np.linalg.norm(n)
n_dot_n = np.dot(n, n)
d_dot_n = np.dot(d, n)
r = d - 2.0 * (d_dot_n / n_dot_n) * n           # reflection vector (unnormalized)

# plane1: Heliostat plane
# plane2: Receiver plane
# plane1_point = np.array([-4.05108223553502, 0.49999999999999994, -0.224009])
# plane1_normal = np.array([-0.4480182, 0, -1.8978355])

# plane2_point = np.array([-1.0, -0.894427, 9.552786])
# plane2_normal = np.array([0, -0.447214, 0.894427])
plane1_point = rect1_3d[0]
plane1_normal = n

plane2_point = rect2_3d[0]
plane2_normal = np.cross(rect2_3d[1] - rect2_3d[0], rect2_3d[2] - rect2_3d[1])
plane2_normal /= np.linalg.norm(plane2_normal)

# Line-plane intersection
def line_plane_intersect(point_3d, dir_3d, plane_point, plane_normal):
    denom = np.dot(dir_3d, plane_normal)
    if abs(denom) < 1e-9:
        return None
    t = np.dot((plane_point - point_3d), plane_normal) / denom
    return point_3d + t * dir_3d

# Project corners of rect_corners onto plane along proj_dir
def project_corners(rect_corners, proj_dir, plane_point, plane_normal):
    out_pts = []
    for corner in rect_corners:
        ipt = line_plane_intersect(corner, proj_dir, plane_point, plane_normal)
        if ipt is not None:
            out_pts.append(ipt)
    return np.array(out_pts)

# 1) Project rect1_3d onto plane2 using the reflection vector r
projected_rect = project_corners(rect1_3d, r, plane2_point, plane2_normal)

# 2) We need a local 2D coordinate system on plane2 for plotting the original plane2 rectangle
#    and the newly projected rectangle. We'll create two orthonormal axes e1, e2 in plane2.
plane2_unit_normal = plane2_normal / np.linalg.norm(plane2_normal)
# Pick any vector not parallel to plane2_normal for e1:
temp_vec = np.array([1, 0, 0]) if abs(np.dot(plane2_unit_normal, [1,0,0])) < 0.99 else np.array([0,1,0])
e1 = np.cross(plane2_unit_normal, temp_vec)
e1 /= np.linalg.norm(e1)
e2 = np.cross(plane2_unit_normal, e1)

def to_plane2_local(pts_3d):
    """Convert 3D points to plane2’s local 2D coords by dotting with e1, e2."""
    out_2d = []
    for p in pts_3d:
        p_rel = p - plane2_point
        x_local = np.dot(p_rel, e1)
        y_local = np.dot(p_rel, e2)
        out_2d.append((x_local, y_local))
    return np.array(out_2d)

# Convert plane2 corners and projected corners into 2D local coords
rect2_2d = to_plane2_local(rect2_3d)
proj_2d = to_plane2_local(projected_rect)

# 3) Use Shapely to compute the actual intersection polygon
poly_plane2 = Polygon(rect2_2d)
poly_projected = Polygon(proj_2d)
print("Plane2 Polygon valid:", poly_plane2.is_valid, "area:", poly_plane2.area)
print("Projected Polygon valid:", poly_projected.is_valid, "area:", poly_projected.area)
intersection_poly = poly_plane2.intersection(poly_projected)

print("Intersection geom type:", intersection_poly.geom_type, "area:", intersection_poly.area if not intersection_poly.is_empty else None)

# 4) Plot the shapes and their intersection
plt.figure()
# Plane2 rectangle
x2, y2 = poly_plane2.exterior.xy
plt.fill(x2, y2, color='blue', alpha=0.3, label='Plane2 Rect')

# Projected rectangle
xp, yp = poly_projected.exterior.xy
plt.fill(xp, yp, color='green', alpha=0.3, label='Projected Rect')

# Intersection
if not intersection_poly.is_empty:
    xi, yi = intersection_poly.exterior.xy
    plt.fill(xi, yi, color='red', alpha=0.5, label='Intersection')

plt.gca().set_aspect('equal', adjustable='box')
plt.title("Overlap of Projected Rectangle onto Another Plane")
plt.legend()
plt.xlabel("Local e1 axis on Plane2")
plt.ylabel("Local e2 axis on Plane2")
plt.show()