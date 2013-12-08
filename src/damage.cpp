#include "item.h"
#include "game.h"
#include "damage.h"

damage_instance::damage_instance() { }
damage_instance damage_instance::physical(float bash, float cut, float stab, int arpen) {
    damage_instance d;
    d.add_damage(DT_BASH, bash, arpen);
    d.add_damage(DT_CUT, cut, arpen);
    d.add_damage(DT_STAB, stab, arpen);
    return d;
}
void damage_instance::add_damage(damage_type dt, float a, int rp, float rm) {
    damage_unit du(dt,a,rp,rm);
    damage_units.push_back(du);
}
void damage_instance::mult_damage(double multiplier) {
    for (std::vector<damage_unit>::iterator it = damage_units.begin();
            it != damage_units.end(); ++it) {
        it->amount *= multiplier;
    }
}
float damage_instance::type_damage(damage_type dt) const {
    float ret = 0;
    for (std::vector<damage_unit>::const_iterator it = damage_units.begin();
            it != damage_units.end(); ++it) {
        if (it->type == dt) ret += it->amount;
    }
    return ret;
}
float damage_instance::total_damage() const {
    float ret = 0;
    for (std::vector<damage_unit>::const_iterator it = damage_units.begin();
            it != damage_units.end(); ++it) {
        ret += it->amount;
    }
    return ret;
}


dealt_damage_instance::dealt_damage_instance() : dealt_dams(NUM_DT, 0) { }
//TODO: add check to ensure length
dealt_damage_instance::dealt_damage_instance(std::vector<int> &dealt) : dealt_dams(dealt) { }
void dealt_damage_instance::set_damage(damage_type dt, int amount) {
    dealt_dams[dt] = amount;
}
int dealt_damage_instance::type_damage(damage_type dt) const {
    return dealt_dams[dt];
}
int dealt_damage_instance::total_damage() const {
    return std::accumulate(dealt_dams.begin(),dealt_dams.end(),0);
}


resistances::resistances() : resist_vals(NUM_DT, 0) { }

resistances::resistances(item& armor) : resist_vals(NUM_DT, 0) {
    if (armor.is_armor()) {
        set_resist(DT_BASH, armor.bash_resist());
        set_resist(DT_CUT, armor.cut_resist());
        set_resist(DT_STAB, 0.8*armor.cut_resist()); // stab dam cares less bout armor
    }
}
void resistances::set_resist(damage_type dt, int amount) {
    resist_vals[dt] = amount;
}
int resistances::type_resist(damage_type dt) const {
    return resist_vals[dt];
}



void ammo_effects(game *g, int x, int y, const std::set<std::string> &effects)
{
  if (effects.count("EXPLOSIVE"))
    g->explosion(x, y, 24, 0, false);

  if (effects.count("FRAG"))
    g->explosion(x, y, 12, 28, false);

  if (effects.count("NAPALM"))
    g->explosion(x, y, 18, 0, true);

  if (effects.count("NAPALM_BIG"))
    g->explosion(x, y, 72, 0, true);

  if (effects.count("MININUKE_MOD")){
    g->explosion(x, y, 200, 0, false);
    int junk;
    for (int i = -4; i <= 4; i++) {
     for (int j = -4; j <= 4; j++) {
      if (g->m.sees(x, y, x + i, y + j, 3, junk) &&
          g->m.move_cost(x + i, y + j) > 0)
       g->m.add_field(g, x + i, y + j, fd_nuke_gas, 3);
     }
    }
  }

  if (effects.count("ACIDBOMB")) {
    for (int i = x - 1; i <= x + 1; i++) {
      for (int j = y - 1; j <= y + 1; j++) {
        g->m.add_field(g, i, j, fd_acid, 3);
      }
    }
  }

  if (effects.count("EXPLOSIVE_BIG"))
    g->explosion(x, y, 40, 0, false);

  if (effects.count("EXPLOSIVE_HUGE"))
    g->explosion(x, y, 80, 0, false);

  if (effects.count("TEARGAS")) {
    for (int i = -2; i <= 2; i++) {
      for (int j = -2; j <= 2; j++)
        g->m.add_field(g, x + i, y + j, fd_tear_gas, 3);
    }
  }

  if (effects.count("SMOKE")) {
    for (int i = -1; i <= 1; i++) {
      for (int j = -1; j <= 1; j++)
        g->m.add_field(g, x + i, y + j, fd_smoke, 3);
    }
  }
  if (effects.count("SMOKE_BIG")) {
    for (int i = -6; i <= 6; i++) {
      for (int j = -6; j <= 6; j++)
        g->m.add_field(g, x + i, y + j, fd_smoke, 18);
    }
  }

  if (effects.count("FLASHBANG"))
    g->flashbang(x, y);

  if (effects.count("FLAME"))
    g->explosion(x, y, 4, 0, true);

  if (effects.count("FLARE"))
    g->m.add_field(g, x, y, fd_fire, 1);

  if (effects.count("LIGHTNING")) {
    for (int i = x - 1; i <= x + 1; i++) {
      for (int j = y - 1; j <= y + 1; j++) {
        g->m.add_field(g, i, j, fd_electricity, 3);
      }
    }
  }

  if (effects.count("PLASMA")) {
    for (int i = x - 1; i <= x + 1; i++) {
      for (int j = y - 1; j <= y + 1; j++) {
        if (one_in(2))
          g->m.add_field(g, i, j, fd_plasma, rng(2,3));
      }
    }
  }

}


