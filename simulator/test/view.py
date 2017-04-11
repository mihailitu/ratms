"""
Produces a map showing London Underground station locations with high
resolution background imagery provided by OpenStreetMap.

"""
from matplotlib.path import Path
import matplotlib.pyplot as plt
import numpy as np

import cartopy.crs as ccrs
import cartopy
from cartopy.io.img_tiles import OSM


def tube_locations():
    """
    Returns an (n, 2) array of selected London Tube locations in Ordnance
    Survey GB coordinates.

    Source: http://www.doogal.co.uk/london_stations.php

    """
    return np.array([[531738., 180890.], [532379., 179734.],
                     [531096., 181642.], [530234., 180492.],
                     [531688., 181150.], [530242., 180982.],
                     [531940., 179144.], [530406., 180380.],
                     [529012., 180283.], [530553., 181488.],
                     [531165., 179489.], [529987., 180812.],
                     [532347., 180962.], [529102., 181227.],
                     [529612., 180625.], [531566., 180025.],
                     [529629., 179503.], [532105., 181261.],
                     [530995., 180810.], [529774., 181354.],
                     [528941., 179131.], [531050., 179933.],
                     [530240., 179718.]])


def london():
    imagery = OSM()

    ax = plt.axes(projection=imagery.crs)
    ax.set_extent((-0.14, -0.1, 51.495, 51.515))

    # Construct concentric circles and a rectangle,
    # suitable for a London Underground logo.
    theta = np.linspace(0, 2 * np.pi, 100)
    circle_verts = np.vstack([np.sin(theta), np.cos(theta)]).T
    concentric_circle = Path.make_compound_path(Path(circle_verts[::-1]),
                                                Path(circle_verts * 0.6))

    rectangle = Path([[-1.1, -0.2], [1, -0.2], [1, 0.3], [-1.1, 0.3]])

    # Add the imagery to the map.
    ax.add_image(imagery, 14)

    # Plot the locations twice, first with the red concentric circles,
    # then with the blue rectangle.
    xs, ys = tube_locations().T
    plt.plot(xs, ys, transform=ccrs.OSGB(),
             marker=concentric_circle, color='red', markersize=9,
             linestyle='')
    plt.plot(xs, ys, transform=ccrs.OSGB(),
             marker=rectangle, color='blue', markersize=11,
             linestyle='')

    plt.title('London underground locations')
    plt.show()


def simple():
    ax = plt.axes(projection=ccrs.PlateCarree())
    ax.coastlines()
    plt.show()


def flight():
    ax = plt.axes(projection=ccrs.PlateCarree())
    ax.stock_img()

    ny_lon, ny_lat = -75, 43
    delhi_lon, delhi_lat = 77.23, 28.61

    plt.plot([ny_lon, delhi_lon], [ny_lat, delhi_lat],
             color='blue', linewidth=2, marker='o',
             transform=ccrs.Geodetic(),
             )

    plt.plot([ny_lon, delhi_lon], [ny_lat, delhi_lat],
             color='gray', linestyle='--',
             transform=ccrs.PlateCarree(),
             )

    plt.text(ny_lon - 3, ny_lat - 12, 'New York',
             horizontalalignment='right',
             transform=ccrs.Geodetic())

    plt.text(delhi_lon + 3, delhi_lat - 12, 'Delhi',
             horizontalalignment='left',
             transform=ccrs.Geodetic())

    plt.show()


def carto():
    import cartopy.io.shapereader as shpreader
    import shapely
    # Downloaded from http://biogeo.ucdavis.edu/data/gadm2/shp/DEU_adm.zip
    fname = 'ned_1arcsec_g.shp'
    adm1_shapes = list(shpreader.Reader(fname).geometries())

    mpl1 = shapely.geometry.multipolygon.MultiPolygon([(-171, - 14), (-170, - 14), (-170, - 15), (-171, - 15), (-171, - 14)])
    print mpl1

    # adm1_shapes = list((
    #     shapely.geometry.multipolygon.MultiPolygon((-171 -14, -170 -14, -170 -15, -171 -15, -171 -14)),
    #     shapely.geometry.multipolygon.MultiPolygon((-170 -14, -169 -14, -169 -15, -170 -15, -170 -14)))
    # )

    print adm1_shapes
    for sh in adm1_shapes:
        print sh

    ax = plt.axes(projection=ccrs.PlateCarree())

    plt.title('Deutschland')

    ax.add_geometries(adm1_shapes, ccrs.PlateCarree(),
                      edgecolor='black', facecolor='gray', alpha=0.5)

    ax.set_extent([4, 16, 47, 56], ccrs.PlateCarree())

    plt.show()


def cartopy_ex():
    from matplotlib import pyplot
    from shapely.geometry import MultiPolygon
    from descartes.patch import PolygonPatch

    #from figures import SIZE

    COLOR = {
        True: '#6699cc',
        False: '#ff3333'
    }

    def v_color(ob):
        return COLOR[ob.is_valid]

    def plot_coords(ax, ob):
        x, y = ob.xy
        ax.plot(x, y, 'o', color='#999999', zorder=1)

    fig = pyplot.figure(1, figsize=(10, 10), dpi=90)

    # 1: valid multi-polygon
    ax = fig.add_subplot(121)

    a = [(0, 0), (0, 1), (1, 1), (1, 0), (0, 0)]
    b = [(1, 1), (1, 2), (2, 2), (2, 1), (1, 1)]

    multi1 = MultiPolygon([[a, []], [b, []]])

    for polygon in multi1:
        print polygon
        plot_coords(ax, polygon.exterior)
        patch = PolygonPatch(polygon, facecolor=v_color(multi1), edgecolor=v_color(multi1), alpha=0.5, zorder=2)
        ax.add_patch(patch)

    ax.set_title('a) valid')

    xrange = [-1, 3]
    yrange = [-1, 3]
    ax.set_xlim(*xrange)
    ax.set_xticks(range(*xrange) + [xrange[-1]])
    ax.set_ylim(*yrange)
    ax.set_yticks(range(*yrange) + [yrange[-1]])
    ax.set_aspect(1)

    # 2: invalid self-touching ring
    ax = fig.add_subplot(122)

    c = [(0, 0), (0, 11), (1, 1.5), (1, 0), (0, 0)]
    d = [(1, 0.5), (1, 2), (2, 2), (2, 0.5), (1, 0.5)]

    multi2 = MultiPolygon([[c, []], [d, []]])

    for polygon in multi2:
        plot_coords(ax, polygon.exterior)
        patch = PolygonPatch(polygon, facecolor=v_color(multi2), edgecolor=v_color(multi2), alpha=0.5, zorder=2)
        ax.add_patch(patch)

    ax.set_title('b) invalid')

    xrange = [-1, 3]
    yrange = [-1, 3]
    ax.set_xlim(*xrange)
    ax.set_xticks(range(*xrange) + [xrange[-1]])
    ax.set_ylim(*yrange)
    ax.set_yticks(range(*yrange) + [yrange[-1]])
    ax.set_aspect(1)

    pyplot.show()

if __name__ == '__main__':
    cartopy_ex()
