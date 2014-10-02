#ifndef DISEASE_H
#define DISEASE_H

#include "rng.h"
#include "game.h"
#include "bodypart.h"
#include <sstream>

class game;

bool dis_msg(dis_type type);

void weed_msg(player *p);

void dis_end_msg(player &p, disease &dis);

void dis_remove_memorial(dis_type type);

void dis_effect(player &p, disease &dis);

int disease_speed_boost(disease dis);

std::string dis_name(disease &dis);
std::string dis_combined_name(disease &dis);

std::string dis_description(disease &dis);

#endif
