#!/usr/bin/env python3
"""
Simple RATMS Test - Monitor simulation via metrics
"""

import requests
import sqlite3
import time

API_BASE_URL = "http://localhost:8080"


def print_header(text):
    print(f"\n{'='*60}")
    print(f"  {text}")
    print(f"{'='*60}")


def test_simulation():
    """Run a simple simulation test"""
    print_header("RATMS Simple Test")

    # 1. Check health
    print("\n1. Health Check...")
    r = requests.get(f"{API_BASE_URL}/api/health")
    health = r.json()
    print(f"   ✓ Server: {health['service']} v{health['version']}")

    # 2. Get network info
    print("\n2. Network Information...")
    r = requests.get(f"{API_BASE_URL}/api/simulation/roads")
    roads = r.json()
    active_roads = [rd for rd in roads['roads'] if rd['maxSpeed'] > 0]
    print(f"   ✓ Roads: {len(active_roads)} active roads")
    for road in active_roads:
        print(f"     - Road {road['id']}: {road['length']}m, "
              f"{road['lanes']} lane(s), max {road['maxSpeed']} m/s")

    # 3. Start simulation
    print("\n3. Starting Simulation...")
    r = requests.post(f"{API_BASE_URL}/api/simulation/start")
    print(f"   ✓ {r.json()['message']}")

    # 4. Monitor for 20 seconds
    print("\n4. Running Simulation (20 seconds)...")
    print("   Watch the server terminal to see vehicle transitions!\n")
    time.sleep(20)

    # 5. Stop simulation
    print("\n5. Stopping Simulation...")
    r = requests.post(f"{API_BASE_URL}/api/simulation/stop")
    print(f"   ✓ {r.json()['message']}")

    # 6. Get simulation history from database
    print("\n6. Simulation Results (from database)...")
    try:
        conn = sqlite3.connect('simulator/build/ratms.db')
        cursor = conn.cursor()

        # Get latest simulation
        cursor.execute("""
            SELECT id, name, created_at, completed_at, status
            FROM simulations
            ORDER BY id DESC
            LIMIT 1
        """)
        sim = cursor.fetchone()
        if sim:
            sim_id, name, created, completed, status = sim
            print(f"   ✓ Simulation #{sim_id}: {status}")

            # Get metrics
            cursor.execute("""
                SELECT timestamp, avg_queue_length, avg_vehicle_speed, vehicles_exited
                FROM metrics
                WHERE simulation_id = ?
                ORDER BY timestamp DESC
                LIMIT 5
            """, (sim_id,))

            metrics = cursor.fetchall()
            if metrics:
                print(f"\n   Recent Metrics (last 5 samples):")
                print(f"   {'Time':>8} | {'Avg Queue':>10} | {'Avg Speed':>10} | {'Exited':>7}")
                print(f"   {'-'*8}-+-{'-'*10}-+-{'-'*10}-+-{'-'*7}")
                for ts, queue, speed, exited in reversed(metrics):
                    print(f"   {ts:8.1f}s | {queue:10.2f} | {speed:8.2f} m/s | {exited:7d}")

        conn.close()
    except Exception as e:
        print(f"   ⚠ Could not read database: {e}")

    print_header("Test Complete!")
    print("\n✓ The simulation is working! Check the server logs above")
    print("  to see vehicles transitioning between roads.\n")
    print("Next steps:")
    print("  - View web dashboard: cd frontend && npm run dev")
    print("  - Run GA optimizer: cd simulator/build && ./ga_optimizer")
    print("  - Query database: sqlite3 simulator/build/ratms.db")


if __name__ == "__main__":
    try:
        test_simulation()
    except KeyboardInterrupt:
        print("\n\nInterrupted by user")
    except Exception as e:
        print(f"\n\nError: {e}")
