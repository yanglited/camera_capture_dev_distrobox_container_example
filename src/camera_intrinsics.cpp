#include "camera_service/camera_intrinsics.hpp"

namespace camera_service {

CameraIntrinsics::CameraIntrinsics(double fx, double fy, double cx, double cy,
                                    const Eigen::Matrix<double, 5, 1>& dist_coeffs)
    : dist_coeffs_(dist_coeffs) {
    k_ << fx, 0, cx,
          0, fy, cy,
          0, 0, 1;
}

Eigen::Vector2d CameraIntrinsics::distortNormalizedPoint(const Eigen::Vector2d& p) const {
    const double k1 = dist_coeffs_(0);
    const double k2 = dist_coeffs_(1);
    const double p1 = dist_coeffs_(2);
    const double p2 = dist_coeffs_(3);
    const double k3 = dist_coeffs_(4);

    const double x = p.x();
    const double y = p.y();
    const double r2 = x * x + y * y;
    const double r4 = r2 * r2;
    const double r6 = r4 * r2;

    const double radial = 1.0 + k1 * r2 + k2 * r4 + k3 * r6;

    const double x_distorted = x * radial + 2 * p1 * x * y + p2 * (r2 + 2 * x * x);
    const double y_distorted = y * radial + p1 * (r2 + 2 * y * y) + 2 * p2 * x * y;

    return {x_distorted, y_distorted};
}

Eigen::Vector3d CameraIntrinsics::pixelToRay(const Eigen::Vector2d& pixel) const {
    const Eigen::Vector3d homogeneous_pixel{pixel.x(), pixel.y(), 1.0};
    return k_.inverse() * homogeneous_pixel;
}

} // namespace camera_service
