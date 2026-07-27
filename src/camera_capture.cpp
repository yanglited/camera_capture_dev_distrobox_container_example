#include "camera_service/camera_capture.hpp"

#include <stdexcept>

namespace camera_service {

CameraCapture::CameraCapture(int device_index, CameraIntrinsics intrinsics)
    : device_index_(device_index),
      intrinsics_(std::move(intrinsics)),
      capture_(device_index) {
    if (!capture_.isOpened()) {
        throw std::runtime_error("failed to open camera device index " +
                                  std::to_string(device_index));
    }
}

CameraCapture::~CameraCapture() = default;

bool CameraCapture::captureFrame(cv::Mat& out_frame) {
    std::lock_guard<std::mutex> lock(capture_mutex_);
    if (!capture_.isOpened()) {
        return false;
    }
    capture_ >> out_frame;
    return !out_frame.empty();
}

} // namespace camera_service
