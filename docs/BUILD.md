# RATMS Build Guide

This document describes how to build RATMS from source.

## Quick Start

```bash
# Install all dependencies and build
./scripts/install-dependencies.sh --build

# Or manually:
./scripts/install-dependencies.sh
cd simulator/build && cmake .. && make api_server
```

## System Requirements

### Minimum Requirements
- **OS**: Linux (Debian/Ubuntu, Fedora/RHEL, Arch) or macOS
- **RAM**: 4 GB (8 GB recommended for parallel builds)
- **Disk**: 500 MB free space

### Software Requirements

| Component | Minimum Version | Notes |
|-----------|-----------------|-------|
| CMake | 3.25+ | Build system |
| GCC/Clang | 10+ / 10+ | C++20 support required |
| SQLite3 | 3.30+ | Database (dev files needed) |
| Expat | 2.0+ | XML parser for OSM import |
| Node.js | 18+ | Frontend only |
| npm | 9+ | Frontend only |

## Dependencies by Platform

### Debian / Ubuntu

```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    libsqlite3-dev \
    libexpat1-dev
```

### Fedora / RHEL / CentOS

```bash
sudo dnf install -y \
    gcc-c++ \
    cmake \
    git \
    pkgconfig \
    sqlite-devel \
    expat-devel
```

### Arch Linux

```bash
sudo pacman -S --noconfirm \
    base-devel \
    cmake \
    git \
    sqlite \
    expat
```

### macOS (Homebrew)

```bash
brew install cmake sqlite3 expat
```

## Backend Dependencies

### Required System Libraries

| Library | Package (Debian) | Package (Fedora) | Package (macOS) | Purpose |
|---------|------------------|------------------|-----------------|---------|
| SQLite3 | `libsqlite3-dev` | `sqlite-devel` | `sqlite3` | Database storage |
| Expat | `libexpat1-dev` | `expat-devel` | `expat` | OSM XML parsing |
| pthreads | (built-in) | (built-in) | (built-in) | Threading |

### Auto-Fetched Dependencies (via CMake)

These are automatically downloaded during the build process:

| Library | Version | Purpose |
|---------|---------|---------|
| [spdlog](https://github.com/gabime/spdlog) | 1.12.0 | Logging |
| [Google Test](https://github.com/google/googletest) | 1.14.0 | Unit testing |

### Header-Only Libraries (included in repo)

Located in `simulator/src/external/`:

| Library | Purpose |
|---------|---------|
| [cpp-httplib](https://github.com/yhirose/cpp-httplib) | HTTP server |
| [nlohmann/json](https://github.com/nlohmann/json) | JSON parsing |

## Frontend Dependencies

### System Requirements

- Node.js 18+ (LTS recommended)
- npm 9+

### Installation

```bash
cd frontend
npm install
```

### Key npm Packages

| Package | Purpose |
|---------|---------|
| React 18 | UI framework |
| TypeScript 5 | Type safety |
| Vite | Build tool |
| Tailwind CSS | Styling |
| Leaflet | Map visualization |
| Recharts | Charts |
| Axios | HTTP client |

## Building

### Backend

```bash
cd simulator
mkdir -p build
cd build
cmake ..
make -j$(nproc)   # Build all targets
# Or build specific targets:
make api_server   # HTTP API server (main executable)
make ga_optimizer # Standalone GA optimization tool
make osm_import   # OpenStreetMap import tool
make unit_tests   # Unit test executable
```

### Frontend

```bash
cd frontend
npm run dev      # Development server (hot reload)
npm run build    # Production build
npm run preview  # Preview production build
```

## Build Targets

| Target | Description | Dependencies |
|--------|-------------|--------------|
| `api_server` | Main HTTP server with simulation engine | SQLite3, spdlog |
| `ga_optimizer` | Standalone GA optimization tool | SQLite3, spdlog |
| `osm_import` | OpenStreetMap XML import tool | Expat, spdlog |
| `unit_tests` | Google Test unit tests | SQLite3, GTest, spdlog |
| `simulator` | Legacy CLI simulator | SQLite3, spdlog |

## Running

### Backend

```bash
# Start API server (default port 8080)
./simulator/build/api_server

# With custom port
./simulator/build/api_server --port 9000
```

### Frontend

```bash
cd frontend
npm run dev
# Open http://localhost:5173
```

## Verifying Installation

Use the installation script to verify all dependencies:

```bash
./scripts/install-dependencies.sh --verify
```

Expected output:
```
[INFO] Verifying backend dependencies...
[INFO] CMake: 3.25.1
[INFO] g++: g++ (GCC) 12.2.0
[INFO] SQLite3: 3.40.1
[INFO] Expat: 2.5.0
[INFO] All backend dependencies verified
[INFO] Verifying frontend dependencies...
[INFO] Node.js: v20.10.0
[INFO] npm: 10.2.3
[INFO] All frontend dependencies verified
```

## Troubleshooting

### CMake: Could NOT find SQLite3

**Error:**
```
Could NOT find SQLite3 (missing: SQLite3_INCLUDE_DIR SQLite3_LIBRARY)
```

**Solution:**
```bash
# Debian/Ubuntu
sudo apt-get install libsqlite3-dev

# Fedora/RHEL
sudo dnf install sqlite-devel

# macOS
brew install sqlite3
```

### CMake: Could NOT find EXPAT

**Error:**
```
Could NOT find EXPAT (missing: EXPAT_LIBRARY EXPAT_INCLUDE_DIR)
```

**Solution:**
```bash
# Debian/Ubuntu
sudo apt-get install libexpat1-dev

# Fedora/RHEL
sudo dnf install expat-devel

# macOS
brew install expat
```

### C++20 Features Not Supported

**Error:**
```
error: 'std::format' is not a member of 'std'
```

**Solution:** Upgrade to GCC 10+ or Clang 10+:
```bash
# Debian/Ubuntu
sudo apt-get install g++-10
export CXX=g++-10

# Or use Clang
sudo apt-get install clang-10
export CXX=clang++-10
```

### Node.js Version Too Old

**Error:**
```
error engine Unsupported engine
```

**Solution:** Install Node.js 18+:
```bash
# Using NodeSource
curl -fsSL https://deb.nodesource.com/setup_20.x | sudo -E bash -
sudo apt-get install -y nodejs

# Or using nvm
nvm install 20
nvm use 20
```

### Frontend Build Fails with Type Errors

The test files may have outdated type definitions. Build with Vite only:
```bash
cd frontend
./node_modules/.bin/vite build
```

## Development Environment

### Recommended IDE Setup

**VS Code Extensions:**
- C/C++ (Microsoft)
- CMake Tools
- ESLint
- Tailwind CSS IntelliSense

**CLion:**
- Open `simulator/` as CMake project
- Set CMake profile to use C++20

### Environment Variables

| Variable | Default | Description |
|----------|---------|-------------|
| `RATMS_DB_PATH` | `database/ratms.db` | SQLite database path |
| `RATMS_LOG_LEVEL` | `info` | Log level (debug, info, warn, error) |

## Clean Build

```bash
# Backend
rm -rf simulator/build
mkdir simulator/build

# Frontend
rm -rf frontend/node_modules frontend/dist
cd frontend && npm install
```

## Cross-Compilation

Not officially supported, but CMake toolchain files can be provided for:
- ARM64 Linux (Raspberry Pi)
- Windows (via MinGW-w64)

Contact maintainers for cross-compilation support.
