#ifndef _TEMPERATURE_H_
#define _TEMPERATURE_H_

#include <cstdlib> // NULL, apparently
#include <stdint.h>

/**
  * Temperature conversion and calculation code.
  *
  * Useful notes:
  *
  * The to_kelvin() family of functions converts from internal representation (Temperature::magnitude or Temperature::delta), to
  * natural representation (float, int, etc.) for display or calculations.
  *
  * to_kelvin() and to_rankine() are a little different from to_celsius() and to_fahrenheit(), as the absolute scale versions (K & R) can
  * be used both for deltas and magnitudes, while the relative scale versions (C & F) can only be used for magnitudes.
  *
  * The kelvin() family of functions converts from natural representation to internal representation.
  *
  * Again, kelvin() and rankine() are a little different from celsius() and fahrenheit(); as the absolute versions can get deltas, and the
  * relative version cannot.
  *
  * If a Celsius or Fahrenheit delta is desired, simply feed the Temperature::delta into the appropriate absolute scale version function
  * (to_kelvin() for C, to_rankine() for F), or feed the temperature difference into the appropriate function to get an internal value
  * (kelvin() for C, rankine() for F).
  *
  * Temperatures in internal representation can be compared using the normal operators, but care should be taken when doing arithmetic,
  * as the internal representation scale is absolute, and therefore unsigned.
  *
  * To this end, the hotter() function compares its two first parameters, and returns true if the first parameter is hotter than the second.
  * Additionally, a pointer to a Temperature::delta can optionally be passed in to also get the difference between the two temperatures.
  * This value, being on an absolute temperature scale, is always positive.
  *
  * Also note that multiplication and division of temperatures rarely, if ever, makes sense.
  *
  * The temperature code has a range of 0.000 K to 4,294,967.295 K, and an idealized internal accuracy of 0.001 K.
  * The closest thing that reliably has a temperature in excess of this range is the Sun, and as such this range is likely adequate.
  *
  * Useful constants:
  *
  * The useful constants represent various (potentially) useful temperature benchmarks; most of which should be self-explanatory.
  */

namespace Temperature {
  typedef uint32_t magnitude;
  typedef uint32_t delta;

  // Useful constant: The Universe
  // Note: This Temperature::magnitude is also the designated OOB value; if any entity has a Temperature::magnitude field set to zero,
  // this should be taken to mean "uninitialized" or "undefined", and should not be taken to mean physically 0 K.
  const magnitude absoluteZero = 0;

  // Useful constants: Water
  const magnitude freezing = 273150;
  const magnitude boiling = 373134;

  // Useful constants: Environment
  const magnitude waterDeep = 277150;
  const magnitude subterranean = 283150;
  const magnitude coldRoom = 290650;
  const magnitude room = 294150;
  const magnitude human = 310150;

  const delta roomTolerance = 2000;

  const magnitude fumarole = 1300000;
  const magnitude lavaSea = 1500000;
  const magnitude fire = 1800000;
  const magnitude sun = 5780000;

  static inline bool hotter (const magnitude a, const magnitude b, delta *difference = NULL) {
    if (difference) {
      if (a > b) {
        *difference = a - b;
      } else {
        *difference = b - a;
      }
    }

    return a > b;
  }

  template <typename T> T to_kelvin (const magnitude mag) {
    T retval = mag / 1000.0;

    return retval;
  }

  template <typename T> magnitude kelvin (const T temp) {
    magnitude retval = temp * 1000.0;

    return retval;
  }

  template <typename T> T to_celsius (const magnitude mag) {
    T retval = (T)(to_kelvin<double>(mag) - 273.15);

    return retval;
  }

  template <typename T> magnitude celsius (const T temp) {
    magnitude retval = kelvin<double>((double)temp + 273.15);

    return retval;
  }

  template <typename T> T to_rankine (const magnitude mag) {
    T retval = to_kelvin<double>(mag) * 9.0 / 5.0;

    return retval;
  }

  template <typename T> magnitude rankine (const T temp) {
    magnitude retval = kelvin((double)temp * 5.0 / 9.0);

    return retval;
  }

  template <typename T> T to_fahrenheit (const magnitude mag) {
    T retval = (T)(to_rankine<double>(mag) - 459.67);

    return retval;
  }

  template <typename T> magnitude fahrenheit (const T temp) {
    magnitude retval = rankine<double>((double)temp + 459.67);

    return retval;
  }
}

#endif // _TEMPERATURE_H_
