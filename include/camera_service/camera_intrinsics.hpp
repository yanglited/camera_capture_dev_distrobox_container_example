#pragma once

#include <Eigen/Dense>

namespace camera_service {

// Pinhole camera model (K matrix) plus radial-tangential distortion,
// expressed with Eigen so pose/geometry code downstream of the captured
// frame (e.g. undistortion, ray casting) doesn't need to round-trip
// through cv::Mat for simple linear algebra.
class CameraIntrinsics {
public:
    CameraIntrinsics(double fx, double fy, double cx, double cy,
                      const Eigen::Matrix<double, 5, 1>& dist_coeffs =
                          Eigen::Matrix<double, 5, 1>::Zero());

    const Eigen::Matrix3d& K() const { return k_; }
    const Eigen::Matrix<double, 5, 1>& distortion() const { return dist_coeffs_; }

    // Applies the radial-tangential distortion model to a point already in
    // normalized (z=1 plane) camera coordinates.
    Eigen::Vector2d distortNormalizedPoint(const Eigen::Vector2d& p) const;

    // Back-projects a pixel coordinate into a camera-space ray (undistorted).
    Eigen::Vector3d pixelToRay(const Eigen::Vector2d& pixel) const;

private:
    Eigen::Matrix3d k_;
    Eigen::Matrix<double, 5, 1> dist_coeffs_;
};

} // namespace camera_service
