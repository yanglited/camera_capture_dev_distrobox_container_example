#pragma once

#include <mutex>

#include <opencv2/opencv.hpp>

#include "camera_service/camera_intrinsics.hpp"

namespace camera_service {

// Thread-safe wrapper around cv::VideoCapture. The server keeps one
// instance per camera index alive for its whole lifetime (opening a
// V4L2 device is comparatively slow), so concurrent client connections
// serialize on capture_mutex_ rather than each opening their own handle.
class CameraCapture {
public:
    CameraCapture(int device_index, CameraIntrinsics intrinsics);
    ~CameraCapture();

    CameraCapture(const CameraCapture&) = delete;
    CameraCapture& operator=(const CameraCapture&) = delete;

    // Grabs one frame. Returns false if the device failed to produce one.
    bool captureFrame(cv::Mat& out_frame);

    const CameraIntrinsics& intrinsics() const { return intrinsics_; }
    bool isOpen() const { return capture_.isOpened(); }

private:
    int device_index_;
    CameraIntrinsics intrinsics_;
    cv::VideoCapture capture_;
    std::mutex capture_mutex_;
};

} // namespace camera_service
