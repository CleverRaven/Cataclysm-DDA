#ifndef _ADDICTION_H_
#define _ADDICTION_H_

#include "rng.h"
#include "morale.h"
#include <sstream>

#define MIN_ADDICTION_LEVEL 3 // Minimum intensity before effects are seen

void addict_effect(addiction &add);

std::string addiction_type_name(add_type cur);

std::string addiction_name(addiction cur);

morale_type addiction_craving(add_type cur);

add_type addiction_type(std::string name);

std::string addiction_text(addiction cur);

#endif
