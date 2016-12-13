#include "simulator.h"

#include <iostream>

#include "logger.h"
#include "road.h"

Simulator::Simulator()
{
    // Default model parameters, from wikipedia IDM
    v0 = 30;    // 30 m/s       - Desired velocity
    T  = 1.5;   // 1.5 s        - Safe time headway
    a  = 1.00;  // 1.00 m/s^2   - Maximum acceleration
    b  = 3.00;  // 3.00 m/s^2   - Desired deceleration
    delta  = 4; // 4            - Acceleration exponent
    s0 = 2;     // 2 m          - Minimum distance
}

Simulator::Simulator( double _v0, double _T, double _a, double _b, double _delta, double _s0 ) :
    v0(_v0),
    T(_T),
    a(_a),
    b(_b),
    delta(_delta),
    s0(_s0)
{

}


void Simulator::run()
{
    log_info("Running the simulator with model params: \n"
             "Desired velocity:      %.2f m/s\n"
             "Safe time headway:     %.2f s\n"
             "Maximum acceleration:  %.2f m/s^2\n"
             "Desired deleration:    %.2f m/s^2\n"
             "Acceleration exponent: %.2f\n"
             "Minimum distance:      %.2f m", v0, T, a, b, delta, s0);

    Road r( 123, 1500, std::make_pair<float, float>(1.1, 1.2), std::make_pair<float, float>(2.1, 2.1));
}
