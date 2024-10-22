FROM ubuntu:latest AS build
WORKDIR /src
RUN apt update && apt upgrade -y

# Install vulkan-sdk
RUN apt install -y wget xz-utils
RUN mkdir deps && cd deps \
    && wget https://sdk.lunarg.com/sdk/download/1.3.296.0/linux/vulkansdk-linux-x86_64-1.3.296.0.tar.xz \
    && tar -xvf ./vulkansdk-linux-x86_64-1.3.296.0.tar.xz \
    && cp -r ./1.3.296.0/x86_64/* /usr \
    && cd .. && rm -rf deps

COPY . .

# Build
RUN apt install -y cmake g++
RUN mkdir build && cd build \
    && cmake .. \
    && cmake --build . --config Release --parallel


FROM ubuntu:latest
WORKDIR /app
RUN apt update && apt upgrade -y

COPY --from=build /src/build/.output/ .

