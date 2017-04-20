import sys
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import matplotlib.animation as animation
import numpy as np

saveAnim = False;

if len(sys.argv) <= 1:
    data = np.loadtxt("cars_test.dat")
else:
    data = np.loadtxt(sys.argv[1])
    dataLight = np.loadtxt(sys.argv[1] + "_lights.dat")
    print dataLight
    dataOb = np.loadtxt(sys.argv[1] + "_obstacles.dat")
    print dataOb
    if len(sys.argv) == 3:
        saveAnim = (sys.argv[2] == "save")

n = (len(data[0]) - 1) // 2
tt = data[:, 0]
xx = data[:, 1:n + 1]
vv = data[:, n + 1:-1]
throughput = data[:, -1]

nLights = int(dataLight[0, 0])
xxLights = dataLight[:, 1::2]
onLights = dataLight[:, 2::2]

nObstacles = int(dataOb[0, 0])
startOb = dataOb[0, 1::2]
endOb = dataOb[0, 2::2]

# traveled distance of car0;
distanceX0 = 0
oldDistance = 0
loops = 0

road_length = np.max(xx);

# First set up the figure, the axis, and the plot element we want to animate
fig = plt.figure()
ax = plt.axes()
plt.axis('equal')
scat = ax.scatter(xx[0, :], np.zeros_like(xx[0, :]))
ax.hold(False)
old_data = xx[0, :];


def drawProgressBar(percent, barLen=50):
    sys.stdout.write("\r")
    progress = ""
    for i in range(barLen):
        if i < int(barLen * percent):
            progress += "="
        else:
            progress += " "
    sys.stdout.write("[ %s ] %.2f%%" % (progress, percent * 100))
    sys.stdout.flush()


def arc_patch(r1, r2, start, end, road_length, axx=None, resolution=50, **kwargs):
    # make sure ax is not empty
    if axx is None:
        axx = plt.gca()

    # calculate theta1 and theta2
    theta1 = start / road_length * 2 * np.pi
    theta2 = end / road_length * 2 * np.pi
    # generate the points
    theta = np.linspace(theta1, theta2, resolution)

    points = np.vstack((np.hstack((r2 * np.cos(theta), r1 * np.cos(theta[::-1]))),
                        np.hstack((r2 * np.sin(theta), r1 * np.sin(theta[::-1])))))
    # build the polygon and add it to the axes
    poly = mpatches.Polygon(points.T, closed=True, **kwargs)
    axx.add_patch(poly)
    return poly


# initialization function: plot the background of each frame
def init():
    ax.hold(False)
    ax.set_xlim([-road_length / 4, road_length / 4])
    ax.set_ylim([-road_length / 4, road_length / 4])
    return ax,


# animation function.  This is called sequentially
def animate(i):
    drawProgressBar(float(i) / len(tt))
    x = road_length / (2 * np.pi) * np.cos(2 * np.pi * xx[i, :] / road_length)
    y = road_length / (2 * np.pi) * np.sin(2 * np.pi * xx[i, :] / road_length)

    # add the cars
    scat = ax.scatter(x, y)
    ax.hold(True)

    # add the obstacles
    if nObstacles > 0:
        # inner radius and outer radius
        r1 = 0.95 * road_length / (2 * np.pi)
        r2 = 1.05 * road_length / (2 * np.pi)
        for ob in range(nObstacles):
            arc_patch(r1, r2, startOb[ob], endOb[ob], road_length,
                      axx=ax, fill=True, alpha=0.5, color='red')

            # calculate the time in minutes, hours and second

    scat = ax.scatter(x[0], y[0], marker="o", color='k', s=110)
    scat = ax.scatter(x[0], y[0], marker="o", color='w', s=70)
    scat = ax.scatter(x[0], y[0], marker="+", color='k', s=80)
    # add the traffic lights
    if nLights > 0:
        xl = 1.1 * road_length / (2 * np.pi) * np.cos(2 * np.pi * xxLights[i, :] / road_length)
        yl = 1.1 * road_length / (2 * np.pi) * np.sin(2 * np.pi * xxLights[i, :] / road_length)
        for l in range(nLights):
            if (onLights[i, l] == 1):
                scat = ax.scatter(xl[l], yl[l], marker="s", color='r', s=50)
            else:
                scat = ax.scatter(xl[l], yl[l], marker="^", color='g', s=50)

    ax.hold(False)
    ax.set_xlim([-road_length / 4, road_length / 4])
    ax.set_ylim([-road_length / 4, road_length / 4])
    plt.xlabel('x[m]')
    plt.ylabel('y[m]')
    m, s = divmod(tt[i], 60)
    h, m = divmod(m, 60)

    ## ADD text informations
    vskip = road_length / 6 - road_length / 7;
    # time informations
    ax.text(road_length / 7, road_length / 6, r"$Time:$ $%d:%02d:%02d$" % (h, m, s))

    # add velocity information of the red car
    ax.text(road_length / 7, road_length / 6 - vskip, r"$v:$      $%d [km/h]$" % int(vv[i, 0] * 3.6))

    global oldDistance
    global distanceX0
    global loops

    distanceX0 = xx[i, 0] + loops * road_length;
    if distanceX0 < loops * road_length + road_length / 2 and oldDistance > loops * road_length + road_length / 2:  # if the car completed a loop
        loops += 1
        distanceX0 += road_length

    oldDistance = distanceX0

    ax.text(road_length / 7, road_length / 6 - 1.7 * vskip, r"$d:$      $%d [m]$" % int(distanceX0))

    # add velocity information of the simulation
    ax.text(road_length / 7, road_length / 6 - 2.7 * vskip, r"$avg_v:$ $%d [km/h]$" % int(np.mean(vv[i, :]) * 3.6))

    return scat,


# call the animator.  blit=True means only re-draw the parts that have changed.
anim = animation.FuncAnimation(fig, animate, init_func=init,
                               frames=len(tt), interval=1, blit=False)

if saveAnim:
    anim.save('basic_animation.mp4', fps=30, extra_args=['-vcodec', 'libx264'])
else:
    plt.show()

# save the animation as an mp4.  This requires ffmpeg or mencoder to be
# installed.  The extra_args ensure that the x264 codec is used, so that
# the video can be embedded in html5.  You may need to adjust this for
# your system: for more information, see
# http://matplotlib.sourceforge.net/api/animation_api.html


