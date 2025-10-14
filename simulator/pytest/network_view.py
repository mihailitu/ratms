#!/usr/bin/env python3
"""
Network Visualizer for Multi-Road Traffic Simulation

Displays all roads in the network and animates vehicle movement across roads.
Click on vehicles to inspect their data.
"""

import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from matplotlib.patches import FancyArrowPatch

# Load road network data
print("Loading road network...")
roads_data = np.loadtxt("roads.dat")

# Parse road data: roadID | startLon | startLat | endLon | endLat | startX | startY | endX | endY | length | maxSpeed | lanes
roads = {}
for road_data in roads_data:
    road_id = int(road_data[0])
    if road_data[10] == 0:  # Skip dummy map boundary road
        continue
    roads[road_id] = {
        'start': (road_data[5], road_data[6]),
        'end': (road_data[7], road_data[8]),
        'length': road_data[9],
        'maxSpeed': road_data[10],
        'lanes': int(road_data[11])
    }

print(f"Loaded {len(roads)} roads")

# Load vehicle simulation data (custom parser for mixed numeric/text data)
print("Loading simulation data...")
data_lines = []
with open("output.dat", 'r') as f:
    for line in f:
        parts = line.strip().split()
        # Convert traffic light states to numbers: G=1, Y=0.5, R=0
        converted = []
        for i, part in enumerate(parts):
            if part in ('G', 'Y', 'R'):
                converted.append(1.0 if part == 'G' else (0.5 if part == 'Y' else 0.0))
            else:
                try:
                    converted.append(float(part))
                except ValueError:
                    converted.append(0.0)  # Unknown values as 0
        if converted:
            data_lines.append(converted)

# Find maximum line length to pad shorter lines
max_len = max(len(line) for line in data_lines) if data_lines else 0
data_padded = []
for line in data_lines:
    padded = line + [np.nan] * (max_len - len(line))
    data_padded.append(padded)

data = np.array(data_padded)
max_time = data[:, 0].max()
print(f"Simulation duration: {max_time:.1f} seconds")

# Global variables for interaction
watch_vehicle_id = None
watch_vehicle_road = None

# Create figure
fig, ax = plt.subplots(figsize=(12, 10))

# Vehicle scatter plot
scat = ax.scatter([], [], c='blue', s=50, zorder=10)
info_text = ax.text(0.02, 0.98, '', transform=ax.transAxes,
                    verticalalignment='top', fontsize=10,
                    bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.8))


def onclick(event):
    """Handle mouse click to select vehicle"""
    global watch_vehicle_id, watch_vehicle_road
    if event.inaxes != ax or event.button != 1:
        return

    # Find closest vehicle to click
    if len(scat.get_offsets()) == 0:
        return

    positions = scat.get_offsets()
    distances = np.sqrt((positions[:, 0] - event.xdata)**2 + (positions[:, 1] - event.ydata)**2)
    closest_idx = np.argmin(distances)

    if distances[closest_idx] < 50:  # Within 50 units
        # Find which vehicle this is
        frame_no = int(ax.get_title().split(':')[1].split('s')[0].strip())
        time_val = frame_no * 0.5
        time_data = data[data[:, 0] == time_val]

        vehicle_idx = 0
        for row in time_data:
            road_id = int(row[1])
            if road_id not in roads:
                continue

            lanes_no = int(row[2])
            num_lights = lanes_no
            vehicles_start_idx = 3 + num_lights

            if vehicles_start_idx >= len(row):
                continue

            # Parse vehicles: x, v, a, id, lane
            vehicles = row[vehicles_start_idx:].reshape(-1, 5)
            for veh in vehicles:
                if vehicle_idx == closest_idx:
                    watch_vehicle_id = int(veh[3])
                    watch_vehicle_road = road_id
                    print(f"Selected vehicle {watch_vehicle_id} on road {road_id}")
                    return
                vehicle_idx += 1


def update(frame_no):
    """Update animation frame"""
    global watch_vehicle_id, watch_vehicle_road

    # Clear plot
    ax.clear()

    # Draw all roads as arrows
    for road_id, road in roads.items():
        arrow = FancyArrowPatch(
            road['start'], road['end'],
            arrowstyle='->', mutation_scale=20, linewidth=2,
            color='gray', alpha=0.6
        )
        ax.add_patch(arrow)

        # Label roads
        mid_x = (road['start'][0] + road['end'][0]) / 2
        mid_y = (road['start'][1] + road['end'][1]) / 2
        ax.text(mid_x, mid_y, f"R{road_id}", fontsize=8,
                bbox=dict(boxstyle='round', facecolor='white', alpha=0.7))

    # Find data for this time frame
    time_val = frame_no * 0.5
    time_data = data[data[:, 0] == time_val]

    if len(time_data) == 0:
        ax.set_title(f"Time: {time_val:.1f}s (no data)")
        return

    # Collect all vehicles across all roads
    all_positions = []
    all_colors = []
    all_vehicle_ids = []
    all_vehicle_data = []

    for row in time_data:
        road_id = int(row[1])
        if road_id not in roads:
            continue

        road = roads[road_id]
        lanes_no = int(row[2])
        num_lights = lanes_no
        vehicles_start_idx = 3 + num_lights

        if vehicles_start_idx >= len(row):
            continue

        # Parse vehicles: x, v, a, id, lane
        vehicle_data = row[vehicles_start_idx:]
        # Remove NaN padding
        vehicle_data = vehicle_data[~np.isnan(vehicle_data)]

        if len(vehicle_data) == 0 or len(vehicle_data) % 5 != 0:
            continue

        vehicles = vehicle_data.reshape(-1, 5)

        # Calculate road direction vector
        dx = road['end'][0] - road['start'][0]
        dy = road['end'][1] - road['start'][1]
        road_length = road['length']

        for veh in vehicles:
            pos_along_road = veh[0]
            vel = veh[1]
            acc = veh[2]
            veh_id = int(veh[3])
            lane = int(veh[4])

            # Interpolate position along road
            if road_length > 0:
                t = pos_along_road / road_length
                t = min(max(t, 0), 1)  # Clamp to [0, 1]
                x = road['start'][0] + t * dx
                y = road['start'][1] + t * dy
            else:
                x, y = road['start']

            all_positions.append([x, y])
            all_vehicle_ids.append(veh_id)
            all_vehicle_data.append((veh_id, road_id, pos_along_road, vel, acc, lane))

            # Color: red if selected, blue otherwise
            if veh_id == watch_vehicle_id:
                all_colors.append('red')
            else:
                all_colors.append('blue')

    # Draw vehicles
    if len(all_positions) > 0:
        positions = np.array(all_positions)
        global scat
        scat = ax.scatter(positions[:, 0], positions[:, 1], c=all_colors, s=50, zorder=10)

        # Label vehicles with IDs
        for i, (x, y) in enumerate(positions):
            ax.text(x, y+20, str(all_vehicle_ids[i]), fontsize=6, ha='center')

    # Display info about selected vehicle
    info_str = f"Frame: {frame_no}\nTime: {time_val:.1f}s\nVehicles: {len(all_positions)}"

    if watch_vehicle_id is not None:
        # Find selected vehicle in current frame
        for veh_id, road_id, pos, vel, acc, lane in all_vehicle_data:
            if veh_id == watch_vehicle_id:
                info_str += f"\n\n=== Vehicle {veh_id} ===\n"
                info_str += f"Road: {road_id}\n"
                info_str += f"Position: {pos:.1f} m\n"
                info_str += f"Speed: {vel * 3.6:.1f} km/h\n"
                info_str += f"Accel: {acc:.2f} m/s²\n"
                info_str += f"Lane: {lane}"
                watch_vehicle_road = road_id
                break
        else:
            # Vehicle not found (left simulation or not yet appeared)
            if watch_vehicle_road is not None:
                info_str += f"\n\n=== Vehicle {watch_vehicle_id} ===\n"
                info_str += f"Last seen on road {watch_vehicle_road}\n"
                info_str += "(left simulation)"

    info_text.set_text(info_str)

    # Set plot properties
    ax.set_aspect('equal')
    ax.set_title(f"Network Simulation - Time: {time_val:.1f}s")
    ax.set_xlabel("X (meters)")
    ax.set_ylabel("Y (meters)")
    ax.grid(True, alpha=0.3)

    # Auto-scale with some padding
    if len(all_positions) > 0 or len(roads) > 0:
        # Get bounds from roads
        x_coords = []
        y_coords = []
        for road in roads.values():
            x_coords.extend([road['start'][0], road['end'][0]])
            y_coords.extend([road['start'][1], road['end'][1]])

        if x_coords:
            padding = 100
            ax.set_xlim(min(x_coords) - padding, max(x_coords) + padding)
            ax.set_ylim(min(y_coords) - padding, max(y_coords) + padding)


# Connect click event
fig.canvas.mpl_connect('button_press_event', onclick)

# Create animation
num_frames = int(max_time / 0.5) + 1
print(f"Creating animation with {num_frames} frames...")
anim = animation.FuncAnimation(fig, update, frames=num_frames, interval=100, repeat=True)

plt.tight_layout()
print("Starting visualization. Click on vehicles to inspect them.")
plt.show()
