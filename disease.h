#ifndef _DISEASE_H_
#define _DISEASE_H_

#include "rng.h"
#include "game.h"
#include "bodypart.h"
#include <sstream>

#define MIN_DISEASE_AGE (-43200) // Permanent disease capped @ 3 days

struct game;

void dis_msg(game *g, dis_type type);

void dis_effect(game *g, player &p, disease &dis);

int disease_speed_boost(disease dis);

std::string dis_name(disease dis);

std::string dis_description(disease dis);

#endif
