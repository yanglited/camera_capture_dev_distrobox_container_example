# camera_service

Reference repo for the **podman + distrobox + neovim** C++ dev workflow:
build a container image with `podman`, turn it into a dev environment with
`distrobox`, and edit/build/run entirely inside it (including clangd/nvim).

The actual example app is secondary: a local camera capture service.
`camera_service_daemon` opens camera devices on demand and answers
requests over a Unix domain socket (`/tmp/camera_service.sock` by default)
with a JPEG-encoded frame. `camera_service_client` is a minimal client
that sends one request and writes the result to disk. It exists to
exercise OpenCV (capture + JPEG encode) and Eigen (`CameraIntrinsics`, in
`include/camera_service/camera_intrinsics.hpp`) together in one CMake
project. Throwaway/example code, not production-hardened (no auth, one
connection at a time, default intrinsics are placeholders).

## 1. Build the container image (podman)

`Containerfile` (podman/OCI's generic name for what docker calls a
`Dockerfile`) lives at repo root — one image, so no reason to nest it in
a subdirectory. It's Arch-based (rolling release, so `pacman -S neovim`
stays current) with opencv, eigen, cmake, ninja, gcc/gdb, and neovim
preinstalled.

```bash
podman build -t yang_dev_container:latest -f Containerfile .
```

Rebuild any time you want package versions refreshed — Arch's rolling
release means this isn't reproducible across builds by design; see
project notes if you need a pinned/reproducible variant instead.

## 2. Create the distrobox

```bash
distrobox create --name yang_dev_container --image yang_dev_container:latest
distrobox enter yang_dev_container
```

`distrobox create` defaults to whatever engine `distrobox` resolves first
(usually podman if both podman and docker are installed). If you built
with docker instead, either set `DBX_CONTAINER_MANAGER=docker` before
`distrobox create`, or make sure podman can see the image (podman and
docker keep separate local image stores).

distrobox shares your host `$HOME`, so this repo and your existing
`~/.config/nvim` are visible inside the container with no extra mounting
— clone/keep this repo under your home dir and it Just Works from either
side.

## 3. Build the project

Inside the distrobox:

```bash
cd ~/camera_service
cmake -B build -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build
```

(`CMAKE_EXPORT_COMPILE_COMMANDS` is already `ON` in `CMakeLists.txt`, the
flag above is redundant but harmless — useful as a reminder of what
makes clangd work at all.)

To force a full rebuild later: `cmake --build build --clean-first`, or
`rm -rf build` and reconfigure from scratch.

## 4. Wire up neovim / clangd

clangd looks for `compile_commands.json` in the project root, not inside
`build/`:

```bash
ln -sf build/compile_commands.json compile_commands.json
```

Run `nvim` from inside the distrobox (`distrobox enter yang_dev_container`
first) so clangd resolves the same compiler and headers (`opencv`,
`eigen3`) recorded in `compile_commands.json` — running nvim on the bare
host will show phantom "file not found" errors for anything the host
doesn't have installed.

## 5. Run

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
Containerfile             podman image: Arch + opencv/eigen/cmake/ninja/neovim
CMakeLists.txt
include/camera_service/
  protocol.hpp             wire format shared by daemon and client
  camera_intrinsics.hpp    Eigen-based pinhole + distortion model
  camera_capture.hpp       thread-safe cv::VideoCapture wrapper
  camera_server.hpp        Unix-socket request/response server
src/                       implementations + daemon main()
tools/client_example.cpp   standalone test client
```
