#ifndef VEHICLE_H
#define VEHICLE_H


class vehicle
{
    /* the length of the car.
     * We can have:
     *      -
     *      - compact car - usual sedan 3.5 - 5 meters long
     *      - van - 7-8 meters long
     *      - bus - 12 - 18 meters long
     *      - truck - 20 -25 meters long
     * The type of the vechicle should be determined trough statistics:
     *      in a regular city, sedans (or shorter should be more probable than trucks, etc
     */
    int length;
    int current_speed;

    // pos on the road. It will be updated through IDM
    int x_pos;
public:
    vehicle();
};

#endif // VEHICLE_H
