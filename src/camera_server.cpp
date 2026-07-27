#include "camera_service/camera_server.hpp"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

#include "camera_service/protocol.hpp"

namespace camera_service {
namespace {

// recv()/send() on a stream socket may return fewer bytes than requested;
// these loop until the full amount has moved or the connection breaks.
bool readAll(int fd, void* buffer, std::size_t size) {
    auto* bytes = static_cast<std::uint8_t*>(buffer);
    std::size_t total_read = 0;
    while (total_read < size) {
        const ssize_t n = ::recv(fd, bytes + total_read, size - total_read, 0);
        if (n <= 0) {
            return false;
        }
        total_read += static_cast<std::size_t>(n);
    }
    return true;
}

bool writeAll(int fd, const void* buffer, std::size_t size) {
    const auto* bytes = static_cast<const std::uint8_t*>(buffer);
    std::size_t total_written = 0;
    while (total_written < size) {
        const ssize_t n = ::send(fd, bytes + total_written, size - total_written, 0);
        if (n <= 0) {
            return false;
        }
        total_written += static_cast<std::size_t>(n);
    }
    return true;
}

void sendError(int client_fd, CaptureStatus status) {
    CaptureResponseHeader header;
    header.status = status;
    header.payload_size = 0;
    writeAll(client_fd, &header, sizeof(header));
}

} // namespace

CameraServer::CameraServer(std::string socket_path) : socket_path_(std::move(socket_path)) {}

CameraServer::~CameraServer() {
    stop();
}

CameraCapture* CameraServer::getOrOpenCamera(int index) {
    std::lock_guard<std::mutex> lock(cameras_mutex_);
    auto it = cameras_.find(index);
    if (it != cameras_.end()) {
        return it->second.get();
    }
    try {
        // Default intrinsics: a stand-in until a real calibration is
        // loaded per device. Downstream geometry code should not treat
        // these as trustworthy without calibrating the actual hardware.
        CameraIntrinsics intrinsics(/*fx=*/600.0, /*fy=*/600.0, /*cx=*/320.0, /*cy=*/240.0);
        auto capture = std::make_unique<CameraCapture>(index, intrinsics);
        CameraCapture* raw = capture.get();
        cameras_.emplace(index, std::move(capture));
        return raw;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "camera %d unavailable: %s\n", index, e.what());
        return nullptr;
    }
}

void CameraServer::handleConnection(int client_fd) {
    CaptureRequest request;
    if (!readAll(client_fd, &request, sizeof(request))) {
        ::close(client_fd);
        return;
    }

    CameraCapture* camera = getOrOpenCamera(static_cast<int>(request.camera_index));
    if (camera == nullptr) {
        sendError(client_fd, CaptureStatus::kCameraUnavailable);
        ::close(client_fd);
        return;
    }

    cv::Mat frame;
    if (!camera->captureFrame(frame)) {
        sendError(client_fd, CaptureStatus::kCaptureFailed);
        ::close(client_fd);
        return;
    }

    std::vector<int> encode_params{cv::IMWRITE_JPEG_QUALITY,
                                    static_cast<int>(request.jpeg_quality)};
    std::vector<uchar> jpeg_bytes;
    if (!cv::imencode(".jpg", frame, jpeg_bytes, encode_params)) {
        sendError(client_fd, CaptureStatus::kEncodeFailed);
        ::close(client_fd);
        return;
    }

    CaptureResponseHeader header;
    header.status = CaptureStatus::kOk;
    header.payload_size = static_cast<std::uint32_t>(jpeg_bytes.size());
    if (writeAll(client_fd, &header, sizeof(header))) {
        writeAll(client_fd, jpeg_bytes.data(), jpeg_bytes.size());
    }
    ::close(client_fd);
}

void CameraServer::run() {
    listen_fd_ = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (listen_fd_ < 0) {
        throw std::runtime_error("socket() failed");
    }

    ::unlink(socket_path_.c_str()); // remove a stale socket file, if any

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, socket_path_.c_str(), sizeof(addr.sun_path) - 1);

    if (::bind(listen_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        throw std::runtime_error("bind() failed for " + socket_path_);
    }
    if (::listen(listen_fd_, /*backlog=*/8) < 0) {
        throw std::runtime_error("listen() failed");
    }

    running_ = true;
    std::fprintf(stderr, "camera_service listening on %s\n", socket_path_.c_str());

    while (running_) {
        const int client_fd = ::accept(listen_fd_, nullptr, nullptr);
        if (client_fd < 0) {
            if (running_) {
                std::fprintf(stderr, "accept() failed: %s\n", std::strerror(errno));
            }
            continue;
        }
        handleConnection(client_fd);
    }
}

void CameraServer::stop() {
    if (!running_) {
        return;
    }
    running_ = false;
    if (listen_fd_ >= 0) {
        ::shutdown(listen_fd_, SHUT_RDWR);
        ::close(listen_fd_);
        listen_fd_ = -1;
    }
    ::unlink(socket_path_.c_str());
}

} // namespace camera_service
