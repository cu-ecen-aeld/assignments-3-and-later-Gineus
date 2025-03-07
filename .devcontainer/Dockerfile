FROM mcr.microsoft.com/vscode/devcontainers/base:ubuntu-22.04

RUN apt-get update && apt-get install -y build-essential git ruby wget gdb file flex bison bc cpio qemu-system-arm qemu-system-aarch64

# Install CMake 3.25.1
RUN wget https://github.com/Kitware/CMake/releases/download/v3.25.1/cmake-3.25.1-linux-x86_64.sh \
    && bash cmake-3.25.1-linux-x86_64.sh --skip-license --prefix=/usr/local \
    && rm cmake-3.25.1-linux-x86_64.sh

# Copy and execute the cross-compile toolchain installation script
COPY ./install_cross_compile_toolchain.sh /tmp/install_cross_compile_toolchain.sh

# Run the script as the vscode user
RUN chown vscode:vscode /tmp/install_cross_compile_toolchain.sh \
    && chmod +x /tmp/install_cross_compile_toolchain.sh \
    && su vscode -c "/tmp/install_cross_compile_toolchain.sh" \
    && rm /tmp/install_cross_compile_toolchain.sh