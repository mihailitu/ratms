#ifndef VEHICLE_H
#define VEHICLE_H


class Vehicle
{
    /* the length of the car.
     * We can have:
     *      - compact car - usual sedan 3.5 - 5 meters long
     *      - van - 7-8 meters long
     *      - bus - 12 - 18 meters long
     *      - truck - 20 -25 meters long
     * The type of the vechicle should be determined trough statistics:
     *      there should be some statistics with vehicle type distribution to a city - from the Cityhall
     *      in a regular city, sedans (or shorter should be more probable than trucks, etc
     */

    int x_orig;     // when a vechicle is created, it has to start(appear) somewhere
    int length;     // vechile length
    int velocity;   // current velocity. It will be update through IDM
    int x_pos;      // current position on the road. It will be updated through IDM
    bool free_road; // if the distance to the vechicle on front is large,
                    // we will not consider other vechicles to update current velocity and current position
public:
    Vehicle( int _length, int _x_orig );
};

#endif // VEHICLE_H
