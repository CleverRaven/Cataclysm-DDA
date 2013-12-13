#ifndef _DAMAGE_H_
#define _DAMAGE_H_

#include "bodypart.h"
#include "color.h"
#include <string>
#include <vector>
#include <set>
#include <algorithm>
#include <numeric>

class game;
struct itype;
struct it_ammo;
class item;
class monster;

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
    damage_instance();
    static damage_instance physical(float bash, float cut, float stab, int arpen = 0);
    void add_damage(damage_type dt, float a, int rp = 0, float rm = 1.0f);
    void mult_damage(double multiplier);
    float type_damage(damage_type dt) const;
    float total_damage() const;
};

struct dealt_damage_instance {
    std::vector<int> dealt_dams;
    body_part bp_hit;

    dealt_damage_instance();
    //TODO: add check to ensure length
    dealt_damage_instance(std::vector<int> &dealt);
    void set_damage(damage_type dt, int amount);
    int type_damage(damage_type dt) const;
    int total_damage() const;
};

struct resistances {
    std::vector<int> resist_vals;

    resistances();

    resistances(item& armor);
    resistances(monster& monster);
    void set_resist(damage_type dt, int amount);
    int type_resist(damage_type dt) const;

    float get_effective_resist(const damage_unit& du);
};

struct projectile {
    damage_instance impact;
    damage_instance payload;
    blast_shape aoe_shape;
    nc_color aoe_color;
    int aoe_size;
    int speed; // how hard is it to dodge? essentially rolls to-hit, bullets have arbitrarily high values but thrown objects have dodgeable values
    bool drops; // does it drop ammo units?
    bool wide; // a shot that "covers" the target, e.g. a shotgun blast or flamethrower napalm

    // TODO: things below here are here temporarily until we finish those
    // systems
    std::set<std::string> proj_effects;
    it_ammo *ammo; // projectile's item that gets spawned at impact location, e.g. thrown weapons/bolts

    projectile() :
        aoe_shape(BS_NONE),
        aoe_color(c_red),
        aoe_size(0),
        speed(0),
        drops(false),
        wide(false),
        ammo(NULL)
    { }
};

void ammo_effects(game *g, int x, int y, const std::set<std::string> &effects);

#endif

