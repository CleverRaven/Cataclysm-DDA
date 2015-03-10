#include "item.h"
#include "monster.h"
#include "game.h"
#include "damage.h"
#include "rng.h"

damage_instance::damage_instance() { }
damage_instance damage_instance::physical(float bash, float cut, float stab, int arpen)
{
    damage_instance d;
    d.add_damage(DT_BASH, bash, arpen);
    d.add_damage(DT_CUT, cut, arpen);
    d.add_damage(DT_STAB, stab, arpen);
    return d;
}
damage_instance::damage_instance(damage_type dt, float a, int rp, float rm)
{
    add_damage( dt, a, rp, rm );
}

void damage_instance::add_damage(damage_type dt, float a, int rp, float rm)
{
    damage_unit du(dt, a, rp, rm);
    damage_units.push_back(du);
}
void damage_instance::add_effect( std::string effect )
{
    effects.insert( effect );
}

void damage_instance::mult_damage(double multiplier)
{
    for( auto &elem : damage_units ) {
        elem.damage_multiplier *= multiplier;
    }
}
float damage_instance::type_damage(damage_type dt) const
{
    float ret = 0;
    for( const auto &elem : damage_units ) {
        if( elem.type == dt ) {
            ret += elem.amount;
        }
    }
    return ret;
}
//This returns the damage from this damage_instance. The damage done to the target will be reduced by their armor.
float damage_instance::total_damage() const
{
    float ret = 0;
    for( const auto &elem : damage_units ) {
        ret += elem.amount;
    }
    return ret;
}
void damage_instance::clear()
{
    damage_units.clear();
    effects.clear();
}

dealt_damage_instance::dealt_damage_instance() : dealt_dams(NUM_DT, 0) { }
//TODO: add check to ensure length
dealt_damage_instance::dealt_damage_instance(std::vector<int> &dealt) : dealt_dams(dealt) { }
void dealt_damage_instance::set_damage(damage_type dt, int amount)
{
    dealt_dams[dt] = amount;
}
int dealt_damage_instance::type_damage(damage_type dt) const
{
    return dealt_dams[dt];
}
int dealt_damage_instance::total_damage() const
{
    return std::accumulate(dealt_dams.begin(), dealt_dams.end(), 0);
}


resistances::resistances() : resist_vals(NUM_DT, 0) { }
resistances::resistances(item &armor) : resist_vals(NUM_DT, 0)
{
    if (armor.is_armor()) {
        set_resist(DT_BASH, armor.bash_resist());
        set_resist(DT_CUT, armor.cut_resist());
        set_resist(DT_STAB, 0.8 * armor.cut_resist()); // stab dam cares less bout armor
        set_resist(DT_ACID, armor.acid_resist());
    }
}
resistances::resistances(monster &monster) : resist_vals(NUM_DT, 0)
{
    set_resist(DT_BASH, monster.type->armor_bash);
    set_resist(DT_CUT, monster.type->armor_cut);
    set_resist(DT_STAB, 0.8 * monster.type->armor_cut); // stab dam cares less bout armor
}
void resistances::set_resist(damage_type dt, int amount)
{
    resist_vals[dt] = amount;
}
int resistances::type_resist(damage_type dt) const
{
    return resist_vals[dt];
}
float resistances::get_effective_resist(const damage_unit &du)
{
    float effective_resist = 0.f;
    switch (du.type) {
    case DT_BASH:
        effective_resist = std::max(type_resist(DT_BASH) - du.res_pen, 0) * du.res_mult;
        break;
    case DT_CUT:
        effective_resist = std::max(type_resist(DT_CUT) - du.res_pen, 0) * du.res_mult;
        break;
    case DT_STAB:
        effective_resist = std::max(type_resist(DT_STAB) - du.res_pen, 0) * du.res_mult;
        break;
    case DT_ACID:
        effective_resist = std::max(type_resist(DT_ACID) - du.res_pen, 0) * du.res_mult;
        break;
    default: // TODO: DT_HEAT vs env protection, DT_COLD vs warmth
        effective_resist = 0;
    }
    return effective_resist;
}



void ammo_effects(int x, int y, const std::set<std::string> &effects)
{
    if (effects.count("EXPLOSIVE")) {
        g->explosion(x, y, 24, 0, false);
    }

    if (effects.count("FRAG")) {
        g->explosion(x, y, 12, 28, false);
    }

    if (effects.count("NAPALM")) {
        g->explosion(x, y, 18, 0, true);
    }

    if (effects.count("NAPALM_BIG")) {
        g->explosion(x, y, 72, 0, true);
    }

    if (effects.count("MININUKE_MOD")) {
        g->explosion(x, y, 450, 0, false);
        int junk;
        for (int i = -6; i <= 6; i++) {
            for (int j = -6; j <= 6; j++) {
                if (g->m.sees(x, y, x + i, y + j, 3, junk) &&
                    g->m.move_cost(x + i, y + j) > 0) {
                    g->m.add_field(x + i, y + j, fd_nuke_gas, 3);
                }
            }
        }
    }

    if (effects.count("ACIDBOMB")) {
        for (int i = x - 1; i <= x + 1; i++) {
            for (int j = y - 1; j <= y + 1; j++) {
                g->m.add_field(i, j, fd_acid, 3);
            }
        }
    }

    if (effects.count("EXPLOSIVE_BIG")) {
        g->explosion(x, y, 40, 0, false);
    }

    if (effects.count("EXPLOSIVE_HUGE")) {
        g->explosion(x, y, 80, 0, false);
    }

    if (effects.count("TEARGAS")) {
        for (int i = -2; i <= 2; i++) {
            for (int j = -2; j <= 2; j++) {
                g->m.add_field(x + i, y + j, fd_tear_gas, 3);
            }
        }
    }

    if (effects.count("SMOKE")) {
        for (int i = -1; i <= 1; i++) {
            for (int j = -1; j <= 1; j++) {
                g->m.add_field(x + i, y + j, fd_smoke, 3);
            }
        }
    }
    if (effects.count("SMOKE_BIG")) {
        for (int i = -6; i <= 6; i++) {
            for (int j = -6; j <= 6; j++) {
                g->m.add_field(x + i, y + j, fd_smoke, 18);
            }
        }
    }

    if (effects.count("FLASHBANG")) {
        g->flashbang(x, y);
    }

    // TODO: g->u? Are NPC not allowed to use those weapons, or do they ignored the flag because they are stupid ncps and have no right to use those flags.
    if (!g->u.weapon.has_flag("NO_BOOM") && effects.count("FLAME")) {
        g->explosion(x, y, 4, 0, true);
    }

    // TODO: g->u? Are NPC not allowed to use those weapons, or do they ignored the flag because they are stupid ncps and have no right to use those flags.
    if (g->u.weapon.has_flag("FLARE") || effects.count("FLARE")) {
        g->m.add_field(x, y, fd_fire, 1);
    }

    if (effects.count("LIGHTNING")) {
        for (int i = x - 1; i <= x + 1; i++) {
            for (int j = y - 1; j <= y + 1; j++) {
                g->m.add_field(i, j, fd_electricity, 3);
            }
        }
    }

    if (effects.count("PLASMA")) {
        for (int i = x - 1; i <= x + 1; i++) {
            for (int j = y - 1; j <= y + 1; j++) {
                if (one_in(2)) {
                    g->m.add_field(i, j, fd_plasma, rng(2, 3));
                }
            }
        }
    }

}


