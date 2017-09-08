import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation

#
#
# *                 |                     | vehicle 0         | vehicle 1 | ...... | vehicle n     | vehicle n+1 |
# * time0 | roadID0 | maxSpeed | lanes_no | x | v | a | l | x | v | a | l | .......| x | v | a | l |
# * time0 | roadID1 | maxSpeed | lanes_no | x | v | a | l | x | v | a | l | .......| x | v | a | l |
# * time1 | roadID0 | maxSpeed | lanes_no | x | v | a | l | x | v | a | l | .......| x | v | a | l |
# * time1 | roadID1 | maxSpeed | lanes_no | x | v | a | l | x | v | a | l | .......| x | v | a | l |

# only one lane on road for now

data = np.loadtxt("simple_road.dat")
# vehicle data for first road at one time (0.0)
vehicle_data = data[0][5:].reshape((-1, 4))

# Single road with one lane and one vehicle
# This is a view for testmap.cpp : getSigleVehicleTestMap()/getFolowingVehicle() test
# Enter values manually

# no of cars
N = len(vehicle_data)
road_length = data[0][2]
max_speed = data[0][3]
lanes_no = data[0][4]
# update interval same as dt from simulation ODE = 0.5 sec
interval = 500
# vehicles data
cars = np.zeros(N, dtype=[('position', float, 2),
                          ('time', float, 1),
                          ('velocity', float, 1),
                          ('acceleration', float, 1),
                          ('lane', int, 1)])

colors = ['blue' for x in range(N)]

# index of the vehicle we want to see data for (speed, acc, etc)
watch_vehicle = 1

# setup view
fig = plt.figure(figsize=(15, 3))
ax = plt.axes(xlim=(0, road_length), ylim=(-1, 1))
scat = ax.scatter(cars['position'][:, 0], cars['position'][:, 1], c=colors)


def onclick(event):
    # print('button=%d, x=%d, y=%d, xdata=%f, ydata=%f' %
    #       (event.button, event.x, event.y, event.xdata, event.ydata))
    global watch_vehicle
    if event.button == 1:  # select the closest vehicle to left mouse clicks
        watch_vehicle = (np.abs(cars['position'][:, 0]-event.xdata)).argmin()


cid = fig.canvas.mpl_connect('button_press_event', onclick)

info = plt.text(road_length, 0.0, 'Some \n text', size=10)


def mps_to_kmph(mps):
    return mps * 3.6


def update(frame_no):
    global vehicle_data, N, cars, colors

    # update vehicle data for current time frame
    vehicle_data = data[frame_no][5:].reshape((-1, 4))
    N = len(vehicle_data)

    cars = np.zeros(N, dtype=[('position', float, 2),
                              ('time', float, 1),
                              ('velocity', float, 1),
                              ('acceleration', float, 1),
                              ('lane', int, 1)])

    # get the x pos of all cars at a time frame
    cars['position'][:, 0] = vehicle_data[:, 0]
    # don't update y pos for single lane - single road test simulation
    cars['position'][:, 1] = np.full((1, N), 0)
    cars['position'][:, 1] = 
    scat.set_offsets(cars['position'])

    cars['lane'] = vehicle_data[:, 3]

    colors = ['blue' for x in range(N)]
    colors[watch_vehicle] = 'red'
    scat.set_facecolor(colors)

    speed = vehicle_data[watch_vehicle][1]
    acc = vehicle_data[watch_vehicle][2]

    info.set_text('Frame:  $%3d$\n'
                  'Length: $%d$ m\n'
                  'Max v:  $%d$ km/h\n'
                  'Lanes:  $%d$\n'
                  '\n'
                  '\n'
                  'Speed: $%d$ km/h\n'
                  'Acc:   $%f$\n'
                  'Lane:  $%d$'
                  % (frame_no, road_length, mps_to_kmph(max_speed), lanes_no, mps_to_kmph(speed),
                     acc, cars['lane'][watch_vehicle]))

    return scat


animation = animation.FuncAnimation(fig, update, interval=10)

plt.show()
