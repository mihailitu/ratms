#include "simulator.h"

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

