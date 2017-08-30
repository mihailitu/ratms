import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation

## TODO: see https://matplotlib.org/examples/animation/rain.html for scatter plots

def update_line(num, data, line):
    print "update_line"
    data[0][0] = data[0][0] + 0.01
    line.set_data(data)
    return line,

fig = plt.figure(figsize=(15, 3))
ax = fig.add_axes([0, 0, 1, 1], frameon=False)
ax.set_xlim(0, 1), ax.set_xticks([])
ax.set_ylim(0, 1), ax.set_yticks([])

# no of cars
N = 2

cars = np.zeros(N, dtype=[('position', float, 2),
                          ('time', float, 1),
                          ('speed', float, 1),
                          ('acceleration', float, 1)])

# x coord of each car
X = np.zeros(N)
# y coord of each car - stay on the middle of the screen
Y = np.full(1, 0.5)

#plot coordinates
data = [X, Y]
# print "data " + str(data)
l, = plt.plot([], [], 'b>')
# plt.show()
# exit()
plt.xlim(0, 1)
plt.ylim(0, 1)
# plt.xlabel('x')
# plt.title('test')

line_ani = animation.FuncAnimation(fig, update_line, 25, fargs=(data, l),
                                   interval=500, blit=True)
plt.show()
