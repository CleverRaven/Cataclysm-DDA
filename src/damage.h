#ifndef DAMAGE_H
#define DAMAGE_H

#include "enums.h"
#include "bodypart.h"
#include "color.h"
#include <string>
#include <vector>
#include <set>
#include <algorithm>
#include <numeric>
#include <memory>

struct itype;
struct tripoint;
class item;
class monster;
class Creature;
class JsonObject;

enum damage_type : int {
    DT_NULL = 0, // null damage, doesn't exist
    DT_TRUE, // typeless damage, should always go through
    DT_BIOLOGICAL, // internal damage, like from smoke or poison
    DT_BASH, // bash damage
    DT_CUT, // cut damage
    DT_ACID, // corrosive damage, e.g. acid
    DT_STAB, // stabbing/piercing damage
    DT_HEAT, // e.g. fire, plasma
    DT_COLD, // e.g. heatdrain, cryogrenades
    DT_ELECTRIC, // e.g. electrical discharge
    NUM_DT
};

struct damage_unit {
    damage_type type;
    float amount;
    int res_pen;
    float res_mult;
    float damage_multiplier;

    damage_unit( damage_type dt, float a, int rp, float rm, float mul ) :
        type( dt ), amount( a ), res_pen( rp ), res_mult( rm ), damage_multiplier( mul ) { }
};


// a single atomic unit of damage from an attack. Can include multiple types
// of damage at different armor mitigation/penetration values
struct damage_instance {
    std::vector<damage_unit> damage_units;
    std::set<std::string> effects;
    damage_instance();
    static damage_instance physical( float bash, float cut, float stab, int arpen = 0 );
    void add_damage( damage_type dt, float a, int rp = 0, float rm = 1.0f, float mul = 1.0f );
    damage_instance( damage_type dt, float a, int rp = 0, float rm = 1.0f, float mul = 1.0f );
    void add_effect( std::string effect );
    void mult_damage( double multiplier );
    float type_damage( damage_type dt ) const;
    float total_damage() const;
    void clear();
};

struct dealt_damage_instance {
    std::vector<int> dealt_dams;
    body_part bp_hit;

    dealt_damage_instance();
    dealt_damage_instance( std::vector<int> &dealt );
    void set_damage( damage_type dt, int amount );
    int type_damage( damage_type dt ) const;
    int total_damage() const;
};

struct resistances {
    std::vector<int> resist_vals;

    resistances();

    // If to_self is true, we want armor's own resistance, not one it provides to wearer
    resistances( item &armor, bool to_self = false );
    resistances( monster &monster );
    void set_resist( damage_type dt, int amount );
    int type_resist( damage_type dt ) const;

    float get_effective_resist( const damage_unit &du ) const;
};

struct projectile {
        damage_instance impact;
        int speed; // how hard is it to dodge? essentially rolls to-hit, bullets have arbitrarily high values but thrown objects have dodgeable values

        std::set<std::string> proj_effects;

        /**
         * Returns an item that should be dropped or an item for which is_null() is true
         *  when item to drop is unset.
         */
        const item &get_drop() const;
        /** Copies item `it` as a drop for this projectile. */
        void set_drop( const item &it );
        void set_drop( item &&it );
        void unset_drop();

        projectile();
        projectile( const projectile & );
        projectile( projectile && ) = default;
        projectile &operator=( const projectile & );

    private:
        std::unique_ptr<item>
        drop; // Actual item used (to drop contents etc.). Null in case of bullets (they aren't "made of cartridges")
};

struct dealt_projectile_attack {
    projectile proj; // What we used to deal the attack
    Creature *hit_critter; // The critter that stopped the projectile or null
    dealt_damage_instance dealt_dam; // If hit_critter isn't null, hit data is written here
    tripoint end_point; // Last hit tile (is hit_critter is null, drops should spawn here)
    double missed_by; // Accuracy of dealt attack
};

void ammo_effects( const tripoint &p, const std::set<std::string> &effects );
int aoe_size( const std::set<std::string> &effects );

damage_type dt_by_name( const std::string &name );
const std::string &name_by_dt( const damage_type &dt );

damage_instance load_damage_instance( JsonObject &jo );
damage_instance load_damage_instance( JsonArray &jarr );

#endif
