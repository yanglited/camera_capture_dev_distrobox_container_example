#pragma once

#include <cstdint>

namespace camera_service {

inline constexpr const char* kSocketPath = "/tmp/camera_service.sock";

// Fixed-layout request sent by the client. No padding surprises: both
// fields are 4 bytes, so this is safe to read/write as raw bytes between
// processes built from the same headers.
struct CaptureRequest {
    std::uint32_t camera_index = 0;
    std::uint32_t jpeg_quality = 90; // 0-100, forwarded to cv::imencode
};

enum class CaptureStatus : std::uint32_t {
    kOk = 0,
    kCameraUnavailable = 1,
    kCaptureFailed = 2,
    kEncodeFailed = 3,
};

// Sent before the JPEG payload so the client knows how many bytes to read.
struct CaptureResponseHeader {
    CaptureStatus status = CaptureStatus::kOk;
    std::uint32_t payload_size = 0;
};

} // namespace camera_service
