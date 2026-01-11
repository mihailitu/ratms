#!/bin/bash
#
# RATMS Dependency Installation Script (Ubuntu/Debian)
#
# Usage: ./scripts/install-dependencies.sh [--build] [--verify]
#

set -e

GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

log() { echo -e "${GREEN}[INFO]${NC} $1"; }
err() { echo -e "${RED}[ERROR]${NC} $1"; }

install_backend() {
    log "Installing backend dependencies..."
    sudo apt-get update
    sudo apt-get install -y \
        build-essential \
        cmake \
        git \
        pkg-config \
        libsqlite3-dev \
        libexpat1-dev
    log "Backend dependencies installed"
}

install_frontend() {
    log "Installing frontend dependencies..."

    # Install Node.js 20 if not present or too old
    if ! command -v node &> /dev/null || [ "$(node -v | cut -d'v' -f2 | cut -d'.' -f1)" -lt 18 ]; then
        curl -fsSL https://deb.nodesource.com/setup_20.x | sudo -E bash -
        sudo apt-get install -y nodejs
    fi

    # Install npm packages
    if [ -d "frontend" ]; then
        cd frontend && npm install && cd ..
    fi
    log "Frontend dependencies installed"
}

verify() {
    log "Verifying dependencies..."
    local ok=true

    command -v cmake &> /dev/null && log "CMake: $(cmake --version | head -1 | cut -d' ' -f3)" || { err "CMake missing"; ok=false; }
    command -v g++ &> /dev/null && log "g++: $(g++ -dumpversion)" || { err "g++ missing"; ok=false; }
    pkg-config --exists sqlite3 2>/dev/null && log "SQLite3: $(pkg-config --modversion sqlite3)" || { err "SQLite3-dev missing"; ok=false; }
    pkg-config --exists expat 2>/dev/null && log "Expat: $(pkg-config --modversion expat)" || { err "Expat-dev missing"; ok=false; }
    command -v node &> /dev/null && log "Node.js: $(node -v)" || { err "Node.js missing"; ok=false; }

    $ok && log "All dependencies OK" || { err "Some dependencies missing"; return 1; }
}

build() {
    log "Building backend..."
    mkdir -p simulator/build
    cd simulator/build
    cmake ..
    make -j$(nproc) api_server ga_optimizer
    cd ../..
    log "Build complete: simulator/build/api_server"
}

# Main
case "${1:-}" in
    --verify) verify ;;
    --build) install_backend; install_frontend; verify && build ;;
    --help|-h) echo "Usage: $0 [--build] [--verify]"; exit 0 ;;
    *) install_backend; install_frontend; verify ;;
esac
