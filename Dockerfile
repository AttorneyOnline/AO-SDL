# =============================================================================
# Kagami game server — multi-stage Docker build
# =============================================================================
# Build:  docker build -t kagami .
# Run:    docker run -p 8080:8080 -p 8081:8081 kagami
# Config: docker run -v ./kagami.json:/app/kagami.json -p 8080:8080 -p 8081:8081 kagami

# ---------------------------------------------------------------------------
# Stage 1: Build
# ---------------------------------------------------------------------------
FROM ubuntu:24.04 AS build

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential cmake ninja-build \
        python3 python3-yaml \
        libssl-dev liburing-dev \
        git ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .

# Initialize submodules if not already present (handles both fresh clones
# and pre-populated source trees from CI).
RUN git submodule update --init --recursive 2>/dev/null || true

# Reset any dirtiness from submodule init so the version string
# (derived from git status --porcelain) doesn't get a -dirty suffix.
RUN git checkout -- . 2>/dev/null || true

RUN cmake -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DAO_BUILD_SERVER_ONLY=ON \
        -DAOSDL_GENERATE_SCHEMAS=ON \
    && cmake --build build --target kagami --parallel "$(nproc)"

RUN strip build/apps/kagami/kagami

# ---------------------------------------------------------------------------
# Stage 2: Runtime
# ---------------------------------------------------------------------------
FROM ubuntu:24.04

RUN apt-get update && apt-get install -y --no-install-recommends \
        libssl3t64 liburing2 ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=build /src/build/apps/kagami/kagami .

EXPOSE 8080 8081

ENTRYPOINT ["./kagami"]
