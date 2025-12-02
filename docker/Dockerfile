# CERBERUS Honeypot - Multi-stage Docker Build
# Bio-adaptive IoT honeypot with morphing capabilities

# =============================================================================
# Stage 1: Builder - Compile the C binaries
# =============================================================================
FROM debian:bookworm-slim AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    gcc \
    make \
    && rm -rf /var/lib/apt/lists/*

# Copy source code
WORKDIR /build
COPY src/ ./src/
COPY include/ ./include/
COPY Makefile ./
COPY profiles.conf ./

# Build the project
RUN make all

# =============================================================================
# Stage 2: Runtime - Minimal image for running the honeypot
# =============================================================================
FROM debian:bookworm-slim AS runtime

# Labels
LABEL org.opencontainers.image.title="CERBERUS Honeypot"
LABEL org.opencontainers.image.description="Bio-adaptive IoT honeypot with morphing capabilities"
LABEL org.opencontainers.image.source="https://github.com/Phantomojo/cerberus-honeypot"
LABEL org.opencontainers.image.vendor="CERBERUS Project"
LABEL maintainer="Phantomojo"

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    python3 \
    python3-pip \
    python3-venv \
    procps \
    net-tools \
    iproute2 \
    curl \
    && rm -rf /var/lib/apt/lists/* \
    && apt-get clean

# Create non-root user for running honeypot
RUN useradd -m -s /bin/bash cerberus \
    && mkdir -p /opt/cerberus/build \
    && mkdir -p /opt/cerberus/logs \
    && mkdir -p /opt/cerberus/data \
    && mkdir -p /opt/cerberus/services \
    && chown -R cerberus:cerberus /opt/cerberus

# Copy built binaries from builder stage
COPY --from=builder /build/build/morph /opt/cerberus/build/
COPY --from=builder /build/build/quorum /opt/cerberus/build/
COPY --from=builder /build/build/state_engine_test /opt/cerberus/build/

# Copy configuration and scripts
COPY profiles.conf /opt/cerberus/
COPY scripts/ /opt/cerberus/scripts/
COPY services/ /opt/cerberus/services/

# Copy documentation
COPY README.md /opt/cerberus/
COPY docs/ /opt/cerberus/docs/

# Set permissions
RUN chmod +x /opt/cerberus/build/* \
    && chmod +x /opt/cerberus/scripts/*.sh 2>/dev/null || true \
    && chown -R cerberus:cerberus /opt/cerberus

# Set working directory
WORKDIR /opt/cerberus

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD ./build/state_engine_test || exit 1

# Environment variables
ENV CERBERUS_HOME=/opt/cerberus
ENV CERBERUS_LOG_DIR=/opt/cerberus/logs
ENV CERBERUS_DATA_DIR=/opt/cerberus/data
ENV PATH="${PATH}:/opt/cerberus/build"

# Expose common honeypot ports
# SSH/Telnet
EXPOSE 22/tcp
EXPOSE 23/tcp
# HTTP/HTTPS
EXPOSE 80/tcp
EXPOSE 443/tcp
# Additional IoT ports
EXPOSE 554/tcp
EXPOSE 8080/tcp
EXPOSE 8443/tcp

# Volume for persistent data and logs
VOLUME ["/opt/cerberus/logs", "/opt/cerberus/data"]

# Switch to non-root user
USER cerberus

# Default command - can be overridden
CMD ["./build/morph", "profiles.conf"]
