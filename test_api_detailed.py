#!/usr/bin/env python3
"""
RATMS API Detailed Test Script
Shows real-time vehicle positions and movements
"""

import requests
import json
import time
import sys

API_BASE_URL = "http://localhost:8080"


def print_header(text: str):
    print(f"\n{'='*70}")
    print(f"  {text}")
    print(f"{'='*70}")


def test_with_streaming():
    """Test with Server-Sent Events for real-time updates"""
    from sseclient import SSEClient  # pip install sseclient-py

    print_header("RATMS Real-Time Vehicle Tracking Test")

    # Start simulation
    print("\n▶ Starting simulation...")
    response = requests.post(f"{API_BASE_URL}/api/simulation/start")
    if response.status_code != 200:
        print(f"✗ Failed to start: {response.text}")
        return

    print("✓ Simulation started")
    print("\n▶ Connecting to real-time stream...")
    print("   (monitoring for 15 seconds)\n")

    # Connect to SSE stream
    try:
        messages = SSEClient(f"{API_BASE_URL}/api/simulation/stream")
        start_time = time.time()
        update_count = 0
        max_duration = 15  # seconds

        for msg in messages:
            if time.time() - start_time > max_duration:
                break

            if msg.event == 'update':
                update_count += 1
                data = json.loads(msg.data)

                step = data.get('step', 0)
                sim_time = data.get('time', 0)
                vehicles = data.get('vehicles', [])
                lights = data.get('trafficLights', [])

                # Print update every 20 steps
                if step % 20 == 0:
                    elapsed = time.time() - start_time
                    print(f"[{elapsed:5.1f}s] Step {step:4d} | Time: {sim_time:6.1f}s | Vehicles: {len(vehicles)}")

                    # Show vehicle details
                    for v in vehicles[:5]:  # Show first 5 vehicles
                        print(f"  └─ Vehicle {v.get('id', '?'):2d}: "
                              f"Road {v.get('roadId', '?')} Lane {v.get('lane', '?')} | "
                              f"Pos: {v.get('position', 0):6.1f}m | "
                              f"Speed: {v.get('velocity', 0):5.2f} m/s")

                    # Show traffic light states
                    light_states = {}
                    for light in lights:
                        road_id = light.get('roadId')
                        state = light.get('state', '?')
                        if road_id not in light_states:
                            light_states[road_id] = []
                        light_states[road_id].append(state)

                    if light_states:
                        print(f"  └─ Lights: ", end="")
                        for road_id, states in sorted(light_states.items()):
                            print(f"Road {road_id}:[{','.join(states)}] ", end="")
                        print()

                    print()

    except KeyboardInterrupt:
        print("\n\n⚠ Interrupted by user")
    except Exception as e:
        print(f"\n✗ Stream error: {e}")
        print("   Note: Install sseclient-py with: pip install sseclient-py")

    # Stop simulation
    print("\n▶ Stopping simulation...")
    response = requests.post(f"{API_BASE_URL}/api/simulation/stop")
    if response.status_code == 200:
        print("✓ Simulation stopped")
    else:
        print(f"✗ Failed to stop: {response.text}")


def test_without_streaming():
    """Test by polling status endpoint (no SSE library needed)"""
    print_header("RATMS Vehicle Tracking Test (Polling Mode)")

    # Start simulation
    print("\n▶ Starting simulation...")
    response = requests.post(f"{API_BASE_URL}/api/simulation/start")
    if response.status_code != 200:
        print(f"✗ Failed to start: {response.text}")
        return

    print("✓ Simulation started")
    print("\n▶ Monitoring simulation (15 seconds)...\n")

    start_time = time.time()
    max_duration = 15
    prev_step = -1

    while time.time() - start_time < max_duration:
        try:
            # Get roads info to see vehicles
            response = requests.get(f"{API_BASE_URL}/api/simulation/roads", timeout=2)
            if response.status_code == 200:
                data = response.json()
                roads = data.get('roads', [])

                # Count total vehicles
                total_vehicles = 0
                vehicle_info = []

                for road in roads:
                    road_id = road.get('id')
                    lanes = road.get('lanesVehicles', [])

                    for lane_idx, lane_vehicles in enumerate(lanes):
                        for v in lane_vehicles:
                            total_vehicles += 1
                            vehicle_info.append({
                                'id': v.get('id'),
                                'road': road_id,
                                'lane': lane_idx,
                                'pos': v.get('position', 0),
                                'vel': v.get('velocity', 0)
                            })

                elapsed = time.time() - start_time
                print(f"[{elapsed:5.1f}s] Active vehicles: {total_vehicles}")

                # Show first 5 vehicles
                for v in vehicle_info[:5]:
                    print(f"  └─ Vehicle {v['id']:2d}: "
                          f"Road {v['road']} Lane {v['lane']} | "
                          f"Pos: {v['pos']:6.1f}m | "
                          f"Speed: {v['vel']:5.2f} m/s")

                if total_vehicles > 5:
                    print(f"  └─ ... and {total_vehicles - 5} more vehicles")

                print()

        except Exception as e:
            print(f"✗ Error: {e}")

        time.sleep(1.5)  # Poll every 1.5 seconds

    # Stop simulation
    print("\n▶ Stopping simulation...")
    response = requests.post(f"{API_BASE_URL}/api/simulation/stop")
    if response.status_code == 200:
        print("✓ Simulation stopped")
    else:
        print(f"✗ Failed to stop: {response.text}")


if __name__ == "__main__":
    # Check server health
    try:
        response = requests.get(f"{API_BASE_URL}/api/health", timeout=2)
        if response.status_code != 200:
            print("❌ Server is not responding. Please start it first:")
            print("   cd simulator/build && ./api_server")
            sys.exit(1)
    except Exception as e:
        print(f"❌ Cannot connect to server: {e}")
        print("   Please start the server: cd simulator/build && ./api_server")
        sys.exit(1)

    # Try streaming first, fall back to polling
    try:
        import sseclient
        test_with_streaming()
    except ImportError:
        print("ℹ  sseclient-py not installed, using polling mode")
        print("   For real-time streaming: pip install sseclient-py\n")
        test_without_streaming()

    print_header("Test Complete")
    print("\n✓ You can now:")
    print("  - Check the web dashboard: cd frontend && npm run dev")
    print("  - View metrics in database: sqlite3 ratms.db")
    print("  - Run GA optimization: cd simulator/build && ./ga_optimizer")
