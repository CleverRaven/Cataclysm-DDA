#include "mondeath.h"
#include "monster.h"
#include "game.h"
#include "debug.h"
#include "map.h"
#include "rng.h"
#include "line.h"
#include "bodypart.h"
#include "material.h"
#include "json.h"
#include "monstergenerator.h"
#include "speech.h"
#include "messages.h"
#include "sounds.h"
#include "monattack.h"
#include "mondefense.h"

#include <algorithm>

//Used for e^(x) functions
#include <stdio.h>
#include <math.h>

// for loading monster dialogue:
#include <iostream>
#include <fstream>

#include <limits>  // std::numeric_limits
#define SKIPLINE(stream) stream.ignore(std::numeric_limits<std::streamsize>::max(), '\n')

// shared utility functions
int within_visual_range(monster *z, int max) {
    int dist;

    dist = rl_dist( z->pos(), g->u.pos() );
    if (dist > max || !z->sees( g->u ) ) {
        return -1;    // Out of range
    }
    return dist;
}

npc make_fake_npc(monster *z, int str, int dex, int inte, int per) {
    npc tmp;
    tmp.name = _("The ") + z->name();
    tmp.set_fake(true);
    tmp.recoil = 0;
    tmp.setx( z->posx() );
    tmp.sety( z->posy() );
    tmp.str_cur = str;
    tmp.dex_cur = dex;
    tmp.int_cur = inte;
    tmp.per_cur = per;
    if( z->friendly != 0 ) {
        tmp.attitude = NPCATT_DEFEND;
    } else {
        tmp.attitude = NPCATT_KILL;
    }
    return tmp;
}

void mattack::none(monster *, int)
{
}

void mattack::antqueen(monster *z, int index)
{
    std::vector<point> egg_points;
    std::vector<int> ants;
    z->reset_special(index); // Reset timer
    // Count up all adjacent tiles the contain at least one egg.
    for (int x = z->posx() - 2; x <= z->posx() + 2; x++) {
        for (int y = z->posy() - 2; y <= z->posy() + 2; y++) {
            for (auto &i : g->m.i_at(x, y)) {
                // is_empty() because we can't hatch an ant under the player, a monster, etc.
                if (i.type->id == "ant_egg" && g->is_empty(x, y)) {
                    egg_points.push_back(point(x, y));
                    break; // Done looking at this tile
                }
                int mondex = g->mon_at(x, y);
                if (mondex != -1 && (g->zombie(mondex).type->id == "mon_ant_larva" ||
                                     g->zombie(mondex).type->id == "mon_ant"         )) {
                    ants.push_back(mondex);
                }
            }
        }
    }

    if (!ants.empty()) {
        z->moves -= 100; // It takes a while
        int mondex = ants[ rng(0, ants.size() - 1) ];
        monster *ant = &(g->zombie(mondex));
        if (g->u.sees( *z ) && g->u.sees( *ant ))
            add_msg(m_warning, _("The %s feeds an %s and it grows!"), z->name().c_str(),
                    ant->name().c_str());
        if (ant->type->id == "mon_ant_larva") {
            ant->poly(GetMType("mon_ant"));
        } else {
            ant->poly(GetMType("mon_ant_soldier"));
        }
    } else if (egg_points.empty()) { // There's no eggs nearby--lay one.
        if (g->u.sees( *z )) {
            add_msg(_("The %s lays an egg!"), z->name().c_str());
        }
        g->m.spawn_item(z->posx(), z->posy(), "ant_egg", 1, 0, calendar::turn);
    } else { // There are eggs nearby.  Let's hatch some.
        z->moves -= 20 * egg_points.size(); // It takes a while
        if (g->u.sees( *z )) {
            add_msg(m_warning, _("The %s tends nearby eggs, and they hatch!"), z->name().c_str());
        }
        for (auto &i : egg_points) {
            for (size_t j = 0; j < g->m.i_at(i.x, i.y).size(); j++) {
                if (g->m.i_at(i.x, i.y)[j].type->id == "ant_egg") {
                    g->m.i_rem(i.x, i.y, j);
                    monster tmp(GetMType("mon_ant_larva"), tripoint( i.x, i.y, z->posz() ) );
                    tmp.faction = z->faction;
                    g->add_zombie(tmp);
                    break; // Max one hatch per tile
                }
            }
        }
    }
}

void mattack::shriek(monster *z, int index)
{
    Creature *target = z->attack_target();
    if( target == nullptr ||
        rl_dist( z->posx(), z->posy(), target->posx(), target->posy() ) > 4 ||
        !z->sees( *target ) ) {
        return;
    }

    z->moves -= 240;   // It takes a while
    z->reset_special(index); // Reset timer
    sounds::sound(z->posx(), z->posy(), 50, _("a terrible shriek!"));
}

void mattack::howl(monster *z, int index)
{
    Creature *target = z->attack_target();
    if( target == nullptr ||
        rl_dist( z->posx(), z->posy(), target->posx(), target->posy() ) > 4 ||
        !z->sees( *target ) ) {
        return;
    }

    z->moves -= 200;   // It takes a while
    z->reset_special(index); // Reset timer
    sounds::sound(z->posx(), z->posy(), 35, _("an ear-piercing howl!"));

    if( z->friendly != 0 ) { // TODO: Make this use mon's faction when those are in
        for( size_t i = 0; i < g->num_zombies(); ++i ) {
            auto &other = g->zombie( i );
            if( other.is_dead() || other.type != z->type || z->friendly != 0 ) {
                continue;
            }
            // Quote KA101: Chance of friendlying other howlers in the area, I'd imagine:
            // wolves use howls for communication and can convey that the ape is on Team Wolf.
            if( one_in( 4 ) ) {
                other.friendly = z->friendly;
                break;
            }
        }
    }
}

void mattack::rattle(monster *z, int index)
{
    // TODO: Let it rattle at non-player friendlies
    const int min_dist = z->friendly != 0 ? 1 : 4;
    Creature *target = &g->u; // Can't use attack_target - the snake has no target
    if( target == nullptr ||
        rl_dist( z->posx(), z->posy(), target->posx(), target->posy() ) > min_dist ||
        !z->sees( *target ) ) {
        return;
    }

    z->moves -= 20;   // It takes a very short while
    z->reset_special(index); // Reset timer
    sounds::sound(z->posx(), z->posy(), 10, _("a sibilant rattling sound!"));
}

void mattack::acid(monster *z, int index)
{
    if( !z->can_act() ) {
        return;
    }

    Creature *target = z->attack_target();
    if( target == nullptr ) {
        return;
    }

    int junk = 0;
    if( !z->sees( *target ) ||
        !g->m.clear_path( z->posx(), z->posy(), target->posx(), target->posy(), 10, 1, 100, junk ) ) {
        return; // Can't see/reach target, no attack
    }
    z->moves -= 300;   // It takes a while
    z->reset_special(index); // Reset timer
    sounds::sound(z->posx(), z->posy(), 4, _("a spitting noise."));
    int hitx = target->posx() + rng(-2, 2);
    int hity = target->posy() + rng(-2, 2);
    std::vector<point> line = line_to(z->posx(), z->posy(), hitx, hity, junk);
    for (auto &i : line) {
        // TODO: Z
        if (g->m.hit_with_acid( tripoint( i.x, i.y, z->posz() ) )) {
            if (g->u.sees( i )) {
                add_msg(_("A glob of acid hits the %s!"),
                        g->m.tername(i.x, i.y).c_str());
            }
            return;
        }
    }
    for (int i = -3; i <= 3; i++) {
        for (int j = -3; j <= 3; j++) {
            if (g->m.move_cost(hitx + i, hity + j) > 0 &&
                g->m.sees(hitx + i, hity + j, hitx, hity, 6, junk) &&
                ((one_in(abs(j)) && one_in(abs(i))) || (i == 0 && j == 0))) {
                g->m.add_field(hitx + i, hity + j, fd_acid, 2);
            }
        }
    }
}

void mattack::shockstorm(monster *z, int index)
{
    if( !z->can_act() ) {
        return;
    }

    Creature *target = z->attack_target();
    if( target == nullptr ) {
        return;
    }

    bool seen = g->u.sees( *z );
    int junk = 0;
    if( !z->sees( *target ) ||
        !g->m.clear_path( z->posx(), z->posy(), target->posx(), target->posy(), 12, 1, 100, junk ) ) {
        return; // Can't see/reach target, no attack
    }
    z->moves -= 50;   // It takes a while
    z->reset_special(index); // Reset timer

    if( seen ) {
        auto msg_type = target == &g->u ? m_bad : m_neutral;
        add_msg( msg_type, _("A bolt of electricity arcs towards %s!"), target->disp_name().c_str() );
    }
    int tarx = target->posx() + rng(-1, 1) + rng(-1, 1);// 3 in 9 chance of direct hit,
    int tary = target->posy() + rng(-1, 1) + rng(-1, 1);// 4 in 9 chance of near hit
    std::vector<point> bolt = line_to(z->posx(), z->posy(), tarx, tary, junk);
    for (auto &i : bolt) { // Fill the LOS with electricity
        if (!one_in(4)) {
            g->m.add_field(i.x, i.y, fd_electricity, rng(1, 3));
        }
    }
    // 5x5 cloud of electricity at the square hit
    for (int i = tarx - 2; i <= tarx + 2; i++) {
        for (int j = tary - 2; j <= tary + 2; j++) {
            if (!one_in(4) || (i == 0 && j == 0)) {
                g->m.add_field(i, j, fd_electricity, rng(1, 3));
            }
        }
    }
}

void mattack::pull_metal_weapon(monster *z, int index)
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constants and Configuration

    // max distance that "pull_metal_weapon" can be applied to the target.
    constexpr auto max_distance = 12;

    // attack movement costs
    constexpr int att_cost_pull = 150;

    // minimum str to resist "pull_metal_weapon"
    constexpr int min_str = 4;

    Creature *target = z->attack_target();
    if( target == nullptr ) {
        return;
    }

    int junk = 0;
    if( !z->sees( *target ) || !g->m.clear_path( z->posx(), z->posy(), target->posx(), target->posy(),
                                                 max_distance, 1, 100, junk ) ) {
        return; // Can't see/reach target, no attack
    }
    player *foe = dynamic_cast< player* >( target );
    if( foe != nullptr ) {
        if ( foe->weapon.made_of("iron") || foe->weapon.made_of("steel") ) {
            int wp_skill = foe->skillLevel("melee");
            z->moves -= att_cost_pull;   // It takes a while
            z->reset_special(index); // Reset timer
            int success = 100;
            if ( foe->str_cur > min_str ) {
                success = std::max(100 - (6 * (foe->str_cur - 6)) - (6 * wp_skill), 0);
            }
            auto m_type = foe == &g->u ? m_bad : m_neutral;
            if ( rng(1, 100) <= success ) {
                target->add_msg_player_or_npc( m_type, _("%s is pulled away from your hands!"),
                                                       _("%s is pulled away from <npcname>'s hands!"), foe->weapon.tname().c_str() );
                z->add_item(foe->remove_weapon());
            } else {
                target->add_msg_player_or_npc( m_type, _("The %s unsuccessfully attempts to pull your weapon away."),
                                                       _("The %s unsuccessfully attempts to pull <npcname>'s weapon away."), z->name().c_str() );
            }
        }
    }
}

void mattack::smokecloud(monster *z, int index)
{
    z->reset_special(index); // Reset timer
    const int monx = z->posx();
    const int mony = z->posy();
    int junk = 0;
    for (int i = -3; i <= 3; i++) {
        for (int j = -3; j <= 3; j++) {
            if( g->m.move_cost( monx + i, mony + j ) != 0 &&
                g->m.clear_path(monx, mony, monx + i, mony + j, 3, 1, 100, junk) ) {
                g->m.add_field(monx + i, mony + j, fd_smoke, 2);
            }
        }
    }
    //Round it out a bit
    for (int i = -2; i <= 2; i++) {
        if( g->m.move_cost( monx + i, mony + 4 ) != 0 &&
            g->m.clear_path(monx, mony, monx + i, mony + 4, 3, 1, 100, junk) ) {
            g->m.add_field(monx + i, mony + 4, fd_smoke, 2);
        }
        if( g->m.move_cost( monx + i, mony - 4 ) != 0 &&
            g->m.clear_path(monx, mony, monx + i, mony - 4, 3, 1, 100, junk) ) {
            g->m.add_field(monx + i, mony - 4, fd_smoke, 2);
        }
        if( g->m.move_cost( monx + 4, mony + i ) != 0 &&
            g->m.clear_path(monx, mony, monx + 4, mony + i, 3, 1, 100, junk) ) {
            g->m.add_field(monx + 4, mony + i, fd_smoke, 2);
        }
        if( g->m.move_cost( monx - 4, mony + i ) != 0 &&
            g->m.clear_path(monx, mony, monx - 4, mony + i, 3, 1, 100, junk) ) {
            g->m.add_field(monx - 4, mony + i, fd_smoke, 2);
        }
    }
}

void mattack::boomer(monster *z, int index)
{
    if( !z->can_act() ) {
        return;
    }

    int t;
    Creature *target = z->attack_target();
    if( target == nullptr ||
        rl_dist( z->pos(), target->pos() ) > 3 ||
        !z->sees( *target, t ) ) {
        return;
    }

    player *foe = dynamic_cast< player* >( target );
    std::vector<point> line = line_to( z->pos(), target->pos(), t );
    z->reset_special(index); // Reset timer
    z->moves -= 250;   // It takes a while
    bool u_see = g->u.sees( *z );
    if( u_see ) {
        add_msg(m_warning, _("The %s spews bile!"), z->name().c_str());
    }
    for (auto &i : line) {
        g->m.add_field(i.x, i.y, fd_bile, 1);
        // If bile hit a solid tile, return.
        if (g->m.move_cost(i.x, i.y) == 0) {
            g->m.add_field(i.x, i.y, fd_bile, 3);
            if (g->u.sees( i ))
                add_msg(_("Bile splatters on the %s!"),
                        g->m.tername(i.x, i.y).c_str());
            return;
        }
    }
    if( !target->uncanny_dodge() ) {
        if (rng(0, 10) > target->get_dodge() || one_in( target->get_dodge() ) ) {
            target->add_env_effect("boomered", bp_eyes, 3, 12);
        } else if( u_see ) {
            target->add_msg_player_or_npc( _("You dodge it!"),
                                           _("<npcname> dodges it!") );
        }
        if( foe != nullptr ) {
            foe->practice( "dodge", 10 );
            foe->ma_ondodge_effects();
        }
    }
}

void mattack::resurrect(monster *z, int index)
{
    if( z->get_speed() < z->get_speed_base() / 2) {
        return;    // We can only resurrect so many times!
    }
    std::vector<point> corpses;
    int junk;
    // Find all corpses that we can see within 4 tiles.
    for (int x = z->posx() - 4; x <= z->posx() + 4; x++) {
        for (int y = z->posy() - 4; y <= z->posy() + 4; y++) {
            if (g->is_empty(x, y) && g->m.sees(z->posx(), z->posy(), x, y, -1, junk)) {
                for (auto &i : g->m.i_at(x, y)) {
                    if (i.is_corpse() && i.get_mtype()->has_flag(MF_REVIVES) &&
                          i.get_mtype()->in_species("ZOMBIE")) {
                        corpses.push_back(point(x, y));
                        break;
                    }
                }
            }
        }
    }
    if (corpses.empty()) { // No nearby corpses
        return;
    }
    z->set_speed_base( (z->get_speed_base() - rng(0, 10)) * 0.8 );
    bool sees_necromancer = g->u.sees(*z);
    if (sees_necromancer) {
        add_msg(_("The %s throws its arms wide..."), z->name().c_str());
    }
    z->reset_special(index); // Reset timer
    z->moves -= 500;   // It takes a while
    int raised = 0;
    for (auto &i : corpses) {
        int x = i.x, y = i.y;
        for (size_t n = 0; n < g->m.i_at(x, y).size(); n++) {
            if (g->m.i_at(x, y)[n].is_corpse() && one_in(2)) {
                if (!g->revive_corpse(x, y, n)) {
                    continue;
                }
                if( z->friendly != 0 ) {
                    g->zombie(g->num_zombies() - 1).friendly = z->friendly;
                }
                if (g->u.sees(x, y)) {
                    raised++;
                }
                break; // Only one body raised per tile
            }
        }
    }
    if (raised > 0) {
        if (raised == 1) {
            add_msg(m_warning, _("A nearby corpse rises from the dead!"));
        } else if (raised < 4) {
            add_msg(m_warning, _("A few corpses rise from the dead!"));
        } else {
            add_msg(m_warning, _("Several corpses rise from the dead!"));
        }
    } else if (sees_necromancer) {
        add_msg(_("...but nothing seems to happen."));
    }
}

void mattack::smash(monster *z, int index)
{
    if( !z->can_act() ) {
        return;
    }

    Creature *target = z->attack_target();
    if( target == nullptr || rl_dist( z->pos(), target->pos() ) > 1 ) {
        return;
    }

    player *foe = dynamic_cast< player* >( target );
    bool seen = g->u.sees( *z );

    z->reset_special( index ); // Reset timer
    // Costs lots of moves to give you a little bit of a chance to get away.
    z->moves -= 400;

    if( target->uncanny_dodge() ) {
        return;
    }

    // Can we dodge the attack? Uses player dodge function % chance (melee.cpp)
    if (dodge_check(z, target)) {
        target->add_msg_player_or_npc( _("The %s takes a powerful swing at you, but you dodge it!"),
                                       _("The %s takes a powerful swing at <npcname>, but <npcname> dodges it!"),
                                       z->name().c_str() );
        if( foe != nullptr ) {
            foe->practice( "dodge", z->type->melee_skill * 2 );
            foe->ma_ondodge_effects();
        }
        return;
    }

    if( seen ) {
        add_msg( _("A blow from the %s sends %s flying!"), z->name().c_str(), target->disp_name().c_str() );
    }
    g->fling_creature( target, g->m.coord_to_angle( z->posx(), z->posy(), target->posx(), target->posy() ),
                       z->type->melee_sides * z->type->melee_dice * 3 );
}

//--------------------------------------------------------------------------------------------------
// TODO: move elsewhere
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Find empty spaces around origin within a radius of N.
 *
 * @returns a pair with first  = array<point, area>; area = (2*N + 1)^2.
 *                      second = the number of empty spaces found.
 */
template <size_t N = 1>
std::pair<std::array<point, (2*N + 1)*(2*N + 1)>, size_t>
find_empty_neighbors(point const origin) {
    constexpr auto r = static_cast<int>(N);

    const int x_min = origin.x - r;
    const int x_max = origin.x + r;
    const int y_min = origin.y - r;
    const int y_max = origin.y + r;

    std::pair<std::array<point, (2*N + 1)*(2*N + 1)>, size_t> result;

    for (auto x = x_min; x <= x_max; ++x) {
        for (auto y = y_min; y <= y_max; ++y) {
            if (g->is_empty(x, y)) {
                result.first[result.second++] = point {x, y};
            }
        }
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Find empty spaces around a creature within a radius of N.
 *
 * @see find_empty_neighbors
 */
 template <size_t N = 1>
std::pair<std::array<point, (2*N + 1)*(2*N + 1)>, size_t>
find_empty_neighbors(Creature const &c) {
    return find_empty_neighbors<N>(c.pos());
}

//--------------------------------------------------------------------------------------------------
/**
 * Get a size_t value in the closed interval [0, size]; a convenience to avoid messy casting.
  */
size_t get_random_index(size_t const size) {
    return static_cast<size_t>(rng(0, static_cast<long>(size - 1)));
}

//--------------------------------------------------------------------------------------------------
/**
 * Get a size_t value in the closed interval [0, c.size() - 1]; a convenience to avoid messy casting.
 */
template <typename Container>
size_t get_random_index(Container const &c) {
    return get_random_index(c.size());
}

void mattack::science(monster *const z, int const index) // I said SCIENCE again!
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constants and Configuration

    // attack types
    enum : int {
        att_shock,
        att_radiation,
        att_manhack,
        att_acid_pool,
        att_flavor,
        att_enum_size
    };

    // max distance that "science" can be applied to the target.
    constexpr auto max_distance = 5;

    // attack movement costs
    constexpr int att_cost_shock   = 0;
    constexpr int att_cost_rad     = 400;
    constexpr int att_cost_manhack = 200;
    constexpr int att_cost_acid    = 100;
    constexpr int att_cost_flavor  = 80;

    // radiation attack behaviour
    constexpr int att_rad_dodge_diff    = 16; // how hard it is to dodge
    constexpr int att_rad_mutate_chance = 6;  // (1/x) inverse chance to cause mutation.
    constexpr int att_rad_dose_min      = 20; // min radiation
    constexpr int att_rad_dose_max      = 50; // max radiation

    // acid attack behaviour
    constexpr int att_acid_density = 3;

    // flavor messages
    static std::array<char const*, 4> const m_flavor = {{
        _("The %s gesticulates wildly!"),
        _("The %s coughs up a strange dust."),
        _("The %s moans softly."),
        _("The %s's skin crackles with electricity."), //special case; leave this last
    }};

    if( !z->can_act() ) {
        return;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Look for a valid target...
    Creature *const target = z->attack_target();
    if (!target) {
        return;
    }

    // too far
    const int dist = rl_dist(z->pos(), target->pos());
    if (dist > max_distance) {
        return;
    }

    // can't attack what you can't see
    if (!z->sees(*target)) {
        return;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // ok, we have a valid target; populate valid attack options...
    std::array<int, att_enum_size> valid_attacks;
    size_t valid_attack_count = 0;

    // can only shock if adjacent
    if (dist == 1) {
        valid_attacks[valid_attack_count++] = att_shock;
    }

    // TODO: mutate() doesn't like non-players right now
    // It will mutate NPCs, but it will say it mutated the player
    player *const foe = dynamic_cast<player*>(target);
    if ((foe == &g->u) && dist <= 2) {
        valid_attacks[valid_attack_count++] = att_radiation;
    }

    // need an open space for these attacks
    auto const empty_neighbors = find_empty_neighbors(*z);
    size_t const empty_neighbor_count = empty_neighbors.second;

    if (empty_neighbor_count) {
        if( z->ammo["bot_manhack"] > 0 ) {
            valid_attacks[valid_attack_count++] = att_manhack;
        }
        valid_attacks[valid_attack_count++] = att_acid_pool;
    }

    // flavor is always ok
    valid_attacks[valid_attack_count++] = att_flavor;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // choose and do a valid attack
    const int attack_index = get_random_index(valid_attack_count);
    switch (valid_attacks[attack_index]) {
    default :
        DebugLog(D_WARNING, D_GAME) << "Bad enum value in science.";
        break;
    case att_shock :
        z->moves -= att_cost_shock;

        // Just reuse the taze - it's a bit different (shocks torso vs all),
        // but let's go for consistency here
        taze(z, target);
        break;
    case att_radiation : {
        z->moves -= att_cost_rad;

        // if the player can see it
        if (g->u.sees(*z)) {
            // TODO: mutate() doesn't like non-players right now
            add_msg(m_bad, _("The %s opens its mouth and a beam shoots towards %s!"),
                z->name().c_str(), target->disp_name().c_str());
        }

        // (1) Give the target a chance at an uncanny_dodge.
        // (2) If that fails, always fail to dodge 1 in dodge_skill times.
        // (3) If ok, dodge if dodge_skill > att_rad_dodge_diff.
        // (4) Otherwise, fail 1 in (att_rad_dodge_diff - dodge_skill) times.
        if (foe->uncanny_dodge()) {
            break;
        }

        int const  dodge_skill  = foe->get_dodge();
        bool const critial_fail = one_in(dodge_skill);
        bool const is_trivial   = dodge_skill > att_rad_dodge_diff;

        if (!critial_fail && (is_trivial || dodge_skill > rng(0, att_rad_dodge_diff))) {
            target->add_msg_player_or_npc(_("You dodge the beam!"),
                                          _("<npcname> dodges the beam!"));
        } else if (one_in(att_rad_mutate_chance)) {
            foe->mutate();
        } else {
            foe->add_msg_if_player(m_bad, _("You get pins and needles all over."));
            foe->radiation += rng(att_rad_dose_min, att_rad_dose_max);
        }
      } break;
    case att_manhack : {
        z->moves -= att_cost_manhack;
        z->ammo["bot_manhack"]--;

        // if the player can see it
        if (g->u.sees(*z)) {
            add_msg(m_warning, _("The %s opens its coat, and a manhack flies out!"),
                z->name().c_str());
        }

        const point where = empty_neighbors.first[get_random_index(empty_neighbor_count)];

        monster manhack {GetMType("mon_manhack")};
        manhack.spawn(where.x, where.y);
        manhack.friendly = z->friendly;
        manhack.faction = z->faction;
        g->add_zombie(manhack);
      } break;
    case att_acid_pool :
        z->moves -= att_cost_acid;

        // if the player can see it
        if (g->u.sees(*z)) {
            add_msg(m_warning, _("The %s drops a flask of acid!"), z->name().c_str());
        }

        // fill empty tiles with acid
        for (size_t i = 0; i < empty_neighbor_count; ++i) {
            const point &p = empty_neighbors.first[i];
            g->m.add_field(p.x, p.y, fd_acid, att_acid_density);
        }

        break;
    case att_flavor : {
        const size_t i = get_random_index(m_flavor);

        // the special case; see above
        if (i == m_flavor.size() - 1) {
            z->moves -= att_cost_flavor;
        }

        // if the player can see it, else forget about it
        if (g->u.sees(*z)) {
            add_msg(m_warning, m_flavor[i], z->name().c_str());
        }
      } break;
    }

    z->reset_special(index); // Reset timer
}

void mattack::growplants(monster *z, int index)
{
    (void)index; //unused
    for (int i = -3; i <= 3; i++) {
        for (int j = -3; j <= 3; j++) {
            if (i == 0 && j == 0) {
                j++;
            }
            tripoint p( z->posx() + i, z->posy() + j, z->posz() );
            if (!g->m.has_flag("DIGGABLE", z->posx() + i, z->posy() + j) && one_in(4)) {
                g->m.ter_set(z->posx() + i, z->posy() + j, t_dirt);
            } else if (one_in(3) && g->m.is_bashable( p )) {
                // Destroy everything
                g->m.bash( p, 999, false, true );
                // And then make the ground fertile
                g->m.ter_set(z->posx() + i, z->posy() + j, t_dirtmound);
            } else {
                if (one_in(4)) { // 1 in 4 chance to grow a tree
                    int mondex = g->mon_at(z->posx() + i, z->posy() + j);
                    if (mondex != -1) {
                        if (g->u.sees(z->posx() + i, z->posy() + j))
                            add_msg(m_warning, _("A tree bursts forth from the earth and pierces the %s!"),
                                    g->zombie(mondex).name().c_str());
                        int rn = rng(10, 30);
                        rn -= g->zombie(mondex).get_armor_cut(bp_torso);
                        if (rn < 0) {
                            rn = 0;
                        }
                        g->zombie( mondex ).apply_damage( z, one_in( 2 ) ? bp_leg_l : bp_leg_r, rn );
                        g->zombie( mondex ).check_dead_state();
                    } else if( z->friendly == 0 && g->u.posx() == z->posx() + i && g->u.posy() == z->posy() + j) {
                        // Player is hit by a growing tree
                        if (!g->u.uncanny_dodge()) {
                            body_part hit = num_bp;
                            if (one_in(2)) {
                                hit = bp_leg_l;
                            } else {
                                hit = bp_leg_r;
                            }
                            if (one_in(4)) {
                                hit = bp_torso;
                            } else if (one_in(2)) {
                                if (one_in(2)) {
                                    hit = bp_foot_l;
                                } else {
                                    hit = bp_foot_r;
                                }
                            }
                            //~ %s is bodypart name in accusative.
                            add_msg(m_bad, _("A tree bursts forth from the earth and pierces your %s!"),
                                    body_part_name_accusative(hit).c_str());
                            g->u.deal_damage( z, hit, damage_instance( DT_STAB, rng( 10, 30 ) ) );
                            g->u.check_dead_state();
                        }
                    } else {
                        int npcdex = g->npc_at(z->posx() + i, z->posy() + j);
                        if (npcdex != -1) { // An NPC got hit
                            // TODO: combine this with the player character code above
                            body_part hit = num_bp;
                            if (one_in(2)) {
                                hit = bp_leg_l;
                            } else {
                                hit = bp_leg_r;
                            }
                            if (one_in(4)) {
                                hit = bp_torso;
                            } else if (one_in(2)) {
                                if (one_in(2)) {
                                    hit = bp_foot_l;
                                } else {
                                    hit = bp_foot_r;
                                }
                            }
                            if (g->u.sees(z->posx() + i, z->posy() + j))
                                //~ 1$s is NPC name, 2$s is bodypart name in accusative.
                                add_msg(m_warning, _("A tree bursts forth from the earth and pierces %1$s's %2$s!"),
                                        g->active_npc[npcdex]->name.c_str(),
                                        body_part_name_accusative(hit).c_str());
                            g->active_npc[npcdex]->deal_damage( z, hit, damage_instance( DT_STAB, rng( 10, 30 ) ) );
                            g->active_npc[npcdex]->check_dead_state();
                        }
                    }
                    g->m.ter_set(z->posx() + i, z->posy() + j, t_tree_young);
                } else if (one_in(3)) { // If no tree, perhaps underbrush
                    g->m.ter_set(z->posx() + i, z->posy() + j, t_underbrush);
                }
            }
        }
    }

    if (one_in(5)) { // 1 in 5 chance of making existing vegetation grow larger
        for (int i = -5; i <= 5; i++) {
            for (int j = -5; j <= 5; j++) {
                if (i != 0 || j != 0) {
                    if (g->m.ter(z->posx() + i, z->posy() + j) == t_tree_young) {
                        g->m.ter_set(z->posx() + i, z->posy() + j, t_tree);    // Young tree => tree
                    } else if (g->m.ter(z->posx() + i, z->posy() + j) == t_underbrush) {
                        // Underbrush => young tree
                        int mondex = g->mon_at(z->posx() + i, z->posy() + j);
                        if (mondex != -1) {
                            if (g->u.sees(z->posx() + i, z->posy() + j))
                                add_msg(m_warning, _("Underbrush forms into a tree, and it pierces the %s!"),
                                        g->zombie(mondex).name().c_str());
                            int rn = rng(10, 30);
                            rn -= g->zombie(mondex).get_armor_cut(bp_torso);
                            if (rn < 0) {
                                rn = 0;
                            }
                            g->zombie( mondex ).apply_damage( z, one_in( 2 ) ? bp_leg_l : bp_leg_r, rn );
                            g->zombie( mondex ).check_dead_state();
                        } else if (z->friendly == 0 && g->u.posx() == z->posx() + i && g->u.posy() == z->posy() + j) {
                            if (!g->u.uncanny_dodge()) {
                                body_part hit = num_bp;
                                if (one_in(2)) {
                                    hit = bp_leg_l;
                                } else {
                                    hit = bp_leg_r;
                                }
                                if (one_in(4)) {
                                    hit = bp_torso;
                                } else if (one_in(2)) {
                                    if (one_in(2)) {
                                        hit = bp_foot_l;
                                    } else {
                                        hit = bp_foot_r;
                                    }
                                }
                                //~ %s is bodypart name in accusative.
                                add_msg(m_bad, _("The underbrush beneath your feet grows and pierces your %s!"),
                                        body_part_name_accusative(hit).c_str());
                                g->u.deal_damage( z, hit, damage_instance( DT_STAB, rng( 10, 30 ) ) );
                                g->u.check_dead_state();
                            }
                        } else {
                            int npcdex = g->npc_at(z->posx() + i, z->posy() + j);
                            if (npcdex != -1) {
                                // TODO: combine with player character code above
                                body_part hit = num_bp;
                                if (one_in(2)) {
                                    hit = bp_leg_l;
                                } else {
                                    hit = bp_leg_r;
                                }
                                if (one_in(4)) {
                                    hit = bp_torso;
                                } else if (one_in(2)) {
                                    if (one_in(2)) {
                                        hit = bp_foot_l;
                                    } else {
                                        hit = bp_foot_r;
                                    }
                                }
                                if (g->u.sees(z->posx() + i, z->posy() + j))
                                    //~ 1$s is NPC name, 2$s is bodypart name in accusative
                                    add_msg(m_warning, _("Underbrush grows into a tree, and it pierces %1$s's %2$s!"),
                                            g->active_npc[npcdex]->name.c_str(),
                                            body_part_name_accusative(hit).c_str());
                                g->active_npc[npcdex]->deal_damage( z, hit, damage_instance( DT_STAB, rng( 10, 30 ) ) );
                                g->active_npc[npcdex]->check_dead_state();
                            }
                        }
                    }
                }
            }
        }
    }
}

void mattack::grow_vine(monster *z, int index)
{
    if( z->friendly ) {
        if( rl_dist( g->u.pos(), z->pos() ) <= 3 ) {
            // Friendly vines keep the area around you free, so you can move.
            return;
        }
    }
    z->reset_special(index); // Reset timer
    z->moves -= 100;
    int xshift = rng(0, 2), yshift = rng(0, 2);
    for (int x = 0; x < 3; x++) {
        for (int y = 0; y < 3; y++) {
            int xvine = z->posx() + (x + xshift) % 3 - 1,
                yvine = z->posy() + (y + yshift) % 3 - 1;
            if (g->is_empty(xvine, yvine)) {
                monster vine(GetMType("mon_creeper_vine"));
                vine.reset_special(0);
                vine.spawn(xvine, yvine);
                vine.faction = z->faction;
                g->add_zombie(vine);
            }
        }
    }
}

void mattack::vine(monster *z, int index)
{
    std::vector<point> grow;
    int vine_neighbors = 0;
    z->reset_special(index); // Reset timer
    z->moves -= 100;
    for (int x = z->posx() - 1; x <= z->posx() + 1; x++) {
        for (int y = z->posy() - 1; y <= z->posy() + 1; y++) {
            Creature *critter = g->critter_at( x, y );
            if( critter != nullptr && z->attitude_to( *critter ) == Creature::Attitude::A_HOSTILE ) {
                if ( critter->uncanny_dodge() ) {
                    return;
                } else {
                    player *foe = dynamic_cast< player* >( critter );
                    body_part bphit = random_body_part();
                    bool seen = g->u.sees( *critter );
                    //~ 1$s monster name(vine), 2$s bodypart in accusative
                    if( critter == &g->u ) {
                        add_msg( m_bad, _("The %1$s lashes your %2$s!"), z->name().c_str(),
                                 body_part_name_accusative(bphit).c_str() );
                    } else if( seen && foe != nullptr ) {
                        add_msg( _("The %1$s lashes %s's %2$s!"), z->name().c_str(),
                                 foe->disp_name().c_str(),
                                 body_part_name_accusative(bphit).c_str() );
                    } else if( seen ) {
                        add_msg( _("The %1$s lashes %s!"), z->name().c_str(),
                                 critter->disp_name().c_str() );
                    }
                    damage_instance d;
                    d.add_damage( DT_CUT, 4 );
                    d.add_damage( DT_BASH, 4 );
                    critter->deal_damage( z, bphit, d );
                    critter->check_dead_state();
                    z->moves -= 100;
                    return;
                }
            } else if( g->is_empty(x, y) ) {
                grow.push_back(point(x, y));
            } else {
                const int zid = g->mon_at(x, y);
                if (zid > -1 && g->zombie(zid).type->id == "mon_creeper_vine") {
                    vine_neighbors++;
                }
            }
        }
    }
    // Calculate distance from nearest hub
    int dist_from_hub = 999;
    for (size_t i = 0; i < g->num_zombies(); i++) {
        if (g->zombie(i).type->id == "mon_creeper_hub") {
            int dist = rl_dist( z->pos(), g->zombie(i).pos() );
            if (dist < dist_from_hub) {
                dist_from_hub = dist;
            }
        }
    }
    if (grow.empty() || vine_neighbors > 5 || one_in(7 - vine_neighbors) ||
        !one_in(dist_from_hub)) {
        return;
    }
    int free_index = rng(0, grow.size() - 1);
    monster vine(GetMType("mon_creeper_vine"));
    vine.reset_special(0);
    vine.spawn(grow[free_index].x, grow[free_index].y);
    vine.faction = z->faction;
    g->add_zombie(vine);
}

void mattack::spit_sap(monster *z, int index)
{
    if( !z->can_act() ) {
        return;
    }

    int t;
    int dist;
    Creature *target = z->attack_target();
    if( target == nullptr ||
        ( dist = rl_dist( z->pos(), target->pos() ) ) > 12 ||
        !z->sees( *target, t ) ) {
        return;
    }

    auto msg_type = target == &g->u ? m_bad : m_neutral;

    z->moves -= 150;
    z->reset_special(index); // Reset timer

    int deviation = rng(1, 10);
    double missed_by = (.0325 * deviation * dist);
    std::set<std::string> no_effects;

    if (missed_by > 1.) {
        if( g->u.sees( *z ) ) {
            add_msg(_("The %s spits sap, but misses %s."), z->name().c_str(), target->disp_name().c_str() );
        }

        tripoint hitp( target->posx() + rng(0 - int(missed_by), int(missed_by)),
                       target->posy() + rng(0 - int(missed_by), int(missed_by)),
                       target->posz() );
        std::vector<tripoint> line = line_to( z->pos3(), hitp, 0, 0 );
        int dam = 5;
        for( auto &i : line ) {
            g->m.shoot( i, dam, false, no_effects);
            if (dam == 0 && g->u.sees( i )) {
                add_msg(_("A glob of sap hits the %s!"),
                        g->m.tername( i ).c_str());
                return;
            }

            if (dam <= 0) {
                break;
            }
        }

        g->m.add_field( hitp, fd_sap, (dam >= 4 ? 3 : 2), 0 );
        return;
    }

    if( g->u.sees( *z ) ) {
        add_msg(_("The %s spits sap!"), z->name().c_str());
    }
    g->m.sees( z->pos(), target->pos(), 60, t);
    std::vector<tripoint> line = line_to( z->pos3(), target->pos3(), t, 0 );
    int dam = 5;
    for (auto &i : line) {
        g->m.shoot( i, dam, false, no_effects );
        if (dam == 0 && g->u.sees( i )) {
            add_msg(_("A glob of sap hits the %s!"),
                    g->m.tername( i ).c_str());
            return;
        }
    }
    if (dam <= 0) {
        return;
    }
    if( target->uncanny_dodge() ) {
        return;
    }
    if( g->u.sees( *target ) ) {
        add_msg( msg_type, _("A glob of sap hits %s!"), target->disp_name().c_str() );
    }
    target->deal_damage( z, bp_torso, damage_instance( DT_BASH, dam ) );
    target->add_effect("sap", dam);
    target->check_dead_state();
}

void mattack::triffid_heartbeat(monster *z, int index)
{
    sounds::sound(z->posx(), z->posy(), 14, _("thu-THUMP."));
    z->moves -= 300;
    z->reset_special(index); // Reset timer
    if ((z->posx() < 0 || z->posx() >= SEEX * MAPSIZE) &&
        (z->posy() < 0 || z->posy() >= SEEY * MAPSIZE)   ) {
        return;
    }
    if( z->friendly ) {
        return;
        // TODO: when friendly: open a way to the stairs, don't spawn monsters
    }
    if (rl_dist( z->posx(), g->u.pos() ) > 5 &&
        !g->m.route( g->u.posx(), g->u.posy(), z->posx(), z->posy(), 10 ).empty()) {
        add_msg(m_warning, _("The root walls creak around you."));
        for (int x = g->u.posx(); x <= z->posx() - 3; x++) {
            for (int y = g->u.posy(); y <= z->posy() - 3; y++) {
                if (g->is_empty(x, y) && one_in(4)) {
                    g->m.ter_set(x, y, t_root_wall);
                } else if (g->m.ter(x, y) == t_root_wall && one_in(10)) {
                    g->m.ter_set(x, y, t_dirt);
                }
            }
        }
        // Open blank tiles as long as there's no possible route
        int tries = 0;
        while (g->m.route( g->u.posx(), g->u.posy(), z->posx(), z->posy(), 10 ).empty() &&
               tries < 20) {
            int x = rng(g->u.posx(), z->posx() - 3), y = rng(g->u.posy(), z->posy() - 3);
            tries++;
            g->m.ter_set(x, y, t_dirt);
            if (rl_dist(x, y, g->u.posx(), g->u.posy()) > 3 && g->num_zombies() < 30 &&
                g->mon_at(x, y) == -1 && one_in(20)) { // Spawn an extra monster
                std::string montype = "mon_triffid";
                if (one_in(4)) {
                    montype = "mon_creeper_hub";
                } else if (one_in(3)) {
                    montype = "mon_biollante";
                }
                monster plant(GetMType(montype));
                plant.spawn(x, y);
                plant.faction = z->faction;
                g->add_zombie(plant);
            }
        }

    } else { // The player is close enough for a fight!

        monster triffid(GetMType("mon_triffid"));
        for (int x = z->posx() - 1; x <= z->posx() + 1; x++) {
            for (int y = z->posy() - 1; y <= z->posy() + 1; y++) {
                if (g->is_empty(x, y) && one_in(2)) {
                    triffid.spawn(x, y);
                    triffid.faction = z->faction;
                    g->add_zombie(triffid);
                }
            }
        }

    }
}

void mattack::fungus(monster *z, int index)
{
    // TODO: Infect NPCs?
    z->moves -= 200;   // It takes a while
    z->reset_special(index); // Reset timer
    if (g->u.has_trait("THRESH_MYCUS")) {
        z->friendly = 1;
    }
    monster spore(GetMType("mon_spore"));
    int sporex, sporey;
    int mondex;
    //~ the sound of a fungus releasing spores
    sounds::sound(z->posx(), z->posy(), 10, _("Pouf!"));
    if (g->u.sees( *z )) {
        add_msg(m_warning, _("Spores are released from the %s!"), z->name().c_str());
    }
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            if (i == 0 && j == 0) {
                continue;
            }
            sporex = z->posx() + i;
            sporey = z->posy() + j;
            mondex = g->mon_at(sporex, sporey);
            if (g->m.move_cost(sporex, sporey) > 0) {
                if (mondex != -1) { // Spores hit a monster
                    if (g->u.sees(sporex, sporey) &&
                        !g->zombie(mondex).type->in_species("FUNGUS")) {
                        add_msg(_("The %s is covered in tiny spores!"),
                                g->zombie(mondex).name().c_str());
                    }
                    monster &critter = g->zombie( mondex );
                    if( !critter.make_fungus() ) {
                        critter.die( z ); // counts as kill by monster z
                    }
                } else if (g->u.posx() == sporex && g->u.posy() == sporey) {
                    // Spores hit the player--is there any hope?
                    if (g->u.has_trait("TAIL_CATTLE") && one_in(20 - g->u.dex_cur - g->u.skillLevel("melee"))) {
                        add_msg(_("The spores land on you, but you quickly swat them off with your tail!"));
                        return;
                    }
                    bool hit = false;
                    if (one_in(4) && g->u.add_env_effect("spores", bp_head, 3, 90, bp_head)) {
                        hit = true;
                    }
                    if (one_in(2) && g->u.add_env_effect("spores", bp_torso, 3, 90, bp_torso)) {
                        hit = true;
                    }
                    if (one_in(4) && g->u.add_env_effect("spores", bp_arm_l, 3, 90, bp_arm_l)) {
                        hit = true;
                    }
                    if (one_in(4) && g->u.add_env_effect("spores", bp_arm_r, 3, 90, bp_arm_r)) {
                        hit = true;
                    }
                    if (one_in(4) && g->u.add_env_effect("spores", bp_leg_l, 3, 90, bp_leg_l)) {
                        hit = true;
                    }
                    if (one_in(4) && g->u.add_env_effect("spores", bp_leg_r, 3, 90, bp_leg_r)) {
                        hit = true;
                    }
                    if ((hit) && (g->u.has_trait("TAIL_CATTLE") &&
                                  one_in(20 - g->u.dex_cur - g->u.skillLevel("melee")))) {
                        add_msg(_("The spores land on you, but you quickly swat them off with your tail!"));
                        hit = false;
                    }
                    if (hit) {
                        add_msg(m_warning, _("You're covered in tiny spores!"));
                    }
                } else if (one_in(4) && g->num_zombies() <= 1000) { // Spawn a spore
                    spore.spawn(sporex, sporey);
                    spore.faction = z->faction;
                    g->add_zombie(spore);
                }
            }
        }
    }
}

void mattack::fungus_haze(monster *z, int index)
{
    z->reset_special(index); // Reset timer
    //~ That spore sound again
    sounds::sound(z->posx(), z->posy(), 10, _("Pouf!"));
    if (g->u.sees( *z )) {
        add_msg(m_info, _("The %s pulses, and fresh fungal material bursts forth."), z->name().c_str());
    }
    z->moves -= 150;
    for (int i = z->posx() - 3; i <= z->posx() + 3; i++) {
        for (int j = z->posy() - 3; j <= z->posy() + 3; j++) {
                g->m.add_field( i, j, fd_fungal_haze, rng(1, 2));
        }
    }
}

void mattack::fungus_big_blossom(monster *z, int index)
{
    z->reset_special(index); // Reset timer
    bool firealarm = false;
    const auto u_see = g->u.sees( *z );
    int monx = z->posx();
    int mony = z->posy();
    // Fungal fire-suppressor! >:D
    for (int i = monx - 6; i <= monx + 6; i++) {
        for (int j = mony - 6; j <= mony + 6; j++) {
            if (g->m.get_field_strength(point (i, j), fd_fire) != 0) {
                firealarm = true;
            }
            if (firealarm) {
                g->m.remove_field( i, j, fd_fire );
                g->m.remove_field( i, j, fd_smoke );
                g->m.add_field(i, j, fd_fungal_haze, 3);
            }
        }
    }
    // Special effects handled outside the loop
    if (firealarm){
        if( u_see ) {
            // Sucks up all the smoke
            add_msg(m_warning, _("The %s suddenly inhales!"), z->name().c_str());
        }
        //~Sound of a giant fungal blossom inhaling
        sounds::sound(monx, mony, 20, _("WOOOSH!"));
        if( u_see ) {
            add_msg(m_bad, _("The %s discharges an immense flow of spores, smothering the flames!"), z->name().c_str());
        }
        //~Sound of a giant fungal blossom blowing out the dangerous fire!
        sounds::sound(monx, mony, 20, _("POUFF!"));
        return;
    }
    // No fire detected, routine haze-emission
    if (!firealarm) {
        //~ That spore sound, much louder
        sounds::sound(monx, mony, 15, _("POUF."));
        if( u_see ) {
            add_msg(m_info, _("The %s pulses, and fresh fungal material bursts forth!"), z->name().c_str());
        }
        z->moves -= 150;
        for (int i = monx - 12; i <= monx + 12; i++) {
            for (int j = mony - 12; j <= mony + 12; j++) {
                g->m.add_field( i, j, fd_fungal_haze, rng(1, 2));
            }
        }
    }
}

void mattack::fungus_inject(monster *z, int index)
{
    Creature *target = &g->u; // For faster copy+paste
    if (rl_dist( z->pos(), g->u.pos() ) > 1) {
        return;
    }

    z->reset_special(index); // Reset timer
    if (g->u.has_trait("THRESH_MARLOSS") || g->u.has_trait("THRESH_MYCUS")) {
        z->friendly = 1;
        return;
    }
    if ( (g->u.has_trait("MARLOSS")) && (g->u.has_trait("MARLOSS_BLUE")) && !g->u.crossed_threshold()) {
        add_msg(m_info, _("The %s seems to wave you toward the tower..."), z->name().c_str());
        z->anger = 0;
        return;
    }
    if( z->friendly ) {
        // TODO: attack other creatures, not just g->u, for now just skip the code below as it
        // only attacks g->u but the monster is friendly.
        return;
    }
    add_msg(m_warning, _("The %s jabs at you with a needlelike point!"), z->name().c_str());
    z->moves -= 150;

    if (g->u.uncanny_dodge()) {
        return;
    }

    // Can we dodge the attack? Uses player dodge function % chance (melee.cpp)
    if (dodge_check(z, target)) {
        target->add_msg_player_or_npc( _("You dodge it!"),
                                       _("<npcname> dodges it!") );
        g->u.practice( "dodge", z->type->melee_skill * 2 );
        g->u.ma_ondodge_effects();
        return;
    }

    body_part hit = random_body_part();
    int dam = rng(5, 11);
    dam = g->u.deal_damage( z, hit, damage_instance( DT_CUT, dam ) ).total_damage();

    if (dam > 0) {
        //~ 1$s is monster name, 2$s bodypart in accusative
        add_msg(m_bad, _("The %1$s sinks its point into your %2$s!"), z->name().c_str(),
                body_part_name_accusative(hit).c_str());

        if(one_in(10 - dam)) {
            g->u.add_effect("fungus", 100, num_bp, true);
            add_msg(m_warning, _("You feel thousands of live spores pumping into you..."));
        }
    } else {
        //~ 1$s is monster name, 2$s bodypart in accusative
        add_msg(_("The %1$s strikes your %2$s, but your armor protects you."), z->name().c_str(),
                body_part_name_accusative(hit).c_str());
    }

    g->u.practice( "dodge", z->type->melee_skill );
    g->u.check_dead_state();
}
void mattack::fungus_bristle(monster *z, int index)
{
    if (g->u.has_trait("THRESH_MARLOSS") || g->u.has_trait("THRESH_MYCUS")) {
        z->friendly = 1;
    }
    Creature *target = z->attack_target();
    if( target == nullptr ||
        rl_dist( z->pos(), target->pos() ) > 1 ||
        !z->sees( *target ) ) {
        return;
    }

    player *foe = dynamic_cast< player* >( target );
    auto msg_type = target == &g->u ? m_warning : m_neutral;
    z->reset_special(index); // Reset timer

    add_msg( msg_type, _("The %s swipes at %s with a barbed tendril!"), z->name().c_str(), target->disp_name().c_str() );
    z->moves -= 150;

    if( target->uncanny_dodge() ) {
        return;
    }

    // Can we dodge the attack? Uses player dodge function % chance (melee.cpp)
    if (dodge_check(z, target)) {
        target->add_msg_player_or_npc( _("You dodge it!"),
                                       _("<npcname> dodges it!") );
        if( foe != nullptr )  {
            foe->practice( "dodge", z->type->melee_skill * 2 );
            foe->ma_ondodge_effects();
        }
        return;
    }

    body_part hit = random_body_part();
    int dam = rng(7, 16);
    dam = target->deal_damage( z, hit, damage_instance( DT_CUT, dam ) ).total_damage();

    if( foe != nullptr && dam > 0 ) {
        //~ 1$s is monster name, 2$s bodypart in accusative
        foe->add_msg_if_player( m_bad, _("The %1$s sinks several needlelike barbs into your %2$s!"), z->name().c_str(),
                                body_part_name_accusative(hit).c_str());

        if(one_in(15 - dam)) {
            foe->add_effect("fungus", 200, num_bp, true);
            foe->add_msg_if_player(m_warning, _("You feel thousands of live spores pumping into you..."));
        }
        foe->practice( "dodge", z->type->melee_skill );
    } else if( foe != nullptr ) {
        //~ 1$s is monster name, 2$s bodypart in accusative
        foe->add_msg_if_player( _("The %1$s slashes your %2$s, but your armor protects you."), z->name().c_str(),
                                body_part_name_accusative(hit).c_str());
        foe->practice( "dodge", z->type->melee_skill );
    }
    target->check_dead_state();
}

void mattack::fungus_growth(monster *z, int index)
{
    (void)index; //unused
    // Young fungaloid growing into an adult
    if (g->u.sees( *z )) {
        add_msg(m_warning, _("The %s grows into an adult!"),
                z->name().c_str());
    }
    z->poly(GetMType("mon_fungaloid"));
}

void mattack::fungus_sprout(monster *z, int index)
{
    z->reset_special(index); // Reset timer
    for (int x = z->posx() - 1; x <= z->posx() + 1; x++) {
        for (int y = z->posy() - 1; y <= z->posy() + 1; y++) {
            if (g->u.posx() == x && g->u.posy() == y) {
                add_msg(m_bad, _("You're shoved away as a fungal wall grows!"));
                g->fling_creature( &g->u, g->m.coord_to_angle(z->posx(), z->posy(), g->u.posx(),
                                   g->u.posy()), rng(10, 50));
            }
            if (g->is_empty(x, y)) {
                monster wall(GetMType("mon_fungal_wall"));
                wall.spawn(x, y);
                wall.faction = z->faction;
                g->add_zombie(wall);
            }
        }
    }
}

void mattack::fungus_fortify(monster *z, int index)
{
    if( z->friendly ) {
        return; // TODO: handle friendly monsters
    }
    Creature *target = &g->u;
    bool mycus = false;
    bool peaceful = true;
    if (g->u.has_trait("THRESH_MARLOSS") || g->u.has_trait("THRESH_MYCUS")) {
        mycus = true; //No nifty support effects.  Yet.  This lets it rebuild hedges.
    }
    if ( (g->u.has_trait("MARLOSS")) && (g->u.has_trait("MARLOSS_BLUE")) &&
         !g->u.crossed_threshold() && !mycus) {
        // You have the other two.  Is it really necessary for us to fight?
        add_msg(m_info, _("The %s spreads its tendrils.  It seems as though it's expecting you..."), z->name().c_str());
        if (rl_dist( z->pos(), g->u.pos() ) < 3) {
            if (query_yn(_("The tower extends and aims several tendrils from its depths.  Hold still?"))) {
                add_msg(m_warning, _("The %s works several tendrils into your arms, legs, torso, and even neck..."), z->name().c_str());
                g->u.hurtall(1, z);
                add_msg(m_warning, _("You see a clear golden liquid pump through the tendrils--and then lose consciousness."));
                g->u.toggle_mutation("MARLOSS");
                g->u.toggle_mutation("MARLOSS_BLUE");
                g->u.toggle_mutation("THRESH_MARLOSS");
                g->m.ter_set(g->u.posx(), g->u.posy(), t_marloss); // We only show you the door.  You walk through it on your own.
                g->u.add_memorial_log(pgettext("memorial_male", "Was shown to the Marloss Gatweay."),
                    pgettext("memorial_female", "Was shown to the Marloss Gateway."));
                g->u.add_msg_if_player(m_good, _("You wake up in a marloss bush.  Almost *cradled* in it, actually, as though it grew there for you."));
                //~ Beginning to hear the Mycus while conscious: this is it speaking
                g->u.add_msg_if_player(m_good, _("assistance, on an arduous quest. unity. together we have reached the door. now to pass through..."));
                z->reset_special(index); // Reset timer
                return;
            } else {
                peaceful = false; // You declined the offer.  Fight!
            }
        }
    } else {
        peaceful = false; // You weren't eligible.  Fight!
    }

    bool fortified = false;
    z->reset_special(index); // Reset timer
    for (int x = z->posx() - 1; x <= z->posx() + 1; x++) {
        for (int y = z->posy() - 1; y <= z->posy() + 1; y++) {
            if (g->u.posx() == x && g->u.posy() == y) {
                add_msg(m_bad, _("You're shoved away as a fungal hedgerow grows!"));
                g->fling_creature( &g->u, g->m.coord_to_angle(z->posx(), z->posy(), g->u.posx(),
                                   g->u.posy()), rng(10, 50));
            }
            if (g->is_empty(x, y)) {
                monster wall(GetMType("mon_fungal_hedgerow"));
                wall.spawn(x, y);
                wall.faction = z->faction;
                g->add_zombie(wall);
                fortified = true;
            }
        }
    }
    if( !fortified && !(mycus || peaceful) ) {
        if (rl_dist( z->pos(), g->u.pos() ) < 12) {
            if (rl_dist( z->pos(), g->u.pos() ) > 3) {
                // Oops, can't reach.  ):
                // How's about we spawn more tendrils? :)
                // Aimed at the player, too?  Sure!
                int i = rng(-1, 1);
                int j = rng(-1, 1);
                if ((i == 0) && (j == 0)) { // Direct hit! :D
                    if (!g->u.uncanny_dodge()) {
                        body_part hit = num_bp;
                        if (one_in(2)) {
                           hit = bp_leg_l;
                        } else {
                            hit = bp_leg_r;
                        }
                        if (one_in(4)) {
                            hit = bp_torso;
                        } else if (one_in(2)) {
                            if (one_in(2)) {
                                hit = bp_foot_l;
                            } else {
                                hit = bp_foot_r;
                            }
                        }
                        //~ %s is bodypart name in accusative.
                        add_msg(m_bad, _("A fungal tendril bursts forth from the earth and pierces your %s!"),
                                body_part_name_accusative(hit).c_str());
                        g->u.deal_damage( z, hit, damage_instance( DT_CUT, rng( 5, 11 ) ) );
                        g->u.check_dead_state();
                        // Probably doesn't have spores available *just* yet.  Let's be nice.
                        } else {
                            add_msg(m_bad, _("A fungal tendril bursts forth from the earth!"));
                        }
                }
                monster tendril(GetMType("mon_fungal_tendril"));
                tendril.spawn(g->u.posx() + i, g->u.posy() + j);
                tendril.faction = z->faction;
                g->add_zombie(tendril);
                return;
            }
            add_msg(m_warning, _("The %s takes aim, and spears at you with a massive tendril!"), z->name().c_str());
            z->moves -= 150;

            if (g->u.uncanny_dodge()) {
                return;
            }
            // Can we dodge the attack? Uses player dodge function % chance (melee.cpp)
            if (dodge_check(z, target)) {
                target->add_msg_player_or_npc( _("You dodge it!"),
                                               _("<npcname> dodges it!") );
                g->u.practice( "dodge", z->type->melee_skill * 2 );
                g->u.ma_ondodge_effects();
                return;
            }

            body_part hit = random_body_part();
            int dam = rng(15, 21);
            dam = g->u.deal_damage( z, hit, damage_instance( DT_STAB, dam ) ).total_damage();

            if (dam > 0) {
                //~ 1$s is monster name, 2$s bodypart in accusative
                add_msg(m_bad, _("The %1$s sinks its point into your %2$s!"), z->name().c_str(),
                    body_part_name_accusative(hit).c_str());
                g->u.add_effect("fungus", 400, num_bp, true);
                add_msg(m_warning, _("You feel millions of live spores pumping into you..."));
                } else {
                    //~ 1$s is monster name, 2$s bodypart in accusative
                    add_msg(_("The %1$s strikes your %2$s, but your armor protects you."), z->name().c_str(),
                            body_part_name_accusative(hit).c_str());
                }

            g->u.practice( "dodge", z->type->melee_skill );
            g->u.check_dead_state();
        }
    }
}

void mattack::leap(monster *z, int index)
{
    if( !z->can_act() ) {
        return;
    }

    int linet = 0;
    std::vector<point> options;
    tripoint target = z->move_target();
    int best = rl_dist( z->pos3(), target );

    for (int x = z->posx() - 3; x <= z->posx() + 3; x++) {
        for (int y = z->posy() - 3; y <= z->posy() + 3; y++) {
            if (x == z->posx() && y == z->posy()) {
                continue;
            }
            if( !z->sees( x, y, linet ) ) {
                continue;
            }
            if (!g->is_empty(x, y)) {
                continue;
            }
            if (rl_dist(target.x, target.y, x, y) > best) {
                continue;
            }
            bool blocked_path = false;
            // check if monster has a clear path to the proposed point
            std::vector<point> line = line_to(z->posx(), z->posy(), x, y, linet);
            for (auto &i : line) {
                if (g->m.move_cost(i.x, i.y) == 0) {
                    blocked_path = true;
                    break;
                }
            }
            if (!blocked_path) {
                options.push_back( point(x, y) );
                best = rl_dist(target.x, target.y, x, y);
            }

        }
    }

    // Go back and remove all options that aren't tied for best
    for (size_t i = 0; i < options.size() && options.size() > 1; i++) {
        point p = options[i];
        if (rl_dist( target.x, target.y, options[i].x, options[i].y ) != best) {
            options.erase(options.begin() + i);
            i--;
        }
    }

    if (options.empty()) {
        return;    // Nowhere to leap!
    }

    z->moves -= 150;
    z->reset_special(index); // Reset timer
    point chosen = options[rng(0, options.size() - 1)];
    bool seen = g->u.sees(*z); // We can see them jump...
    z->setpos(chosen);
    seen |= g->u.sees(*z); // ... or we can see them land
    if (seen) {
        add_msg(_("The %s leaps!"), z->name().c_str());
    }
}

void mattack::dermatik(monster *z, int index)
{
    if( !z->can_act() ) {
        return;
    }

    Creature *target = z->attack_target();
    if( target == nullptr ||
        rl_dist( z->pos(), target->pos() ) > 1 ||
        !z->sees( *target ) ) {
        return;
    }

    z->reset_special(index); // Reset timer
    if( target->uncanny_dodge() ) {
        return;
    }
    player *foe = dynamic_cast< player* >( target );
    if( foe == nullptr ) {
        return; // No implanting monsters for now
    }
    // Can we dodge the attack? Uses player dodge function % chance (melee.cpp)
    if (dodge_check(z, target)) {
        if( target == &g->u ) {
            add_msg(_("The %s tries to land on you, but you dodge."), z->name().c_str());
        }
        z->stumble(false);
        if( foe != nullptr ) {
            foe->practice( "dodge", z->type->melee_skill * 2 );
            foe->ma_ondodge_effects();
        }
        return;
    }

    // Can we swat the bug away?
    int dodge_roll = z->dodge_roll();
    int swat_skill = ( foe->skillLevel("melee") + foe->skillLevel("unarmed") * 2) / 3;
    int player_swat = dice(swat_skill, 10);
    if( foe->has_trait("TAIL_CATTLE") ) {
        foe->add_msg_if_player(_("You swat at the %s with your tail!"), z->name().c_str());
        player_swat += ( ( foe->dex_cur + foe->skillLevel("unarmed") ) / 2 );
    }
    if( player_swat > dodge_roll ) {
        foe->add_msg_if_player(_("The %s lands on you, but you swat it off."), z->name().c_str());
        if (z->get_hp() >= z->get_hp_max() / 2) {
            z->apply_damage( &g->u, bp_torso, 1 );
            z->check_dead_state();
        }
        if (player_swat > dodge_roll * 1.5) {
            z->stumble(false);
        }
        return;
    }

    // Can the bug penetrate our armor?
    body_part targeted = random_body_part();
    if (4 < g->u.get_armor_cut(targeted) / 3) {
        //~ 1$s monster name(dermatic), 2$s bodypart name in accusative.
        foe->add_msg_if_player( _("The %1$s lands on your %2$s, but can't penetrate your armor."),
                                z->name().c_str(), body_part_name_accusative(targeted).c_str());
        z->moves -= 150; // Attempted laying takes a while
        return;
    }

    // Success!
    z->moves -= 500; // Successful laying takes a long time
    //~ 1$s monster name(dermatic), 2$s bodypart name in accusative.
    foe->add_msg_if_player( m_bad, _("The %1$s sinks its ovipositor into your %2$s!"), z->name().c_str(),
                            body_part_name_accusative(targeted).c_str());
    if ( !foe->has_trait("PARAIMMUNE") || !foe->has_trait("ACIDBLOOD") ) {
        foe->add_effect("dermatik", 1, targeted, true);
        foe->add_memorial_log(pgettext("memorial_male", "Injected with dermatik eggs."),
                              pgettext("memorial_female", "Injected with dermatik eggs."));
    }
}

void mattack::dermatik_growth(monster *z, int index)
{
    (void)index; //unused
    // Dermatik larva growing into an adult
    if (g->u.sees( *z )) {
        add_msg(m_warning, _("The %s dermatik larva grows into an adult!"),
                z->name().c_str());
    }
    z->poly(GetMType("mon_dermatik"));
}

void mattack::plant(monster *z, int index)
{
    (void)index; //unused
    // Spores taking seed and growing into a fungaloid
    if (!g->spread_fungus( z->pos3() ) && one_in(20)) {
        if (g->u.sees( *z )) {
            add_msg(m_warning, _("The %s takes seed and becomes a young fungaloid!"),
                    z->name().c_str());
        }
        z->poly(GetMType("mon_fungaloid_young"));
        z->moves -= 1000; // It takes a while
    } else {
        if (g->u.sees( *z )) {
            add_msg(_("The %s falls to the ground and bursts!"),
                    z->name().c_str());
        }
        z->set_hp( 0 );
    }
}

void mattack::disappear(monster *z, int index)
{
    (void)index; //unused
    z->set_hp( 0 );
}

void mattack::formblob(monster *z, int index)
{
    if( z->friendly ) {
        return; // TODO: handle friendly monsters
    }
    bool didit = false;
    int thatmon = -1;
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            thatmon = g->mon_at(z->posx() + i, z->posy() + j);
            if (g->u.posx() == z->posx() + i && g->u.posy() == z->posy() + i) {
                // If we hit the player, cover them with slime
                didit = true;
                g->u.add_effect("slimed", rng(0, z->get_hp()));
            } else if (thatmon != -1) {
                monster &othermon = g->zombie(thatmon);
                // Hit a monster.  If it's a blob, give it our speed.  Otherwise, blobify it?
                if( z->get_speed_base() > 40 && othermon.type->in_species( "BLOB" ) ) {
                    if( othermon.type->id == "mon_blob_brain" ) {
                        // Brain blobs don't get sped up, they heal at the cost of the other blob.
                        // But only if they are hurt badly.
                        if( othermon.get_hp() < othermon.get_hp_max() / 2 ) {
                            didit = true;
                            othermon.heal( z->get_speed_base() );
                            z->set_hp( 0 );
                            return;
                        }
                        continue;
                    }
                    didit = true;
                    othermon.set_speed_base( othermon.get_speed_base() + 5 );
                    z->set_speed_base( z->get_speed_base() - 5 );
                    if (othermon.type->id == "mon_blob_small" && othermon.get_speed_base() >= 60) {
                        othermon.poly(GetMType("mon_blob"));
                    } else if ( othermon.type->id == "mon_blob" && othermon.get_speed_base() >= 80) {
                        othermon.poly(GetMType("mon_blob_large"));
                    }
                } else if( (othermon.made_of("flesh") ||
                            othermon.made_of("veggy") ||
                            othermon.made_of("iflesh") ) &&
                           rng(0, z->get_hp()) > rng(0, othermon.get_hp())) { // Blobify!
                    didit = true;
                    othermon.poly(GetMType("mon_blob"));
                    othermon.set_speed_base( othermon.get_speed_base() - rng(5, 25) );
                    othermon.set_hp( othermon.get_speed_base() );
                }
            } else if (z->get_speed_base() >= 85 && rng(0, 250) < z->get_speed_base()) {
                // If we're big enough, spawn a baby blob.
                didit = true;
                z->mod_speed_bonus( -15 );
                monster blob(GetMType("mon_blob_small"));
                blob.spawn(z->posx() + i, z->posy() + j);
                blob.set_speed_base( blob.get_speed_base() - rng(30, 60) );
                blob.set_hp( blob.get_speed_base() );
                blob.faction = z->faction;
                g->add_zombie(blob);
            }
        }
        if (didit) { // We did SOMEthing.
            if (z->type->id == "mon_blob" && z->get_speed_base() <= 50) { // We shrank!
                z->poly(GetMType("mon_blob_small"));
            } else if (z->type->id == "mon_blob_large" && z->get_speed_base() <= 70) { // We shrank!
                z->poly(GetMType("mon_blob"));
            }

            z->moves = 0;
            z->reset_special(index); // Reset timer
            return;
        }
    }
}

void mattack::callblobs(monster *z, int index)
{
    if( z->friendly ) {
        return; // TODO: handle friendly monsters
    }
    // The huge brain blob interposes other blobs between it and any threat.
    // For the moment just target the player, this gets a bit more complicated
    // if we want to deal with NPCS and friendly monsters as well.
    // The strategy is to send about 1/3 of the available blobs after the player,
    // and keep the rest near the brain blob for protection.
    point enemy( g->u.posx(), g->u.posy() );
    std::list<monster *> allies;
    std::vector<point> nearby_points = closest_points_first( 3, z->pos() );
    // Iterate using horrible creature_tracker API.
    for( size_t i = 0; i < g->num_zombies(); i++ ) {
        monster *candidate = &g->zombie( i );
        if( candidate->type->in_species("BLOB") && candidate->type->id != "mon_blob_brain" ) {
            // Just give the allies consistent assignments.
            // Don't worry about trying to make the orders optimal.
            allies.push_back( candidate );
        }
    }
    // 1/3 of the available blobs, unless they would fill the entire area near the brain.
    const int num_guards = std::min( allies.size() / 3, nearby_points.size() );
    int guards = 0;
    for( std::list<monster *>::iterator ally = allies.begin();
         ally != allies.end(); ++ally, ++guards ) {
        point post = enemy;
        if( guards < num_guards ) {
            // Each guard is assigned a spot in the nearby_points vector based on their order.
            int assigned_spot = (nearby_points.size() * guards) / num_guards;
            post = nearby_points[ assigned_spot ];
        }
        int trash = 0;
        (*ally)->set_dest( tripoint( post.x, post.y, z->posz() ), trash );
        if (!(*ally)->has_effect("controlled")) {
            (*ally)->add_effect("controlled", 1, num_bp, true);
        }
    }
    // This is telepathy, doesn't take any moves.
    z->reset_special(index); // Reset timer
}

void mattack::jackson(monster *z, int index)
{
    // Jackson draws nearby zombies into the dance.
    std::list<monster *> allies;
    std::vector<point> nearby_points = closest_points_first( 3, z->pos() );
    // Iterate using horrible creature_tracker API.
    for( size_t i = 0; i < g->num_zombies(); i++ ) {
        monster *candidate = &g->zombie( i );
        if(candidate->type->in_species("ZOMBIE") && candidate->type->id != "mon_zombie_jackson") {
            // Just give the allies consistent assignments.
            // Don't worry about trying to make the orders optimal.
            allies.push_back( candidate );
        }
    }
    const int num_dancers = std::min( allies.size(), nearby_points.size() );
    int dancers = 0;
    bool converted = false;
    for( auto ally = allies.begin(); ally != allies.end(); ++ally, ++dancers ) {
        point post = z->pos();
        if( dancers < num_dancers ) {
            // Each dancer is assigned a spot in the nearby_points vector based on their order.
            int assigned_spot = (nearby_points.size() * dancers) / num_dancers;
            post = nearby_points[ assigned_spot ];
        }
        if ((*ally)->type->id != "mon_zombie_dancer") {
            (*ally)->poly(GetMType("mon_zombie_dancer"));
            converted = true;
        }
        int trash = 0;
        (*ally)->set_dest( tripoint( post.x, post.y, z->posz() ), trash );
        if (!(*ally)->has_effect("controlled")) {
            (*ally)->add_effect("controlled", 1, num_bp, true);
        }
    }
    // Did we convert anybody?
    if (converted) {
        if (g->u.sees( *z )) {
            add_msg(m_warning, _("The %s lets out a high-pitched cry!"), z->name().c_str());
        }
    }
    // This is telepathy, doesn't take any moves.
    z->reset_special(index); // Reset timer
}


void mattack::dance(monster *z, int index)
{
    if (g->u.sees( *z )) {
        switch (rng(1,10)) {
            case 1:
                add_msg(m_neutral, _("The %s swings its arms from side to side!"), z->name().c_str());
                break;
            case 2:
                add_msg(m_neutral, _("The %s does some fancy footwork!"), z->name().c_str());
                break;
            case 3:
                add_msg(m_neutral, _("The %s shrugs its shoulders!"), z->name().c_str());
                break;
            case 4:
                add_msg(m_neutral, _("The %s spins in place!"), z->name().c_str());
                break;
            case 5:
                add_msg(m_neutral, _("The %s crouches on the ground!"), z->name().c_str());
                break;
            case 6:
                add_msg(m_neutral, _("The %s looks left and right!"), z->name().c_str());
                break;
            case 7:
                add_msg(m_neutral, _("The %s jumps back and forth!"), z->name().c_str());
                break;
            case 8:
                add_msg(m_neutral, _("The %s raises its arms in the air!"), z->name().c_str());
                break;
            case 9:
                add_msg(m_neutral, _("The %s swings its hips!"), z->name().c_str());
                break;
            case 10:
                add_msg(m_neutral, _("The %s claps!"), z->name().c_str());
                break;
        }
    }
    z->reset_special(index); // Reset timer
}

void mattack::dogthing(monster *z, int index)
{
    (void)index; //unused
    if (!one_in(3) || !g->u.sees(*z)) {
        return;
    }

    add_msg(_("The %s's head explodes in a mass of roiling tentacles!"),
            z->name().c_str());

    for (int x = z->posx() - 2; x <= z->posx() + 2; x++) {
        for (int y = z->posy() - 2; y <= z->posy() + 2; y++) {
            if (rng(0, 2) >= rl_dist(z->posx(), z->posy(), x, y)) {
                g->m.add_field(x, y, fd_blood, 2);
            }
        }
    }

    z->friendly = 0;
    z->poly(GetMType("mon_headless_dog_thing"));
}

void mattack::tentacle(monster *z, int index)
{
    if( z->friendly ) {
        return; // TODO: handle friendly monsters
    }
    Creature *target = &g->u;
    int t;
    if (!z->sees( g->u, t )) {
        return;
    }
    add_msg(m_bad, _("The %s lashes its tentacle at you!"), z->name().c_str());
    z->moves -= 100;
    z->reset_special(index); // Reset timer

    std::vector<tripoint> line = line_to( z->pos3(), g->u.pos3(), t, 0 );
    std::set<std::string> no_effects;
    for (auto &i : line) {
        int tmpdam = 20;
        g->m.shoot( i, tmpdam, true, no_effects );
    }

    if (g->u.uncanny_dodge()) {
        return;
    }
    // Can we dodge the attack? Uses player dodge function % chance (melee.cpp)
    if (dodge_check(z, target)) {
        target->add_msg_player_or_npc( _("You dodge it!"),
                                       _("<npcname> dodges it!") );
        g->u.practice( "dodge", z->type->melee_skill * 2 );
        g->u.ma_ondodge_effects();
        return;
    }

    body_part hit = random_body_part();
    int dam = rng(10, 20);
    //~ 1$s is bodypart name, 2$d is damage value.
    add_msg(m_bad, _("Your %1$s is hit for %2$d damage!"), body_part_name(hit).c_str(), dam);
    g->u.deal_damage( z, hit, damage_instance( DT_BASH, dam ) );
    g->u.practice( "dodge", z->type->melee_skill );
    g->u.check_dead_state();
}

void mattack::vortex(monster *z, int index)
{
    // Make sure that the player's butchering is interrupted!
    if (g->u.activity.type == ACT_BUTCHER &&
        rl_dist( z->pos(), g->u.pos() ) <= 2) {
        add_msg(m_warning, _("The buffeting winds interrupt your butchering!"));
        g->u.activity.type = ACT_NULL;
    }
    // Moves are NOT used up by this attack, as it is "passive"
    z->reset_special(index); // Reset timer
    // Before anything else, smash terrain!
    for (int x = z->posx() - 2; x <= z->posx() + 2; x++) {
        for (int y = z->posx() - 2; y <= z->posy() + 2; y++) {
            if (x == z->posx() && y == z->posy()) { // Don't throw us!
                y++;
            }
            g->m.bash( tripoint( x, y, z->posz() ), 14 );
        }
    }
    std::set<std::string> no_effects;

    for (int x = z->posx() - 2; x <= z->posx() + 2; x++) {
        for (int y = z->posx() - 2; y <= z->posy() + 2; y++) {
            if (x == z->posx() && y == z->posy()) { // Don't throw us!
                y++;
            }
            // TODO: Z
            std::vector<tripoint> from_monster = line_to( z->pos3(), tripoint( x, y, z->posz() ), 0, 0 );
            while (!g->m.i_at(x, y).empty()) {
                item thrown = g->m.i_at(x, y)[index];
                g->m.i_rem(x, y, 0);
                int distance = 5 - (thrown.weight() / 1700);
                if (distance > 0) {
                    int dam = (thrown.weight() / 113) / double(3 + double(thrown.volume() / 6));
                    std::vector<tripoint> traj = continue_line( from_monster, distance );
                    for (size_t i = 0; i < traj.size() && dam > 0; i++) {
                        g->m.shoot( traj[i], dam, false, no_effects );
                        int mondex = g->mon_at( traj[i] );
                        if (mondex != -1) {
                            g->zombie( mondex ).apply_damage( z, random_body_part(), dam );
                            g->zombie( mondex ).check_dead_state();
                            dam = 0;
                        }
                        if (g->m.move_cost( traj[i] ) == 0) {
                            dam = 0;
                            i--;
                        } else if( traj[i] == g->u.pos3() ) {
                            if (! g->u.uncanny_dodge()) {
                                body_part hit = random_body_part();
                                //~ 1$s is item name, 2$s is bodypart in accusative, 3$d is damage value.
                                add_msg(m_bad, _("A %1$s hits your %2$s for %3$d damage!"), thrown.tname().c_str(),
                                        body_part_name_accusative(hit).c_str(), dam);
                                g->u.deal_damage( z, hit, damage_instance( DT_BASH, dam ) );
                                g->u.check_dead_state();
                                dam = 0;
                            }
                        }
                        // TODO: Hit NPCs
                        if (dam == 0 || i == traj.size() - 1) {
                            if (thrown.made_of("glass")) {
                                if (g->u.sees( traj[i] )) {
                                    add_msg(m_warning, _("The %s shatters!"), thrown.tname().c_str());
                                }
                                for (auto &n : thrown.contents) {
                                    g->m.add_item_or_charges(traj[i].x, traj[i].y, n);
                                }
                                sounds::sound(traj[i].x, traj[i].y, 16, _("glass breaking!"));
                            } else {
                                g->m.add_item_or_charges(traj[i].x, traj[i].y, thrown);
                            }
                        }
                    }
                } // Done throwing item
            } // Done getting items
            // Throw monsters
            int mondex = g->mon_at(x, y);
            if (mondex != -1) {
                int distance = 0, damage = 0;
                monster *thrown = &(g->zombie(mondex));
                switch (thrown->type->size) {
                case MS_TINY:
                    distance = 10;
                    break;
                case MS_SMALL:
                    distance = 6;
                    break;
                case MS_MEDIUM:
                    distance = 4;
                    break;
                case MS_LARGE:
                    distance = 2;
                    break;
                case MS_HUGE:
                    distance = 0;
                    break;
                }
                damage = distance * 3;
                // subtract 1 unit of distance for every 10 units of density
                // subtract 5 units of damage for every 10 units of density
                material_type *mon_mat = material_type::find_material(thrown->type->mat);
                distance -= mon_mat->density() / 10;
                damage -= mon_mat->density() / 5;

                if (distance > 0) {
                    if (g->u.sees(*thrown)) {
                        add_msg(_("The %s is thrown by winds!"), thrown->name().c_str());
                    }
                    std::vector<tripoint> traj = continue_line(from_monster, distance);
                    bool hit_wall = false;
                    for (size_t i = 0; i < traj.size() && !hit_wall; i++) {
                        int monhit = g->mon_at(traj[i].x, traj[i].y);
                        if (i > 0 && monhit != -1 && !g->zombie(monhit).digging()) {
                            if (g->u.sees( traj[i] ))
                                add_msg(_("The %s hits a %s!"), thrown->name().c_str(),
                                        g->zombie(monhit).name().c_str());
                            g->zombie( monhit ).apply_damage( z, bp_torso, damage );
                            g->zombie( monhit ).check_dead_state();
                            hit_wall = true;
                            thrown->setpos(traj[i - 1]);
                        } else if (g->m.move_cost(traj[i].x, traj[i].y) == 0) {
                            hit_wall = true;
                            thrown->setpos(traj[i - 1]);
                        }
                        int damage_copy = damage;
                        g->m.shoot( traj[i], damage_copy, false, no_effects );
                        if (damage_copy < damage) {
                            thrown->apply_damage( nullptr, bp_torso, damage - damage_copy );
                        }
                    }
                    if (hit_wall) {
                        damage *= 2;
                    } else {
                        thrown->setpos(traj[traj.size() - 1]);
                    }
                    thrown->apply_damage( z, bp_torso, damage );
                    thrown->check_dead_state();
                } // if (distance > 0)
            } // if (mondex != -1)

            if (g->u.posx() == x && g->u.posy() == y) { // Throw... the player?! D:
                bool immune = false;
                if (g->u.has_trait("LEG_TENT_BRACE") && (!g->u.footwear_factor() ||
                        (g->u.footwear_factor() == .5 && one_in(2)))) {
                    add_msg(_("You secure yourself using your tentacles!"));
                    immune = true;
                }
                if (g->u.is_throw_immune()) {
                    add_msg(_("You deftly maintain your footing!"));
                    immune = true;
                }
                if (!g->u.uncanny_dodge() && !immune) {
                    std::vector<tripoint> traj = continue_line(from_monster, rng(2, 3));
                    add_msg(m_bad, _("You're thrown by winds!"));
                    bool hit_wall = false;
                    int damage = rng(5, 10);
                    for (size_t i = 0; i < traj.size() && !hit_wall; i++) {
                        int monhit = g->mon_at(traj[i].x, traj[i].y);
                        if (i > 0 && monhit != -1 && !g->zombie(monhit).digging()) {
                            if (g->u.sees( traj[i] )) {
                                add_msg(m_bad, _("You hit a %s!"), g->zombie(monhit).name().c_str());
                            }
                            g->zombie( monhit ).apply_damage( &g->u, bp_torso, damage ); // We get the kill :)
                            g->zombie( monhit ).check_dead_state();
                            hit_wall = true;
                            g->u.setx( traj[i - 1].x );
                            g->u.sety( traj[i - 1].y );
                        } else if (g->m.move_cost(traj[i].x, traj[i].y) == 0) {
                            add_msg(m_bad, _("You slam into a %s"),
                                    g->m.tername(traj[i].x, traj[i].y).c_str());
                            hit_wall = true;
                            g->u.setx( traj[i - 1].x );
                            g->u.sety( traj[i - 1].y );
                        }
                        int damage_copy = damage;
                        g->m.shoot( traj[i], damage_copy, false, no_effects);
                        if (damage_copy < damage) {
                            g->u.deal_damage( z, bp_torso, damage_instance( DT_BASH, damage - damage_copy ) );
                            g->u.check_dead_state();
                        }
                    }
                    if (hit_wall) {
                        damage *= 2;
                    } else {
                        g->u.setx( traj[traj.size() - 1].x );
                        g->u.sety( traj[traj.size() - 1].y );
                    }
                    g->u.deal_damage( z, bp_torso, damage_instance( DT_BASH, damage ) );
                    g->u.check_dead_state();
                    g->update_map(&(g->u));
                } // Done with checking for player
            }
        }
    } // Done with loop!
}

void mattack::gene_sting(monster *z, int index)
{
    if( z->friendly ) {
        return; // TODO: handle friendly monsters
    }
    if (within_visual_range(z, 7) < 0) return;

    if (g->u.uncanny_dodge()) {
        return;
    }
    z->moves -= 150;
    z->reset_special(index); // Reset timer
    add_msg(m_bad, _("The %s shoots a dart into you!"), z->name().c_str());
    g->u.mutate();
}

void mattack::para_sting(monster *z, int index)
{
    Creature *target = z->attack_target();
    if( target == nullptr ) {
        return;
    }
    if( rl_dist( z->pos(), target->pos() ) > 4 ) {
        return;
    }

    if( target->uncanny_dodge() ) {
        return;
    }
    z->moves -= 150;
    z->reset_special(index); // Reset timer
    target->add_msg_if_player(m_bad, _("The %s shoots a dart into you!"), z->name().c_str());
    target->add_msg_if_player(m_bad, _("You feel poison enter your body!"));
    target->add_effect("paralyzepoison", 50);
}

void mattack::triffid_growth(monster *z, int index)
{
    (void)index; //unused
    // Young triffid growing into an adult
    if (g->u.sees( *z )) {
        add_msg(m_warning, _("The %s young triffid grows into an adult!"),
                z->name().c_str());
    }
    z->poly(GetMType("mon_triffid"));
}

void mattack::stare(monster *z, int index)
{
    if( z->friendly ) {
        return; // TODO: handle friendly monsters
    }
    z->moves -= 200;
    z->reset_special(index); // Reset timer
    if( z->sees( g->u ) ) {
        add_msg(m_bad, _("The %s stares at you, and you shudder."), z->name().c_str());
        g->u.add_effect("teleglow", 800);
    } else {
        add_msg(m_bad, _("A piercing beam of light bursts forth!"));
        std::vector<tripoint> sight = line_to( z->pos3(), g->u.pos3(), 0, 0 );
        for (auto &i : sight) {
            if( g->m.ter( i ) == t_reinforced_glass ) {
                break;
            } else if( g->m.is_bashable( i ) ) {
                //Destroy it
                g->m.bash( i, 999, false, true );
            }
        }
    }
}

void mattack::fear_paralyze(monster *z, int index)
{
    if( z->friendly ) {
        return; // TODO: handle friendly monsters
    }
    if (g->u.sees( *z )) {
        z->reset_special(index); // Reset timer
        if (g->u.has_artifact_with(AEP_PSYSHIELD) || (g->u.is_wearing("tinfoil_hat") && one_in(4))) {
            add_msg(_("The %s probes your mind, but is rebuffed!"), z->name().c_str());
        } else if (rng(1, 20) > g->u.int_cur) {
            add_msg(m_bad, _("The terrifying visage of the %s paralyzes you."),
                    z->name().c_str());
            g->u.moves -= 100;
        } else
            add_msg(_("You manage to avoid staring at the horrendous %s."),
                    z->name().c_str());
    }
}

void mattack::photograph(monster *z, int index)
{
    if( within_visual_range(z, 6) < 0 ) {
        return;
    }

    // Badges should NOT be swappable between roles.
    // Hence separate checking.
    // If you are in fact listed as a police officer
    if (g->u.has_trait("PROF_POLICE")) {
        // And you're wearing your badge
        if (g->u.is_wearing("badge_deputy")) {
            if (one_in(3)) {
                add_msg(m_info, _("The %s flashes a LED and departs.  Human officer on scene."), z->name().c_str());
                z->no_corpse_quiet = true;
                z->no_extra_death_drops = true;
                z->die(nullptr);
                return;
            } else {
                add_msg(m_info, _("The %s acknowledges you as an officer responding, but hangs around to watch."), z->name().c_str());
                add_msg(m_info, _("Probably some now-obsolete Internal Affairs subroutine..."));
                z->reset_special(index); // Reset timer
                return;
            }
        }
    }

    if (g->u.has_trait("PROF_PD_DET")) {
        // And you have your shield on
        if (g->u.is_wearing("badge_detective")) {
            if (one_in(4)) {
                add_msg(m_info, _("The %s flashes a LED and departs.  Human officer on scene."), z->name().c_str());
                z->no_corpse_quiet = true;
                z->no_extra_death_drops = true;
                z->die(nullptr);
                return;
            } else {
                add_msg(m_info, _("The %s acknowledges you as an officer responding, but hangs around to watch."), z->name().c_str());
                add_msg(m_info, _("Ops used to do that in case you needed backup..."));
                z->reset_special(index); // Reset timer
                return;
            }
        }
    } else if (g->u.has_trait("PROF_SWAT")) {
        // And you're wearing your badge
        if (g->u.is_wearing("badge_swat")) {
            if (one_in(3)) {
                add_msg(m_info, _("The %s flashes a LED and departs.  SWAT's working the area."), z->name().c_str());
                z->no_corpse_quiet = true;
                z->no_extra_death_drops = true;
                z->die(nullptr);
                return;
            } else {
                add_msg(m_info, _("The %s acknowledges you as SWAT onsite, but hangs around to watch."), z->name().c_str());
                add_msg(m_info, _("Probably some now-obsolete Internal Affairs subroutine..."));
                z->reset_special(index); // Reset timer
                return;
            }
        }
    } else if (g->u.has_trait("PROF_CYBERCOP")) {
        // And you're wearing your badge
        if (g->u.is_wearing("badge_cybercop")) {
            if (one_in(3)) {
                add_msg(m_info, _("The %s winks a LED and departs.  One machine to another?"), z->name().c_str());
                z->no_corpse_quiet = true;
                z->no_extra_death_drops = true;
                z->die(nullptr);
                return;
            } else {
                add_msg(m_info, _("The %s acknowledges you as an officer responding, but hangs around to watch."), z->name().c_str());
                add_msg(m_info, _("Apparently yours aren't the only systems kept alive post-apocalypse."));
                z->reset_special(index); // Reset timer
                return;
            }
        }
    }

    if (g->u.has_trait("PROF_FED")) {
        // And you're wearing your badge
        if (g->u.is_wearing("badge_marshal")) {
            add_msg(m_info, _("The %s flashes a LED and departs.  The Feds have this."), z->name().c_str());
            z->no_corpse_quiet = true;
            z->no_extra_death_drops = true;
            z->die(nullptr);
            return;
        }
    }

    if( z->friendly ) {
        // Friendly (hacked?) bot ignore the player.
        // TODO: might need to be revisited when it can target npcs.
        return;
    }
    z->reset_special(index); // Reset timer
    z->moves -= 150;
    add_msg(m_warning, _("The %s takes your picture!"), z->name().c_str());
    // TODO: Make the player known to the faction
    g->add_event(EVENT_ROBOT_ATTACK, int(calendar::turn) + rng(15, 30), 0,
                 g->u.global_sm_location());
}

void mattack::tazer( monster *z, int index )
{
    if( z->friendly != 0 ) {
        // Let friendly bots taze too
        for( size_t i = 0; i < g->num_zombies(); i++ ) {
            monster &tmp = g->zombie( i );
            if( tmp.friendly == 0 && !tmp.is_dead() ) {
                int d = rl_dist( z->pos(), tmp.pos() );
                if ( d < 2 ) {
                    z->reset_special( index ); // Reset timer
                    taze( z, &tmp );
                    return;
                }
            }
        }
        // Taze NPCs too
        for( auto &n : g->active_npc ) {
            if( n->attitude == NPCATT_KILL ) {
            int d = rl_dist( z->pos(), n->pos() );
                if ( d < 2 ) {
                    z->reset_special( index ); // Reset timer
                    taze( z, n );
                    return;
                }
            }
        }
        return;
    }

    if( within_visual_range(z, 1) < 0 ) {
        // Try to taze non-hostile NPCs
        for( auto &n : g->active_npc ) {
            if( n->attitude != NPCATT_KILL ) {
            int d = rl_dist( z->pos(), n->pos() );
                if ( d < 2 ) {
                    z->reset_special( index ); // Reset timer
                    taze( z, n );
                    return;
                }
            }
        }
        return;
    }

    z->reset_special( index ); // Reset timer
    taze( z, &g->u );
}

void mattack::taze( monster *z, Creature *target )
{
    z->moves -= 200;   // It takes a while
    player *foe = dynamic_cast< player* >( target );
    if( target == nullptr || target->uncanny_dodge() ) {
        return;
    }

    if( target->is_elec_immune() ) {
        target->add_msg_player_or_npc( _("The %s unsuccessfully attempts to shock you."),
                                       _("The %s unsuccessfully attempts to shock <npcname>."),
                                       z->name().c_str() );
    }

    if( foe != nullptr ) {
        int shock = rng(1, 5);
        foe->apply_damage( z, bp_torso, shock * rng( 1, 3 ) );
        foe->moves -= shock * 20;
        auto m_type = foe == &g->u ? m_bad : m_neutral;
        foe->add_msg_player_or_npc( m_type, _("The %s shocks you!"),
                                            _("The %s shocks <npcname>!"),
                                            z->name().c_str() );
        foe->check_dead_state();
    } else if( target->is_monster() ) {
        // From iuse::tazer, but simplified
        monster *mon = dynamic_cast< monster* >( target );
        int shock = rng(5, 25);
        mon->moves -= shock * 100;
        mon->apply_damage( z, bp_torso, shock );
        add_msg( _("The %s shocks the %s!"), z->name().c_str(), mon->name().c_str() );
        mon->check_dead_state();
    }
}

void mattack::smg(monster *z, int index)
{
    const std::string ammo_type("9mm");
    // Make sure our ammo isn't weird.
    if (z->ammo[ammo_type] > 1000) {
        debugmsg("Generated too much ammo (%d) for %s in mattack::smg", z->ammo[ammo_type], z->name().c_str());
        z->ammo[ammo_type] = 1000;
    }

    z->reset_special(index); // Reset timer
    Creature *target = nullptr;

    if (z->friendly != 0) {
        // Attacking monsters, not the player!
        int boo_hoo;
        target = z->auto_find_hostile_target( 18, boo_hoo );
        if( target == nullptr ) {// Couldn't find any targets!
            if(boo_hoo > 0 && g->u.sees( *z ) ) { // because that stupid oaf was in the way!
                add_msg(m_warning, ngettext("Pointed in your direction, the %s emits an IFF warning beep.",
                                            "Pointed in your direction, the %s emits %d annoyed sounding beeps.",
                                            boo_hoo),
                        z->name().c_str(), boo_hoo);
            }
            return;
        }
    } else {
        // Not friendly; hence, firing at the player too
        target = z->attack_target();
        if( target == nullptr ) {
            return;
        }
        int dist = rl_dist( z->pos(), target->pos() );
        if( dist > 18 ) {
            return;
        }

        if( !z->has_effect("targeted") ) {
            sounds::sound(z->posx(), z->posy(), 6, _("beep-beep-beep!"));
            z->add_effect("targeted", 8);
            z->moves -= 100;
            return;
        }
    }
    npc tmp = make_fake_npc(z, 16, 8, 8, 12);
    tmp.skillLevel("smg").level(8);
    tmp.skillLevel("gun").level(4);
    z->moves -= 150;   // It takes a while

    if (z->ammo[ammo_type] <= 0) {
        if (one_in(3)) {
            sounds::sound(z->posx(), z->posy(), 2, _("a chk!"));
        } else if (one_in(4)) {
            sounds::sound(z->posx(), z->posy(), 6, _("boop-boop!"));
        }
        return;
    }
    if (g->u.sees( *z )) {
        add_msg(m_warning, _("The %s fires its smg!"), z->name().c_str());
    }
    tmp.weapon = item("hk_mp5", 0);
    tmp.weapon.set_curammo( ammo_type );
    tmp.weapon.charges = std::max(z->ammo[ammo_type], 10);
    z->ammo[ammo_type] -= tmp.weapon.charges;
    tmp.fire_gun(target->posx(), target->posy(), true);
    z->ammo[ammo_type] += tmp.weapon.charges;
    if (target == &g->u) {
        z->add_effect("targeted", 3);
    }
}

void mattack::laser(monster *z, int index)
{
    bool sunlight = g->is_in_sunlight(z->posx(), z->posy());

    z->reset_special(index); // Reset timer
    Creature *target = nullptr;

    if (z->friendly != 0) {   // Attacking monsters, not the player!
        int boo_hoo;
        target = z->auto_find_hostile_target( 18, boo_hoo);
        if( target == nullptr ) {// Couldn't find any targets!
            if(boo_hoo > 0 && g->u.sees( *z ) ) { // because that stupid oaf was in the way!
                add_msg(m_warning, ngettext("Pointed in your direction, the %s emits an IFF warning beep.",
                                            "Pointed in your direction, the %s emits %d annoyed sounding beeps.",
                                            boo_hoo),
                        z->name().c_str(), boo_hoo);
            }
            return;
        }
    } else {
        // Not friendly; hence, firing at the player too
        target = z->attack_target();
        if( target == nullptr ) {
            return;
        }
        int dist = rl_dist( z->pos(), target->pos() );
        if( dist > 18 ) {
            return;
        }

        if (!z->has_effect("targeted")) {
            sounds::sound(z->posx(), z->posy(), 6, _("beep-beep-beep!"));
            z->add_effect("targeted", 8);
            z->moves -= 100;
            return;
        }
    }
    npc tmp = make_fake_npc(z, 16, 8, 8, 12);
    tmp.skillLevel("rifle").level(8);
    tmp.skillLevel("gun").level(4);
    z->moves -= 150;   // It takes a while
    if (!sunlight) {
        if (one_in(3)) {
            if (g->u.sees( *z )) {
                add_msg(_("The %s's barrel spins but nothing happens!"), z->name().c_str());
            }
        } else if (one_in(4)) {
            sounds::sound(z->posx(), z->posy(), 6, _("boop-boop!"));
        }
        return;
    }
    if (g->u.sees( *z )) {
        add_msg(m_warning, _("The %s's barrel spins and fires!"), z->name().c_str());
    }
    tmp.weapon = item("cerberus_laser", 0);
    tmp.weapon.set_curammo( "laser_capacitor" );
    tmp.weapon.charges = 100;
    tmp.fire_gun(target->posx(), target->posy(), true);
    if (target == &g->u) {
        z->add_effect("targeted", 3);
    }
}

void mattack::rifle_tur(monster *z, int index)
{
    Creature *target;
    if (z->friendly != 0) {
        // Attacking monsters, not the player!
        int boo_hoo;
        target = z->auto_find_hostile_target( 18, boo_hoo );
        if( target == nullptr ) {// Couldn't find any targets!
            if( boo_hoo > 0 && g->u.sees( *z ) ) { // because that stupid oaf was in the way!
                add_msg(m_warning, ngettext("Pointed in your direction, the %s emits an IFF warning beep.",
                                            "Pointed in your direction, the %s emits %d annoyed sounding beeps.",
                                            boo_hoo),
                        z->name().c_str(), boo_hoo);
            }
            return;
        }

        z->reset_special(index);
        rifle( z, target );
    } else {
        // (This is a bit generous: 5.56 has 38 range.)
        // Not friendly; hence, firing at the player too
        target = z->attack_target();
        if( target == nullptr ) {
            return;
        }
        int dist = rl_dist( z->pos(), target->pos() );
        if( dist > 18 ) {
            return;
        }

        z->reset_special(index);
        rifle( z, target );
    }
}

void mattack::rifle( monster *z, Creature *target )
{
    const std::string ammo_type("556");
    // Make sure our ammo isn't weird.
    if (z->ammo[ammo_type] > 2000) {
        debugmsg("Generated too much ammo (%d) for %s in mattack::rifle", z->ammo[ammo_type], z->name().c_str());
        z->ammo[ammo_type] = 2000;
    }

    npc tmp = make_fake_npc(z, 16, 10, 8, 12);
    tmp.skillLevel("rifle").level(8);
    tmp.skillLevel("gun").level(6);

    if( target == &g->u ) {
        if (!z->has_effect("targeted")) {
            sounds::sound(z->posx(), z->posy(), 8, _("beep-beep."));
            z->add_effect("targeted", 8);
            z->moves -= 100;
            return;
        }
    }
    z->moves -= 150;   // It takes a while

    if (z->ammo[ammo_type] <= 0) {
        if (one_in(3)) {
            sounds::sound(z->posx(), z->posy(), 2, _("a chk!"));
        } else if (one_in(4)) {
            sounds::sound(z->posx(), z->posy(), 6, _("boop!"));
        }
        return;
    }
    if (g->u.sees( *z )) {
        add_msg(m_warning, _("The %s opens up with its rifle!"), z->name().c_str());
    }
    tmp.weapon = item("m4a1", 0);
    tmp.weapon.set_curammo( ammo_type );
    tmp.weapon.charges = std::max(z->ammo[ammo_type], 30);
    z->ammo[ammo_type] -= tmp.weapon.charges;
    tmp.fire_gun(target->posx(), target->posy(), true);
    z->ammo[ammo_type] += tmp.weapon.charges;
    if (target == &g->u) {
        z->add_effect("targeted", 3);
    }
}

void mattack::frag( monster *z, Creature *target ) // This is for the bots, not a standalone turret
{
    const std::string ammo_type("40mm_frag");
    // Make sure our ammo isn't weird.
    if (z->ammo[ammo_type] > 100) {
        debugmsg("Generated too much ammo (%d) for %s in mattack::frag", z->ammo[ammo_type], z->name().c_str());
        z->ammo[ammo_type] = 100;
    }

    if( target == &g->u ) {
        if (!z->has_effect("targeted")) {
            //~Potential grenading detected.
            add_msg(m_warning, _("Those laser dots don't seem very friendly...") );
            g->u.add_effect("laserlocked", 3); // Effect removed in game.cpp, duration doesn't much matter
            sounds::sound(z->posx(), z->posy(), 10, _("Targeting."));
            z->add_effect("targeted", 5);
            z->moves -= 150;
            // Should give some ability to get behind cover,
            // even though it's patently unrealistic.
            return;
        }
    }
    npc tmp = make_fake_npc(z, 16, 10, 8, 12);
    tmp.skillLevel("launcher").level(8);
    tmp.skillLevel("gun").level(6);
    z->moves -= 150;   // It takes a while

    if (z->ammo[ammo_type] <= 0) {
        if (one_in(3)) {
            sounds::sound(z->posx(), z->posy(), 2, _("a chk!"));
        } else if (one_in(4)) {
            sounds::sound(z->posx(), z->posy(), 6, _("boop!"));
        }
        return;
    }
    if (g->u.sees( *z )) {
        add_msg(m_warning, _("The %s's grenade launcher fires!"), z->name().c_str());
    }
    tmp.weapon = item("mgl", 0);
    tmp.weapon.set_curammo( ammo_type );
    tmp.weapon.charges = std::max(z->ammo[ammo_type], 30);
    z->ammo[ammo_type] -= tmp.weapon.charges;
    tmp.fire_gun(target->posx(), target->posy(), true);
    z->ammo[ammo_type] += tmp.weapon.charges;
    if (target == &g->u) {
        z->add_effect("targeted", 3);
    }
}

void mattack::bmg_tur(monster *z, int index)
{
    const std::string ammo_type("50bmg");
    // Make sure our ammo isn't weird.
    if (z->ammo[ammo_type] > 500) {
        debugmsg("Generated too much ammo (%d) for %s in mattack::bmg_tur", z->ammo[ammo_type], z->name().c_str());
        z->ammo[ammo_type] = 500;
    }

    Creature *target = nullptr;

    if (z->friendly != 0) {
        // Attacking monsters, not the player!
        int boo_hoo;
        target = z->auto_find_hostile_target( 40, boo_hoo );
        if( target == nullptr ) {// Couldn't find any targets!
            if(boo_hoo > 0 && g->u.sees( *z ) ) { // because that stupid oaf was in the way!
                add_msg(m_warning, ngettext("Pointed in your direction, the %s emits an IFF warning beep.",
                                            "Pointed in your direction, the %s emits %d annoyed sounding beeps.",
                                            boo_hoo),
                        z->name().c_str(), boo_hoo);
            }
            return;
        }
        z->reset_special(index);
    } else {
        // (Be grateful for safety precautions.  50BMG has range 90.)
        target = z->attack_target();
        if( target == nullptr ) {
            return;
        }
        int dist = rl_dist( z->pos(), target->pos() );
        if( dist > 18 ) {
            return;
        }

        z->reset_special(index);

        if (!z->has_effect("targeted")) {
            //~There will be a .50BMG shell sent at high speed to your location next turn.
            target->add_msg_if_player( m_warning, _("Why is there a laser dot on your torso..?") );
            sounds::sound(z->posx(), z->posy(), 10, _("Hostile detected."));
            target->add_effect( "laserlocked", 3 );
            z->add_effect("targeted", 8);
            z->moves -= 100;
            return;
        }
    }
    npc tmp = make_fake_npc(z, 16, 10, 8, 12);
    tmp.skillLevel("rifle").level(8);
    tmp.skillLevel("gun").level(6);
    z->moves -= 150;   // It takes a while

    if (z->ammo[ammo_type] <= 0) {
        if (one_in(3)) {
            sounds::sound(z->posx(), z->posy(), 2, _("a chk!"));
        } else if (one_in(4)) {
            sounds::sound(z->posx(), z->posy(), 6, _("boop!"));
        }
        return;
    }
    sounds::sound(z->posx(), z->posy(), 10, _("Interdicting target."));
    if (g->u.sees( *z )) {
        add_msg(m_warning, _("The %s aims and fires!"), z->name().c_str());
    }
    tmp.weapon = item("m107a1", 0);
    tmp.weapon.set_curammo( ammo_type );
    tmp.weapon.charges = std::max(z->ammo[ammo_type], 30);
    z->ammo[ammo_type] -= tmp.weapon.charges;
    tmp.fire_gun(target->posx(), target->posy(), false);
    z->ammo[ammo_type] += tmp.weapon.charges;
    if (target == &g->u) {
        z->add_effect("targeted", 3);
    }
}

void mattack::tankgun( monster *z, Creature *target )
{
    const std::string ammo_type("120mm_HEAT");
    // Make sure our ammo isn't weird.
    if (z->ammo[ammo_type] > 40) {
        debugmsg("Generated too much ammo (%d) for %s in mattack::tankgun", z->ammo[ammo_type], z->name().c_str());
        z->ammo[ammo_type] = 40;
    }

    point aim_point;
    int dist = rl_dist( z->pos(), target->pos() );
    aim_point = target->pos();
    if( dist > 50 ) {
        return;
    }

    if (!z->has_effect("targeted")) {
        //~ There will be a 120mm HEAT shell sent at high speed to your location next turn.
        target->add_msg_if_player( m_warning, _("You're not sure why you've got a laser dot on you...") );
        //~ Sound of a tank turret swiveling into place
        sounds::sound(z->posx(), z->posy(), 10, _("whirrrrrclick."));
        z->add_effect("targeted", 5);
        target->add_effect( "laserlocked", 5 );
        z->moves -= 200;
        // Should give some ability to get behind cover,
        // even though it's patently unrealistic.
        return;
    }
    // Target the vehicle itself instead if there is one.
    vehicle *veh = g->m.veh_at( target->posx(), target->posy() );
    if( veh != nullptr ) {
        veh->center_of_mass( aim_point.x, aim_point.y );
        aim_point += veh->global_pos();
    }
    // kevingranade KA101: yes, but make it really inaccurate
    // Sure thing.
    npc tmp = make_fake_npc(z, 12, 8, 8, 8);
    tmp.skillLevel("launcher").level(1);
    tmp.skillLevel("gun").level(1);
    z->moves -= 150;   // It takes a while

    if (z->ammo[ammo_type] <= 0) {
        if (one_in(3)) {
            sounds::sound(z->posx(), z->posy(), 2, _("a chk!"));
        } else if (one_in(4)) {
            sounds::sound(z->posx(), z->posy(), 6, _("clank!"));
        }
        return;
    }
    if (g->u.sees( *z )) {
        add_msg(m_warning, _("The %s's 120mm cannon fires!"), z->name().c_str());
    }
    tmp.weapon = item("TANK", 0);
    tmp.weapon.set_curammo( ammo_type );
    tmp.weapon.charges = std::max(z->ammo[ammo_type], 5);
    z->ammo[ammo_type] -= tmp.weapon.charges;
    tmp.fire_gun( aim_point.x, aim_point.y, false );
    z->ammo[ammo_type] += tmp.weapon.charges;
}

void mattack::searchlight(monster *z, int index)
{

    z->reset_special(index); // Reset timer

    int max_lamp_count = 3;
    if (z->get_hp() < z->get_hp_max()) {
        max_lamp_count--;
    }
    if (z->get_hp() < z->get_hp_max() / 3) {
        max_lamp_count--;
    }

    const int zposx = z->posx();
    const int zposy = z->posy();

    //this searchlight is not initialized
    if (z->inv.size() == 0) {

        for (int i = 0; i < max_lamp_count; i++) {

            item settings("processor", 0);

            settings.set_var( "SL_PREFER_UP", "TRUE" );
            settings.set_var( "SL_PREFER_DOWN", "TRUE" );
            settings.set_var( "SL_PREFER_RIGHT", "TRUE" );
            settings.set_var( "SL_PREFER_LEFT", "TRUE" );

            for (int x = zposx - 24; x < zposx + 24; x++)
                for (int y = zposy - 24; y < zposy + 24; y++) {
                    if (g->mon_at(x, y) != -1 && g->zombie(g->mon_at(x, y)).type->id == "mon_turret_searchlight") {
                        if (x < zposx) {
                            settings.set_var( "SL_PREFER_LEFT", "FALSE" );
                        }
                        if (x > zposx) {
                            settings.set_var( "SL_PREFER_RIGHT", "FALSE" );
                        }
                        if (y < zposy) {
                            settings.set_var( "SL_PREFER_UP", "FALSE" );
                        }
                        if (y > zposy) {
                            settings.set_var( "SL_PREFER_DOWN", "FALSE" );
                        }
                    }

                }

            settings.set_var( "SL_SPOT_X", 0 );
            settings.set_var( "SL_SPOT_Y", 0 );

            z->add_item(settings);
        }
    }

    //battery charge from the generator is enough for some time of work
    if (calendar::turn % 100 == 0) {

        bool generator_ok = false;

        for (int x = zposx - 24; x < zposx + 24; x++)
            for (int y = zposy - 24; y < zposy + 24; y++) if (g->m.ter_at(x, y).id == "t_plut_generator") {
                    generator_ok = true;
                }

        if (!generator_ok) {
            item &settings = z->inv[index];
            settings.set_var( "SL_POWER", "OFF" );

            return;
        }
    }

    for (int i = 0; i < max_lamp_count; i++) {

        item &settings = z->inv[i];

        if (settings.get_var( "SL_POWER" )  == "OFF") {
            return;
        }

        const int rng_dir = rng(0, 7);

        if (one_in(5)) {

            if (!one_in(5)) {
                settings.set_var( "SL_DIR", rng_dir );
            } else {
                const int rng_pref = rng(0, 3) * 2;
                if (rng_pref == 0 && settings.get_var( "SL_PREFER_UP" ) == "TRUE") {
                    settings.set_var( "SL_DIR", rng_pref );
                } else            if (rng_pref == 2 && settings.get_var( "SL_PREFER_RIGHT" ) == "TRUE") {
                    settings.set_var( "SL_DIR", rng_pref );
                } else            if (rng_pref == 4 && settings.get_var( "SL_PREFER_DOWN" ) == "TRUE") {
                    settings.set_var( "SL_DIR", rng_pref );
                } else            if (rng_pref == 6 && settings.get_var( "SL_PREFER_LEFT" ) == "TRUE") {
                    settings.set_var( "SL_DIR", rng_pref );
                }
            }
        }


        int x = zposx + settings.get_var( "SL_SPOT_X", 0 );
        int y = zposy + settings.get_var( "SL_SPOT_Y", 0 );
        int shift = 0;

        for (int i = 0; i < rng(1, 2); i++) {

            if (!z->sees( g->u )) {
                shift = settings.get_var( "SL_DIR", shift );

                switch (shift) {
                    case 0:
                        y--;
                        break;
                    case 1:
                        y--;
                        x++;
                        break;
                    case 2:
                        x++;
                        break;
                    case 3:
                        x++;
                        y++;
                        break;
                    case 4:
                        y++;
                        break;
                    case 5:
                        y++;
                        x--;
                        break;
                    case 6:
                        x--;
                        break;
                    case 7:
                        x--;
                        y--;
                        break;

                    default:
                        break;
                }

            } else {
                if (x < g->u.posx()) {
                    x++;
                }
                if (x > g->u.posx()) {
                    x--;
                }
                if (y < g->u.posy()) {
                    y++;
                }
                if (y > g->u.posy()) {
                    y--;
                }
            }

            if (rl_dist(x, y, zposx, zposy) > 50) {
                if (x > zposx) {
                    x--;
                }
                if (x < zposx) {
                    x++;
                }
                if (y > zposy) {
                    y--;
                }
                if (y < zposy) {
                    y++;
                }
            }
        }

        settings.set_var( "SL_SPOT_X", x - zposx );
        settings.set_var( "SL_SPOT_Y", y - zposy );

        g->m.add_field(x, y, fd_spotlight, 1);

    }
}

void mattack::flamethrower(monster *z, int index)
{
    if( z->friendly ) {
        return; // TODO: handle friendly monsters
    }
    if (z->friendly != 0) {
        z->reset_special(index); // Reset timer
        Creature *target = nullptr;

        // Attacking monsters, not the player!
        int boo_hoo;
        target = z->auto_find_hostile_target( 5, boo_hoo );
        if (target == NULL) {// Couldn't find any targets!
            if(boo_hoo > 0 && g->u.sees( *z ) ) { // because that stupid oaf was in the way!
                add_msg(m_warning, ngettext("Pointed in your direction, the %s emits an IFF warning beep.",
                                            "Pointed in your direction, the %s emits %d annoyed sounding beeps.",
                                            boo_hoo),
                        z->name().c_str(), boo_hoo);
            }
            return;
        }
        z->reset_special( index );
        flame( z, target );
        return;
    }

    int dist = within_visual_range(z, 5);
    if (dist < 0) {
        return;
    }

    z->reset_special( index );
    flame( z, &g->u );
}

void mattack::flame( monster *z, Creature *target )
{
    int bres = 0;
    int dist = rl_dist( z->pos(), target->pos() );
    if( target != &g->u ) {
      // friendly
      z->moves -= 500;   // It takes a while
      int bres = 0;
      if( !g->m.sees( z->pos(), target->pos(), dist, bres ) ) {
        // shouldn't happen
        debugmsg( "mattack::flame invoked on invisible target" );
      }
      std::vector<tripoint> traj = line_to( z->pos3(), target->pos3(), bres, 0 );

      for (auto &i : traj) {
          // break out of attack if flame hits a wall
          // TODO: Z
          if (g->m.hit_with_fire( tripoint( i.x, i.y, z->posz() ) )) {
              if (g->u.sees( i ))
                  add_msg(_("The tongue of flame hits the %s!"),
                          g->m.tername(i.x, i.y).c_str());
              return;
          }
          g->m.add_field(i.x, i.y, fd_fire, 1);
      }
      target->add_effect("onfire", 8);

      return;
    }

    z->moves -= 500;   // It takes a while
    if( !g->m.sees( z->pos(), target->pos(), dist + 1, bres ) ) {
        // shouldn't happen
        debugmsg( "mattack::flame invoked on invisible target" );
    }
    std::vector<tripoint> traj = line_to( z->pos3(), target->pos3(), bres, 0 );

    for (auto &i : traj) {
        // break out of attack if flame hits a wall
        if (g->m.hit_with_fire( tripoint( i.x, i.y, z->posz() ) )) {
            if (g->u.sees( i ))
                add_msg(_("The tongue of flame hits the %s!"),
                        g->m.tername(i.x, i.y).c_str());
            return;
        }
        g->m.add_field(i.x, i.y, fd_fire, 1);
    }
    if( !target->uncanny_dodge() && !target->has_trait("M_SKIN2")) {
        target->add_effect("onfire", 8);
    }
}

void mattack::copbot(monster *z, int index)
{
    if( z->friendly ) {
        return; // TODO: handle friendly monsters
    }
    bool sees_u = z->sees( g->u );
    bool cuffed = g->u.weapon.type->id == "e_handcuffs";
    z->reset_special(index); // Reset timer
    if (within_visual_range(z, 2) < 0) {
        if (one_in(3)) {
            if (sees_u) {
                if (g->u.unarmed_attack()) {
                    sounds::sound(z->posx(), z->posy(), 18, _("a robotic voice boom, \"Citizen, Halt!\""));
                } else if (!cuffed) {
                    sounds::sound(z->posx(), z->posy(), 18, _("a robotic voice boom, \"\
Please put down your weapon.\""));
                }
            } else
                sounds::sound(z->posx(), z->posy(), 18,
                         _("a robotic voice boom, \"Come out with your hands up!\""));
        } else {
            sounds::sound(z->posx(), z->posy(), 18, _("a police siren, whoop WHOOP"));
        }
        return;
    }
    // only taze uncuffed victims, erm, perpetrators
    if( !cuffed ) {
        taze( z, &g->u );
        return;
    }
    // If cuffed don't attack the player, unless the bot is damaged
    // presumably because of the player's actions
    if (z->get_hp() == z->get_hp_max()) {
        z->anger = 1;
    } else {
        z->anger = z->type->agro;
    }
}

void mattack::chickenbot(monster *z, int index)
{
    int mode = 0;
    int boo_hoo = 0;
    Creature *target;
    if( z->friendly == 0 ) {
        target = z->attack_target();
        if( target == nullptr ) {
            return;
        }
    } else {
        target = z->auto_find_hostile_target( 38, boo_hoo );
        if( target == nullptr ) {
            if( boo_hoo > 0 && g->u.sees( *z ) ) { // because that stupid oaf was in the way!
                add_msg(m_warning, ngettext("Pointed in your direction, the %s emits an IFF warning beep.",
                                            "Pointed in your direction, the %s emits %d annoyed sounding beeps.",
                                            boo_hoo),
                        z->name().c_str(), boo_hoo);
            }
            return;
        }
    }

    int cap = target->power_rating() - 1;
    monster *mon = dynamic_cast< monster* >( target );
    // Their attitude to us and not ours to them, so that bobcats won't get gunned down
    // Only monster-types for now - assuming humans are smart enough not to make it obvious
    // Unless damaged - then everything is hostile
    if( z->get_hp() <= z->get_hp_max() ||
        ( mon != nullptr && mon->attitude_to( *z ) == Creature::Attitude::A_HOSTILE ) ) {
        cap += 2;
    }

    int dist = rl_dist( z->pos(), target->pos() );
    if( dist == 1 && one_in(2) ) {
        mode = 1;
    } else if( ( dist >= 12) ||
               ( ( z->friendly != 0 || g->u.in_vehicle ) && dist >= 6 ) ||
               cap > 2 ) {
        mode = 3;
    } else if( dist >= 4) {
        mode = 2;
    }

    if (mode == 0) {
        return;    // No attacks were valid!
    }

    if( mode > cap ) {
        mode = cap;
    }
    switch (mode) {
    case 0:
    case 1:
        if( dist <= 1 ) {
            taze( z, target );
        }
        break;
    case 2:
        if( dist <= 20 ) {
            rifle( z, target );
        }
        break;
    case 3:
        if( dist == 38 ) {
            frag( z, target );
        }
        break;
    default:
        return; // Weak stuff, shouldn't bother with
    }

    z->reset_special( index );
}

void mattack::multi_robot(monster *z, int index)
{
    int mode = 0;
    int boo_hoo = 0;
    Creature *target;
    if( z->friendly == 0 ) {
        target = z->attack_target();
        if( target == nullptr ) {
            return;
        }
    } else {
        target = z->auto_find_hostile_target( 48, boo_hoo );
        if( target == nullptr ) {
            if( boo_hoo > 0 && g->u.sees( *z ) ) { // because that stupid oaf was in the way!
                add_msg(m_warning, ngettext("Pointed in your direction, the %s emits an IFF warning beep.",
                                            "Pointed in your direction, the %s emits %d annoyed sounding beeps.",
                                            boo_hoo),
                        z->name().c_str(), boo_hoo);
            }
            return;
        }
    }

    int cap = target->power_rating();
    monster *mon = dynamic_cast< monster* >( target );
    // Their attitude to us and not ours to them, so that bobcats won't get gunned down
    // Only monster-types for now - assuming humans are smart enough not to make it obvious
    // Unless damaged - then everything is hostile
    if( z->get_hp() <= z->get_hp_max() ||
        ( mon != nullptr && mon->attitude_to( *z ) == Creature::Attitude::A_HOSTILE ) ) {
        cap += 2;
    }

    int dist = rl_dist( z->pos(), target->pos() );
    if( dist == 1 && one_in(2) ) {
        mode = 1;
    } else if( dist <= 5 ) {
        mode = 2;
    } else if( dist <= 20 ) {
        mode = 3;
    } else if( dist <= 30 ) {
        mode = 4;
    } else if( ( target == &g->u && g->u.in_vehicle ) ||
                 z->friendly != 0 ||
                 cap > 4 ) {
        // Primary only kicks in if you're in a vehicle or are big enough to be mistaken for one.
        // Or if you've hacked it so the turret's on your side.  ;-)
        if( dist >= 35 && dist < 50 ) {
            // Enforced max-range of 50.
            mode = 5;
            cap = 5;
        }
    }

    if( mode == 0 ) {
        return;    // No attacks were valid!
    }

    if( mode > cap ) {
        mode = cap;
    }
    switch (mode) {
    case 1:
        if( dist <= 1 ) {
            taze( z, target );
        }
        break;
    case 2:
        if( dist <= 5 ) {
            flame( z, target );
        }
        break;
    case 3:
        if( dist <= 20 ) {
            rifle( z, target );
        }
        break;
    case 4:
        if( dist <= 30 ) {
            frag( z, target );
        }
        break;
    case 5:
        if( dist <= 50 ) {
            tankgun( z, target );
        }
        break;
    default:
        return; // Weak stuff, shouldn't bother with
    }

    z->reset_special( index );
}

void mattack::ratking(monster *z, int index)
{
    if( z->friendly ) {
        return; // TODO: handle friendly monsters
    }
    if (rl_dist( z->pos(), g->u.pos() ) > 50) {
        return;
    }
    z->reset_special(index); // Reset timer

    switch (rng(1, 5)) { // What do we say?
    case 1:
        add_msg(m_warning, _("\"YOU... ARE FILTH...\""));
        break;
    case 2:
        add_msg(m_warning, _("\"VERMIN... YOU ARE VERMIN...\""));
        break;
    case 3:
        add_msg(m_warning, _("\"LEAVE NOW...\""));
        break;
    case 4:
        add_msg(m_warning, _("\"WE... WILL FEAST... UPON YOU...\""));
        break;
    case 5:
        add_msg(m_warning, _("\"FOUL INTERLOPER...\""));
        break;
    }
    if (rl_dist( z->pos(), g->u.pos() ) <= 10) {
        g->u.add_effect("rat", 30);
    }
}

void mattack::generator(monster *z, int index)
{
    (void)index; //unused
    sounds::sound(z->posx(), z->posy(), 100, "");
    if (int(calendar::turn) % 10 == 0 && z->get_hp() < z->get_hp_max()) {
        z->heal( 1 );
    }
}

void mattack::upgrade(monster *z, int index)
{
    std::vector<int> targets;
    for (size_t i = 0; i < g->num_zombies(); i++) {
        monster &zed = g->zombie(i);
        if( zed.type->id == "mon_zombie" &&
            rl_dist( z->pos(), zed.pos() ) <= 5 &&
            z->attitude_to( zed ) != Creature::Attitude::A_HOSTILE ) {
            targets.push_back(i);
        }
    }
    if (targets.empty()) {
        return;
    }
    z->reset_special(index); // Reset timer
    z->moves -= 150;   // It takes a while

    monster *target = &( g->zombie( targets[ rng(0, targets.size() - 1) ] ) );

    const auto monsters = MonsterGroupManager::GetMonstersFromGroup("GROUP_ZOMBIE_UPGRADE");
    const std::string newtype = monsters[rng(0, monsters.size() - 1)];

    const auto could_see = g->u.sees( *target );
    target->poly(GetMType(newtype));
    const auto can_now_see = g->u.sees( *target );
    if (g->u.sees( *z )) {
        add_msg(m_warning, _("The black mist around the %s grows..."), z->name().c_str());
    }
    if( could_see && can_now_see ) {
        add_msg(m_warning, _("...a zombie becomes a %s!"), target->name().c_str());
    } else if( could_see ) {
        add_msg( m_warning, _( "...a zombie vanishes!" ) );
    } else if( can_now_see ) {
        add_msg( m_warning, _( "...a %s appears!"), target->name().c_str() );
    }
}

void mattack::breathe(monster *z, int index)
{
    z->reset_special(index); // Reset timer
    z->moves -= 100;   // It takes a while

    bool able = (z->type->id == "mon_breather_hub");
    if (!able) {
        for (int x = z->posx() - 3; x <= z->posx() + 3 && !able; x++) {
            for (int y = z->posy() - 3; y <= z->posy() + 3 && !able; y++) {
                int mondex = g->mon_at(x, y);
                if (mondex != -1 && g->zombie(mondex).type->id == "mon_breather_hub") {
                    able = true;
                }
            }
        }
    }
    if (!able) {
        return;
    }

    std::vector<point> valid;
    for (int x = z->posx() - 1; x <= z->posx() + 1; x++) {
        for (int y = z->posy() - 1; y <= z->posy() + 1; y++) {
            if (g->is_empty(x, y)) {
                valid.push_back( point(x, y) );
            }
        }
    }

    if (!valid.empty()) {
        point place = valid[ rng(0, valid.size() - 1) ];
        monster spawned(GetMType("mon_breather"));
        spawned.reset_special(0);
        spawned.spawn(place.x, place.y);
        spawned.faction = z->faction;
        g->add_zombie(spawned);
    }
}

void mattack::bite(monster *z, int index)
{
    if( !z->can_act() ) {
        return;
    }

    // Let it be used on non-player creatures
    Creature *target = z->attack_target();
    if( target == nullptr || rl_dist( z->pos(), target->pos() ) > 1 ) {
        return;
    }

    player *foe = dynamic_cast< player* >( target );
    bool seen = g->u.sees( *z );

    z->reset_special(index); // Reset timer
    z->moves -= 100;
    bool uncanny = foe != nullptr && foe->uncanny_dodge();
    // Can we dodge the attack? Uses player dodge function % chance (melee.cpp)
    if( uncanny || dodge_check(z, target) ){
        if( foe != nullptr ) {
            if( seen ) {
                auto msg_type = foe == &g->u ? m_warning : m_info;
                foe->add_msg_player_or_npc( msg_type, _("The %s lunges at you, but you dodge!"),
                                                      _("The %s lunges at <npcname>, but they dodge!"),
                                            z->name().c_str() );
            }
            if( !uncanny ) {
                foe->practice( "dodge", z->type->melee_skill * 2 );
                foe->ma_ondodge_effects();
            }
        } else if( seen ) {
            add_msg( _("The %s lunges at %s, but misses!"), z->name().c_str(), target->disp_name().c_str() );
        }
        return;
    }

    body_part hit = foe != nullptr ? random_body_part() : bp_torso;
    int dam = rng(5, 10);
    dam = target->deal_damage( z, hit, damage_instance( DT_BASH, dam ) ).total_damage();

    if( dam > 0 && foe != nullptr ) {
        if( seen ) {
            auto msg_type = foe == &g->u ? m_bad : m_info;
            //~ 1$s is monster name, 2$s bodypart in accusative
            foe->add_msg_player_or_npc( msg_type,
                                        _("The %1$s bites your %2$s!"),
                                        _("The %1$s bites <npcname>'s %2$s!"),
                                        z->name().c_str(),
                                        body_part_name_accusative( hit ).c_str() );
        }
        foe->practice( "dodge", z->type->melee_skill );
        if( one_in( 14 - dam ) ) {
            if( foe->has_effect("bite", hit)) {
                foe->add_effect("bite", 400, hit, true);
            } else if( foe->has_effect( "infected", hit ) ) {
                foe->add_effect( "infected", 250, hit, true );
            } else {
                foe->add_effect( "bite", 1, hit, true );
            }
        }
        foe->check_dead_state();
    } else if( foe != nullptr ) {
        if( seen ) {
            foe->add_msg_player_or_npc( _("The %1$s bites your %2$s, but fails to penetrate armor!"),
                                        _("The %1$s bites <npcname>'s %2$s, but fails to penetrate armor!"),
                                        z->name().c_str(),
                                        body_part_name_accusative( hit ).c_str() );
        }
    } else if( seen ) {
        add_msg( _("The %s bites %s!"), z->name().c_str(), target->disp_name().c_str() );
    }
    target->check_dead_state();
}

void mattack::stretch_bite(monster *z, int index)
{
    if( !z->can_act() ) {
        return;
    }

    // Let it be used on non-player creatures
    // can be used at close range too!
    int t;
    Creature *target = z->attack_target();
    if( target == nullptr || rl_dist( z->pos(), target->pos() ) > 3
            || !z->sees(*target, t)) {
        return;
    }

    player *foe = dynamic_cast< player* >( target );
    std::vector<point> line = line_to( z->pos(), target->pos(), t );
    bool seen = g->u.sees( *z );

    z->reset_special(index); // Reset timer
    z->moves -= 150;

    ter_t terrain;
    for (auto &i : line){
        terrain = g->m.ter_at(i.x, i.y);
        //head's not going to fit through the bars
        if (terrain.movecost == 0 ){
            z->add_effect("stunned", 6);
            add_msg( _("The %s stretches its head at you, but bounces off the %s"), z->name().c_str(), terrain.name.c_str() );
            return;
        }
    }
    bool uncanny = foe != nullptr && foe->uncanny_dodge();
    // Can we dodge the attack? Uses player dodge function % chance (melee.cpp)
    if( uncanny || dodge_check(z, target) ) {
        z->moves -=150;
        z->add_effect("stunned", 3);
        if( foe != nullptr ) {
            if( seen ) {
                auto msg_type = foe == &g->u ? m_warning : m_info;
                foe->add_msg_player_or_npc( msg_type, _("The %s's head extends to bite you, but you dodge and the head sails past!"),
                                                      _("The %s's head extends to bite <npcname>, but they dodge and the head sails past!"),
                                            z->name().c_str() );
            }
            if( !uncanny ) {
                foe->practice( "dodge", z->type->melee_skill * 2 );
                foe->ma_ondodge_effects();
            }
        } else if( seen ) {
            add_msg( _("The %s's head extends at %s, but misses and sails past!"), z->name().c_str(), target->disp_name().c_str() );
        }
        return;
    }

    body_part hit = foe != nullptr ? random_body_part() : bp_torso;
    int dam = rng(5, 15); //more damage due to the speed of the moving head
    dam = target->deal_damage( z, hit, damage_instance( DT_STAB, dam ) ).total_damage();

    if( dam > 0 && foe != nullptr ) {
        if( seen ) {
            auto msg_type = foe == &g->u ? m_bad : m_info;
            //~ 1$s is monster name, 2$s bodypart in accusative
            foe->add_msg_player_or_npc( msg_type,
                                        _("The %1$s's teeth sink into your %2$s!"),
                                        _("The %1$s's teeth sink into <npcname>'s %2$s!"),
                                        z->name().c_str(),
                                        body_part_name_accusative( hit ).c_str() );
        }
        foe->practice( "dodge", z->type->melee_skill );
        if( one_in( 16 - dam ) ) {
            if( foe->has_effect("bite", hit)) {
                foe->add_effect("bite", 400, hit, true);
            } else if( foe->has_effect( "infected", hit ) ) {
                foe->add_effect( "infected", 250, hit, true );
            } else {
                foe->add_effect( "bite", 1, hit, true );
            }
        }
        foe->check_dead_state();
    } else if( foe != nullptr ) {
        if( seen ) {
            foe->add_msg_player_or_npc( _("The %1$s's head hits your %2$s, but glances off your armor!"),
                                        _("The %1$s's head hits <npcname>'s %2$s, but glances off armor!"),
                                        z->name().c_str(),
                                        body_part_name_accusative( hit ).c_str() );
        }
    } else if( seen ) {
        add_msg( _("The %s's teeth sink into %s!"), z->name().c_str(), target->disp_name().c_str() );
    }
    target->check_dead_state();
}

void mattack::brandish(monster *z, int index)
{
    if( z->friendly ) {
        return; // TODO: handle friendly monsters
    }
    if( !z->sees( g->u ) ) {
        return; // Only brandish if we can see you!
    }
    z->reset_special(index); // Reset timer
    add_msg(m_warning, _("He's brandishing a knife!"));
    add_msg(_("Quiet, quiet"));
}

void mattack::flesh_golem(monster *z, int index)
{
    if( !z->can_act() ) {
        return;
    }

    int dist;
    Creature *target = z->attack_target();
    if( target == nullptr ||
        ( dist = rl_dist( z->pos(), target->pos() ) ) > 20 ||
        !z->sees( *target ) ) {
        return;
    }

    player *foe = dynamic_cast< player* >( target );
    if( dist > 1 ) {
        if( one_in(12) ) {
            z->moves -= 200;
            z->reset_special(index); // Reset timer
            sounds::sound(z->posx(), z->posy(), 80, _("a terrifying roar that nearly deafens you!"));
        }
        return;
    }
    z->reset_special(index); // Reset timer
    if( g->u.sees( *z ) ) {
        add_msg(_("The %s swings a massive claw at %s!"), z->name().c_str(), target->disp_name().c_str() );
    }
    z->moves -= 100;

    if( target->uncanny_dodge() ) {
        return;
    }

    // Can we dodge the attack? Uses player dodge function % chance (melee.cpp)
    if (dodge_check(z, target)) {
        target->add_msg_player_or_npc( _("You dodge it!"),
                                       _("<npcname> dodges it!") );
        if( foe != nullptr ) {
            foe->practice( "dodge", z->type->melee_skill * 2 );
            foe->ma_ondodge_effects();
        }
        return;
    }
    body_part hit = random_body_part();
    int dam = rng(5, 10);
    //~ 1$s is bodypart name, 2$d is damage value.
    target->deal_damage( z, hit, damage_instance( DT_BASH, dam ) );
    if( one_in( 6 ) &&
        ( foe == nullptr || !foe->is_throw_immune() ||
          ( !foe->has_trait("LEG_TENT_BRACE") ||
            foe->footwear_factor() == 1 || ( foe->footwear_factor() == .5 && one_in(2) ) ) ) ) {
        target->add_effect("downed", 30);
    }
    if( foe != nullptr ) {
        foe->practice( "dodge", z->type->melee_skill );
        foe->add_msg_if_player(m_bad, _("Your %1$s is battered for %2$d damage!"), body_part_name(hit).c_str(), dam);
    }
    target->check_dead_state();
}

void mattack::lunge(monster *z, int index)
{
    if( !z->can_act() ) {
        return;
    }

    int dist;
    Creature *target = z->attack_target();
    if( target == nullptr ||
        ( dist = rl_dist( z->pos(), target->pos() ) ) > 20 ||
        !z->sees( *target ) ) {
        return;
    }

    player *foe = dynamic_cast< player* >( target );
    bool seen = g->u.sees( *z );
    if( dist > 1 ) {
        if (one_in(5)) {
            if( dist > 4 || !z->sees( *target ) ) {
                return; // Out of range
            }
            z->moves += 200;
            z->reset_special(index); // Reset timer
            if( seen ) {
                add_msg(_("The %s lunges for %s!"), z->name().c_str(), target->disp_name().c_str() );
            }
        }
        return;
    }
    z->reset_special(index); // Reset timer
    if( seen ) {
        add_msg(_("The %s lunges straight into %s!"), z->name().c_str(), target->disp_name().c_str() );
    }
    z->moves -= 100;

    if( target->uncanny_dodge()) {
        return;
    }

    // Can we dodge the attack? Uses player dodge function % chance (melee.cpp)
    if (dodge_check(z, target)) {
        target->add_msg_player_or_npc( _("You sidestep it!"),
                                       _("<npcname> sidesteps it!") );
        if( foe != nullptr ) {
            foe->practice( "dodge", z->type->melee_skill * 2 );
            foe->ma_ondodge_effects();
        }
        return;
    }
    body_part hit = random_body_part();
    int dam = rng(3, 7);

    target->deal_damage( z, hit, damage_instance( DT_BASH, dam ) );
    if( one_in( 6 ) &&
        ( foe == nullptr || !foe->is_throw_immune() ||
          ( !foe->has_trait("LEG_TENT_BRACE") ||
            foe->footwear_factor() == 1 || ( foe->footwear_factor() == .5 && one_in(2) ) ) ) ) {
        target->add_effect("downed", 3);
    }
    if( foe != nullptr ) {
        foe->practice( "dodge", z->type->melee_skill );
        foe->add_msg_if_player(m_bad, _("Your %1$s is battered for %2$d damage!"), body_part_name(hit).c_str(), dam);
    }
    target->check_dead_state();
}

void mattack::longswipe(monster *z, int index)
{
    if( z->friendly ) {
        return; // TODO: handle friendly monsters
    }
    Creature *target = z->attack_target();
    if (rl_dist( z->pos(), g->u.pos() ) > 1) {
        if (one_in(5)) {
            if (rl_dist( z->pos(), g->u.pos() ) > 3 ||
                !z->sees( g->u ) ) {
                return; // Out of range
            }
            z->moves -= 150;
            z->reset_special(index); // Reset timer
            add_msg(_("The %s thrusts a claw at you!"), z->name().c_str());

            if (g->u.uncanny_dodge()) {
                return;
            }
            // Can we dodge the attack? Uses player dodge function % chance (melee.cpp)
            if (dodge_check(z, target)) {
                add_msg(_("You evade it!"));
                g->u.practice( "dodge", z->type->melee_skill * 2 );
                g->u.ma_ondodge_effects();
                return;
            }
            body_part hit = random_body_part();
            int dam = rng(3, 7);
            //~ 1$s is bodypart name, 2$d is damage value.
            add_msg(m_bad, _("Your %1$s is slashed for %2$d damage!"), body_part_name(hit).c_str(), dam);
            g->u.deal_damage( z, hit, damage_instance( DT_CUT, dam ) );
            g->u.practice( "dodge", z->type->melee_skill );
            g->u.check_dead_state();
        }
        return;
    }
    z->reset_special(index); // Reset timer
    add_msg(_("The %s slashes at your neck!"), z->name().c_str());
    z->moves -= 100;

    if (g->u.uncanny_dodge()) {
        return;
    }

    // Can we dodge the attack? Uses player dodge function % chance (melee.cpp)
    if (dodge_check(z, target)) {
        add_msg(_("You duck!"));
        g->u.practice( "dodge", z->type->melee_skill * 2 );
        g->u.ma_ondodge_effects();
        return;
    }
    body_part hit = bp_head;
    int dam = rng(6, 10);
    //~ %d is damage value.
    add_msg(m_bad, _("Your throat is slashed for %d damage!"), dam);
    g->u.deal_damage( z, hit, damage_instance( DT_CUT, dam ) );
    g->u.add_effect("bleed", 100, hit);
    g->u.practice( "dodge", z->type->melee_skill );
    g->u.check_dead_state();
}


void mattack::parrot(monster *z, int index)
{
    if (one_in(20)) {
        z->moves -= 100;  // It takes a while
        z->reset_special(index); // Reset timer
        const SpeechBubble speech = get_speech( z->type->id );
        sounds::sound(z->posx(), z->posy(), speech.volume, speech.text);
    }
}

void mattack::darkman(monster *z, int index)
{
    if( z->friendly ) {
        return; // TODO: handle friendly monsters
    }
    if( rl_dist( z->pos(), g->u.pos() ) > 40 ) {
        return;
    }
    z->reset_special(index); // Reset timer
    std::vector<point> free;
    for( int x = z->posx() - 1; x <= z->posx() + 1; x++ ) {
        for( int y = z->posy() - 1; y <= z->posy() + 1; y++ ) {
            if( g->is_empty(x, y) ) {
                free.push_back(point(x, y));
            }
        }
    }
    int free_index = rng( 0, free.size() - 1 );
    monster tmp( GetMType("mon_shadow") );
    z->moves -= 10;
    tmp.spawn( free[free_index].x, free[free_index].y );
    tmp.faction = z->faction;
    g->add_zombie( tmp );
    if( g->u.sees( *z ) ) {
        add_msg(m_warning, _("A shadow splits from the %s!"),
                z->name().c_str() );
    }
    if( !z->sees( g->u ) ) {
        return; // Wont do the combat stuff unless it can see you
    }
    switch (rng(1, 7)) { // What do we say?
    case 1:
        add_msg(_("\"Stop it please\""));
        break;
    case 2:
        add_msg(_("\"Let us help you\""));
        break;
    case 3:
        add_msg(_("\"We wish you no harm\""));
        break;
    case 4:
        add_msg(_("\"Do not fear\""));
        break;
    case 5:
        add_msg(_("\"We can help you\""));
        break;
    case 6:
        add_msg(_("\"We are friendly\""));
        break;
    case 7:
        add_msg(_("\"Please dont\""));
        break;
    }
    g->u.add_effect( "darkness", 1, num_bp, true );
}

void mattack::slimespring(monster *z, int index)
{
    if (rl_dist( z->pos(), g->u.pos() ) > 30) {
        return;
    }
    z->reset_special(index); // Reset timer

    if (g->u.morale_level() <= 1) {
        switch (rng(1, 3)) { //~ Your slimes try to cheer you up!
        //~ Lowercase is intended: they're small voices.
        case 1:
            add_msg(m_good, _("\"hey, it's gonna be all right!\""));
            g->u.add_morale(MORALE_SUPPORT, 10, 50);
            break;
        case 2:
            add_msg(m_good, _("\"we'll get through this!\""));
            g->u.add_morale(MORALE_SUPPORT, 10, 50);
            break;
        case 3:
            add_msg(m_good, _("\"i'm here for you!\""));
            g->u.add_morale(MORALE_SUPPORT, 10, 50);
            break;
        }
    }
    if (rl_dist( z->pos(), g->u.pos() ) <= 3) {
        if ( (g->u.has_effect("bleed")) || (g->u.has_effect("bite")) ) {
            add_msg(_("\"let me help!\""));
            // Yes, your slimespring(s) handle/don't all Bad Damage at the same time.
            if (g->u.has_effect("bite")) {
                if (one_in(3)) {
                    g->u.remove_effect("bite");
                    add_msg(m_good, _("The slime cleans you out!"));
                } else {
                    add_msg(_("The slime flows over you, but your gouges still ache."));
                }
            }
            if (g->u.has_effect("bleed")) {
                if (one_in(2)) {
                    g->u.remove_effect("bleed");
                    add_msg(m_good, _("The slime seals up your leaks!"));
                } else {
                    add_msg(_("The slime flows over you, but your fluids are still leaking."));
                }
            }
        }
    }
}

bool mattack::thrown_by_judo(monster *z, int index)
{
    (void)index; //unused
    Creature *target = z->attack_target();
    if( target == nullptr ||
        rl_dist( z->posx(), z->posy(), target->posx(), target->posy() ) > 1 ||
        !z->sees( *target ) ) {
        return false;
    }

    player *foe = dynamic_cast< player* >( target );
    if( foe == nullptr ) {
        // No mons for now
        return false;
    }
    // "Wimpy" Judo is about to pay off... :D
    if( foe->is_throw_immune() ) {
        // DX + Unarmed
        if ( ((foe->dex_cur + foe->skillLevel("unarmed")) > (z->type->melee_skill + rng(0, 3))) ) {
            foe->add_msg_if_player( m_good, _("but you grab its arm and flip it to the ground!") );

            // most of the time, when not isolated
            if ( !one_in(4) && !target->is_elec_immune() ) {
                // If it all pans out, we're zap the player's arm as he flips the monster.
                foe->add_msg_if_player(_("The flip does shock you..."));
                // Discounted electric damage for quick flip
                damage_instance shock;
                shock.add_damage(DT_ELECTRIC, rng(1, 3));
                foe->deal_damage(z, bp_arm_l, shock);
                foe->deal_damage(z, bp_arm_r, shock);
                foe->check_dead_state();
            }
            // Monster is down,
            z->add_effect("downed", 2);
            // Here, have a crit!
            const auto damage = foe->roll_bash_damage( true ) + 3; // Bonus for the takedown.
            z->apply_damage( foe, bp_torso, damage );
            z->check_dead_state();
        } else {
            // Still avoids the major hit!
            foe->add_msg_if_player(_("but you deftly spin out of its grasp!"));
        }
        return true;
    } else {
        return false;
    }
}

void mattack::riotbot(monster *z, int index)
{
    if( z->friendly ) {
        return; // TODO: handle friendly monsters
    }
    z->reset_special(index); // Reset timer

    const int monx = z->posx();
    const int mony = z->posy();

    if (calendar::turn % 10 == 0) {

        int junk = 0;
        for (int i = -4; i <= 4; i++) {
            for (int j = -4; j <= 4; j++) {
                if( g->m.move_cost( monx + i, mony + j ) != 0 &&
                    g->m.clear_path(monx, mony, monx + i, mony + j, 3, 1, 100, junk) ) {
                    g->m.add_field(monx + i, mony + j, fd_relax_gas, rng(1, 3));
                }
            }
        }
    }

    //already arrested?
    //and yes, if the player has no hands, we are not going to arrest him.
    if (g->u.weapon.type->id == "e_handcuffs" || !g->u.has_two_arms()) {
        z->anger = 0;

        if (calendar::turn % 25 == 0) {
            sounds::sound(monx, mony, 10,
                     _("Halt and submit to arrest, citizen! The police will be here any moment."));
        }

        return;
    }

    if (z->anger < z->type->agro) {
        z->anger += z->type->agro / 20;
        return;
    }

    const int dist = rl_dist(z->pos(), g->u.pos());

    //we need empty hands to arrest
    if (!g->u.is_armed()) {

        sounds::sound(monx, mony, 15, _("Please stay in place, citizen, do not make any movements!"));

        //we need to come closer and arrest
        if (dist > 1) {
            return;
        }

        //Strain the atmosphere, forcing the player to wait. Let him feel the power of law!
        if (!one_in(10)) {
            if (g->u.sees( *z )) {
                add_msg(_("The robot carefully scans you."));
            }
            return;
        }

        enum {ur_arrest, ur_resist, ur_trick};

        //arrest!
        uimenu amenu;
        amenu.selected = 0;
        amenu.text = _("The riotbot orders you to present your hands and be cuffed.");

        amenu.addentry(ur_arrest, true, 'a', _("Allow yourself to be arrested."));
        amenu.addentry(ur_resist, true, 'r', _("Resist arrest!"));
        if (g->u.int_cur > 12 || (g->u.int_cur > 10 && !one_in(g->u.int_cur - 8))) {
            amenu.addentry(ur_trick, true, 't', _("Feign death."));
        }

        amenu.query();
        const int choice = amenu.ret;

        if (choice == ur_arrest) {
            z->anger = 0;

            item handcuffs("e_handcuffs", 0);
            handcuffs.charges = handcuffs.type->maximum_charges();
            handcuffs.active = true;
            handcuffs.set_var( "HANDCUFFS_X", g->u.posx() );
            handcuffs.set_var( "HANDCUFFS_Y", g->u.posy() );

            const bool is_uncanny = g->u.has_active_bionic("bio_uncanny_dodge") && g->u.power_level > 74 &&
                                    !one_in(3);
            const bool is_dex = g->u.dex_cur > 13 && !one_in(g->u.dex_cur - 11);

            if (is_uncanny || is_dex) {

                if (is_uncanny) {
                    g->u.charge_power(-75);
                }

                add_msg(m_good,
                        _("You deftly slip out of the handcuffs just as the robot closes them.  The robot didn't seem to notice!"));
                g->u.i_add(handcuffs);
            } else {
                handcuffs.item_tags.insert("NO_UNWIELD");
                g->u.wield(&(g->u.i_add(handcuffs)));
                g->u.moves -= 300;
                add_msg(_("The robot puts handcuffs on you."));
            }

            sounds::sound(z->posx(), z->posy(), 5,
                     _("You are under arrest, citizen.  You have the right to remain silent.  If you do not remain silent, anything you say may be used against you in a court of law."));
            sounds::sound(z->posx(), z->posy(), 5,
                     _("You have the right to an attorney.  If you cannot afford an attorney, one will be provided at no cost to you.  You may have your attorney present during any questioning."));
            sounds::sound(z->posx(), z->posy(), 5,
                     _("If you do not understand these rights, an officer will explain them in greater detail when taking you into custody."));
            sounds::sound(z->posx(), z->posy(), 5,
                     _("Do not attempt to flee or to remove the handcuffs, citizen.  That can be dangerous to your health."));

            z->moves -= 300;

            return;
        }

        bool bad_trick = false;

        if (choice == ur_trick) {

            if (!one_in(g->u.int_cur - 10)) {

                add_msg(m_good,
                        _("You fall to the ground and feign a sudden convulsive attack.  Though you're obviously still alive, the riotbot cannot tell the difference between your 'attack' and a potentially fatal medical condition.  It backs off, signaling for medical help."));

                z->moves -= 300;
                z->anger = -rng(0, 50);
                return;
            } else {
                add_msg(m_bad, _("Your awkward movements do not fool the riotbot."));
                g->u.moves -= 100;
                bad_trick = true;
            }
        }

        if ((choice == ur_resist) || bad_trick) {

            add_msg(m_bad, _("The robot sprays tear gas!"));
            z->moves -= 200;

            int junk = 0;
            for (int i = -2; i <= 2; i++) {
                for (int j = -2; j <= 2; j++) {
                    if( g->m.move_cost( monx + i, mony + j ) != 0 &&
                        g->m.clear_path(monx, mony, monx + i, mony + j, 3, 1, 100, junk) ) {
                        g->m.add_field(monx + i, mony + j, fd_tear_gas, rng(1, 3));
                    }
                }
            }

            return;
        }

        return;
    }

    if (calendar::turn % 5 == 0) {
        sounds::sound(monx, mony, 25, _("Empty your hands and hold your position, citizen!"));
    }

    if (dist > 5 && dist < 18 && one_in(10)) {

        z->moves -= 50;

        int delta = dist / 4 + 1;  //precautionary shot
        if (z->get_hp() < z->get_hp_max()) {
            delta = 1;    //precision shot
        }

        int x = g->u.posx() + rng(0, delta) - rng(0, delta);
        int y = g->u.posy() + rng(0, delta) - rng(0, delta);

        //~ Sound of a riotbot using its blinding flash
        sounds::sound(x, y, 3, _("fzzzzzt"));

        std::vector <point> traj = line_to(monx, mony, x, y, 0);
        for( auto &elem : traj ) {
            if( !g->m.trans( elem.x, elem.y ) ) {
                break;
            }
            g->m.add_field( elem.x, elem.y, fd_dazzling, 1 );
        }
        return;

    }

    return;
}

void mattack::bio_op_takedown(monster *z, int index)
{
    if( !z->can_act() ) {
        return;
    }

    Creature *target = z->attack_target();
    if( target == nullptr ||
        rl_dist( z->pos(), target->pos() ) > 1 ||
        !z->sees( *target ) ) {
        return;
    }

    bool seen = g->u.sees( *z );
    player *foe = dynamic_cast< player* >( target );
    z->reset_special(index); // Reset timer
    if( seen ) {
        add_msg(_("The %s mechanically grabs at %s!"), z->name().c_str(), target->disp_name().c_str() );
    }
    z->moves -= 100;

    if( target->uncanny_dodge() ) {
        return;
    }

    // Can we dodge the attack? Uses player dodge function % chance (melee.cpp)
    if (dodge_check(z, target)) {
        target->add_msg_player_or_npc( _("You dodge it!"),
                                       _("<npcname> dodges it!") );
        if( foe != nullptr ) {
            foe->practice( "dodge", z->type->melee_skill * 2 );
            foe->ma_ondodge_effects();
        }
        return;
    }
    int dam = rng(3, 9);
    if( foe == nullptr ) {
        // Handle mons earlier - less to check for
        dam = rng(6, 18); // Always aim for the torso
        target->deal_damage( z, bp_torso, damage_instance( DT_BASH, dam ) ); // Two hits - "leg" and torso
        target->deal_damage( z, bp_torso, damage_instance( DT_BASH, dam ) );
        target->add_effect("downed", 3);
        if( seen ) {
            add_msg(_("%s slams %s to the ground!"), z->name().c_str(), target->disp_name().c_str() );
        }
        target->check_dead_state();
        return;
    }
    // Yes, it has the CQC bionic.
    body_part hit = num_bp;
    if (one_in(2)) {
        hit = bp_leg_l;
    } else {
        hit = bp_leg_r;
    }
    // Weak kick to start with, knocks you off your footing

    // Literally "The zombie kicks" vvvvv |  FIXME FIX message or comment why Literally.
    //~ 1$s is bodypart name in accusative, 2$d is damage value.
    foe->add_msg_if_player(m_bad, _("The zombie kicks your %1$s for %2$d damage..."),
                           body_part_name_accusative(hit).c_str(), dam);
    foe->deal_damage( z, hit, damage_instance( DT_BASH, dam ) );
    // At this point, Judo or Tentacle Bracing can make this much less painful
    if ( !foe->is_throw_immune()) {
        if( !foe->has_trait("LEG_TENT_BRACE") && (foe->footwear_factor() == 1 ||
                (foe->footwear_factor() == .5 && one_in(2))) ) {
            if (one_in(4)) {
                hit = bp_head;
                dam = rng(9, 21); // 50% damage buff for the headshot.
                foe->add_msg_if_player(m_bad, _("and slams you, face first, to the ground for %d damage!"), dam);
                foe->deal_damage( z, hit, damage_instance( DT_BASH, dam ) );
            } else {
                hit = bp_torso;
                dam = rng(6, 18);
                foe->add_msg_if_player(m_bad, _("and slams you to the ground for %d damage!"), dam);
                foe->deal_damage( z, hit, damage_instance( DT_BASH, dam ) );
            }
            foe->add_effect("downed", 3);
        }
    } else if( !thrown_by_judo(z, -1) ) {
        // Saved by the tentacle-bracing! :)
        hit = bp_torso;
        dam = rng(3, 9);
        foe->add_msg_if_player(m_bad, _("and slams you for %d damage!"), dam);
        foe->deal_damage( z, hit, damage_instance( DT_BASH, dam ) );
    }
    foe->practice( "dodge", z->type->melee_skill );
    foe->check_dead_state();
}

void mattack::suicide(monster *z, int index)
{
    (void)index;
    Creature *target = z->attack_target();
    if( target == nullptr ||
        rl_dist( z->pos(), target->pos() ) > 2 ||
        !z->sees( *target ) ) {
        return;
    }
    z->die(z);
}

void mattack::stretch_attack(monster *z, int index){
    if( !z->can_act() ) {
        return;
    }

    int t;
    Creature *target = z->attack_target();

    if (target == nullptr || rl_dist(z->pos(), target->pos()) > 3 || !z->sees(*target, t)){
        return;
    }
    int distance = rl_dist(z->pos(), target->pos());
    player *foe = dynamic_cast< player* >( target );
    bool seen = g->u.sees( *z );
    std::vector<point> line = line_to( z->pos(), target->pos(), t );
    int dam = rng(5, 10);
    if (distance >= 2 && distance <= 3){
        z->moves -=100;
        z->reset_special(index);
        ter_t terrain;
        for (auto &i : line){
                terrain = g->m.ter_at(i.x, i.y);
                if (!(terrain.id == "t_bars") && terrain.movecost == 0 ){
                    add_msg( _("The %s thrusts its arm at you but bounces off the %s"), z->name().c_str(), terrain.name.c_str() );
                    return;
                }
        }
        if ( foe != nullptr){
            auto msg_type = foe == &g->u ? m_warning : m_info;
            foe->add_msg_player_or_npc(msg_type, _("The %s thrusts its arm at you, stretching to reach you from afar"),
                                                 _("The %s thrusts its arm at <npcname>, stretching to reach <npcname> from afar"),
                z->name().c_str());
        } else {
            add_msg( _("The %s thrusts its arm at %s."),
                        z->name().c_str(), target->disp_name().c_str());
        }
        if (dodge_check(z, target) || g->u.uncanny_dodge()) {
            if ( foe != nullptr) {
                auto msg_type = foe == &g->u ? m_warning : m_info;
                foe->add_msg_player_or_npc(msg_type, _("You evade the stretched arm and it sails past you!"),
                                                     _("<npcname> evades the stretched arm and it sails past <npcname>!"),
                                                    z->name().c_str());
                foe->practice( "dodge", z->type->melee_skill * 2 );
                foe->ma_ondodge_effects();
            } else {
                add_msg( _("The %s evades the stretched arm and it sails past %s!"),
                            target->disp_name().c_str());
            }
            //takes some time to retract the arm
            z->moves -= 150;
            return;
        }
        body_part hit = foe != nullptr ? random_body_part() : bp_torso;
        dam = target->deal_damage( z, hit, damage_instance( DT_STAB, dam ) ).total_damage();

        if(foe != nullptr && dam > 0 ) {
            if( seen ) {
                auto msg_type = foe == &g->u ? m_bad : m_info;
                //~ 1$s is monster name, 2$s bodypart in accusative
                foe->add_msg_player_or_npc( msg_type,
                                        _("The %1$s's arm pierces your %2$s!"),
                                        _("The %1$s arm pierces <npcname>'s %2$s!"),
                                        z->name().c_str(),
                                        body_part_name_accusative( hit ).c_str() );
            }
            foe->practice( "dodge", z->type->melee_skill );
            foe->ma_ondodge_effects();
            foe->check_dead_state();
        } else if(foe != nullptr ) {
            if( seen ) {
                foe->add_msg_player_or_npc( _("The %1$s arm hits your %2$s, but glances off your armor!"),
                                        _("The %1$s hits <npcname>'s %2$s, but glances off armor!"),
                                        z->name().c_str(),
                                        body_part_name_accusative( hit ).c_str() );
            }
        } else if( seen ) {
            add_msg( _("The %s's arm pierces %s!"), z->name().c_str(), target->disp_name().c_str() );
        }
    }
    target->check_dead_state();
}

bool mattack::dodge_check(monster *z, Creature *target){
    int dodge = std::max( target->get_dodge() - rng(0, z->type->melee_skill), 0L );
    if (rng(0, 10000) < 10000 / (1 + (99 * exp(-.6 * dodge)))) {
        return true;
    }
    return false;
}
