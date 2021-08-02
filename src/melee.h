#pragma once
#ifndef CATA_SRC_MELEE_H
#define CATA_SRC_MELEE_H

/*
 * statistics data for melee attacks, used for test purposes
 */
struct melee_statistic_data {
    int attack_count = 0;
    int hit_count = 0;
    int double_crit_count = 0;
    int crit_count = 0;
    int actual_crit_count = 0;
    double double_crit_chance = 0.0;
    double crit_chance = 0.0;
    int damage_amount = 0;
};

namespace melee
{

float melee_hit_range( float accuracy );
melee_statistic_data get_stats();
void clear_stats();
extern melee_statistic_data melee_stats;
} // namespace melee

#endif // CATA_SRC_MELEE_H
