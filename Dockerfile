# Production-ready multi-stage Dockerfile for Medislot C++ Backend
FROM ubuntu:22.04 AS builder

# Prevent interactive prompts
ENV DEBIAN_FRONTEND=noninteractive

# Install g++, make, PostgreSQL development headers, and threading support
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    g++ \
    make \
    cmake \
    libpq-dev \
    libpqxx-dev \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy project files
COPY Makefile ./
COPY third_party/ ./third_party/
COPY sql/ ./sql/
COPY src/ ./src/

# Compile medislot binary
RUN make

# Runtime container stage
FROM ubuntu:22.04 AS runner

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    libpq5 \
    libpqxx-6.4 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=builder /app/medislot ./medislot
COPY sql/ ./sql/

EXPOSE 8080

CMD ["./medislot"]
