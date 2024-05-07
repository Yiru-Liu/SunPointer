#include <iostream>
#include <cmath>

#define DEG_TO_RAD 0.017453292519943295769236907684886

struct CelestialCoords
{
    float ra;
    float dec;
};

CelestialCoords calculateSun(float jdSince2000) {
  // Algorithm from https://aa.usno.navy.mil/faq/sun_approx
  float g = fmod(357.529 + 0.98560028 * jdSince2000, 360) * DEG_TO_RAD;
  float q = fmod(280.459 + 0.98564736 * jdSince2000, 360) * DEG_TO_RAD;
  float l = q + ((1.915 * sin(g) + 0.020 * sin(2*g)) * DEG_TO_RAD);
  float e = (23.439 - 0.00000036 * jdSince2000) * DEG_TO_RAD;
  float ra = atan2(cos(e) * sin(l), cos(l));
  float d = asin(sin(e) * sin(l));
  CelestialCoords c = {ra, d};
  return c;
}

int main()
{
    auto results = calculateSun(2460523.0 - 2451545.0);
    std::cout << results.ra / DEG_TO_RAD << std::endl << results.dec / DEG_TO_RAD << std::endl;
}