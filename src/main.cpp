#include <csignal>
#include <cstdio>
#include <exception>
#include <string>

#include "camera_service/camera_server.hpp"

namespace {
camera_service::CameraServer* g_server = nullptr;

void handleSigint(int) {
    if (g_server != nullptr) {
        g_server->stop();
    }
}
} // namespace

int main(int argc, char** argv) {
    const std::string socket_path = argc > 1 ? argv[1] : camera_service::kSocketPath;

    camera_service::CameraServer server(socket_path);
    g_server = &server;
    std::signal(SIGINT, handleSigint);
    std::signal(SIGTERM, handleSigint);

    try {
        server.run();
    } catch (const std::exception& e) {
        std::fprintf(stderr, "camera_service_daemon failed: %s\n", e.what());
        return 1;
    }
    return 0;
}
