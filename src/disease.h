#ifndef DISEASE_H
#define DISEASE_H

#include "rng.h"
#include "game.h"
#include "bodypart.h"
#include <sstream>

class game;

void dis_effect(player &p, disease &dis);

std::string dis_name(disease &dis);

std::string dis_description(disease &dis);

#endif
