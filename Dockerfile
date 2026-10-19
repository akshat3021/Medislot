# Multi-stage Dockerfile for Medislot: C++ backend + React/Vite frontend

# Stage 1: Build the React frontend
FROM node:20-slim AS frontend-builder
WORKDIR /app/frontend
COPY frontend/package*.json ./
RUN npm install
COPY frontend/ ./
RUN npm run build

# Stage 2: Build the C++ binary
FROM gcc:13 AS backend-builder
WORKDIR /app
RUN apt-get update && apt-get install -y libpq-dev make
COPY . .
RUN make clean && make

# Stage 3: Minimal runtime image
FROM debian:bookworm-slim
WORKDIR /app

# Install runtime dependencies for libpq and certificates
RUN apt-get update && apt-get install -y \
    libpq5 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Copy built binary, SQL files, and frontend dist
COPY --from=backend-builder /app/medislot /app/medislot
COPY --from=backend-builder /app/sql /app/sql
COPY --from=frontend-builder /app/frontend/dist /app/frontend/dist

# Default port
ENV PORT=8080
EXPOSE 8080

CMD ["./medislot"]
