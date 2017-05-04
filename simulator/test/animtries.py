#https://nickcharlton.net/posts/drawing-animating-shapes-matplotlib.html
#http://jakevdp.github.io/blog/2012/08/18/matplotlib-animation-tutorial/

import numpy as np
from matplotlib import pyplot as plt
from matplotlib import animation

fig = plt.figure()
fig.set_dpi(100)
fig.set_size_inches(7, 6.5)

ax = plt.axes(xlim=(0, 10), ylim=(0, 10))

rect = plt.Rectangle((1, 1), 2, 2, fc='r')
dotted_line = plt.Line2D((2, 8), (4, 4), lw=5.,
                         ls='-.', marker='.',
                         markersize=50,
                         markerfacecolor='r',
                         markeredgecolor='r',
                         alpha=0.5)


points = [[2, 4], [2, 8], [4, 6], [6, 8]]
line = plt.Polygon(points, closed=None, fill=None, edgecolor='r')
circle = plt.Circle((10, 10), radius=0.1, fc='b')

#plt.axis('scaled')


def init():
    ax.add_line(line)
    ax.add_line(dotted_line)
    ax.add_patch(rect)
    ax.add_patch(circle)
    return rect, circle

def animate(dt):
    x, y = rect.get_xy()
    x = x + 0.1
    y = y + 0.2
    rect.set_xy((x, y))
    x, y = circle.center
    x = x - 0.1
    y = y - 0.2
    circle.center = (x, y)
    return rect, circle

anim = animation.FuncAnimation(fig, animate,
                               init_func=init,
                               frames=360,
                               interval=100,
                               blit=True)

plt.show()