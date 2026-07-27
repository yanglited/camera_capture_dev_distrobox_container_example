# camera_service

Example project: a local camera capture service. `camera_service_daemon`
opens camera devices on demand and answers requests over a Unix domain
socket (`/tmp/camera_service.sock` by default) with a JPEG-encoded frame.
`camera_service_client` is a minimal client that sends one request and
writes the result to disk.

Built to exercise OpenCV (capture + JPEG encode) and Eigen
(`CameraIntrinsics`, in `include/camera_service/camera_intrinsics.hpp`)
together in one CMake project. This is throwaway/example code, not
production-hardened (no auth, one connection at a time, default
intrinsics are placeholders).

## Build

Run inside the `yang_dev_container` distrobox (has opencv/eigen/cmake/ninja):

```bash
distrobox enter yang_dev_container
cmake -B build -G Ninja
cmake --build build
```

## Run

```bash
# terminal 1
./build/camera_service_daemon

# terminal 2 — captures from /dev/video0, writes capture.jpg
./build/camera_service_client 0 capture.jpg
```

Ctrl-C the daemon to stop it (SIGINT/SIGTERM close the socket and unlink
the socket file cleanly).

## Layout

```
include/camera_service/
  protocol.hpp          wire format shared by daemon and client
  camera_intrinsics.hpp Eigen-based pinhole + distortion model
  camera_capture.hpp    thread-safe cv::VideoCapture wrapper
  camera_server.hpp     Unix-socket request/response server
src/                    implementations + daemon main()
tools/client_example.cpp  standalone test client
```
