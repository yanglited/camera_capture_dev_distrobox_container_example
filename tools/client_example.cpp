// Minimal client for camera_service_daemon: sends one CaptureRequest over
// the Unix domain socket and writes the returned JPEG to disk. Exists to
// demonstrate/exercise the wire protocol without pulling in a client
// library dependency.
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

#include "camera_service/protocol.hpp"

namespace {

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

} // namespace

int main(int argc, char** argv) {
    const int camera_index = argc > 1 ? std::atoi(argv[1]) : 0;
    const std::string out_path = argc > 2 ? argv[2] : "capture.jpg";
    const std::string socket_path = camera_service::kSocketPath;

    const int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        std::fprintf(stderr, "socket() failed: %s\n", std::strerror(errno));
        return 1;
    }

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path) - 1);

    if (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::fprintf(stderr, "connect() to %s failed: %s\n", socket_path.c_str(),
                      std::strerror(errno));
        return 1;
    }

    camera_service::CaptureRequest request;
    request.camera_index = static_cast<std::uint32_t>(camera_index);
    request.jpeg_quality = 90;
    if (::send(fd, &request, sizeof(request), 0) != static_cast<ssize_t>(sizeof(request))) {
        std::fprintf(stderr, "failed to send request\n");
        return 1;
    }

    camera_service::CaptureResponseHeader header;
    if (!readAll(fd, &header, sizeof(header))) {
        std::fprintf(stderr, "failed to read response header\n");
        return 1;
    }
    if (header.status != camera_service::CaptureStatus::kOk) {
        std::fprintf(stderr, "server returned error status %u\n",
                      static_cast<unsigned>(header.status));
        return 1;
    }

    std::vector<std::uint8_t> jpeg_bytes(header.payload_size);
    if (!readAll(fd, jpeg_bytes.data(), jpeg_bytes.size())) {
        std::fprintf(stderr, "failed to read %u byte payload\n", header.payload_size);
        return 1;
    }
    ::close(fd);

    std::ofstream out(out_path, std::ios::binary);
    out.write(reinterpret_cast<const char*>(jpeg_bytes.data()), jpeg_bytes.size());
    std::printf("wrote %zu bytes to %s\n", jpeg_bytes.size(), out_path.c_str());
    return 0;
}
