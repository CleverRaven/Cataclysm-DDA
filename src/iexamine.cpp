//
//  iexamine.cpp
//  Cataclysm
//
//  Livingstone
//

#include "item_factory.h"
#include "iuse.h"
#include "game.h"
#include "mapdata.h"
#include "keypress.h"
#include "output.h"
#include "rng.h"
#include "line.h"
#include "player.h"
#include "translations.h"
#include "monstergenerator.h"
#include <sstream>
#include <algorithm>

void iexamine::none(player *p, map *m, int examx, int examy)
{
    (void)p; //unused
    g->add_msg(_("That is a %s."), m->name(examx, examy).c_str());
};

void iexamine::gaspump(player *p, map *m, int examx, int examy) {
 if (!query_yn(_("Use the %s?"),m->tername(examx, examy).c_str())) {
  none(p, m, examx, examy);
  return;
 }

 for (int i = 0; i < m->i_at(examx, examy).size(); i++) {
  if (m->i_at(examx, examy)[i].made_of(LIQUID)) {
   item* liq = &(m->i_at(examx, examy)[i]);

   if (one_in(10 + p->dex_cur)) {
    g->add_msg(_("You accidentally spill the %s."), liq->type->name.c_str());
    item spill(liq->type, g->turn);
    spill.charges = rng(dynamic_cast<it_ammo*>(liq->type)->count,
                        dynamic_cast<it_ammo*>(liq->type)->count * (float)(8 / p->dex_cur));
    m->add_item_or_charges(p->posx, p->posy, spill, 1);
    liq->charges -= spill.charges;
    if (liq->charges < 1) {
     m->i_at(examx, examy).erase(m->i_at(examx, examy).begin() + i);
    }
   } else {
    p->moves -= 300;
    if (g->handle_liquid(*liq, true, false)) {
     g->add_msg(_("With a clang and a shudder, the %s pump goes silent."), liq->type->name.c_str());
     m->i_at(examx, examy).erase(m->i_at(examx, examy).begin() + i);
    }
   }
   return;
  }
 }
 g->add_msg(_("Out of order."));
}

void iexamine::toilet(player *p, map *m, int examx, int examy) {
    std::vector<item>& items = m->i_at(examx, examy);
    int waterIndex = -1;
    for (int i = 0; i < items.size(); i++) {
        if (items[i].typeId() == "water") {
            waterIndex = i;
            break;
        }
    }

    if (waterIndex < 0) {
        g->add_msg(_("This toilet is empty."));
    } else {
        bool drained = false;

        item& water = items[waterIndex];
        // Use a different poison value each time water is drawn from the toilet.
        water.poison = one_in(3) ? 0 : rng(1, 3);

        // First try handling/bottling, then try drinking.
        if (g->handle_liquid(water, true, false))
        {
            p->moves -= 100;
            drained = true;
        }
        else if (query_yn(_("Drink from your hands?")))
        {
            // Create a dose of water no greater than the amount of water remaining.
            item water_temp(item_controller->find_template("water"), 0);
            water_temp.poison = water.poison;
            water_temp.charges = std::min(water_temp.charges, water.charges);

            p->inv.push_back(water_temp);
            p->consume(p->inv.position_by_type(water_temp.typeId()));
            p->moves -= 350;

            water.charges -= water_temp.charges;
            if (water.charges <= 0) {
                drained = true;
            }
        }

        if (drained) {
            items.erase(items.begin() + waterIndex);
        }
    }
}

void iexamine::elevator(player *p, map *m, int examx, int examy)
{
    (void)p; //unused
    if (!query_yn(_("Use the %s?"), m->tername(examx, examy).c_str())) {
        return;
    }
    int movez = (g->levz < 0 ? 2 : -2);
    g->vertical_move( movez, false );
}

void iexamine::controls_gate(player *p, map *m, int examx, int examy) {
 if (!query_yn(_("Use the %s?"),m->tername(examx, examy).c_str())) {
  none(p, m, examx, examy);
  return;
 }
 g->open_gate(examx,examy, (ter_id)m->ter(examx,examy));
}

void iexamine::cardreader(player *p, map *m, int examx, int examy) {
 itype_id card_type = (m->ter(examx, examy) == t_card_science ? "id_science" :
                       "id_military");
 if (p->has_amount(card_type, 1) && query_yn(_("Swipe your ID card?"))) {
  p->moves -= 100;
  for (int i = -3; i <= 3; i++) {
   for (int j = -3; j <= 3; j++) {
    if (m->ter(examx + i, examy + j) == t_door_metal_locked)
     m->ter_set(examx + i, examy + j, t_floor);
     }
  }
  for (int i = 0; i < g->num_zombies(); i++) {
   if (g->zombie(i).type->id == "mon_turret") {
    g->remove_zombie(i);
    i--;
   }
  }
  g->add_msg(_("You insert your ID card."));
  g->add_msg(_("The nearby doors slide into the floor."));
  p->use_amount(card_type, 1);
 } else {
  bool using_electrohack = (p->has_amount("electrohack", 1) &&
                            query_yn(_("Use electrohack on the reader?")));
  bool using_fingerhack = (!using_electrohack && p->has_bionic("bio_fingerhack") &&
                           p->power_level > 0 &&
                           query_yn(_("Use fingerhack on the reader?")));
  if (using_electrohack || using_fingerhack) {
   p->moves -= 500;
   p->practice(g->turn, "computer", 20);
   int success = rng(p->skillLevel("computer") / 4 - 2, p->skillLevel("computer") * 2);
   success += rng(-3, 3);
   if (using_fingerhack)
    success++;
   if (p->int_cur < 8)
    success -= rng(0, int((8 - p->int_cur) / 2));
    else if (p->int_cur > 8)
     success += rng(0, int((p->int_cur - 8) / 2));
     if (success < 0) {
      g->add_msg(_("You cause a short circuit!"));
      if (success <= -5) {
       if (using_electrohack) {
        g->add_msg(_("Your electrohack is ruined!"));
        p->use_amount("electrohack", 1);
       } else {
        g->add_msg(_("Your power is drained!"));
        p->charge_power(0 - rng(0, p->power_level));
       }
      }
      m->ter_set(examx, examy, t_card_reader_broken);
     } else if (success < 6)
      g->add_msg(_("Nothing happens."));
      else {
       g->add_msg(_("You activate the panel!"));
       g->add_msg(_("The nearby doors slide into the floor."));
       m->ter_set(examx, examy, t_card_reader_broken);
       for (int i = -3; i <= 3; i++) {
        for (int j = -3; j <= 3; j++) {
         if (m->ter(examx + i, examy + j) == t_door_metal_locked)
          m->ter_set(examx + i, examy + j, t_floor);
          }
       }
      }
  } else {
   g->add_msg(_("Looks like you need a %s."),itypes[card_type]->name.c_str());
  }
 }
}

void iexamine::rubble(player *p, map *m, int examx, int examy) {
    if (!(p->has_amount("shovel", 1) || p->has_amount("primitive_shovel", 1)|| p->has_amount("e_tool", 1))) {
        g->add_msg(_("If only you had a shovel..."));
        return;
    }
    std::string xname = m->tername(examx, examy);
    if (query_yn(_("Clear up that %s?"), xname.c_str())) {
        // "Remove"
        p->moves -= 200;

        // "Replace"
        if(m->ter(examx,examy) == t_rubble) {
            item rock(itypes["rock"], g->turn);
            m->add_item_or_charges(p->posx, p->posy, rock);
            m->add_item_or_charges(p->posx, p->posy, rock);
        }

        // "Refloor"
        if (g->levz < 0) {
            m->ter_set(examx, examy, t_rock_floor);
        } else {
            m->ter_set(examx, examy, t_dirt);
        }

        // "Remind"
        g->add_msg(_("You clear up that %s."), xname.c_str());
    }
}

void iexamine::chainfence(player *p, map *m, int examx, int examy) {
 if (!query_yn(_("Climb %s?"),m->tername(examx, examy).c_str())) {
  none(p, m, examx, examy);
  return;
 }
 p->moves -= 400;
 if (one_in(p->dex_cur)) {
  g->add_msg(_("You slip whilst climbing and fall down again"));
 } else {
  p->moves += p->dex_cur * 10;
  p->posx = examx;
  p->posy = examy;
 }
}

void iexamine::tent(player *p, map *m, int examx, int examy) {
 if (!query_yn(_("Take down your tent?"))) {
  none(p, m, examx, examy);
  return;
 }
 p->moves -= 200;
 for (int i = -1; i <= 1; i++)
  for (int j = -1; j <= 1; j++)
   m->furn_set(examx + i, examy + j, f_null);
 g->add_msg(_("You take down the tent"));
 item dropped(itypes["tent_kit"], g->turn);
 m->add_item_or_charges(examx, examy, dropped);
}

void iexamine::shelter(player *p, map *m, int examx, int examy) {
 if (!query_yn(_("Take down %s?"),m->furnname(examx, examy).c_str())) {
  none(p, m, examx, examy);
  return;
 }
 p->moves -= 200;
 for (int i = -1; i <= 1; i++)
  for (int j = -1; j <= 1; j++)
   m->furn_set(examx + i, examy + j, f_null);
 g->add_msg(_("You take down the shelter"));
 item dropped(itypes["shelter_kit"], g->turn);
 m->add_item_or_charges(examx, examy, dropped);
}

void iexamine::wreckage(player *p, map *m, int examx, int examy) {
 if (!(p->has_amount("shovel", 1) || p->has_amount("primitive_shovel", 1)|| p->has_amount("e_tool", 1))) {
  g->add_msg(_("If only you had a shovel..."));
  return;
 }

 if (query_yn(_("Clear up that wreckage?"))) {
  p->moves -= 200;
  m->ter_set(examx, examy, t_dirt);
  item chunk(itypes["steel_chunk"], g->turn);
  item scrap(itypes["scrap"], g->turn);
  item pipe(itypes["pipe"], g->turn);
  item wire(itypes["wire"], g->turn);
  m->add_item_or_charges(examx, examy, chunk);
  m->add_item_or_charges(examx, examy, scrap);
  if (one_in(5)) {
   m->add_item_or_charges(examx, examy, pipe);
   m->add_item_or_charges(examx, examy, wire); }
  g->add_msg(_("You clear the wreckage up"));
 }
}

void iexamine::pit(player *p, map *m, int examx, int examy)
{
    inventory map_inv;
    map_inv.form_from_map(point(p->posx, p->posy), 1);

    bool player_has = p->has_amount("2x4", 1);
    bool map_has = map_inv.has_amount("2x4", 1);

    // return if there is no 2x4 around
    if (!player_has && !map_has)
    {
        none(p, m, examx, examy);
        return;
    }

    if (query_yn(_("Place a plank over the pit?")))
    {
        // if both have, then ask to use the one on the map
        if (player_has && map_has)
        {
            if (query_yn(_("Use the plank at your feet?")))
            {
                m->use_amount(point(p->posx, p->posy), 1, "2x4", 1, false);
            }
            else
            {
                p->use_amount("2x4", 1);
            }
        }
        else if (player_has && !map_has)    // only player has plank
        {
            p->use_amount("2x4", 1);
        }
        else if (!player_has && map_has)    // only map has plank
        {
            m->use_amount(point(p->posx, p->posy), 1, "2x4", 1, false);
        }

        if( m->ter(examx, examy) == t_pit )
        {
            m->ter_set(examx, examy, t_pit_covered);
        }
        else if( m->ter(examx, examy) == t_pit_spiked )
        {
            m->ter_set(examx, examy, t_pit_spiked_covered);
        }
        g->add_msg(_("You place a plank of wood over the pit."));
    }
}

void iexamine::pit_covered(player *p, map *m, int examx, int examy)
{
    if(!query_yn(_("Remove cover?")))
    {
        none(p, m, examx, examy);
        return;
    }

    item plank(itypes["2x4"], g->turn);
    g->add_msg(_("You remove the plank."));
    m->add_item_or_charges(p->posx, p->posy, plank);

    if( m->ter(examx, examy) == t_pit_covered )
    {
        m->ter_set(examx, examy, t_pit);
    }
    else if( m->ter(examx, examy) == t_pit_spiked_covered )
    {
        m->ter_set(examx, examy, t_pit_spiked);
    }
}

void iexamine::fence_post(player *p, map *m, int examx, int examy) {

 int ch = menu(true, _("Fence Construction:"), _("Rope Fence"),
               _("Wire Fence"), _("Barbed Wire Fence"), _("Cancel"), NULL);
 switch (ch){
  case 1:{
   if (p->has_amount("rope_6", 2)) {
    p->use_amount("rope_6", 2);
    m->ter_set(examx, examy, t_fence_rope);
    p->moves -= 200;
   } else
    g->add_msg(_("You need 2 six-foot lengths of rope to do that"));
  } break;

  case 2:{
   if (p->has_amount("wire", 2)) {
    p->use_amount("wire", 2);
    m->ter_set(examx, examy, t_fence_wire);
    p->moves -= 200;
   } else
    g->add_msg(_("You need 2 lengths of wire to do that!"));
  } break;

  case 3:{
   if (p->has_amount("wire_barbed", 2)) {
    p->use_amount("wire_barbed", 2);
    m->ter_set(examx, examy, t_fence_barbed);
    p->moves -= 200;
   } else
    g->add_msg(_("You need 2 lengths of barbed wire to do that!"));
  } break;

  case 4:
  default:
   break;
 }
}

void iexamine::remove_fence_rope(player *p, map *m, int examx, int examy) {
 if(!query_yn(_("Remove %s?"),m->tername(examx, examy).c_str())) {
  none(p, m, examx, examy);
  return;
 }
 item rope(itypes["rope_6"], g->turn);
 m->add_item_or_charges(p->posx, p->posy, rope);
 m->add_item_or_charges(p->posx, p->posy, rope);
 m->ter_set(examx, examy, t_fence_post);
 p->moves -= 200;

}

void iexamine::remove_fence_wire(player *p, map *m, int examx, int examy) {
 if(!query_yn(_("Remove %s?"),m->tername(examx, examy).c_str())) {
  none(p, m, examx, examy);
  return;
 }

 item rope(itypes["wire"], g->turn);
 m->add_item_or_charges(p->posx, p->posy, rope);
 m->add_item_or_charges(p->posx, p->posy, rope);
 m->ter_set(examx, examy, t_fence_post);
 p->moves -= 200;
}

void iexamine::remove_fence_barbed(player *p, map *m, int examx, int examy) {
 if(!query_yn(_("Remove %s?"),m->tername(examx, examy).c_str())) {
  none(p, m, examx, examy);
  return;
 }

 item rope(itypes["wire_barbed"], g->turn);
 m->add_item_or_charges(p->posx, p->posy, rope);
 m->add_item_or_charges(p->posx, p->posy, rope);
 m->ter_set(examx, examy, t_fence_post);
 p->moves -= 200;
}

void iexamine::slot_machine(player *p, map *m, int examx, int examy)
{
    (void)m; (void)examx; (void)examy; //unused
    if (p->cash < 10) {
        g->add_msg(_("You need $10 to play."));
    } else if (query_yn(_("Insert $10?"))) {
        do {
            if (one_in(5)) {
                popup(_("Three cherries... you get your money back!"));
            } else if (one_in(20)) {
                popup(_("Three bells... you win $50!"));
                p->cash += 40; // Minus the $10 we wagered
            } else if (one_in(50)) {
                popup(_("Three stars... you win $200!"));
                p->cash += 190;
            } else if (one_in(1000)) {
                popup(_("JACKPOT!  You win $5000!"));
                p->cash += 4990;
            } else {
                popup(_("No win."));
                p->cash -= 10;
            }
        } while (p->cash >= 10 && query_yn(_("Play again?")));
    }
}

void iexamine::safe(player *p, map *m, int examx, int examy) {
  if (!p->has_amount("stethoscope", 1)) {
    g->add_msg(_("You need a stethoscope for safecracking."));
    return;
  }

  if (query_yn(_("Attempt to crack the safe?"))) {
    bool success = true;

    if (success) {
      m->furn_set(examx, examy, f_safe_o);
      g->add_msg(_("You successfully crack the safe!"));
    } else {
      g->add_msg(_("The safe resists your attempt at cracking it."));
    }
  }
}

void iexamine::bulletin_board(player *p, map *m, int examx, int examy) {
    (void)g; (void)p; //unused
    basecamp *camp = m->camp_at(examx, examy);
    if (camp && camp->board_x() == examx && camp->board_y() == examy) {
        std::vector<std::string> options;
        options.push_back(_("Cancel"));
        // Causes a warning due to being unused, but don't want to delete
        // since it's clearly what's intened for future functionality.
        //int choice = menu_vec(true, camp->board_name().c_str(), options) - 1;
    } else {
        bool create_camp = m->allow_camp(examx, examy);
        std::vector<std::string> options;
        if (create_camp) {
            options.push_back(_("Create camp"));
        }
        options.push_back(_("Cancel"));
        // TODO: Other Bulletin Boards
        int choice = menu_vec(true, _("Bulletin Board"), options) - 1;
        if (choice >= 0 && choice < options.size()) {
            if (options[choice] == _("Create camp")) {
                // TODO: Allow text entry for name
                m->add_camp(_("Home"), examx, examy);
            }
        }
    }
}

void iexamine::fault(player *p, map *m, int examx, int examy)
{
    (void)g; (void)p; (void)m; (void)examx; (void)examy; //unused
    popup(_("\
This wall is perfectly vertical.  Odd, twisted holes are set in it, leading\n\
as far back into the solid rock as you can see.  The holes are humanoid in\n\
shape, but with long, twisted, distended limbs."));
}

void iexamine::pedestal_wyrm(player *p, map *m, int examx, int examy) {
 if (!m->i_at(examx, examy).empty()) {
  none(p, m, examx, examy);
  return;
 }
 g->add_msg(_("The pedestal sinks into the ground..."));
 m->ter_set(examx, examy, t_rock_floor);
 g->add_event(EVENT_SPAWN_WYRMS, int(g->turn) + rng(5, 10));
}

void iexamine::pedestal_temple(player *p, map *m, int examx, int examy) {

 if (m->i_at(examx, examy).size() == 1 &&
     m->i_at(examx, examy)[0].type->id == "petrified_eye") {
  g->add_msg(_("The pedestal sinks into the ground..."));
  m->ter_set(examx, examy, t_dirt);
  m->i_at(examx, examy).clear();
  g->add_event(EVENT_TEMPLE_OPEN, int(g->turn) + 4);
 } else if (p->has_amount("petrified_eye", 1) &&
            query_yn(_("Place your petrified eye on the pedestal?"))) {
  p->use_amount("petrified_eye", 1);
  g->add_msg(_("The pedestal sinks into the ground..."));
  m->ter_set(examx, examy, t_dirt);
  g->add_event(EVENT_TEMPLE_OPEN, int(g->turn) + 4);
 } else
  g->add_msg(_("This pedestal is engraved in eye-shaped diagrams, and has a \
large semi-spherical indentation at the top."));
}

void iexamine::fswitch(player *p, map *m, int examx, int examy)
{
    if(!query_yn(_("Flip the %s?"), m->tername(examx, examy).c_str())) {
        none(p, m, examx, examy);
        return;
    }
    ter_id terid = m->ter(examx, examy);
    p->moves -= 100;
    for (int y = examy; y <= examy + 5; y++) {
        for (int x = 0; x < SEEX * MAPSIZE; x++) {
            if ( terid == t_switch_rg ) {
                if (m->ter(x, y) == t_rock_red) {
                    m->ter_set(x, y, t_floor_red);
                } else if (m->ter(x, y) == t_floor_red) {
                    m->ter_set(x, y, t_rock_red);
                } else if (m->ter(x, y) == t_rock_green) {
                    m->ter_set(x, y, t_floor_green);
                } else if (m->ter(x, y) == t_floor_green) {
                    m->ter_set(x, y, t_rock_green);
                }
            } else if ( terid == t_switch_gb ) {
                if (m->ter(x, y) == t_rock_blue) {
                    m->ter_set(x, y, t_floor_blue);
                } else if (m->ter(x, y) == t_floor_blue) {
                    m->ter_set(x, y, t_rock_blue);
                } else if (m->ter(x, y) == t_rock_green) {
                    m->ter_set(x, y, t_floor_green);
                } else if (m->ter(x, y) == t_floor_green) {
                    m->ter_set(x, y, t_rock_green);
                }
            } else if ( terid == t_switch_rb ) {
                if (m->ter(x, y) == t_rock_blue) {
                    m->ter_set(x, y, t_floor_blue);
                } else if (m->ter(x, y) == t_floor_blue) {
                    m->ter_set(x, y, t_rock_blue);
                } else if (m->ter(x, y) == t_rock_red) {
                    m->ter_set(x, y, t_floor_red);
                } else if (m->ter(x, y) == t_floor_red) {
                    m->ter_set(x, y, t_rock_red);
                }
            } else if ( terid == t_switch_even ) {
                if ((y - examy) % 2 == 1) {
                    if (m->ter(x, y) == t_rock_red) {
                        m->ter_set(x, y, t_floor_red);
                    } else if (m->ter(x, y) == t_floor_red) {
                        m->ter_set(x, y, t_rock_red);
                    } else if (m->ter(x, y) == t_rock_green) {
                        m->ter_set(x, y, t_floor_green);
                    } else if (m->ter(x, y) == t_floor_green) {
                        m->ter_set(x, y, t_rock_green);
                    } else if (m->ter(x, y) == t_rock_blue) {
                        m->ter_set(x, y, t_floor_blue);
                    } else if (m->ter(x, y) == t_floor_blue) {
                        m->ter_set(x, y, t_rock_blue);
                    }
                }
            }
        }
    }
    g->add_msg(_("You hear the rumble of rock shifting."));
    g->add_event(EVENT_TEMPLE_SPAWN, g->turn + 3);
}

void iexamine::flower_poppy(player *p, map *m, int examx, int examy) {
  if(!query_yn(_("Pick %s?"),m->furnname(examx, examy).c_str())) {
    none(p, m, examx, examy);
    return;
  }

  int resist = p->get_env_resist(bp_mouth);

  if (resist < 10) {
    // Can't smell the flowers with a gas mask on!
    g->add_msg(_("This flower has a heady aroma"));
  }

  if (one_in(3) && resist < 5)  {
    // Should user player::infect, but can't!
    // player::infect needs to be restructured to return a bool indicating success.
    g->add_msg(_("You fall asleep..."));
    p->fall_asleep(1200);
    g->add_msg(_("Your legs are covered by flower's roots!"));
    p->hurt(bp_legs, 0, 4);
    p->moves -=50;
  }

  m->furn_set(examx, examy, f_null);
  m->spawn_item(examx, examy, "poppy_flower");
  m->spawn_item(examx, examy, "poppy_bud");
}

void iexamine::fungus(player *p, map *m, int examx, int examy) {
    // TODO: Infect NPCs?
    monster spore(GetMType("mon_spore"));
    int mondex;
    g->add_msg(_("The %s crumbles into spores!"), m->furnname(examx, examy).c_str());
    for (int i = examx - 1; i <= examx + 1; i++) {
        for (int j = examy - 1; j <= examy + 1; j++) {
            mondex = g->mon_at(i, j);
            if (g->m.move_cost(i, j) > 0 || (i == examx && j == examy)) {
                if (mondex != -1) { // Spores hit a monster
                    if (g->u_see(i, j) && 
                            !g->zombie(mondex).type->in_species("FUNGUS")) {
                        g->add_msg(_("The %s is covered in tiny spores!"),
                                        g->zombie(mondex).name().c_str());
                    }
                    if (!g->zombie(mondex).make_fungus()) {
                        g->kill_mon(mondex, false);
                    }
                } else if (g->u.posx == i && g->u.posy == j) {
                    // Spores hit the player
                    bool hit = false;
                    if (one_in(4) && g->u.infect("spores", bp_head, 3, 90, false, 1, 3, 120, 1, true)) {
                        hit = true;
                    }
                    if (one_in(2) && g->u.infect("spores", bp_torso, 3, 90, false, 1, 3, 120, 1, true)) {
                        hit = true;
                    }
                    if (one_in(4) && g->u.infect("spores", bp_arms, 3, 90, false, 1, 3, 120, 1, true, 1)) {
                        hit = true;
                    }
                    if (one_in(4) && g->u.infect("spores", bp_arms, 3, 90, false, 1, 3, 120, 1, true, 0)) {
                        hit = true;
                    }
                    if (one_in(4) && g->u.infect("spores", bp_legs, 3, 90, false, 1, 3, 120, 1, true, 1)) {
                        hit = true;
                    }
                    if (one_in(4) && g->u.infect("spores", bp_legs, 3, 90, false, 1, 3, 120, 1, true, 0)) {
                        hit = true;
                    }
                    if (hit) {
                        g->add_msg(_("You're covered in tiny spores!"));
                    }
                } else if (((i == examx && j == examy) || one_in(4)) &&
                              g->num_zombies() <= 1000) { // Spawn a spore
                    spore.spawn(i, j);
                    g->add_zombie(spore);
                }
            }
        }
    }
    m->furn_set(examx, examy, f_null);
    p->moves -=50;
}

void iexamine::dirtmound(player *p, map *m, int examx, int examy) {

    if (g->get_temperature() < 50) { // semi-appropriate temperature for most plants
        g->add_msg(_("It is too cold to plant anything now."));
        return;
    }
    /* ambient_light_at() not working?
    if (m->ambient_light_at(examx, examy) < LIGHT_AMBIENT_LOW) {
        g->add_msg(_("It is too dark to plant anything now."));
        return;
    }*/
    if (!p->has_item_with_flag("SEED")){
        g->add_msg(_("You have no seeds to plant."));
        return;
    }
    if (m->i_at(examx, examy).size() != 0){
        g->add_msg(_("Something's lying there..."));
        return;
    }

    // Get list of all inv+wielded seeds
    std::vector<item*> seed_inv = p->inv.all_items_with_flag("SEED");
    if (g->u.weapon.has_flag("SEED"))
        seed_inv.push_back(&g->u.weapon);

    // Make lists of unique seed types and names for the menu(no multiple hemp seeds etc)
    std::vector<itype_id> seed_types;
    std::vector<std::string> seed_names;
    for (std::vector<item*>::iterator it = seed_inv.begin() ; it != seed_inv.end(); it++){
        if (std::find(seed_types.begin(), seed_types.end(), (*it)->typeId()) == seed_types.end()){
            seed_types.push_back((*it)->typeId());
            seed_names.push_back((*it)->name);
        }
    }

    // Choose seed if applicable
    int seed_index = 0;
    if (seed_types.size() > 1) {
        seed_names.push_back("Cancel");
        seed_index = menu_vec(false, _("Use which seed?"), seed_names) - 1; // TODO: make cancelable using ESC
        if (seed_index == seed_names.size() - 1)
            seed_index = -1;
    } else {
        if (!query_yn(_("Plant %s here?"), seed_names[0].c_str()))
            seed_index = -1;
    }

    // Did we cancel?
    if (seed_index < 0) {
        g->add_msg(_("You saved your seeds for later.")); // huehuehue
        return;
    }

    // Actual planting
    std::list<item> planted = p->inv.use_charges(seed_types[seed_index], 1);
    if (planted.empty()) { // nothing was removed from inv => weapon is the SEED
        if (g->u.weapon.charges > 1) {
            g->u.weapon.charges--;
        } else {
            g->u.remove_weapon();
        }
    }
    m->spawn_item(examx, examy, seed_types[seed_index], 1, 1, g->turn);
    m->set(examx, examy, t_dirt, f_plant_seed);
    p->moves -= 500;
    g->add_msg(_("Planted %s"), seed_names[seed_index].c_str());
}

void iexamine::aggie_plant(player *p, map *m, int examx, int examy) {
    if (m->furn(examx, examy) == f_plant_harvest && query_yn(_("Harvest plant?"))) {
        itype_id seedType = m->i_at(examx, examy)[0].typeId();
        if (seedType == "fungal_seeds") {
            fungus(p, m, examx, examy);
            for (int k = 0; k < g->m.i_at(examx, examy).size(); k++) {
                g->m.i_rem(examx, examy, k);
            }
        } else {
            m->i_clear(examx, examy);
            m->furn_set(examx, examy, f_null);

            int skillLevel = p->skillLevel("survival");
            int plantCount = rng(skillLevel / 2, skillLevel);
            if (plantCount >= 12) {
                plantCount = 12;
            }

            m->spawn_item(examx, examy, seedType.substr(5), plantCount, 0, g->turn);
            if(item_controller->find_template(seedType)->count_by_charges()) {
                m->spawn_item(examx, examy, seedType, 1, rng(plantCount / 4, plantCount / 2));
            } else {
                m->spawn_item(examx, examy, seedType, rng(plantCount / 4, plantCount / 2));
            }

            p->moves -= 500;
        }
    } else if (m->furn(examx,examy) != f_plant_harvest && m->i_at(examx, examy).size() == 1 &&
                 p->charges_of("fertilizer_liquid") && query_yn(_("Fertilize plant"))) {
        unsigned int fertilizerEpoch = 14400 * 2;

        if (m->i_at(examx, examy)[0].bday > fertilizerEpoch) {
            m->i_at(examx, examy)[0].bday -= fertilizerEpoch;
        } else {
            m->i_at(examx, examy)[0].bday = 0;
        }
        p->use_charges("fertilizer_liquid", 1);
        m->i_at(examx, examy).push_back(item_controller->create("fertilizer", (int) g->turn));
    }
}

void iexamine::pick_plant(player *p, map *m, int examx, int examy, std::string itemType, int new_ter, bool seeds) {
  if (!query_yn(_("Pick %s?"), m->tername(examx, examy).c_str())) {
    none(p, m, examx, examy);
    return;
  }

  SkillLevel& survival = p->skillLevel("survival");
  if (survival < 1)
    p->practice(g->turn, "survival", rng(5, 12));
  else if (survival < 6)
    p->practice(g->turn, "survival", rng(1, 12 / survival));

  int plantCount = rng(survival / 2, survival);
  if (plantCount > 12)
    plantCount = 12;

  m->spawn_item(examx, examy, itemType, plantCount, 0, g->turn);

  if (seeds) {
    m->spawn_item(examx, examy, "seed_" + itemType, 1, rng(plantCount / 4, plantCount / 2), g->turn);
  }

  m->ter_set(examx, examy, (ter_id)new_ter);
}

void iexamine::tree_apple(player *p, map *m, int examx, int examy) {
  pick_plant(p, m, examx, examy, "apple", t_tree);
}

void iexamine::shrub_blueberry(player *p, map *m, int examx, int examy) {
  pick_plant(p, m, examx, examy, "blueberries", t_shrub, true);
}

void iexamine::shrub_strawberry(player *p, map *m, int examx, int examy) {
  pick_plant(p, m, examx, examy, "strawberries", t_shrub, true);
}

void iexamine::shrub_marloss(player *p, map *m, int examx, int examy) {
  pick_plant(p, m, examx, examy, "marloss_berry", t_shrub_fungal);
}

void iexamine::shrub_wildveggies(player *p, map *m, int examx, int examy) {
 if(!query_yn(_("Pick %s?"),m->tername(examx, examy).c_str())) return;

 p->assign_activity(ACT_FORAGE, 500 / (p->skillLevel("survival") + 1), 0);
 p->activity.placement = point(examx, examy);
}

int sum_up_item_weight_by_material(std::vector<item> &items, const std::string &material, bool remove_items) {
    int sum_weight = 0;
    for (int i = items.size() - 1; i >= 0; i--) {
        const item &it = items[i];
        if (it.made_of(material) && it.weight() > 0) {
            sum_weight += it.weight();
            if (remove_items) {
                items.erase(items.begin() + i);
            }
        }
    }
    return sum_weight;
}

void add_recyle_menu_entry(uimenu &menu, int w, char hk, const std::string &type) {
    const itype *itt = item_controller->find_template(type);
    menu.entries.push_back(
        uimenu_entry(
            menu.entries.size() + 1, // value return by uimenu for this entry
            true, // enabled
            hk, // hotkey
            string_format(_("about %d %s"), (int) (w / itt->weight), itt->name.c_str())
        )
    );
}

void iexamine::recycler(player *p, map *m, int examx, int examy) {
    std::vector<item> &items_on_map = m->i_at(examx, examy);

    // check for how much steel, by weight, is in the recycler
    // only items made of STEEL are checked
    // IRON and other metals cannot be turned into STEEL for now
    int steel_weight = sum_up_item_weight_by_material(items_on_map, "steel", false);
    if (steel_weight == 0)
    {
        g->add_msg(_("The recycler is currently empty.  Drop some metal items onto it and examine it again."));
        return;
    }
    // See below for recover_factor (rng(6,9)/10), this
    // is the normal value of that recover factor.
    static const double norm_recover_factor = 8.0 / 10.0;
    const int norm_recover_weight = steel_weight * norm_recover_factor;
    uimenu as_m;
    // Get format for printing weights, convert weight to that format,
    const std::string format = OPTIONS["USE_METRIC_WEIGHTS"].getValue() == "lbs" ? _("%.3f lbs") : _("%.3f kg");
    const std::string weight_str = string_format(format, g->u.convert_weight(steel_weight));
    as_m.text = string_format(_("Recycle %s metal into:"), weight_str.c_str());
    add_recyle_menu_entry(as_m, norm_recover_weight, 'l', "steel_lump");
    add_recyle_menu_entry(as_m, norm_recover_weight, 'S', "sheet_metal");
    add_recyle_menu_entry(as_m, norm_recover_weight, 'c', "steel_chunk");
    add_recyle_menu_entry(as_m, norm_recover_weight, 's', "scrap");
    as_m.entries.push_back(uimenu_entry(0, true, 'c', _("Cancel")));
    as_m.selected = 4;
    as_m.query(); /* calculate key and window variables, generate window, and loop until we get a valid answer */
    int ch = as_m.ret;
    int num_lumps = 0;
    int num_sheets = 0;
    int num_chunks = 0;
    int num_scraps = 0;

    if (ch >= 5 || ch <= 0)
    {
        g->add_msg(_("Never mind."));
        return;
    }

    // Sum up again, this time remove the items,
    // ignore result, should be the same as before.
    sum_up_item_weight_by_material(items_on_map, "steel", true);

    double recover_factor = rng(6, 9) / 10.0;
    steel_weight = (int)(steel_weight * recover_factor);

    g->sound(examx, examy, 80, _("Ka-klunk!"));

    int lump_weight = item_controller->find_template("steel_lump")->weight;
    int sheet_weight = item_controller->find_template("sheet_metal")->weight;
    int chunk_weight = item_controller->find_template("steel_chunk")->weight;
    int scrap_weight = item_controller->find_template("scrap")->weight;

    if (steel_weight < scrap_weight)
    {
        g->add_msg(_("The recycler chews up all the items in its hopper."));
        g->add_msg(_("The recycler beeps: \"No steel to process!\""));
        return;
    }

    switch(ch)
    {
        case 1: // 1 steel lump = weight 1360
            num_lumps = steel_weight / (lump_weight);
            steel_weight -= num_lumps * (lump_weight);
            num_sheets = steel_weight / (sheet_weight);
            steel_weight -= num_sheets * (sheet_weight);
            num_chunks = steel_weight / (chunk_weight);
            steel_weight -= num_chunks * (chunk_weight);
            num_scraps = steel_weight / (scrap_weight);
            if (num_lumps == 0)
            {
                g->add_msg(_("The recycler beeps: \"Insufficient steel!\""));
                g->add_msg(_("It spits out an assortment of smaller pieces instead."));
            }
            break;

        case 2: // 1 metal sheet = weight 1000
            num_sheets = steel_weight / (sheet_weight);
            steel_weight -= num_sheets * (sheet_weight);
            num_chunks = steel_weight / (chunk_weight);
            steel_weight -= num_chunks * (chunk_weight);
            num_scraps = steel_weight / (scrap_weight);
            if (num_sheets == 0)
            {
                g->add_msg(_("The recycler beeps: \"Insufficient steel!\""));
                g->add_msg(_("It spits out an assortment of smaller pieces instead."));
            }
            break;

        case 3: // 1 steel chunk = weight 340
            num_chunks = steel_weight / (chunk_weight);
            steel_weight -= num_chunks * (chunk_weight);
            num_scraps = steel_weight / (scrap_weight);
            if (num_chunks == 0)
            {
                g->add_msg(_("The recycler beeps: \"Insufficient steel!\""));
                g->add_msg(_("It spits out an assortment of smaller pieces instead."));
            }
            break;

        case 4: // 1 metal scrap = weight 113
            num_scraps = steel_weight / (scrap_weight);
            break;
    }

    for (int i = 0; i < num_lumps; i++)
    {
        m->spawn_item(p->posx, p->posy, "steel_lump");
    }

    for (int i = 0; i < num_sheets; i++)
    {
        m->spawn_item(p->posx, p->posy, "sheet_metal");
    }

    for (int i = 0; i < num_chunks; i++)
    {
        m->spawn_item(p->posx, p->posy, "steel_chunk");
    }

    for (int i = 0; i < num_scraps; i++)
    {
        m->spawn_item(p->posx, p->posy, "scrap");
    }
}

void iexamine::trap(player *p, map *m, int examx, int examy) {
 if (g->traps[m->tr_at(examx, examy)]->difficulty < 99 &&
     p->per_cur-p->encumb(bp_eyes) >= g->traps[m->tr_at(examx, examy)]->visibility &&
     query_yn(_("There is a %s there.  Disarm?"),
              g->traps[m->tr_at(examx, examy)]->name.c_str())) {
     m->disarm_trap(examx, examy);
 }
}

void iexamine::water_source(player *p, map *m, const int examx, const int examy)
{
    item water = m->water_from(examx, examy);
    // Try to handle first (bottling) drink after.
    // changed boolean, large sources should be infinite
    if (g->handle_liquid(water, true, true))
    {
        p->moves -= 100;
    }
    else if (query_yn(_("Drink from your hands?")))
    {
        p->inv.push_back(water);
        p->consume(p->inv.position_by_type(water.typeId()));
        p->moves -= 350;
    }
}

void iexamine::acid_source(player *p, map *m, const int examx, const int examy)
{
    item acid = m->acid_from(examx, examy);
    if (g->handle_liquid(acid, true, true))
    {
        p->moves -= 100;
    }
}

/**
 * Given then name of one of the above functions, returns the matching function
 * pointer. If no match is found, defaults to iexamine::none but prints out a
 * debug message as a warning.
 * @param function_name The name of the function to get.
 * @return A function pointer to the specified function.
 */
void (iexamine::*iexamine_function_from_string(std::string function_name))(player*, map*, int, int) {
  if ("none" == function_name) {
    return &iexamine::none;
  }
  if ("gaspump" == function_name) {
    return &iexamine::gaspump;
  }
  if ("toilet" == function_name) {
    return &iexamine::toilet;
  }
  if ("elevator" == function_name) {
    return &iexamine::elevator;
  }
  if ("controls_gate" == function_name) {
    return &iexamine::controls_gate;
  }
  if ("cardreader" == function_name) {
    return &iexamine::cardreader;
  }
  if ("rubble" == function_name) {
    return &iexamine::rubble;
  }
  if ("chainfence" == function_name) {
    return &iexamine::chainfence;
  }
  if ("tent" == function_name) {
    return &iexamine::tent;
  }
  if ("shelter" == function_name) {
    return &iexamine::shelter;
  }
  if ("wreckage" == function_name) {
    return &iexamine::wreckage;
  }
  if ("pit" == function_name) {
    return &iexamine::pit;
  }
  if ("pit_covered" == function_name) {
    return &iexamine::pit_covered;
  }
  if ("fence_post" == function_name) {
    return &iexamine::fence_post;
  }
  if ("remove_fence_rope" == function_name) {
    return &iexamine::remove_fence_rope;
  }
  if ("remove_fence_wire" == function_name) {
    return &iexamine::remove_fence_wire;
  }
  if ("remove_fence_barbed" == function_name) {
    return &iexamine::remove_fence_barbed;
  }
  if ("slot_machine" == function_name) {
    return &iexamine::slot_machine;
  }
  if ("safe" == function_name) {
    return &iexamine::safe;
  }
  if ("bulletin_board" == function_name) {
    return &iexamine::bulletin_board;
  }
  if ("fault" == function_name) {
    return &iexamine::fault;
  }
  if ("pedestal_wyrm" == function_name) {
    return &iexamine::pedestal_wyrm;
  }
  if ("pedestal_temple" == function_name) {
    return &iexamine::pedestal_temple;
  }
  if ("fswitch" == function_name) {
    return &iexamine::fswitch;
  }
  if ("flower_poppy" == function_name) {
    return &iexamine::flower_poppy;
  }
  if ("fungus" == function_name) {
    return &iexamine::fungus;
  }
  if ("dirtmound" == function_name) {
    return &iexamine::dirtmound;
  }
  if ("aggie_plant" == function_name) {
    return &iexamine::aggie_plant;
  }
  //pick_plant deliberately missing due to different function signature
  if ("tree_apple" == function_name) {
    return &iexamine::tree_apple;
  }
  if ("shrub_blueberry" == function_name) {
    return &iexamine::shrub_blueberry;
  }
  if ("shrub_strawberry" == function_name) {
    return &iexamine::shrub_strawberry;
  }
  if ("shrub_marloss" == function_name) {
    return &iexamine::shrub_marloss;
  }
  if ("shrub_wildveggies" == function_name) {
    return &iexamine::shrub_wildveggies;
  }
  if ("recycler" == function_name) {
    return &iexamine::recycler;
  }
  if ("trap" == function_name) {
    return &iexamine::trap;
  }
  if ("water_source" == function_name) {
    return &iexamine::water_source;
  }
  if ("acid_source" == function_name) {
    return &iexamine::acid_source;
  }

  //No match found
  debugmsg("Could not find an iexamine function matching '%s'!", function_name.c_str());
  return &iexamine::none;

}
