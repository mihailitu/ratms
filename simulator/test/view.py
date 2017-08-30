import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation


def update_line(num, data, line):
    print "update_line"
    data[0][0] = data[0][0] + 0.01
    line.set_data(data)
    return line,

fig1 = plt.figure()

# no of cars
N = 2
# x coord of each car
X = np.zeros(N)
# y coord of each car - stay on the middle of the screen
Y = np.full(1, 0.5)

#plot coordinates
data = [X, Y]
print "data " + str(data)
l, = plt.plot([], [], 'b>')
# plt.show()
# exit()
plt.xlim(0, 1)
plt.ylim(0, 1)
plt.xlabel('x')
plt.title('test')

line_ani = animation.FuncAnimation(fig1, update_line, 25, fargs=(data, l),
                                   interval=500, blit=True)
plt.show()
