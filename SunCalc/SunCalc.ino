#include <fp64lib.h>

#define BAUD_RATE 115200

// Converting double to unsigned long long:
// #include <bit>
// #include <iostream>
//
// int main() {
//     double d = 1.0027379;
//     std::cout << std::hex << std::uppercase << "0x" << std::bit_cast<unsigned long long>(d) << "ULL" << std::endl;
//
//     // unsigned long long x = 0x3ff921fb54442d18LLU;
//     // std::cout << std::bit_cast<double>(x) << std::endl;
// }

constexpr float64_t F64_DEG_TO_RAD = 0x3F91DF46A2529D39ULL; // 0.017453292519943295769236907684886
constexpr float64_t F64_RAD_TO_DEG = 0x404CA5DC1A63C1F8ULL; // 360/(2pi) = 57.295779513082320876798154814105
constexpr float64_t F64_H_TO_RAD = 0x3FD0C152382D7366ULL; // 2pi/24 = 0.26179938779914943653855361527329

struct CelestialCoords {
  float64_t ra;
  float64_t dec;
};

struct EarthCoords {
  float64_t lat;
  float64_t lon; // Positive to east, negative to west
};

struct DateTime {
  unsigned int year;
  byte month;
  byte day;
  byte hour;
  byte minute;
  byte second;
  byte centisecond;
};

struct ClockParams {
  float64_t lha;
  float64_t alt;
};

void printlnFloat64(float64_t val) {
  Serial.println(fp64_to_decimalExp(val, 17, 0, NULL));
}

void printFloat64(float64_t val) {
  Serial.print(fp64_to_decimalExp(val, 17, 0, NULL));
}

void print2Digit(byte val) {
  char char1, char2;
  if (val > 99) {
    char1 = 'X';
    char2 = 'X';
  } else {
    char1 = val / 10 + '0';
    char2 = val % 10 + '0';
  }

  Serial.print(char1);
  Serial.print(char2);
}

void printlnDateTime(DateTime dt) {
  Serial.print(dt.year);
  Serial.print('-');
  print2Digit(dt.month);
  Serial.print('-');
  print2Digit(dt.day);
  Serial.print('T');
  print2Digit(dt.hour);
  Serial.print(':');
  print2Digit(dt.minute);
  Serial.print(':');
  print2Digit(dt.second);
  Serial.print('.');
  print2Digit(dt.centisecond);
  Serial.println('Z');
}

void printDegrees(float64_t valRadians) {
  printFloat64(fp64_mul(valRadians, F64_RAD_TO_DEG));
  Serial.print(" deg");
}

void printlnEarthCoords(EarthCoords ec) {
  printDegrees(ec.lat);
  Serial.print(",");
  printDegrees(ec.lon);
  Serial.println();
}

float64_t toJdSince2000(DateTime dt) {
  // Returns the Julian Date from 2000-01-01T12:00:00Z.
  // Algorithm from https://aa.usno.navy.mil/faq/JD_formula
  // (367 * year - (7*(year + (month + 9)/12))/4 + (275*month)/9 + day) - 730531.5
  //  + (hour*3600 + minute*60 + second)/86400.0
  return fp64_add(
    fp64_sub(
      fp64_long_to_float64(
        367L * dt.year - (7L*(dt.year + (dt.month + 9L)/12L))/4L + (275L*dt.month)/9L + dt.day
      ),
      0x41264B4700000000ULL // 730531.5
    ),
    fp64_mul(
      fp64_long_to_float64(dt.hour*360000L + dt.minute*6000L + dt.second*100L + dt.centisecond),
      0x3E7F11A4A4DF2035ULL // 1/(86400 * 100)
    )
  );
}

CelestialCoords calculateSun(float64_t jdSince2000) {
  // Returns the right ascension and declination of the Sun in radians.
  // Algorithm from https://aa.usno.navy.mil/faq/sun_approx
  // float g = fmod(357.529 + 0.98560028 * jdSince2000, 360) * DEG_TO_RAD;
  float64_t g = fp64_mul(
    fp64_fmod(
      fp64_add(
        0x40765876C8B43958ULL, // 357.529
        fp64_mul(0x3FEF8A099930E901ULL /* 0.98560028 */, jdSince2000)
      ),
      0x4076800000000000ULL // 360
    ),
    F64_DEG_TO_RAD
  );
  // float q = fmod(280.459 + 0.98564736 * jdSince2000, 360) * DEG_TO_RAD;
  float64_t q = fp64_mul(
    fp64_fmod(
      fp64_add(
        0x4071875810624DD3ULL, // 280.459
        fp64_mul(0x3FEF8A6C5512D6F2ULL /* 0.98564736 */, jdSince2000)
      ),
      0x4076800000000000ULL // 360
    ),
    F64_DEG_TO_RAD
  );
  // float l = q + ((1.915 * sin(g) + 0.020 * sin(2*g)) * DEG_TO_RAD);
  float64_t l = fp64_add(
    q,
    fp64_mul(
      fp64_add(
        fp64_mul(0x3FFEA3D70A3D70A4ULL /* 1.915 */, fp64_sin(g)),
        fp64_mul(0x3F947AE147AE147BULL /* 0.020 */, fp64_sin(fp64_ldexp(g, 1)))
      ),
      F64_DEG_TO_RAD
    )
  );
  // float e = (23.439 - 0.00000036 * jdSince2000) * DEG_TO_RAD;
  float64_t e = fp64_mul(
    fp64_sub(
      0x403770624DD2F1AAULL, // 23.439
      fp64_mul(0x3E9828C0BE769DC1ULL /* 0.00000036 */, jdSince2000)
    ),
    F64_DEG_TO_RAD
  );
  // float ra = atan2(cos(e) * sin(l), cos(l));
  float64_t ra = fp64_atan2(
    fp64_mul(fp64_cos(e), fp64_sin(l)),
    fp64_cos(l)
  );
  // float d = asin(sin(e) * sin(l));
  float64_t d = fp64_asin(
    fp64_mul(fp64_sin(e), fp64_sin(l))
  );
  CelestialCoords c = {ra, d};
  return c;
}

float64_t calculateGMST(float64_t jdSince2000) {
  // Returns Greenwich mean sidereal time in hours.
  // Algorithm from https://aa.usno.navy.mil/faq/GAST
  // Assuming JD_{TT} = JD_{UT}
  float64_t minusHalfDay = fp64_sub(jdSince2000, 0x3FE0000000000000ULL /* 0.5 */);
  float64_t lastMidnight;
  float64_t h = fp64_mul(
    fp64_modf(minusHalfDay, &lastMidnight),
    0x4038000000000000ULL /* 24 */
  );
  lastMidnight = fp64_add(lastMidnight, 0x3FE0000000000000ULL /* 0.5 */);

  // float t = jdSince2000 / 36525.0;
  // float64_t t = fp64_mul(jdSince2000, 0x3EFCB55CBC173DD3ULL /* 1/36525 */);
  // float gmst = fmod(6.697375 + 0.065709824279 * lastMidnight + 1.0027379 * h, 24);
  float64_t gmst = fp64_fmod(
    fp64_add(
      0x401ACA1CAC083127ULL, // 6.697375
      fp64_add(
        fp64_mul(0x3FB0D25BEA4DE0D3ULL /* 0.065709824279 */, lastMidnight),
        fp64_mul(0x3FF00B36E56F5B02ULL /* 1.0027379 */, h)
      )
    ),
    0x4038000000000000ULL /* 24 */
  );

  return gmst;
}

ClockParams calculateLhaAndAltitude(CelestialCoords coords, float64_t gast,
  EarthCoords loc) {
  float64_t gast_radians = fp64_mul(gast, F64_H_TO_RAD);
  // float lha = (gast_radians - coords.ra) + loc.lon;
  float64_t lha = fp64_fmod(
    fp64_add(
      fp64_add(fp64_sub(gast_radians, coords.ra), loc.lon), 
      float64_NUMBER_2PI
    ),
    float64_NUMBER_2PI
  );
  // float alt = asin(cos(lha) * cos(coords.dec) * cos(loc.lat) + sin(coords.dec) * sin(loc.lat));
  float64_t alt = fp64_asin(
    fp64_add(
      fp64_mul(fp64_mul(fp64_cos(lha), fp64_cos(coords.dec)), fp64_cos(loc.lat)),
      fp64_mul(fp64_sin(coords.dec), fp64_sin(loc.lat))
    )
  );
  ClockParams p = {lha, alt};
  return p;
}

ClockParams getClockParams(EarthCoords loc, DateTime dt) {
  float64_t jdSince2000 = toJdSince2000(dt);
  CelestialCoords sunPos = calculateSun(jdSince2000);
  float64_t gmst = calculateGMST(jdSince2000);
  ClockParams p = calculateLhaAndAltitude(sunPos, gmst, loc);
  return p;
}

void setup() {
  Serial.begin(BAUD_RATE);
  Serial.println("Serial ready.");

  DateTime currentDt = {
    2024,5,7,6,0,0,0
  };
  EarthCoords location = {fp64_atof("0.72997009947020340177521334443046"), fp64_atof("-1.2462238739814193364146237855904")};

  ClockParams p = getClockParams(location, currentDt);

  Serial.print("Datetime: ");
  printlnDateTime(currentDt);
  Serial.print("Location: ");
  printlnEarthCoords(location);

  printDegrees(p.lha);
  Serial.println();
  printDegrees(p.alt);
}

void loop() {
}
