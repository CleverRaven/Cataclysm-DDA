#ifndef _DAMAGE_H_
#define _DAMAGE_H_

#include "color.h"
#include <string>
#include <vector>
#include <set>
#include <algorithm>
#include <numeric>

class game;

enum damage_type
{
    DT_NONE= 0, // null damage, doesn't exist
    DT_TRUE, // typeless damage, should always go through
    DT_INTERNAL, // internal damage, like from smoke or poison
    DT_BASH, // bash damage
    DT_CUT, // cut damage
    DT_ACID, // corrosive damage, e.g. acid
    DT_STAB, // stabbing/piercing damage
    DT_HEAT, // e.g. fire, plasma
    DT_COLD, // e.g. heatdrain, cryogrenades
    DT_ELECTRIC, // e.g. electrical discharge
    NUM_DT
};

enum blast_shape
{
    BS_NONE = 0, // no aoe
    BS_BLAST, // generic "blast" effect, like grenades
    BS_RAYS, // randomly scattered rays of damage propragating from impact point
    BS_CONE, // cone-shaped effect, starting at impact and hitting enemies behind in a cone
    NUM_BS
};

struct damage_unit {
    damage_type type;
    float amount;
    int res_pen;
    float res_mult;

    damage_unit(damage_type dt, float a, int rp, float rm) :
        type(dt),
        amount(a),
        res_pen(rp),
        res_mult(rm)
    { }
};


// a single atomic unit of damage from an attack. Can include multiple types
// of damage at different armor mitigation/penetration values
struct damage_instance {
    std::vector<damage_unit> damage_units;
    damage_instance() { }
    static damage_instance physical(float bash, float cut, float stab, int arpen = 0) {
        damage_instance d;
        d.add_damage(DT_BASH, bash, arpen);
        d.add_damage(DT_CUT, cut, arpen);
        d.add_damage(DT_STAB, stab, arpen);
        return d;
    }
    void add_damage(damage_type dt, float a, int rp = 0, float rm = 1.0f) {
        damage_unit du(dt,a,rp,rm);
        damage_units.push_back(du);
    }
    float type_damage(damage_type dt) const {
        float ret = 0;
        for (std::vector<damage_unit>::const_iterator it = damage_units.begin();
                it != damage_units.end(); ++it) {
            if (it->type == dt) ret += it->amount;
        }
        return ret;
    }
    float total_damage() const {
        float ret = 0;
        for (std::vector<damage_unit>::const_iterator it = damage_units.begin();
                it != damage_units.end(); ++it) {
            ret += it->amount;
        }
        return ret;
    }
};

struct dealt_damage_instance {
    std::vector<int> dealt_dams;

    dealt_damage_instance() : dealt_dams(NUM_DT, 0) { }
    dealt_damage_instance(std::vector<int> &dealt) : dealt_dams(dealt) { }
    void set_damage(damage_type dt, int amount) {
        dealt_dams[dt] = amount;
    }
    int type_damage(damage_type dt) const {
        return dealt_dams[dt];
    }
    int total_damage() const {
        return std::accumulate(dealt_dams.begin(),dealt_dams.end(),0);
    }
};

class itype;
class it_ammo;

struct projectile {
    damage_instance impact;
    damage_instance payload;
    blast_shape aoe_shape;
    nc_color aoe_color;
    int aoe_size;

    // TODO: things below here are here temporarily until we finish those
    // systems
    std::set<std::string> proj_effects;
    it_ammo *ammo; // projectile's item that gets spawned at impact location, e.g. thrown weapons/bolts
    int num_spawn;

    projectile() :
        aoe_shape(BS_NONE),
        aoe_color(c_red),
        aoe_size(0),
        ammo(NULL),
        num_spawn(0)
    { }
};

void ammo_effects(game *g, int x, int y, const std::set<std::string> &effects);

#endif

