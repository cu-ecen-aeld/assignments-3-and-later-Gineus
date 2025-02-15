FROM mcr.microsoft.com/vscode/devcontainers/base:ubuntu-22.04

# Install build-essential and wget
RUN apt-get update && apt-get install -y build-essential wget

# Install CMake 3.25.1
RUN wget https://github.com/Kitware/CMake/releases/download/v3.25.1/cmake-3.25.1-linux-x86_64.sh \
    && bash cmake-3.25.1-linux-x86_64.sh --skip-license --prefix=/usr/local \
    && rm cmake-3.25.1-linux-x86_64.sh

# Install ruby
RUN apt-get update && apt-get install -y ruby