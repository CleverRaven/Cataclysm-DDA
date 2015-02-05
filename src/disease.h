#ifndef DISEASE_H
#define DISEASE_H

#include "rng.h"
#include "game.h"
#include "bodypart.h"
#include <sstream>

// TODO: Convert martial art buffs over to the effects system and cut out this and disease.cpp completely
// NOTE: DO NOT PUT MORE STUFF IN THIS FILE, THIS IS ONLY HERE FOR LEGACY PURPOSES UNTIL IT CAN BE REMOVED

class game;

void dis_effect(player &p, disease &dis);

std::string dis_name(disease &dis);

std::string dis_description(disease &dis);

#endif
