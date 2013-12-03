#ifndef _DAMAGE_H_
#define _DAMAGE_H_

#include "enums.h"
#include <algorithm>
#include <numeric>

struct damage_unit {
    damage_type type;
    int amount;
    int res_pen;
    float res_mult;
    damage_unit(damage_type dt, int a, int rp, float rm) :
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
    static damage_instance physical(int bash, int cut, int stab) {
        damage_instance d;
        d.add_damage(DT_BASH, bash);
        d.add_damage(DT_CUT, cut);
        d.add_damage(DT_STAB, stab);
        return d;
    }
    void add_damage(damage_type dt, int a, int rp = 0, float rm = 1.0f) {
        damage_unit du(dt,a,rp,rm);
        damage_units.push_back(du);
    }
    int type_damage(damage_type dt) const {
        int ret = 0;
        for (std::vector<damage_unit>::const_iterator it = damage_units.begin();
                it != damage_units.end(); ++it) {
            if (it->type == dt) ret += it->amount;
        }
        return ret;
    }
    int total_damage() const {
        int ret = 0;
        for (std::vector<damage_unit>::const_iterator it = damage_units.begin();
                it != damage_units.end(); ++it) {
            ret += it->amount;
        }
        return ret;
    }
};

struct dealt_damage_instance {
    std::vector<int> dealt_dams;

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

damage_instance physical_damage_instance(int bash, int cut, int stab);

#endif

