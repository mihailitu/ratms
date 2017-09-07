import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation

#
#
# *                 |                    | vehicle 0  | vehicle 1 | ...... | vehicle n | vehicle n+1 |
# * time0 | roadID0 | roadlen | maxSpeed |  x | v | a | x | v | a | .......|
# * time0 | roadID1 | roadlen | maxSpeed |  x | v | a | x | v | a | .......| x | v | a |
# * time1 | roadID0 | roadlen | maxSpeed |  x | v | a | x | v | a | .......| x | v | a |
# * time1 | roadID1 | roadlen | maxSpeed |  x | v | a | x | v | a | .......| x | v | a |

# only one lane on road for now

data = np.loadtxt("v1test.dat")
# vehicle data for first road at one time (0.0)
vehicle_data = data[0][4:].reshape((-1, 3))

# Single road with one lane and one vehicle
# This is a view for testmap.cpp : getSigleVehicleTestMap() test
# Enter values manually

# no of cars
N = len(vehicle_data)
road_length = data[0][2]
max_speed = data[0][3]
# update interval same as dt from simulation ODE = 0.5 sec
interval = 500
# vehicles data
cars = np.zeros(N, dtype=[('position', float, 2),
                          ('time', float, 1),
                          ('velocity', float, 1),
                          ('acceleration', float, 1),
                          ('lane', int, 1),
                          # if this vehicle is selected for data info,
                          # change it's color
                          ('color', int, 1)])

cars['color'] = np.full((1, N), 255)
# index of the vehicle we want to see data for (speed, acc, etc)
watch_vehicle = 1

# setup view
fig = plt.figure(figsize=(15, 3))
ax = plt.axes(xlim=(0, road_length), ylim=(-1, 1))
scat = plt.scatter(cars['position'][:, 0], cars['position'][:, 1])


# construct the info text: frame number, running time, velocity, acceleration and lane of vehicle
def info_text(frame, time, velocity, acc, lane):
    text = 'Frame: $%4d$\n' \
          'Time: $%', frame
    return text


info = plt.text(road_length, 0.0, 'Some \n text', size=10)


def mps_to_kmph(mps):
    return mps * 3.6


def update(frame_no):
    global vehicle_data, N, cars, scat

    # update vehicle data for current time frame
    vehicle_data = data[frame_no][4:].reshape((-1, 3))
    N = len(vehicle_data)

    cars = np.zeros(N, dtype=[('position', float, 2),
                              ('time', float, 1),
                              ('velocity', float, 1),
                              ('acceleration', float, 1),
                              ('lane', int, 1),
                              ('color', int, 1)])

    # get the x pos of all cars at a time frame
    cars['position'][:, 0] = vehicle_data[:, 0]
    # don't update y pos for single lane - single road test simulation
    cars['position'][:, 1] = np.full((1, N), 0)

    scat.set_offsets(cars['position'])

    # for i in range(0, len(cars)):
    #     if i == watch_vehicle:
    #         scat = plt.scatter(cars['position'][watch_vehicle][0], cars['position'][watch_vehicle][1])
    #     else:
    #         scat = plt.scatter(cars['position'][i][0], cars['position'][i][1])

    speed = vehicle_data[watch_vehicle][1]
    acc = vehicle_data[watch_vehicle][2]
    info.set_text('Frame:  $%3d$\n'
                  'Length: $%d$ m\n'
                  'Max v:  $%d$ km/h\n'
                  '\n'
                  '\n'
                  'Speed: $%d$ km/h\n'
                  'Acc:   $%f$\n'
                  % (frame_no, road_length, mps_to_kmph(max_speed), mps_to_kmph(speed), acc))

    return scat


animation = animation.FuncAnimation(fig, update, interval=10)

plt.show()
