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

# Reset submodule dirtiness from init so the version string
# doesn't get a -dirty suffix. Only touch third-party directories.
RUN git checkout -- third-party/ 2>/dev/null || true

RUN cmake -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DAO_BUILD_SERVER_ONLY=ON \
        -DAOSDL_GENERATE_SCHEMAS=ON \
    && cmake --build build --target kagami --parallel "$(nproc)"

RUN strip build/apps/kagami/kagami

# Build the firewall helper (small C binary, no cmake needed)
RUN cc -O2 -Wall -Wextra -o build/kagami-fw-helper tools/kagami-fw-helper.c \
    && strip build/kagami-fw-helper

# ---------------------------------------------------------------------------
# Stage 2: Runtime
# ---------------------------------------------------------------------------
FROM ubuntu:24.04

RUN apt-get update && apt-get install -y --no-install-recommends \
        libssl3t64 liburing2 ca-certificates nftables libcap2-bin \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=build /src/build/apps/kagami/kagami .
COPY --from=build /src/build/kagami-fw-helper .
RUN setcap cap_net_admin+ep /app/kagami-fw-helper

EXPOSE 8080 8081

ENTRYPOINT ["./kagami"]
