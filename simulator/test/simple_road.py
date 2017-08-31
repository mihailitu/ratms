import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation

# TODO: see https://matplotlib.org/examples/animation/rain.html for scatter plots

# no of cars
N = 2
road_length = 5000
# update interval same as dt from simulation ODE = 0.5 sec
interval = 500
# vehicles data
cars = np.zeros(N, dtype=[('position', float, 2),
                          ('time', float, 1),
                          ('velocity', float, 1),
                          ('acceleration', float, 1)])

# don't update y pos
cars['position'][:, 1] = np.full((1, N), 0)

# setup view
fig = plt.figure(figsize=(15, 3))
ax = plt.axes(xlim=(0, road_length), ylim=(-1, 1))
scat = plt.scatter(cars['position'][:, 0], cars['position'][:, 1])

#
info = plt.text(0, 1, 'Some \n text', size=10)


def update(frame_no):
    cars['position'][0][0] += 10
    cars['position'][1][0] += (frame_no % 2) * 10

    scat.set_offsets(cars['position'])
    info.set_text('Frame: $%3d$ \nSpeed: ' % frame_no)
    fig.canvas.draw()

animation = animation.FuncAnimation(fig, update, interval=10)

plt.show()
