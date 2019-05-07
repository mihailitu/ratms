#ifndef UTILS_H
#define UTILS_H

namespace simulator
{

/**
 * @brief mps_to_kmh - meters per second to kilometers per hour
 * @param mps - meters per second
 * @return  kilometers per hour
 */
static double mps_to_kmh(double mps)
{
    return (mps * 3.6); //(mps * 18) / 5;
}

} // namespace simulator

#endif // UTILS_H
