#!/usr/bin/env python3
"""
RATMS API Test Script
Tests the traffic simulation API with Python instead of the web frontend
"""

import requests
import json
import time
import sys
from typing import Optional, Dict, Any

# Configuration
API_BASE_URL = "http://localhost:8080"
TIMEOUT = 5  # seconds


class RATMSAPIClient:
    """Client for interacting with RATMS API"""

    def __init__(self, base_url: str = API_BASE_URL):
        self.base_url = base_url
        self.session = requests.Session()

    def health_check(self) -> Dict[str, Any]:
        """Check if the API server is running"""
        response = self.session.get(f"{self.base_url}/api/health", timeout=TIMEOUT)
        response.raise_for_status()
        return response.json()

    def get_status(self) -> Dict[str, Any]:
        """Get current simulation status"""
        response = self.session.get(f"{self.base_url}/api/simulation/status", timeout=TIMEOUT)
        response.raise_for_status()
        return response.json()

    def start_simulation(self) -> Dict[str, Any]:
        """Start the simulation"""
        response = self.session.post(f"{self.base_url}/api/simulation/start", timeout=TIMEOUT)
        response.raise_for_status()
        return response.json()

    def stop_simulation(self) -> Dict[str, Any]:
        """Stop the simulation"""
        response = self.session.post(f"{self.base_url}/api/simulation/stop", timeout=TIMEOUT)
        response.raise_for_status()
        return response.json()

    def get_roads(self) -> Dict[str, Any]:
        """Get road network information"""
        response = self.session.get(f"{self.base_url}/api/simulation/roads", timeout=TIMEOUT)
        response.raise_for_status()
        return response.json()

    def get_simulations(self) -> Dict[str, Any]:
        """Get simulation history"""
        response = self.session.get(f"{self.base_url}/api/simulations", timeout=TIMEOUT)
        response.raise_for_status()
        return response.json()


def print_header(text: str):
    """Print a formatted header"""
    print(f"\n{'='*60}")
    print(f"  {text}")
    print(f"{'='*60}")


def print_section(text: str):
    """Print a formatted section"""
    print(f"\n{'-'*60}")
    print(f"  {text}")
    print(f"{'-'*60}")


def test_health(client: RATMSAPIClient):
    """Test 1: Health check"""
    print_header("TEST 1: Health Check")
    try:
        health = client.health_check()
        print(f"✓ Server is running")
        print(f"  Service: {health.get('service', 'N/A')}")
        print(f"  Version: {health.get('version', 'N/A')}")
        print(f"  Status: {health.get('status', 'N/A')}")
        return True
    except Exception as e:
        print(f"✗ Health check failed: {e}")
        return False


def test_road_network(client: RATMSAPIClient):
    """Test 2: Get road network information"""
    print_header("TEST 2: Road Network Information")
    try:
        roads_data = client.get_roads()
        roads = roads_data.get('roads', [])
        print(f"✓ Found {len(roads)} roads in the network")

        for i, road in enumerate(roads, 1):
            print(f"\n  Road {i}:")
            print(f"    ID: {road.get('id', 'N/A')}")
            print(f"    Length: {road.get('length', 'N/A')} meters")
            print(f"    Lanes: {road.get('lanes', 'N/A')}")
            print(f"    Max Speed: {road.get('maxSpeed', 'N/A')} m/s")
            print(f"    Start: ({road.get('startLon', 'N/A')}, {road.get('startLat', 'N/A')})")
            print(f"    End: ({road.get('endLon', 'N/A')}, {road.get('endLat', 'N/A')})")

        return True
    except Exception as e:
        print(f"✗ Failed to get road network: {e}")
        return False


def test_initial_status(client: RATMSAPIClient):
    """Test 3: Get initial status"""
    print_header("TEST 3: Initial Simulation Status")
    try:
        status = client.get_status()
        print(f"✓ Status retrieved successfully")
        print(f"  Running: {status.get('isRunning', False)}")
        print(f"  Current Step: {status.get('currentStep', 0)}")
        print(f"  Simulation Time: {status.get('simulationTime', 0):.2f}s")
        print(f"  Total Vehicles: {status.get('totalVehicles', 0)}")
        print(f"  Active Vehicles: {status.get('activeVehicles', 0)}")
        return status
    except Exception as e:
        print(f"✗ Failed to get status: {e}")
        return None


def test_start_simulation(client: RATMSAPIClient):
    """Test 4: Start simulation"""
    print_header("TEST 4: Start Simulation")
    try:
        result = client.start_simulation()
        print(f"✓ Simulation started")
        print(f"  Message: {result.get('message', 'N/A')}")
        print(f"  Status: {result.get('status', 'N/A')}")
        return True
    except Exception as e:
        print(f"✗ Failed to start simulation: {e}")
        return False


def test_monitor_simulation(client: RATMSAPIClient, duration: int = 10):
    """Test 5: Monitor simulation for a period"""
    print_header(f"TEST 5: Monitor Simulation ({duration}s)")
    print("Monitoring simulation progress...\n")

    try:
        start_time = time.time()
        prev_step = 0

        while time.time() - start_time < duration:
            status = client.get_status()

            current_step = status.get('currentStep', 0)
            sim_time = status.get('simulationTime', 0)
            active_vehicles = status.get('activeVehicles', 0)
            total_vehicles = status.get('totalVehicles', 0)

            # Only print when step changes
            if current_step != prev_step:
                elapsed = time.time() - start_time
                print(f"  [{elapsed:5.1f}s] Step {current_step:4d} | "
                      f"SimTime: {sim_time:6.1f}s | "
                      f"Vehicles: {active_vehicles}/{total_vehicles}")
                prev_step = current_step

            time.sleep(0.5)  # Poll every 500ms

        print(f"\n✓ Monitoring completed")
        return True
    except Exception as e:
        print(f"\n✗ Monitoring failed: {e}")
        return False


def test_stop_simulation(client: RATMSAPIClient):
    """Test 6: Stop simulation"""
    print_header("TEST 6: Stop Simulation")
    try:
        result = client.stop_simulation()
        print(f"✓ Simulation stopped")
        print(f"  Message: {result.get('message', 'N/A')}")
        print(f"  Status: {result.get('status', 'N/A')}")
        return True
    except Exception as e:
        print(f"✗ Failed to stop simulation: {e}")
        return False


def test_final_status(client: RATMSAPIClient):
    """Test 7: Get final status"""
    print_header("TEST 7: Final Simulation Status")
    try:
        status = client.get_status()
        print(f"✓ Final status retrieved")
        print(f"  Running: {status.get('isRunning', False)}")
        print(f"  Final Step: {status.get('currentStep', 0)}")
        print(f"  Final Time: {status.get('simulationTime', 0):.2f}s")
        print(f"  Total Vehicles: {status.get('totalVehicles', 0)}")
        print(f"  Active Vehicles: {status.get('activeVehicles', 0)}")
        return status
    except Exception as e:
        print(f"✗ Failed to get final status: {e}")
        return None


def main():
    """Run all tests"""
    print_header("RATMS API Test Suite")
    print(f"Testing API at: {API_BASE_URL}")

    client = RATMSAPIClient()

    # Test 1: Health check
    if not test_health(client):
        print("\n❌ Server is not running. Please start the API server first:")
        print("   cd simulator/build && ./api_server")
        sys.exit(1)

    # Test 2: Road network
    test_road_network(client)

    # Test 3: Initial status
    test_initial_status(client)

    # Test 4: Start simulation
    if not test_start_simulation(client):
        sys.exit(1)

    # Test 5: Monitor simulation
    test_monitor_simulation(client, duration=10)

    # Test 6: Stop simulation
    test_stop_simulation(client)

    # Test 7: Final status
    test_final_status(client)

    # Summary
    print_header("Test Suite Complete")
    print("✓ All tests passed successfully!")
    print("\nNext steps:")
    print("  - View simulation history: GET /api/simulations")
    print("  - Try the web dashboard: cd frontend && npm run dev")
    print("  - Run GA optimization: ./ga_optimizer --help")


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\nTest interrupted by user")
        sys.exit(0)
    except Exception as e:
        print(f"\n\n❌ Unexpected error: {e}")
        sys.exit(1)
