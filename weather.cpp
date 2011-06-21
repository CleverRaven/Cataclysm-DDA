#include "weather.h"
#include "game.h"
#include <vector>

#define PLAYER_OUTSIDE (g->m.is_outside(g->u.posx, g->u.posy) && g->levz >= 0)
#define THUNDER_CHANCE 50
#define LIGHTNING_CHANCE 600

void weather_effect::glare(game *g)
{
 if (g->is_in_sunlight(g->u.posx, g->u.posy))
  g->u.infect(DI_GLARE, bp_eyes, 1, 2, g);
}

void weather_effect::wet(game *g)
{
 if (!g->u.is_wearing(itm_coat_rain) && PLAYER_OUTSIDE)
  g->u.add_morale("Wet", -1, -50);
 for (int x = 0; x < SEEX * 3; x++) {
  for (int y = 0; y < SEEY * 3; y++) {
   if (g->m.is_outside(x, y)) {
    field *fd = &(g->m.field_at(x, y));
    if (fd->type == fd_fire)
     fd->age += 15;
   }
  }
 }
}

void weather_effect::very_wet(game *g)
{
 if (!g->u.is_wearing(itm_coat_rain) && PLAYER_OUTSIDE)
  g->u.add_morale("Wet", -2, -100);
 for (int x = 0; x < SEEX * 3; x++) {
  for (int y = 0; y < SEEY * 3; y++) {
   if (g->m.is_outside(x, y)) {
    field *fd = &(g->m.field_at(x, y));
    if (fd->type == fd_fire)
     fd->age += 45;
   }
  }
 }
}

void weather_effect::thunder(game *g)
{
 if (one_in(THUNDER_CHANCE)) {
  if (g->levz >= 0)
   g->add_msg("You hear a distant rumble of thunder.");
  else if (!g->u.has_trait(PF_BADHEARING) && one_in(1 - 3 * g->levz))
   g->add_msg("You hear a rumble of thunder from above.");
 }
 weather_effect tmp;
 tmp.very_wet(g);
}

void weather_effect::lightning(game *g)
{
 if (one_in(LIGHTNING_CHANCE)) {
  std::vector<point> strike;
  for (int x = 0; x < SEEX * 3; x++) {
   for (int y = 0; y < SEEY * 3; y++) {
    if (g->m.move_cost(x, y) == 0 && g->m.is_outside(x, y))
     strike.push_back(point(x, y));
   }
  }
  point hit;
  if (strike.size() == 0) {
   hit.x = rng(0, SEEX * 3 - 1);
   hit.y = rng(0, SEEY * 3 - 1);
  } else
   hit = strike[rng(0, strike.size() - 1)];
  g->add_msg("Lightning strikes nearby!");
  g->explosion(hit.x, hit.y, 10, 0, one_in(4));
 }
 weather_effect tmp;
 tmp.thunder(g);
}

void weather_effect::light_acid(game *g)
{
 if (g->turn % 10 == 0 && PLAYER_OUTSIDE)
  g->add_msg("The acid rain stings, but is harmless for now...");
 weather_effect tmp;
 tmp.wet(g);
}

void weather_effect::acid(game *g)
{
 if (PLAYER_OUTSIDE) {
  g->add_msg("The acid rain burns!");
  if (one_in(3))
   g->u.hit(g, bp_head, 0, 0, 1);
  if (one_in(5)) {
   g->u.hit(g, bp_legs, 0, 0, 1);
   g->u.hit(g, bp_legs, 1, 0, 1);
  }
  if (one_in(8)) {
   g->u.hit(g, bp_feet, 0, 0, 1);
   g->u.hit(g, bp_feet, 1, 0, 1);
  }
  if (one_in(3))
   g->u.hit(g, bp_torso, 0, 0, 1);
  if (one_in(3)) {
   g->u.hit(g, bp_arms, 0, 0, 1);
   g->u.hit(g, bp_arms, 1, 0, 1);
  }
 }
 if (g->levz >= 0) {
  for (int x = 0; x < SEEX * 3; x++) {
   for (int y = 0; y < SEEY * 3; y++) {
    if (!g->m.has_flag(diggable, x, y) && !g->m.has_flag(noitem, x, y) &&
        g->m.move_cost(x, y) > 0 && g->m.is_outside(x, y) && one_in(400))
     g->m.add_field(g, x, y, fd_acid, 1);
   }
  }
 }
 for (int i = 0; i < g->z.size(); i++) {
  if (g->m.is_outside(g->z[i].posx, g->z[i].posy)) {
   if (!g->z[i].has_flag(MF_ACIDPROOF))
    g->z[i].hurt(1);
  }
 }
}
