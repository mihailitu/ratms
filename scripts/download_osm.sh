#!/bin/bash
#
# Download OpenStreetMap data for RATMS traffic simulation
#
# This script downloads road network data from OpenStreetMap using the Overpass API
# and converts it to the JSON format used by the RATMS simulator.
#
# Prerequisites:
#   - wget or curl
#   - RATMS osm_import tool (build with: cd simulator/build && make osm_import)
#
# Usage:
#   ./scripts/download_osm.sh [area_name] [bbox]
#
# Examples:
#   ./scripts/download_osm.sh                                    # Default: Munich Schwabing
#   ./scripts/download_osm.sh munich_city 11.54,48.12,11.60,48.16
#   ./scripts/download_osm.sh berlin 13.36,52.50,13.42,52.54
#
# Bounding box format: min_lon,min_lat,max_lon,max_lat

set -e

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
OSM_DIR="$PROJECT_ROOT/data/osm"
MAPS_DIR="$PROJECT_ROOT/data/maps"
OSM_IMPORT="$PROJECT_ROOT/simulator/build/osm_import"

# Default values (Munich Schwabing - ~2km x 2km area)
AREA_NAME="${1:-munich_schwabing}"
BBOX="${2:-11.56,48.15,11.60,48.17}"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

error() {
    echo -e "${RED}[ERROR]${NC} $1"
    exit 1
}

# Check prerequisites
check_prerequisites() {
    if ! command -v wget &> /dev/null && ! command -v curl &> /dev/null; then
        error "wget or curl is required but not installed"
    fi

    if [ ! -x "$OSM_IMPORT" ]; then
        warn "osm_import tool not found at $OSM_IMPORT"
        info "Building osm_import..."
        cd "$PROJECT_ROOT/simulator/build"
        if [ ! -f "Makefile" ]; then
            cmake ..
        fi
        make osm_import
        cd "$PROJECT_ROOT"
    fi
}

# Create directories
setup_directories() {
    mkdir -p "$OSM_DIR"
    mkdir -p "$MAPS_DIR"
}

# Download OSM data
download_osm() {
    local osm_file="$OSM_DIR/${AREA_NAME}.osm"
    local overpass_url="https://overpass-api.de/api/map?bbox=${BBOX}"

    info "Downloading OSM data for $AREA_NAME..."
    info "Bounding box: $BBOX"
    info "URL: $overpass_url"

    if command -v wget &> /dev/null; then
        wget -O "$osm_file" "$overpass_url"
    else
        curl -o "$osm_file" "$overpass_url"
    fi

    if [ ! -s "$osm_file" ]; then
        error "Download failed or file is empty"
    fi

    local size=$(du -h "$osm_file" | cut -f1)
    info "Downloaded: $osm_file ($size)"
}

# Convert OSM to JSON
convert_to_json() {
    local osm_file="$OSM_DIR/${AREA_NAME}.osm"
    local json_file="$MAPS_DIR/${AREA_NAME}.json"

    info "Converting OSM to JSON format..."

    "$OSM_IMPORT" "$osm_file" "$json_file" "$AREA_NAME"

    if [ ! -s "$json_file" ]; then
        error "Conversion failed or output file is empty"
    fi

    local size=$(du -h "$json_file" | cut -f1)
    info "Created: $json_file ($size)"
}

# Print summary
print_summary() {
    local json_file="$MAPS_DIR/${AREA_NAME}.json"

    echo ""
    info "Download complete!"
    echo ""
    echo "Files created:"
    echo "  OSM data: $OSM_DIR/${AREA_NAME}.osm"
    echo "  JSON map: $json_file"
    echo ""
    echo "To run the simulation with this map:"
    echo "  cd simulator/build"
    echo "  ./api_server --network $json_file"
    echo ""
    echo "Then start the frontend:"
    echo "  cd frontend"
    echo "  npm run dev"
    echo ""
}

# Main
main() {
    info "RATMS OpenStreetMap Data Downloader"
    echo ""

    check_prerequisites
    setup_directories
    download_osm
    convert_to_json
    print_summary
}

main
