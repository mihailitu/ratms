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
#   ./scripts/download_osm.sh munich_center 11.54,48.12,11.60,48.16
#   ./scripts/download_osm.sh bucharest_victoriei 26.08,44.44,26.12,44.46
#
# Bounding box format: min_lon,min_lat,max_lon,max_lat
#
# Preset areas (copy and use as arguments):
#
# --- Neighborhoods (small, ~2x2 km, fast to process) ---
# munich_schwabing      11.56,48.15,11.60,48.17
# bucharest_victoriei   26.08,44.44,26.12,44.46
# bucharest_dristor     26.12,44.40,26.16,44.43
# bucharest_obor        26.12,44.44,26.16,44.47
# paris_marais          2.34,48.85,2.37,48.86
# london_soho           -0.14,51.51,-0.12,51.52
# frankfurt_sachsenhausen 8.67,50.09,8.71,50.11
#
# --- City centers (medium, ~5x5 km) ---
# munich_center         11.54,48.12,11.60,48.16
# bucharest_center      26.06,44.42,26.14,44.46
# paris_center          2.32,48.84,2.38,48.88
# london_center         -0.15,51.49,-0.08,51.53
# frankfurt_center      8.65,50.10,8.72,50.14
#
# --- Whole cities (large, may be slow to download/process) ---
# munich_full           11.36,48.06,11.72,48.25
# bucharest_full        25.95,44.35,26.25,44.55
# paris_full            2.22,48.80,2.47,48.92
# london_full           -0.25,51.45,0.05,51.58
# frankfurt_full        8.55,50.05,8.80,50.20

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
