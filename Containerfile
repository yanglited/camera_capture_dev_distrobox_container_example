FROM archlinux:latest

# Single pacman invocation: sync + upgrade + install together to avoid
# Arch's "partial upgrade" problem (never split -Sy from package installs).
RUN pacman -Syu --noconfirm --needed \
        base-devel \
        git \
        cmake \
        ninja \
        neovim \
        opencv \
        eigen \
        gcc \
        gdb \
        pkgconf \
        ripgrep \
        fd \
        fzf \
        unzip \
        wget \
        curl \
        sudo \
        which \
        man-db \
    && pacman -Scc --noconfirm

WORKDIR /workspace
