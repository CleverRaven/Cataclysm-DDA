#ifndef _DISEASE_H_
#define _DISEASE_H_

#include "rng.h"
#include "game.h"
#include "bodypart.h"
#include <sstream>

#define MIN_DISEASE_AGE (-43200) // Permanent disease capped @ 3 days

class game;

void dis_msg(game *g, dis_type type);

void dis_remove_memorial(game *g, dis_type type);

void dis_effect(game *g, player &p, disease &dis);

int disease_speed_boost(disease dis);

std::string dis_name(disease dis);

std::string dis_description(disease dis);

// Todo: Move helper functions into a DiseaseHandler Class.
// Should standardize parameters so we can make function pointers.

void manage_fire_exposure(player& p, int fireStrength=1);
void manage_fungal_infection(game* g, player& p, disease& dis);
void manage_sleep(game* g, player& p, disease& dis);

void handle_alcohol(game* g, player& p, disease& dis);
void handle_bite_wound(game* g, player& p, disease& dis);
void handle_cough(player& p, int volume=12);
void handle_deliriant(game* g, player& p, disease& dis);
void handle_evil(player& p, disease& dis);
void handle_insect_parasites(game* g, player& p, disease& dis);

bool will_vomit(player& p, int chance=1000);

#endif
