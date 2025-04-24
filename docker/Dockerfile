# Use Debian Bookworm Slim as the base image for building
ARG BUILDER_BASE=debian:bookworm-slim
FROM ${BUILDER_BASE} AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    ninja-build \
    pkg-config \
    libbrotli-dev \
    && rm -rf /var/lib/apt/lists/*

# Copy the source code into the container
WORKDIR /src
COPY .. .

# Configure and build jpegli tools
RUN rm -rf build && \
    cmake -S . -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_TESTING=OFF \
        -DJPEGXL_ENABLE_TOOLS=ON \
        -DJPEGXL_ENABLE_BENCHMARK=OFF \
        -DJPEGXL_ENABLE_EXAMPLES=OFF \
        -DJPEGXL_ENABLE_JNI=OFF \
        -DJPEGXL_ENABLE_PLUGINS=OFF \
        -DJPEGXL_ENABLE_DOXYGEN=OFF \
        -DJPEGXL_ENABLE_MANPAGES=OFF \
        -DJPEGXL_ENABLE_TRANSCODE_JPEG=OFF \
        -DJPEGXL_ENABLE_VIEWERS=OFF \
        -DJPEGXL_ENABLE_BOXES=OFF \
        -DJPEGXL_FORCE_SYSTEM_BROTLI=ON \
    && cmake --build build --target cjpegli djpegli

# Use a minimal final image
ARG FINAL_BASE=debian:bookworm-slim
FROM debian:bookworm-slim

# Add non-root user and group
RUN groupadd --system --gid 1001 app && \
    useradd --system --uid 1001 --gid app --home-dir /work --no-create-home app

# Install runtime dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    libbrotli1 \
    && rm -rf /var/lib/apt/lists/*

# Copy the built binaries from the builder stage
COPY --from=builder /src/build/tools/cjpegli /usr/local/bin/
COPY --from=builder /src/build/tools/djpegli /usr/local/bin/
COPY --from=builder /src/build/lib/libjxl_cms.so.0.12 /usr/local/lib/
RUN mkdir -p /usr/local/share/doc/jpegli
COPY docker/README.md /usr/local/share/doc/jpegli/README.md
COPY docker/entrypoint.sh /usr/local/bin/entrypoint.sh

# Ensure the linker finds the copied library and make entrypoint executable
RUN ldconfig && chmod +x /usr/local/bin/entrypoint.sh

# Verify the tools are runnable (optional but good practice)
RUN cjpegli -h
RUN djpegli -h

# Set working directory
WORKDIR /work
RUN chown app:app /work

# Switch to non-root user
USER app

# Set entrypoint and default command
ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]
CMD ["--help"] # Default action: show cjpegli help 