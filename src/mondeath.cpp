#include "mondeath.h"
#include "monster.h"
#include "game.h"
#include "map.h"
#include "map_iterator.h"
#include "rng.h"
#include "line.h"
#include "messages.h"
#include "sounds.h"
#include "mondeath.h"
#include "iuse_actor.h"
#include "translations.h"
#include "morale.h"
#include "event.h"
#include "itype.h"
#include "mtype.h"
#include "field.h"

#include <math.h>  // rounding
#include <sstream>

const mtype_id mon_blob( "mon_blob" );
const mtype_id mon_blob_brain( "mon_blob_brain" );
const mtype_id mon_blob_small( "mon_blob_small" );
const mtype_id mon_breather( "mon_breather" );
const mtype_id mon_breather_hub( "mon_breather_hub" );
const mtype_id mon_creeper_hub( "mon_creeper_hub" );
const mtype_id mon_creeper_vine( "mon_creeper_vine" );
const mtype_id mon_halfworm( "mon_halfworm" );
const mtype_id mon_sewer_rat( "mon_sewer_rat" );
const mtype_id mon_thing( "mon_thing" );
const mtype_id mon_zombie_dancer( "mon_zombie_dancer" );
const mtype_id mon_zombie_hulk( "mon_zombie_hulk" );
const mtype_id mon_giant_cockroach( "mon_giant_cockroach" );
const mtype_id mon_giant_cockroach_nymph( "mon_giant_cockroach_nymph" );
const mtype_id mon_pregnant_giant_cockroach("mon_pregnant_giant_cockroach");

const species_id ZOMBIE( "ZOMBIE" );
const species_id BLOB( "BLOB" );

void mdeath::normal(monster *z)
{
    if ((g->u.sees(*z)) && (!z->no_corpse_quiet)) {
        add_msg(m_good, _("The %s dies!"),
                z->name().c_str()); //Currently it is possible to get multiple messages that a monster died.
    }
    if ( z->type->in_species( ZOMBIE )) {
            sfx::play_variant_sound( "mon_death", "zombie_death", sfx::get_heard_volume(z->pos()));
        }
    m_size monSize = (z->type->size);
    bool leaveCorpse = !((z->type->has_flag(MF_VERMIN)) || (z->no_corpse_quiet));

    // leave some blood if we have to
    if (!z->has_flag(MF_VERMIN)) {
        field_id type_blood = z->bloodType();
        if (type_blood != fd_null) {
            g->m.add_field( z->pos(), type_blood, 1, 0 );
        }
    }

    int maxHP = z->get_hp_max();
    if (!maxHP) {
        maxHP = 1;
    }

    float overflowDamage = std::max( -z->get_hp(), 0 );
    float corpseDamage = 5 * (overflowDamage / (maxHP * 2));

    if (leaveCorpse) {
        int gibAmount = int(floor(corpseDamage)) - 1;
        // allow one extra gib per 5 HP
        int gibLimit = 1 + (maxHP / 5.0);
        if (gibAmount > gibLimit) {
            gibAmount = gibLimit;
        }
        bool pulverized = (corpseDamage > 5 && overflowDamage > 150);
        if (!pulverized) {
            make_mon_corpse(z, int(floor(corpseDamage)));
        } else if (monSize >= MS_MEDIUM) {
            gibAmount += rng(1, 6);
            sfx::play_variant_sound( "mon_death", "zombie_gibbed", sfx::get_heard_volume(z->pos()));
        }
        // Limit chunking to flesh, veggy and insect creatures until other kinds are supported.
        bool leaveGibs = (z->made_of("flesh") || z->made_of("hflesh") || z->made_of("veggy") ||
                          z->made_of("iflesh"));
        if (leaveGibs) {
            make_gibs( z, gibAmount );
        }
    }
}

void mdeath::acid(monster *z)
{
    if (g->u.sees(*z)) {
        if(z->type->dies.size() ==
           1) { //If this death function is the only function. The corpse gets dissolved.
            add_msg(m_mixed, _("The %s's body dissolves into acid."), z->name().c_str());
        } else {
            add_msg(m_warning, _("The %s's body leaks acid."), z->name().c_str());
        }
    }
    g->m.add_field(z->pos(), fd_acid, 3, 0);
}

void mdeath::boomer(monster *z)
{
    std::string explode = string_format(_("a %s explode!"), z->name().c_str());
    sounds::sound(z->pos(), 24, explode);
    for( auto &&dest : g->m.points_in_radius( z->pos(), 1 ) ) {
        g->m.bash( dest, 10 );
        g->m.add_field( dest, fd_bile, 1, 0 );
        int mondex = g->mon_at( dest );
        if (mondex != -1) {
            g->zombie(mondex).stumble();
            g->zombie(mondex).moves -= 250;
        }
    }
    if (rl_dist( z->pos(), g->u.pos() ) == 1) {
        g->u.add_env_effect("boomered", bp_eyes, 2, 24);
    }
}

void mdeath::boomer_glow(monster *z)
{
    std::string explode = string_format(_("a %s explode!"), z->name().c_str());
    sounds::sound(z->pos(), 24, explode);

    for( auto &&dest : g->m.points_in_radius( z->pos(), 1 ) ) {
        g->m.bash(dest , 10 );
        g->m.add_field(dest , fd_bile, 1, 0);
        int mondex = g->mon_at(dest);
        Creature *critter = g->critter_at(dest);
        if (mondex != -1) {
            g->zombie(mondex).stumble();
            g->zombie(mondex).moves -= 250;
        }
        if (critter != nullptr){
            critter->add_env_effect("boomered", bp_eyes, 5, 25);
            for (int i = 0; i < rng(2,4); i++){
                body_part bp = random_body_part();
                critter->add_env_effect("glowing", bp, 4, 40);
                if (critter != nullptr && critter->has_effect("glowing")){
                    break;
                }
            }
        }
    }
}

void mdeath::kill_vines(monster *z)
{
    std::vector<int> vines;
    std::vector<int> hubs;
    for (size_t i = 0; i < g->num_zombies(); i++) {
        bool isHub = g->zombie(i).type->id == mon_creeper_hub;
        if (isHub && (g->zombie(i).posx() != z->posx() || g->zombie(i).posy() != z->posy())) {
            hubs.push_back(i);
        }
        if (g->zombie(i).type->id == mon_creeper_vine) {
            vines.push_back(i);
        }
    }

    for( auto &i : vines ) {
        monster *vine = &(g->zombie(i));
        int dist = rl_dist( vine->pos(), z->pos() );
        bool closer = false;
        for (auto &j : hubs) {
            if (rl_dist(vine->pos(), g->zombie(j).pos()) < dist) {
                break;
            }
        }
        if (!closer) {
            vine->die(z);
        }
    }
}

void mdeath::vine_cut(monster *z)
{
    std::vector<int> vines;
    tripoint tmp = z->pos();
    int &x = tmp.x;
    int &y = tmp.y;
    for( x = z->posx() - 1; x <= z->posx() + 1; x++ ) {
        for( y = z->posy() - 1; y <= z->posy() + 1; y++ ) {
            if( tmp == z->pos() ) {
                y++; // Skip ourselves
            }
            int mondex = g->mon_at( tmp );
            if (mondex != -1 && g->zombie(mondex).type->id == mon_creeper_vine) {
                vines.push_back(mondex);
            }
        }
    }

    for (auto &i : vines) {
        bool found_neighbor = false;
        monster *vine = &(g->zombie( i ));
        tmp = vine->pos();
        for( x = vine->posx() - 1; x <= vine->posx() + 1 && !found_neighbor; x++ ) {
            for( y = vine->posy() - 1; y <= vine->posy() + 1 && !found_neighbor; y++ ) {
                if (x != z->posx() || y != z->posy()) {
                    // Not the dying vine
                    int mondex = g->mon_at( { x, y, z->posz() } );
                    if (mondex != -1 && (g->zombie(mondex).type->id == mon_creeper_hub ||
                                         g->zombie(mondex).type->id == mon_creeper_vine)) {
                        found_neighbor = true;
                    }
                }
            }
        }
        if (!found_neighbor) {
            vine->die(z);
        }
    }
}

void mdeath::triffid_heart(monster *z)
{
    if (g->u.sees(*z)) {
        add_msg(m_warning, _("The surrounding roots begin to crack and crumble."));
    }
    g->add_event(EVENT_ROOTS_DIE, int(calendar::turn) + 100);
}

void mdeath::fungus(monster *z)
{
    //~ the sound of a fungus dying
    sounds::sound(z->pos(), 10, _("Pouf!"));

    for( auto &&sporep : g->m.points_in_radius( z->pos(), 1 ) ) {
        if( g->m.impassable( sporep ) ) {
            continue;
        }
        // z is dead, don't credit it with the kill
        // Maybe credit z's killer?
        g->m.fungalize( sporep, nullptr, 0.25 );
    }
}

void mdeath::disintegrate(monster *z)
{
    if (g->u.sees(*z)) {
        add_msg(m_good, _("The %s disintegrates!"), z->name().c_str());
    }
}

void mdeath::worm(monster *z)
{
    if (g->u.sees(*z)) {
        if(z->type->dies.size() == 1) {
            add_msg(m_good, _("The %s splits in two!"), z->name().c_str());
        } else {
            add_msg(m_warning, _("Two worms crawl out of the %s's corpse."), z->name().c_str());
        }
    }

    std::vector <tripoint> wormspots;
    for( auto &&wormp : g->m.points_in_radius( z->pos(), 1 ) ) {
        if (g->m.has_flag("DIGGABLE", wormp) && g->is_empty( wormp ) ) {
            wormspots.push_back(wormp);
        }
    }
    int worms = 0;
    while(worms < 2 && !wormspots.empty()) {
        const tripoint target = random_entry_removed( wormspots );
        if(-1 == g->mon_at( target )) {
            g->summon_mon(mon_halfworm, target);
            worms++;
        }
    }
}

void mdeath::disappear(monster *z)
{
    if (g->u.sees(*z)) {
        add_msg(m_good, _("The %s disappears."), z->name().c_str());
    }
}

void mdeath::guilt(monster *z)
{
    const int MAX_GUILT_DISTANCE = 5;
    int kill_count = g->kill_count(z->type->id);
    int maxKills = 100; // this is when the player stop caring altogether.

    // different message as we kill more of the same monster
    std::string msg = _("You feel guilty for killing %s."); // default guilt message
    game_message_type msgtype = m_bad; // default guilt message type
    std::map<int, std::string> guilt_tresholds;
    guilt_tresholds[75] = _("You feel ashamed for killing %s.");
    guilt_tresholds[50] = _("You regret killing %s.");
    guilt_tresholds[25] = _("You feel remorse for killing %s.");

    if (g->u.has_trait("PSYCHOPATH") || g->u.has_trait("PRED3") || g->u.has_trait("PRED4") ) {
        return;
    }
    if (rl_dist( z->pos(), g->u.pos() ) > MAX_GUILT_DISTANCE) {
        // Too far away, we can deal with it.
        return;
    }
    if (z->get_hp() >= 0) {
        // We probably didn't kill it
        return;
    }
    if (kill_count >= maxKills) {
        // player no longer cares
        if (kill_count == maxKills) {
            //~ Message after killing a lot of monsters which would normally affect the morale negatively. %s is the monster name, it will be pluralized with a number of 100.
            add_msg(m_good, _("After killing so many bloody %s you no longer care "
                              "about their deaths anymore."), z->name(maxKills).c_str());
        }
        return;
    } else if ((g->u.has_trait("PRED1")) || (g->u.has_trait("PRED2"))) {
        msg = (_("Culling the weak is distasteful, but necessary."));
        msgtype = m_neutral;
    } else {
        msgtype = m_bad;
        for( auto &guilt_treshold : guilt_tresholds ) {
            if( kill_count >= guilt_treshold.first ) {
                msg = guilt_treshold.second;
                break;
            }
        }
    }

    add_msg(msgtype, msg.c_str(), z->name().c_str());

    int moraleMalus = -50 * (1.0 - ((float) kill_count / maxKills));
    int maxMalus = -250 * (1.0 - ((float) kill_count / maxKills));
    int duration = 300 * (1.0 - ((float) kill_count / maxKills));
    int decayDelay = 30 * (1.0 - ((float) kill_count / maxKills));
    if (z->type->in_species( ZOMBIE )) {
        moraleMalus /= 10;
        if (g->u.has_trait("PACIFIST")) {
            moraleMalus *= 5;
        } else if (g->u.has_trait("PRED1")) {
            moraleMalus /= 4;
        } else if (g->u.has_trait("PRED2")) {
            moraleMalus /= 5;
        }
    }
    g->u.add_morale(MORALE_KILLED_MONSTER, moraleMalus, maxMalus, duration, decayDelay);

}

void mdeath::blobsplit(monster *z)
{
    int speed = z->get_speed() - rng(30, 50);
    g->m.spawn_item(z->pos(), "slime_scrap", 1, 0, calendar::turn, rng(1, 4));
    if( z->get_speed() <= 0) {
        if (g->u.sees(*z)) {
            //  TODO:  Add vermin-tagged tiny versions of the splattered blob  :)
            add_msg(m_good, _("The %s splatters apart."), z->name().c_str());
        }
        return;
    }
    if (g->u.sees(*z)) {
        if(z->type->dies.size() == 1) {
            add_msg(m_good, _("The %s splits in two!"), z->name().c_str());
        } else {
            add_msg(m_bad, _("Two small blobs slither out of the corpse."), z->name().c_str());
        }
    }
    std::vector <tripoint> valid;

    for( auto &&dest : g->m.points_in_radius( z->pos(), 1 ) ) {
        bool moveOK = g->m.passable( dest );
        bool monOK = g->mon_at( dest ) == -1;
        bool posOK = (g->u.pos() != dest);
        if (moveOK && monOK && posOK) {
            valid.push_back( dest );
        }
    }

    for (int s = 0; s < 2 && !valid.empty(); s++) {
        const tripoint target = random_entry_removed( valid );
        if (g->summon_mon(speed < 50 ? mon_blob_small : mon_blob, target)) {
            monster *blob = g->monster_at( target );
            blob->make_ally(z);
            blob->set_speed_base(speed);
            blob->set_hp(speed);
        }
    }
}

void mdeath::brainblob(monster *z) {
    for( size_t i = 0; i < g->num_zombies(); i++ ) {
        monster *candidate = &g->zombie( i );
        if(candidate->type->in_species( BLOB ) && candidate->type->id != mon_blob_brain ) {
            candidate->remove_effect("controlled");
        }
    }
    blobsplit(z);
}

void mdeath::jackson(monster *z) {
    for( size_t i = 0; i < g->num_zombies(); i++ ) {
        monster *candidate = &g->zombie( i );
        if(candidate->type->id == mon_zombie_dancer ) {
            candidate->poly( mon_zombie_hulk );
            candidate->remove_effect("controlled");
        }
        if (g->u.sees( *z )) {
            add_msg(m_warning, _("The music stops!"));
        }
    }
}

void mdeath::melt(monster *z)
{
    if (g->u.sees(*z)) {
        add_msg(m_good, _("The %s melts away."), z->name().c_str());
    }
}

void mdeath::amigara(monster *z)
{
    for (size_t i = 0; i < g->num_zombies(); i++) {
        const monster &critter = g->zombie( i );
        if( critter.type == z->type && !critter.is_dead() ) {
            return;
        }
    }

    // We were the last!
    if (g->u.has_effect("amigara")) {
        g->u.remove_effect("amigara");
        add_msg(_("Your obsession with the fault fades away..."));
    }

    g->m.spawn_artifact( z->pos() );
}

void mdeath::thing(monster *z)
{
    g->summon_mon(mon_thing, z->pos());
}

void mdeath::explode(monster *z)
{
    int size = 0;
    switch (z->type->size) {
    case MS_TINY:
        size = 4;
        break;
    case MS_SMALL:
        size = 8;
        break;
    case MS_MEDIUM:
        size = 14;
        break;
    case MS_LARGE:
        size = 20;
        break;
    case MS_HUGE:
        size = 26;
        break;
    }
    g->explosion( z->pos(), size );
}

void mdeath::focused_beam(monster *z)
{

    for (int k = g->m.i_at(z->pos()).size() - 1; k >= 0; k--) {
        if (g->m.i_at(z->pos())[k].type->id == "processor") {
            g->m.i_rem(z->pos(), k);
        }
    }

    if (z->inv.size() > 0) {

        if (g->u.sees(*z)) {
            add_msg(m_warning, _("As the final light is destroyed, it erupts in a blinding flare!"));
        }

        item &settings = z->inv[0];

        int x = z->posx() + settings.get_var( "SL_SPOT_X", 0 );
        int y = z->posy() + settings.get_var( "SL_SPOT_Y", 0 );
        tripoint p( x, y, z->posz() );

        std::vector <tripoint> traj = line_to(z->pos(), p, 0, 0);
        for( auto &elem : traj ) {
            if( !g->m.trans( elem ) ) {
                break;
            }
            g->m.add_field( elem, fd_dazzling, 2, 0 );
        }
    }

    z->inv.clear();

    g->explosion( z->pos(), 8 );
}

void mdeath::broken(monster *z) {
    // Bail out if flagged (simulates eyebot flying away)
    if (z->no_corpse_quiet) {
        return;
    }
    std::string item_id = z->type->id.str();
    if (item_id.compare(0, 4, "mon_") == 0) {
        item_id.erase(0, 4);
    }
    // make "broken_manhack", or "broken_eyebot", ...
    item_id.insert(0, "broken_");
    g->m.spawn_item(z->pos(), item_id, 1, 0, calendar::turn);
    if (g->u.sees(z->pos())) {
        add_msg(m_good, _("The %s collapses!"), z->name().c_str());
    }
}

void mdeath::ratking(monster *z)
{
    g->u.remove_effect("rat");
    if (g->u.sees(*z)) {
        add_msg(m_warning, _("Rats suddenly swarm into view."));
    }

    std::vector <tripoint> ratspots;
    for( auto &&ratp : g->m.points_in_radius( z->pos(), 1 ) ) {
        if (g->is_empty(ratp)) {
            ratspots.push_back(ratp);
        }
    }
    for (int rats = 0; rats < 7 && !ratspots.empty(); rats++) {
        g->summon_mon( mon_sewer_rat, random_entry_removed( ratspots ) );
    }
}

void mdeath::darkman(monster *z)
{
    g->u.remove_effect("darkness");
    if (g->u.sees(*z)) {
        add_msg(m_good, _("The %s melts away."), z->name().c_str());
    }
}

void mdeath::gas(monster *z)
{
    std::string explode = string_format(_("a %s explode!"), z->name().c_str());
    sounds::sound(z->pos(), 24, explode);
    for( auto &&dest : g->m.points_in_radius( z->pos(), 2 ) ) {
        g->m.add_field(dest, fd_toxic_gas, 3, 0);
        int mondex = g->mon_at(dest);
        if (mondex != -1) {
            g->zombie(mondex).stumble();
            g->zombie(mondex).moves -= 250;
        }
    }
}

void mdeath::smokeburst(monster *z)
{
    std::string explode = string_format(_("a %s explode!"), z->name().c_str());
    sounds::sound(z->pos(), 24, explode);
    for( auto &&dest : g->m.points_in_radius( z->pos(), 1 ) ) {
        g->m.add_field( dest, fd_smoke, 3, 0 );
        int mondex = g->mon_at( dest );
        if (mondex != -1) {
            g->zombie(mondex).stumble();
            g->zombie(mondex).moves -= 250;
        }
    }
}

void mdeath::jabberwock(monster *z)
{
    player *ch = dynamic_cast<player*>( z->get_killer() );
    if( ch != nullptr && ch->is_player() && rl_dist( z->pos(), g->u.pos() ) <= 1  &&
         ch->weapon.has_flag("VORPAL")) {
        static const matec_id VORPAL( "VORPAL" );
        if (!ch->weapon.has_technique( VORPAL )) {
            if (g->u.sees(*z)) {
                //~ %s is the possessive form of the monster's name
                add_msg(m_info, _("As the flames in %s eyes die out, your weapon seems to shine slightly brighter."),
                        z->disp_name(true).c_str());
            }
            ch->weapon.add_technique( VORPAL );
        }
    }
    mdeath::normal(z);
}

void mdeath::gameover(monster *z)
{
    add_msg(m_bad, _("The %s was destroyed!  GAME OVER!"), z->name().c_str());
    g->u.hp_cur[hp_torso] = 0;
}

void mdeath::kill_breathers(monster *z)
{
    (void)z; //unused
    for (size_t i = 0; i < g->num_zombies(); i++) {
        const mtype_id& monID = g->zombie(i).type->id;
        if (monID == mon_breather_hub || monID == mon_breather) {
            g->zombie(i).die( nullptr );
        }
    }
}

void mdeath::detonate(monster *z)
{
    weighted_int_list<std::string> amm_list;
    for (auto amm : z->ammo) {
        amm_list.add(amm.first, amm.second);
    }

    std::vector<std::string> pre_dets;
    for (int i = 0; i < 3; i++) {
        if (amm_list.get_weight() <= 0) {
            break;
        }
        // Grab one item
        std::string tmp = *amm_list.pick();
        // and reduce its weight by 1
        amm_list.add_or_replace(tmp, amm_list.get_specific_weight(tmp) - 1);
        // and stash it for use
        pre_dets.push_back(tmp);
    }

    // Update any hardcoded explosion equivalencies
    std::vector<std::pair<std::string, long>> dets;
    for (auto bomb_id : pre_dets) {
        if (bomb_id == "bot_grenade_hack") {
            dets.push_back(std::make_pair("grenade_act", 5));
        } else if (bomb_id == "bot_flashbang_hack") {
            dets.push_back(std::make_pair("flashbang_act", 5));
        } else if (bomb_id == "bot_gasbomb_hack") {
            dets.push_back(std::make_pair("gasbomb_act", 20));
        } else if (bomb_id == "bot_c4_hack") {
            dets.push_back(std::make_pair("c4armed", 10));
        } else if (bomb_id == "bot_mininuke_hack") {
            dets.push_back(std::make_pair("mininuke_act", 20));
        } else {
            // Get the transformation item
            const iuse_transform *actor = dynamic_cast<const iuse_transform *>(
                            item::find_type(bomb_id)->get_use( "transform" )->get_actor_ptr() );
            if( actor == nullptr ) {
                // Invalid bomb item, move to the next ammo item
                add_msg(m_debug, "Invalid bomb type in detonate mondeath for %s.", z->name().c_str());
                continue;
            }
            dets.push_back(std::make_pair(actor->target_id, actor->target_charges));
        }
    }


    if (g->u.sees(*z)) {
        if (dets.empty()) {
            //~ %s is the possessive form of the monster's name
            add_msg(m_info, _("The %s's hands fly to its pockets, but there's nothing left in them."), z->name().c_str());
        } else {
            //~ %s is the possessive form of the monster's name
            add_msg(m_bad, _("The %s's hands fly to its remaining pockets, opening them!"), z->name().c_str());
        }
    }
    const tripoint det_point = z->pos();
    // HACK, used to stop them from having ammo on respawn
    z->add_effect("no_ammo", 1, num_bp, true);

    // First die normally
    mdeath::normal(z);
    // Then detonate our suicide bombs
    for (auto bombs : dets) {
        item bomb_item(bombs.first, 0);
        bomb_item.charges = bombs.second;
        bomb_item.active = true;
        g->m.add_item_or_charges(z->pos(), bomb_item);
    }
}

void mdeath::broken_ammo(monster *z)
{
    if (g->u.sees(z->pos())) {
        //~ %s is the possessive form of the monster's name
        add_msg(m_info, _("The %s's interior compartment sizzles with destructive energy."),
                            z->name().c_str());
    }
    mdeath::broken(z);
}

void make_gibs(monster *z, int amount)
{
    if (amount <= 0) {
        return;
    }

    field_id type_blood = z->bloodType();

    const auto random_pt = []( const tripoint &p ) {
        return tripoint( p.x + rng( -1, 1 ), p.y + rng( -1, 1 ), p.z );
    };

    for (int i = 0; i < amount; i++) {
        // leave gibs, if there are any
        tripoint pt = random_pt( z->pos() );
        const int gibDensity = rng(1, i + 1);
        if( z->gibType() != fd_null ) {
            if(  g->m.clear_path( z->pos(), pt, 2, 1, 100 ) ) {
                // Only place gib if there's a clear path for it to get there.
                g->m.add_field( pt, z->gibType(), gibDensity, 0 );
            }
        }
        pt = random_pt( z->pos() );
        if( type_blood != fd_null ) {
            if( g->m.clear_path( z->pos(), pt, 2, 1, 100 ) ) {
                // Only place blood if there's a clear path for it to get there.
                g->m.add_field( pt, type_blood, 1, 0 );
            }
        }
    }
}

void make_mon_corpse(monster *z, int damageLvl)
{
    const int MAX_DAM = 4;
    item corpse;
    corpse.make_corpse( z->type->id, calendar::turn, z->unique_name );
    corpse.damage = damageLvl > MAX_DAM ? MAX_DAM : damageLvl;
    if( z->has_effect("pacified") && z->type->in_species( ZOMBIE ) ) {
        // Pacified corpses have a chance of becoming un-pacified when regenerating.
        corpse.set_var( "zlave", one_in(2) ? "zlave" : "mutilated" );
    }
    if (z->has_effect("no_ammo")) {
        corpse.set_var("no_ammo", "no_ammo");
    }
    g->m.add_item_or_charges(z->pos(), corpse);
}

void mdeath::preg_roach( monster *z )
{
    int num_roach = rng( 1, 3 );
    std::vector <tripoint> roachspots;
    for( const auto &roachp : g->m.points_in_radius( z->pos(), 1 ) ) {
        if( g->is_empty( roachp ) ) {
            roachspots.push_back( roachp );
        }
    }

    while( !roachspots.empty() ) {
        const tripoint target = random_entry_removed( roachspots );
        if( -1 == g->mon_at( target ) ) {
            g->summon_mon( mon_giant_cockroach_nymph, target );
            num_roach--;
            if( g->u.sees(*z) ) {
                add_msg(m_warning, _("A cockroach nymph crawls out of the pregnant giant cockroach corpse."));
            }
        }
        if( num_roach == 0 ) {
            break;
        }
    }
}
