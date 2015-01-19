#ifndef  HELPER_INC
#define  HELPER_INC

/**
 * Re-added this file because even though all the stuffs has been moved to
 * compability.h, there still remains a need for some reduction of redundancy.
 *
 * This file will be used for simple functions that are here to be "helpers."
 *
 * There is no namespace anymore, just function prototypes.
 */

/**
 * determine if a number is between two values and optionally
 * check if the number is equal to `low` or `high` by setting
 * `equal_to` to true.
 */
bool is_between(int low, int in, int high, bool equal_to=false);
// clamp a number to range defined by `low` and `high`
int clamp_value(int low, int in, int high);

#endif   /* ----- #ifndef HELPER_INC  ----- */
