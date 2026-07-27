#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "camera_service/camera_capture.hpp"
#include "camera_service/protocol.hpp"

namespace camera_service {

// Unix-domain-socket server that answers CaptureRequest messages with a
// JPEG-encoded frame from the requested camera index. Cameras are opened
// lazily on first request and then kept alive for reuse.
class CameraServer {
public:
    explicit CameraServer(std::string socket_path = kSocketPath);
    ~CameraServer();

    CameraServer(const CameraServer&) = delete;
    CameraServer& operator=(const CameraServer&) = delete;

    // Binds the socket and blocks handling connections one at a time until
    // stop() is called (typically from a signal handler on another thread).
    void run();
    void stop();

private:
    void handleConnection(int client_fd);
    CameraCapture* getOrOpenCamera(int index);

    std::string socket_path_;
    int listen_fd_ = -1;
    std::atomic<bool> running_{false};
    std::unordered_map<int, std::unique_ptr<CameraCapture>> cameras_;
    std::mutex cameras_mutex_;
};

} // namespace camera_service
