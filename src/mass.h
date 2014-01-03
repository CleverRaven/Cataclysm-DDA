#ifndef _MASS_H_
#define _MASS_H_

#include <cstdlib> // NULL, apparently
#include <stdint.h> // uint32_t

/**
  * Mass conversion and calculation code.
  *
  * Useful notes:
  *
  * The to_kilogram() family of functions converts from internal representation (Mass::magnitude)
  * to natural representation (int, float, etc.) for display or calculations.
  *
  * The kilogram() family of functions likewise convert from natural representations to the internal
  * representation.
  *
  * The internal representation has a range of 0 to 4,294,967.295 kg (giga-gram scale);
  * and a sensitivity of 1 g.
  *
  * For scale, the representation is large enough for weighing a space shuttle on the launch pad;
  * but not large enough to weigh a nuclear submarine.
  *
  * Note that for most arithmetic, the internal representation can be used directly,
  * but care should be taken, as it is an unsigned quantity.
  *
  */

namespace Mass {
  typedef uint32_t magnitude;

  // Useful constants
  const magnitude massless = 0;

  const magnitude gram = 1;
  const magnitude kilogram = 1000;

  const magnitude ounce = 28;
  const magnitude pound = 454;

  template <typename T> T to_kilograms (const magnitude mag) {
    T retval = mag / 1000.0;

    return retval;
  }

  template <typename T> magnitude kilograms (const T mass) {
    return mass * 1000.0;
  }

  template <typename T> T to_grams (const magnitude mag) {
    T retval = mag / 1.0;

    return retval;
  }

  template <typename T> magnitude grams (const T mass) {
    return mass * 1.0;
  }

  template <typename T> to_pounds (const magnitude mag) {
    T retval = mag / 453.59237;

    return retval;
  }

  template <typename T> magnitude pounds (const T mass) {
    return mass * 453.59237;
  }

  template <typename T> to_ounces (const magnitude mag) {
    T retval = mag / 28.349523125;

    return retval;
  }

  template <typename T> ounces (const T mass) {
    return mass * 28.349523125;
  }
}

#endif // _MASS_H_
