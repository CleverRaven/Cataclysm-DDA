#include "debug.h"
#include "game.h"
#include "player.h"
#include "player_activity.h"
#include "translations.h"

#include <sstream>

#define dbg(x) DebugLog((DebugLevel)(x),D_GAME) << __FILE__ << ":" << __LINE__ << ": "


extern game *g;


// activity_item_handling.cpp
void activity_on_turn_drop();
void activity_on_turn_move_items();
void activity_on_turn_pickup();
void activity_on_turn_stash();

// advanced_inv.cpp
void advanced_inv();

// veh_interact.cpp
void complete_vehicle();


player_activity::player_activity(activity_type t, int turns, int Index, int pos,
                                 std::string name_in) :
    JsonSerializer(), JsonDeserializer(), type(t), moves_left(turns), index(Index),
    position(pos), name(name_in), ignore_trivial(false), values(), str_values(),
    placement(-1, -1), warned_of_proximity(false), auto_resume(false)
{
}

const std::string &player_activity::get_stop_phrase() const
{
    static const std::string stop_phrase[NUM_ACTIVITIES] = {
        _(" Stop?"), _(" Stop reloading?"),
        _(" Stop reading?"), _(" Stop playing?"),
        _(" Stop waiting?"), _(" Stop crafting?"),
        _(" Stop crafting?"), _(" Stop disassembly?"),
        _(" Stop butchering?"), _(" Stop salvaging?"), _(" Stop foraging?"),
        _(" Stop construction?"), _(" Stop construction?"),
        _(" Stop pumping gas?"), _(" Stop training?"),
        _(" Stop waiting?"), _(" Stop using first aid?"),
        _(" Stop fishing?"), _(" Stop mining?"), _(" Stop burrowing?"),
        _(" Stop smashing?"), _(" Stop de-stressing?"),
        _(" Stop cutting tissues?"), _(" Stop dropping?"),
        _(" Stop stashing?"), _(" Stop picking up?"),
        _(" Stop moving items?"),
        _(" Stop interacting with inventory?"),
        _(" Stop lighting the fire?"), _(" Stop filling the container?"),
        _(" Stop hotwiring the vehicle?"),
        _(" Stop aiming?")
    };
    return stop_phrase[type];
}

bool player_activity::is_abortable() const
{
    switch(type) {
        case ACT_READ:
        case ACT_BUILD:
        case ACT_CRAFT:
        case ACT_LONGCRAFT:
        case ACT_REFILL_VEHICLE:
        case ACT_WAIT:
        case ACT_WAIT_WEATHER:
        case ACT_FIRSTAID:
        case ACT_PICKAXE:
        case ACT_BURROW:
        case ACT_PULP:
        case ACT_MAKE_ZLAVE:
        case ACT_DROP:
        case ACT_STASH:
        case ACT_PICKUP:
        case ACT_HOTWIRE_CAR:
        case ACT_MOVE_ITEMS:
        case ACT_ADV_INVENTORY:
        case ACT_START_FIRE:
        case ACT_FILL_LIQUID:
            return true;
        default:
            return false;
    }
}


bool player_activity::is_complete() const
{
    return moves_left <= 0;
}


bool player_activity::is_suspendable() const
{
    switch(type) {
        case ACT_NULL:
        case ACT_RELOAD:
        case ACT_DISASSEMBLE:
        case ACT_MAKE_ZLAVE:
        case ACT_DROP:
        case ACT_STASH:
        case ACT_PICKUP:
        case ACT_MOVE_ITEMS:
        case ACT_ADV_INVENTORY:
        case ACT_AIM:
            return false;
        default:
            return true;
    }
}


int player_activity::get_value(size_t index, int def) const
{
    return (index < values.size()) ? values[index] : def;
}

std::string player_activity::get_str_value(size_t index, std::string def) const
{
    return (index < str_values.size()) ? str_values[index] : def;
}

void player_activity::do_turn()
{
    switch (type) {
        case ACT_WAIT:
        case ACT_WAIT_WEATHER:
            // Based on time, not speed
            moves_left -= 100;
            g->u.rooted();
            g->u.pause();
            break;
        case ACT_PICKAXE:
            // Based on speed, not time
            moves_left -= g->u.moves;
            g->u.moves = 0;
            pickaxe_do_turn(&g->u);
            break;
        case ACT_BURROW:
            // Based on speed, not time
            moves_left -= g->u.moves;
            g->u.moves = 0;
            burrow_do_turn(&g->u);
            break;
        case ACT_AIM:
            if( index == 0 ) {
                g->plfire(false);
            }
            break;
        case ACT_GAME:
            // Takes care of u.activity.moves_left
            game_do_turn();
            break;
        case ACT_VIBE:
            // Takes care of u.activity.moves_left
            vibe_do_turn();
            break;
        case ACT_REFILL_VEHICLE:
            // Takes care of u.activity.moves_left
            refill_vehicle_do_turn();
            break;
        case ACT_PULP:
            // does not really use u.activity.moves_left, stops itself when finished
            pulp_do_turn();
            break;
        case ACT_FISH:
            // Based on time, not speed--or it should be
            // (Being faster doesn't make the fish bite quicker)
            moves_left -= 100;
            g->u.rooted();
            g->u.pause();
            break;
        case ACT_DROP:
            activity_on_turn_drop();
            break;
        case ACT_STASH:
            activity_on_turn_stash();
            break;
        case ACT_PICKUP:
            activity_on_turn_pickup();
            break;
        case ACT_MOVE_ITEMS:
            activity_on_turn_move_items();
            break;
        case ACT_ADV_INVENTORY:
            g->u.cancel_activity();
            advanced_inv();
            break;
        case ACT_START_FIRE:
            moves_left -= 100; // based on time
            if (g->u.i_at(position).has_flag("LENS")) { // if using a lens, handle potential changes in weather
                start_fire_lens_do_turn();
            }
            g->u.rooted();
            g->u.pause();
            break;
        case ACT_FILL_LIQUID:
            fill_liquid_do_turn();
            break;
        default:
            // Based on speed, not time
            if( g->u.moves <= moves_left ) {
                moves_left -= g->u.moves;
                g->u.moves = 0;
            } else {
                g->u.moves -= moves_left;
                moves_left = 0;
            }
    }

    if (is_complete()) {
        finish();
    }
}


void player_activity::finish()
{
    switch (type) {
        case ACT_RELOAD:
            reload_finish();
            break;
        case ACT_READ:
            g->u.do_read(&(g->u.i_at(position)));
            if (type == ACT_NULL) {
                add_msg(_("You finish reading."));
            }
            break;
        case ACT_WAIT:
        case ACT_WAIT_WEATHER:
            add_msg(_("You finish waiting."));
            type = ACT_NULL;
            break;
        case ACT_CRAFT:
            g->u.complete_craft();
            type = ACT_NULL;
            break;
        case ACT_LONGCRAFT:
            g->u.complete_craft();
            type = ACT_NULL;
            {
                int batch_size = values.front();
                if( g->u.making_would_work( g->u.lastrecipe, batch_size ) ) {
                    g->u.make_all_craft(g->u.lastrecipe, batch_size);
                }
            }
            break;
        case ACT_FORAGE:
            forage_finish();
            type = ACT_NULL;
            break;
        case ACT_DISASSEMBLE:
            g->u.complete_disassemble();
            type = ACT_NULL;
            break;
        case ACT_BUTCHER:
            butcher_finish();
            type = ACT_NULL;
            break;
        case ACT_LONGSALVAGE:
            longsalvage_finish();
            break;
        case ACT_VEHICLE:
            vehicle_finish();
            break;
        case ACT_BUILD:
            complete_construction();
            type = ACT_NULL;
            break;
        case ACT_TRAIN:
            train_finish();
            break;
        case ACT_FIRSTAID:
            firstaid_finish();
            break;
        case ACT_FISH:
            fish_finish();
            break;
        case ACT_PICKAXE:
            pickaxe_finish(&g->u);
            type = ACT_NULL;
            break;
        case ACT_BURROW:
            burrow_finish(&g->u);
            type = ACT_NULL;
            break;
        case ACT_VIBE:
            add_msg(m_good, _("You feel much better."));
            type = ACT_NULL;
            break;
        case ACT_MAKE_ZLAVE:
            make_zlave_finish();
            type = ACT_NULL;
            break;
        case ACT_PICKUP:
        case ACT_MOVE_ITEMS:
            // Do nothing, the only way this happens is if we set this activity after
            // entering the advanced inventory menu as an activity, and we want it to play out.
            break;
        case ACT_START_FIRE:
            start_fire_finish();
            break;
        case ACT_HOTWIRE_CAR:
            hotwire_finish();
        case ACT_AIM:
            // Aim bails itself by resetting itself every turn,
            // you only re-enter if it gets set again.
            break;
        default:
            type = ACT_NULL;
    }
    if (type == ACT_NULL) {
        // Make sure data of previous activity is cleared
        g->u.activity = player_activity();
        if( !g->u.backlog.empty() && g->u.backlog.front().auto_resume ) {
            g->u.activity = g->u.backlog.front();
            g->u.backlog.pop_front();
        }
    }
}


void player_activity::burrow_do_turn(player *p)
{
    if (calendar::turn % MINUTES(1) == 0) { // each turn is too much
        //~ Sound of a Rat mutant burrowing!
        g->sound(placement.x, placement.y, 10, _("ScratchCrunchScrabbleScurry."));
        if (moves_left <= 91000 && moves_left > 89000) {
            p->add_msg_if_player(m_info, _("You figure it'll take about an hour and a half at this rate."));
        }
        if (moves_left <= 71000 && moves_left > 69000) {
            p->add_msg_if_player(m_info, _("About an hour left to go."));
        }
        if (moves_left <= 31000 && moves_left > 29000) {
            p->add_msg_if_player(m_info, _("Shouldn't be more than half an hour or so now!"));
        }
        if (moves_left <= 11000 && moves_left > 9000) {
            p->add_msg_if_player(m_info, _("Almost there! Ten more minutes of work and you'll be through."));
        }
    }
}


void player_activity::burrow_finish(player *p)
{
    const int dirx = placement.x;
    const int diry = placement.y;
    if (g->m.is_bashable(dirx, diry) && g->m.has_flag("SUPPORTS_ROOF", dirx, diry) &&
        g->m.ter(dirx, diry) != t_tree) {
        // Tunneling through solid rock is hungry, sweaty, tiring, backbreaking work
        // Not quite as bad as the pickaxe, though
        p->hunger += 10;
        p->fatigue += 15;
        p->thirst += 10;
        p->mod_pain(3 * rng(1, 3));
        // Mining is construction work!
        p->practice("carpentry", 5);
    } else if (g->m.move_cost(dirx, diry) == 2 && g->levz == 0 &&
               g->m.ter(dirx, diry) != t_dirt && g->m.ter(dirx, diry) != t_grass) {
        //Breaking up concrete on the surface? not nearly as bad
        p->hunger += 5;
        p->fatigue += 10;
        p->thirst += 5;
    }
    g->m.destroy(dirx, diry, true);
}


void player_activity::butcher_finish()
{
    // corpses can disappear (rezzing!), so check for that
    if (static_cast<int>(g->m.i_at(g->u.posx(), g->u.posy()).size()) <= index ||
        g->m.i_at(g->u.posx(), !g->u.posy())[index].is_corpse() ||
        g->m.i_at(g->u.posx(), g->u.posy())[index].typeId() != "corpse") {
        add_msg(m_info, _("There's no corpse to butcher!"));
        return;
    }
    mtype *corpse = g->m.i_at(g->u.posx(), g->u.posy())[index].get_mtype();
    std::vector<item> contents = g->m.i_at(g->u.posx(), g->u.posy())[index].contents;
    int age = g->m.i_at(g->u.posx(), g->u.posy())[index].bday;
    g->m.i_rem(g->u.posx(), g->u.posy(), index);
    int factor = g->u.butcher_factor();
    int pieces = 0, skins = 0, bones = 0, fats = 0, sinews = 0, feathers = 0;
    double skill_shift = 0.;

    int sSkillLevel = g->u.skillLevel("survival");

    switch (corpse->size) {
        case MS_TINY:
            pieces = 1;
            skins = 1;
            bones = 1;
            fats = 1;
            sinews = 1;
            feathers = 2;
            break;
        case MS_SMALL:
            pieces = 2;
            skins = 2;
            bones = 4;
            fats = 2;
            sinews = 4;
            feathers = 6;
            break;
        case MS_MEDIUM:
            pieces = 4;
            skins = 4;
            bones = 9;
            fats = 4;
            sinews = 9;
            feathers = 11;
            break;
        case MS_LARGE:
            pieces = 8;
            skins = 8;
            bones = 14;
            fats = 8;
            sinews = 14;
            feathers = 17;
            break;
        case MS_HUGE:
            pieces = 16;
            skins = 16;
            bones = 21;
            fats = 16;
            sinews = 21;
            feathers = 24;
            break;
    }

    skill_shift += rng(0, sSkillLevel - 3);
    skill_shift += rng(0, g->u.dex_cur - 8) / 4;
    if (g->u.str_cur < 4) {
        skill_shift -= rng(0, 5 * (4 - g->u.str_cur)) / 4;
    }
    if( factor < 0 ) {
        skill_shift -= rng( 0, -factor / 5 );
    }

    int practice = 4 + pieces;
    if (practice > 20) {
        practice = 20;
    }
    g->u.practice("survival", practice);

    pieces += int(skill_shift);
    if (skill_shift < 5)  { // Lose some skins and bones
        skins += ((int)skill_shift - 4);
        bones += ((int)skill_shift - 2);
        fats += ((int)skill_shift - 4);
        sinews += ((int)skill_shift - 8);
        feathers += ((int)skill_shift - 1);
    }

    if (bones > 0) {
        if (corpse->mat == "veggy") {
            g->m.spawn_item(g->u.posx(), g->u.posy(), "plant_sac", bones, 0, age);
            add_msg(m_good, _("You harvest some fluid bladders!"));
        } else if (corpse->has_flag(MF_BONES) && corpse->has_flag(MF_POISON)) {
            g->m.spawn_item(g->u.posx(), g->u.posy(), "bone_tainted", bones / 2, 0, age);
            add_msg(m_good, _("You harvest some salvageable bones!"));
        } else if (corpse->has_flag(MF_BONES) && corpse->has_flag(MF_HUMAN)) {
            g->m.spawn_item(g->u.posx(), g->u.posy(), "bone_human", bones, 0, age);
            add_msg(m_good, _("You harvest some salvageable bones!"));
        } else if (corpse->has_flag(MF_BONES)) {
            g->m.spawn_item(g->u.posx(), g->u.posy(), "bone", bones, 0, age);
            add_msg(m_good, _("You harvest some usable bones!"));
        }
    }

    if (sinews > 0) {
        if (corpse->has_flag(MF_BONES) && !corpse->has_flag(MF_POISON)) {
            g->m.spawn_item(g->u.posx(), g->u.posy(), "sinew", sinews, 0, age);
            add_msg(m_good, _("You harvest some usable sinews!"));
        } else if (corpse->mat == "veggy") {
            g->m.spawn_item(g->u.posx(), g->u.posy(), "plant_fibre", sinews, 0, age);
            add_msg(m_good, _("You harvest some plant fibers!"));
        }
    }

    if ((corpse->has_flag(MF_FUR) || corpse->has_flag(MF_LEATHER) ||
         corpse->has_flag(MF_CHITIN)) && skins > 0) {
        add_msg(m_good, _("You manage to skin the %s!"), corpse->nname().c_str());
        int fur = 0;
        int leather = 0;
        int chitin = 0;

        while (skins > 0) {
            if (corpse->has_flag(MF_CHITIN)) {
                chitin = rng(0, skins);
                skins -= chitin;
                skins = std::max(skins, 0);
            }
            if (corpse->has_flag(MF_FUR)) {
                fur = rng(0, skins);
                skins -= fur;
                skins = std::max(skins, 0);
            }
            if (corpse->has_flag(MF_LEATHER)) {
                leather = rng(0, skins);
                skins -= leather;
                skins = std::max(skins, 0);
            }
        }

        if (chitin) {
            g->m.spawn_item(g->u.posx(), g->u.posy(), "chitin_piece", chitin, 0, age);
        }
        if (fur) {
            g->m.spawn_item(g->u.posx(), g->u.posy(), "raw_fur", fur, 0, age);
        }
        if (leather) {
            g->m.spawn_item(g->u.posx(), g->u.posy(), "raw_leather", leather, 0, age);
        }
    }

    if (feathers > 0) {
        if (corpse->has_flag(MF_FEATHER)) {
            g->m.spawn_item(g->u.posx(), g->u.posy(), "feather", feathers, 0, age);
            add_msg(m_good, _("You harvest some feathers!"));
        }
    }

    if (fats > 0) {
        if (corpse->has_flag(MF_FAT) && corpse->has_flag(MF_POISON)) {
            g->m.spawn_item(g->u.posx(), g->u.posy(), "fat_tainted", fats, 0, age);
            add_msg(m_good, _("You harvest some gooey fat!"));
        } else if (corpse->has_flag(MF_FAT)) {
            g->m.spawn_item(g->u.posx(), g->u.posy(), "fat", fats, 0, age);
            add_msg(m_good, _("You harvest some fat!"));
        }
    }

    //Add a chance of CBM recovery. For shocker and cyborg corpses.
    if (corpse->has_flag(MF_CBM_CIV)) {
        //As long as the factor is above -4 (the sinew cutoff), you will be able to extract cbms
        if (skill_shift >= 0) {
            add_msg(m_good, _("You discover a CBM in the %s!"), corpse->nname().c_str());
            //To see if it spawns a battery
            if (rng(0, 1) == 1) { //The battery works
                g->m.spawn_item(g->u.posx(), g->u.posy(), "bio_power_storage", 1, 0, age);
            } else { //There is a burnt out CBM
                g->m.spawn_item(g->u.posx(), g->u.posy(), "burnt_out_bionic", 1, 0, age);
            }
        }
        if (skill_shift >= 0) {
            //To see if it spawns a random additional CBM
            if (rng(0, 1) == 1) { //The CBM works
                g->m.put_items_from_loc( "bionics_common", g->u.posx(), g->u.posy(), age );
            } else { //There is a burnt out CBM
                g->m.spawn_item(g->u.posx(), g->u.posy(), "burnt_out_bionic", 1, 0, age);
            }
        }
    }

    // Zombie scientist bionics
    if (corpse->has_flag(MF_CBM_SCI)) {
        //As long as the factor is above -4 (the sinew cutoff), you will be able to extract cbms
        if (skill_shift >= 0) {
            add_msg(m_good, _("You discover a CBM in the %s!"), corpse->nname().c_str());
            //To see if it spawns a battery
            if (rng(0, 1) == 1) { //The battery works
                g->m.spawn_item(g->u.posx(), g->u.posy(), "bio_power_storage", 1, 0, age);
            } else { //There is a burnt out CBM
                g->m.spawn_item(g->u.posx(), g->u.posy(), "burnt_out_bionic", 1, 0, age);
            }
        }
        if (skill_shift >= 0) {
            //To see if it spawns a random additional CBM
            if (rng(0, 1) == 1) { //The CBM works
                g->m.put_items_from_loc( "bionics_sci", g->u.posx(), g->u.posy(), age );
            } else { //There is a burnt out CBM
                g->m.spawn_item(g->u.posx(), g->u.posy(), "burnt_out_bionic", 1, 0, age);
            }
        }
    }

    // Zombie technician bionics
    if (corpse->has_flag(MF_CBM_TECH)) {
        if (skill_shift >= 0) {
            add_msg(m_good, _("You discover a CBM in the %s!"), corpse->nname().c_str());
            //To see if it spawns a battery
            if (rng(0, 1) == 1) { //The battery works
                g->m.spawn_item(g->u.posx(), g->u.posy(), "bio_power_storage", 1, 0, age);
            } else { //There is a burnt out CBM
                g->m.spawn_item(g->u.posx(), g->u.posy(), "burnt_out_bionic", 1, 0, age);
            }
        }
        if (skill_shift >= 0) {
            //To see if it spawns a random additional CBM
            if (rng(0, 1) == 1) { //The CBM works
                g->m.put_items_from_loc( "bionics_tech", g->u.posx(), g->u.posy(), age );
            } else { //There is a burnt out CBM
                g->m.spawn_item(g->u.posx(), g->u.posy(), "burnt_out_bionic", 1, 0, age);
            }
        }
    }

    // Substation mini-boss bionics
    if (corpse->has_flag(MF_CBM_SUBS)) {
        if (skill_shift >= 0) {
            add_msg(m_good, _("You discover a CBM in the %s!"), corpse->nname().c_str());
            //To see if it spawns a battery
            if (rng(0, 1) == 1) { //The battery works
                g->m.spawn_item(g->u.posx(), g->u.posy(), "bio_power_storage", 1, 0, age);
            } else { //There is a burnt out CBM
                g->m.spawn_item(g->u.posx(), g->u.posy(), "burnt_out_bionic", 1, 0, age);
            }
        }
        if (skill_shift >= 0) {
            //To see if it spawns a random additional CBM
            if (rng(0, 1) == 1) { //The CBM works
                g->m.put_items_from_loc( "bionics_subs", g->u.posx(), g->u.posy(), age );
            } else { //There is a burnt out CBM
                g->m.spawn_item(g->u.posx(), g->u.posy(), "burnt_out_bionic", 1, 0, age);
            }
        }
        if (skill_shift >= 0) {
            //To see if it spawns a random additional CBM
            if (rng(0, 1) == 1) { //The CBM works
                g->m.put_items_from_loc( "bionics_subs", g->u.posx(), g->u.posy(), age );
            } else { //There is a burnt out CBM
                g->m.spawn_item(g->u.posx(), g->u.posy(), "burnt_out_bionic", 1, 0, age);
            }
        }
    }

    // Payoff for butchering the zombie bio-op
    if (corpse->has_flag(MF_CBM_OP)) {
        //As long as the factor is above -4 (the sinew cutoff), you will be able to extract cbms
        if (skill_shift >= 0) {
            add_msg(m_good, _("You discover a CBM in the %s!"), corpse->nname().c_str());
            //To see if it spawns a battery
            if (rng(0, 1) == 1) { //The battery works
                g->m.spawn_item(g->u.posx(), g->u.posy(), "bio_power_storage_mkII", 1, 0, age);
            } else { //There is a burnt out CBM
                g->m.spawn_item(g->u.posx(), g->u.posy(), "burnt_out_bionic", 1, 0, age);
            }
        }
        if (skill_shift >= 0) {
            //To see if it spawns a random additional CBM
            if (rng(0, 1) == 1) { //The CBM works
                g->m.put_items_from_loc( "bionics_op", g->u.posx(), g->u.posy(), age );
            } else { //There is a burnt out CBM
                g->m.spawn_item(g->u.posx(), g->u.posy(), "burnt_out_bionic", 1, 0, age);
            }
        }
    }

    //Add a chance of CBM power storage recovery.
    if (corpse->has_flag(MF_CBM_POWER)) {
        //As long as the factor is above -4 (the sinew cutoff), you will be able to extract cbms
        if (skill_shift >= 0) {
            //To see if it spawns a battery
            if (one_in(3)) { //The battery works 33% of the time.
                add_msg(m_good, _("You discover a power storage in the %s!"), corpse->nname().c_str());
                g->m.spawn_item(g->u.posx(), g->u.posy(), "bio_power_storage", 1, 0, age);
            } else { //There is a burnt out CBM
                add_msg(m_good, _("You discover a fused lump of bio-circuitry in the %s!"),
                        corpse->nname().c_str());
                g->m.spawn_item(g->u.posx(), g->u.posy(), "burnt_out_bionic", 1, 0, age);
            }
        }
    }


    // Recover hidden items
    for( auto & content : contents ) {
        if ((skill_shift + 10) * 5 > rng(0, 100)) {
            add_msg( m_good, _( "You discover a %s in the %s!" ), content.tname().c_str(),
                     corpse->nname().c_str() );
            g->m.add_item_or_charges( g->u.posx(), g->u.posy(), content );
        } else if( content.is_bionic() ) {
            g->m.spawn_item(g->u.posx(), g->u.posy(), "burnt_out_bionic", 1, 0, age);
        }
    }

    if (pieces <= 0) {
        add_msg(m_bad, _("Your clumsy butchering destroys the meat!"));
    } else {
        add_msg(m_good, _("You butcher the corpse."));
        const itype_id meat = corpse->get_meat_itype();
        if( meat == "null" ) {
            return;
        }
        item tmpitem(meat, age);
        tmpitem.set_mtype( corpse );
        while ( pieces > 0 ) {
            pieces--;
            g->m.add_item_or_charges(g->u.posx(), g->u.posy(), tmpitem);
        }
    }
}


void player_activity::fill_liquid_do_turn()
{
    //Filling a container takes time, not speed
    moves_left -= 100;

    item *container = &g->u.i_at(position);
    item water = item(str_values[0], values[1]);
    water.poison = values[0];
    // Fill up 10 charges per time
    water.charges = 10;

    if (g->handle_liquid(water, true, true, NULL, container) == false) {
        moves_left = 0;
    }

    g->u.rooted();
    g->u.pause();
}


void player_activity::firstaid_finish()
{
    item &it = g->u.i_at(position);
    iuse tmp;
    tmp.completefirstaid(&g->u, &it, false, g->u.pos());
    g->u.reduce_charges(position, 1);
    // Erase activity and values.
    type = ACT_NULL;
    values.clear();
}


static void rod_fish(int sSkillLevel, int fishChance) // fish-with-rod fish catching function
{
    if (sSkillLevel > fishChance) {
        std::vector<monster *> fishables = g->get_fishable(60); //get the nearby fish list.
        //if the vector is empty (no fish around) the player is still given a small chance to get a (let us say it was hidden) fish
        if (fishables.size() < 1) {
            if (one_in(20)) {
                item fish;
                std::vector<std::string> fish_group = MonsterGroupManager::GetMonstersFromGroup("GROUP_FISH");
                std::string fish_mon = fish_group[rng(1, fish_group.size()) - 1];
                fish.make_corpse(GetMType(fish_mon), calendar::turn);
                g->m.add_item_or_charges(g->u.posx(), g->u.posy(), fish);
                g->u.add_msg_if_player(m_good, _("You caught a %s."), GetMType(fish_mon)->nname().c_str());
            } else {
                g->u.add_msg_if_player(_("You didn't catch anything."));
            }
        } else {
            g->catch_a_monster(fishables, g->u.posx(), g->u.posy(), &g->u, 30000);
        }

    } else {
        g->u.add_msg_if_player(_("You didn't catch anything."));
    }
}

void player_activity::fish_finish()
{
    item &it = g->u.i_at(g->u.activity.position);
    int sSkillLevel = 0;
    int fishChance = 20;
    if (it.has_flag("FISH_POOR")) {
        sSkillLevel = g->u.skillLevel("survival") + dice(1, 6);
        fishChance = dice(1, 20);
    } else if (it.has_flag("FISH_GOOD")) {
        // Much better chances with a good fishing implement.
        sSkillLevel = g->u.skillLevel("survival")*1.5 + dice(1, 6) + 3;
        fishChance = dice(1, 20);
    }
    rod_fish(sSkillLevel,fishChance);
    g->u.practice("survival", rng(5, 15));
    g->u.activity.type = ACT_NULL;
}

void player_activity::forage_finish()
{
    int veggy_chance = rng(1, 100);
    bool found_something = false;

    if (one_in(12)) {
        add_msg(m_good, _("You found some trash!"));
        g->m.put_items_from_loc( "trash_forest", g->u.posx(), g->u.posy(), calendar::turn );
        found_something = true;
    }
    // Compromise: Survival gives a bigger boost, and Peception is leveled a bit.
    if (veggy_chance < ((g->u.skillLevel("survival") * 1.5) + ((g->u.per_cur / 2 - 4) + 3))) {
        items_location loc;
        switch (calendar::turn.get_season()) {
            case SPRING:
                loc = "forage_spring";
                break;
            case SUMMER:
                loc = "forage_summer";
                break;
            case AUTUMN:
                loc = "forage_autumn";
                break;
            case WINTER:
                loc = "forage_winter";
                break;
        }
        int cnt = g->m.put_items_from_loc(loc, g->u.posx(), g->u.posy(),
                                          calendar::turn); // returns zero if location has no defined items
        if (cnt > 0) {
            add_msg(m_good, _("You found something!"));
            g->m.ter_set(placement.x, placement.y, t_dirt);
            found_something = true;
        }
    } else {
        if (one_in(2)) {
            g->m.ter_set(placement.x, placement.y, t_dirt);
        }
    }
    if (!found_something) {
        add_msg(_("You didn't find anything."));
    }

    //Determinate maximum level of skill attained by foraging using ones intelligence score
    int max_forage_skill = 0;
    if (g->u.int_cur < 4) {
        max_forage_skill = 1;
    } else if (g->u.int_cur < 6) {
        max_forage_skill = 2;
    } else if (g->u.int_cur < 8) {
        max_forage_skill = 3;
    } else if (g->u.int_cur < 11) {
        max_forage_skill = 4;
    } else if (g->u.int_cur < 15) {
        max_forage_skill = 5;
    } else if (g->u.int_cur < 20) {
        max_forage_skill = 6;
    } else if (g->u.int_cur < 26) {
        max_forage_skill = 7;
    } else if (g->u.int_cur > 25) {
        max_forage_skill = 8;
    }

    const int max_exp {(max_forage_skill * 2) - (g->u.skillLevel("survival") * 2)};
    //Award experience for foraging attempt regardless of success
    g->u.practice("survival", rng(1, max_exp), max_forage_skill);
}


void player_activity::game_do_turn()
{
    //Gaming takes time, not speed
    moves_left -= 100;

    item &game_item = g->u.i_at(position);

    //Deduct 1 battery charge for every minute spent playing
    if (int(calendar::turn) % 10 == 0) {
        game_item.charges--;
        g->u.add_morale(MORALE_GAME, 1, 100); //1 points/min, almost 2 hours to fill
    }
    if (game_item.charges == 0) {
        moves_left = 0;
        add_msg(m_info, _("The %s runs out of batteries."), game_item.tname().c_str());
    }

    g->u.rooted();
    g->u.pause();
}


void player_activity::hotwire_finish()
{
    //Grab this now, in case the vehicle gets shifted
    vehicle *veh = g->m.veh_at(values[0], values[1]);
    if (veh) {
        int mech_skill = values[2];
        if (mech_skill > (int)rng(1, 6)) {
            //success
            veh->is_locked = false;
            add_msg(_("This wire will start the engine."));
        } else if (mech_skill > (int)rng(0, 4)) {
            //soft fail
            veh->is_locked = false;
            veh->is_alarm_on = veh->has_security_working();
            add_msg(_("This wire will probably start the engine."));
        } else if (veh->is_alarm_on) {
            veh->is_locked = false;
            add_msg(_("By process of elimination, this wire will start the engine."));
        } else {
            //hard fail
            veh->is_alarm_on = veh->has_security_working();
            add_msg(_("The red wire always starts the engine, doesn't it?"));
        }
    } else {
        dbg(D_ERROR) << "game:process_activity: ACT_HOTWIRE_CAR: vehicle not found";
        debugmsg("process_activity ACT_HOTWIRE_CAR: vehicle not found");
    }
    type = ACT_NULL;
}


void player_activity::longsalvage_finish()
{
    bool has_salvage_tool = g->u.has_items_with_quality( "CUT", 1, 1 );
    if( !has_salvage_tool ) {
        add_msg(m_bad, _("You no longer have the necessary tools to keep salvaging!"));
    }

    auto items = g->m.i_at(g->u.posx(), g->u.posy());
    item salvage_tool( "toolset", calendar::turn ); // TODO: Use actual tool
    for( auto it = items.begin(); it != items.end(); ++it ) {
        if( iuse::valid_to_cut_up( &*it ) ) {
            iuse::cut_up( &g->u, &salvage_tool, &*it, false );
            g->u.assign_activity( ACT_LONGSALVAGE, 0 );
            return;
        }
    }

    add_msg(_("You finish salvaging."));
    type = ACT_NULL;
}


void player_activity::make_zlave_finish()
{
    static const int full_pulp_threshold = 4;

    auto items = g->m.i_at(g->u.posx(), g->u.posy());
    std::string corpse_name = str_values[0];
    item *body = NULL;

    for( auto it = items.begin(); it != items.end(); ++it ) {
        if( it->display_name() == corpse_name ) {
            body = &*it;
        }
    }

    if (body == NULL) {
        add_msg(m_info, _("There's no corpse to make into a zombie slave!"));
        return;
    }

    int success = values[0];

    if (success > 0) {

        g->u.practice("firstaid", rng(2, 5));
        g->u.practice("survival", rng(2, 5));

        g->u.add_msg_if_player(m_good,
                               _("You slice muscles and tendons, and remove body parts until you're confident the zombie won't be able to attack you when it reainmates."));

        body->set_var( "zlave", "zlave" );
        //take into account the chance that the body yet can regenerate not as we need.
        if (one_in(10)) {
            body->set_var( "zlave", "mutilated" );
        }

    } else {

        if (success > -20) {

            g->u.practice("firstaid", rng(3, 6));
            g->u.practice("survival", rng(3, 6));

            g->u.add_msg_if_player(m_warning,
                                   _("You hack into the corpse and chop off some body parts.  You think the zombie won't be able to attack when it reanimates."));

            success += rng(1, 20);

            if (success > 0 && !one_in(5)) {
                body->set_var( "zlave", "zlave" );
            } else {
                body->set_var( "zlave", "mutilated" );
            }

        } else {

            g->u.practice("firstaid", rng(1, 8));
            g->u.practice("survival", rng(1, 8));

            int pulp = rng(1, full_pulp_threshold);

            body->damage += pulp;

            if (body->damage >= full_pulp_threshold) {
                body->damage = full_pulp_threshold;
                body->active = false;

                g->u.add_msg_if_player(m_warning, _("You cut up the corpse too much, it is thoroughly pulped."));
            } else {
                g->u.add_msg_if_player(m_warning,
                                       _("You cut into the corpse trying to make it unable to attack, but you don't think you have it right."));
            }
        }
    }
}


void player_activity::pickaxe_do_turn(player *p)
{
    const int dirx = p->activity.placement.x;
    const int diry = p->activity.placement.y;
    if (calendar::turn % MINUTES(1) == 0) { // each turn is too much
        //~ Sound of a Pickaxe at work!
        g->sound(dirx, diry, 30, _("CHNK! CHNK! CHNK!"));
        if (p->activity.moves_left <= 91000 && p->activity.moves_left > 89000) {
            p->add_msg_if_player(m_info,
                                 _("Ugh.  You figure it'll take about an hour and a half at this rate."));
        }
        if (p->activity.moves_left <= 71000 && p->activity.moves_left > 69000) {
            p->add_msg_if_player(m_info, _("If it keeps up like this, you might be through in an hour."));
        }
        if (p->activity.moves_left <= 31000 && p->activity.moves_left > 29000) {
            p->add_msg_if_player(m_info,
                                 _("Feels like you're making good progress.  Another half an hour, maybe?"));
        }
        if (p->activity.moves_left <= 11000 && p->activity.moves_left > 9000) {
            p->add_msg_if_player(m_info, _("That's got it.  Ten more minutes of work and it's open."));
        }
    }
}


void player_activity::pickaxe_finish(player *p)
{
    const int dirx = p->activity.placement.x;
    const int diry = p->activity.placement.y;
    item *it = &p->i_at(p->activity.position);
    if (g->m.is_bashable(dirx, diry) && g->m.has_flag("SUPPORTS_ROOF", dirx, diry) &&
        g->m.ter(dirx, diry) != t_tree) {
        // Tunneling through solid rock is hungry, sweaty, tiring, backbreaking work
        // Betcha wish you'd opted for the J-Hammer ;P
        p->hunger += 15;
        if (p->has_trait("STOCKY_TROGLO")) {
            p->fatigue += 20; // Yep, dwarves can dig longer before tiring
        } else {
            p->fatigue += 30;
        }
        p->thirst += 15;
        p->mod_pain(2 * rng(1, 3));
        // Mining is construction work!
        p->practice("carpentry", 5);
    } else if (g->m.move_cost(dirx, diry) == 2 && g->levz == 0 &&
               g->m.ter(dirx, diry) != t_dirt && g->m.ter(dirx, diry) != t_grass) {
        //Breaking up concrete on the surface? not nearly as bad
        p->hunger += 5;
        p->fatigue += 10;
        p->thirst += 5;
    }
    g->m.destroy(dirx, diry, true);
    it->charges = std::max(long(0), it->charges - it->type->charges_to_use());
    if (it->charges == 0 && it->destroyed_at_zero_charges()) {
        p->i_rem(p->activity.position);
    }
}


void player_activity::pulp_do_turn()
{
    const int smashx = placement.x;
    const int smashy = placement.y;
    static const int full_pulp_threshold = 4;
    const int move_cost = int(g->u.weapon.is_null() ? 80 : g->u.weapon.attack_time() * 0.8);

    // numbers logic: a str 8 character with a butcher knife (4 bash, 18 cut)
    // should have at least a 50% chance of damaging an intact zombie corpse (75 volume).
    // a str 8 character with a baseball bat (28 bash, 0 cut) should have around a 25% chance.

    int cut_power = g->u.weapon.type->melee_cut;
    // stabbing weapons are a lot less effective at pulping
    if (g->u.weapon.has_flag("STAB") || g->u.weapon.has_flag("SPEAR")) {
        cut_power /= 2;
    }
    double pulp_power = sqrt((double)(g->u.str_cur + g->u.weapon.type->melee_dam)) *
                        sqrt((double)(cut_power + 1));
    pulp_power = std::min(pulp_power, (double)g->u.str_cur);
    pulp_power *= 20; // constant multiplier to get the chance right
    int moves = 0;
    int &num_corpses = index; // use this to collect how many corpse are pulped
    auto corpse_pile = g->m.i_at(smashx, smashy);
    for( auto corpse = corpse_pile.begin(); corpse != corpse_pile.end(); ++corpse ) {
        if (!(corpse->type->id == "corpse" && corpse->damage < full_pulp_threshold)) {
            continue; // no corpse or already pulped
        }
        int damage = pulp_power / corpse->volume();
        //Determine corpse's blood type.
        field_id type_blood = corpse->get_mtype()->bloodType();
        do {
            moves += move_cost;
            // Increase damage as we keep smashing,
            // to insure that we eventually smash the target.
            if( x_in_y(pulp_power, corpse->volume()) ) {
                corpse->damage++;
                g->u.handle_melee_wear();
            }
            // Splatter some blood around
            if (type_blood != fd_null) {
                for (int x = smashx - 1; x <= smashx + 1; x++) {
                    for (int y = smashy - 1; y <= smashy + 1; y++) {
                        if (!one_in(damage + 1) && type_blood != fd_null) {
                            g->m.add_field(x, y, type_blood, 1);
                        }
                    }
                }
            }
            if( corpse->damage >= full_pulp_threshold ) {
                corpse->damage = full_pulp_threshold;
                corpse->active = false;
                num_corpses++;
            }
            if (moves >= g->u.moves) {
                // enough for this turn;
                g->u.moves -= moves;
                return;
            }
        } while( corpse->damage < full_pulp_threshold );
    }
    // If we reach this, all corpses have been pulped, finish the activity
    moves_left = 0;
    // TODO: Factor in how long it took to do the smashing.
    add_msg(ngettext("The corpse is thoroughly pulped.",
                     "The corpses are thoroughly pulped.", num_corpses));
}


void player_activity::refill_vehicle_do_turn()
{
    vehicle *veh = NULL;
    veh = g->m.veh_at(placement.x, placement.y);
    if (!veh) {  // Vehicle must've moved or something!
        moves_left = 0;
        return;
    }
    bool fuel_pumped = false;
    for(int i = -1; i <= 1; i++) {
        for(int j = -1; j <= 1; j++) {
            if( g->m.ter(g->u.posx() + i, g->u.posy() + j) == t_gas_pump ||
                g->m.ter_at(g->u.posx() + i, g->u.posy() + j).id == "t_gas_pump_a" ||
                g->m.ter(g->u.posx() + i, g->u.posy() + j) == t_diesel_pump ) {
                auto maybe_gas = g->m.i_at(g->u.posx() + i, g->u.posy() + j);
                for( auto gas = maybe_gas.begin(); gas != maybe_gas.end(); ) {
                    if( gas->type->id == "gasoline" || gas->type->id == "diesel" ) {
                        fuel_pumped = true;
                        int lack = std::min( veh->fuel_capacity(gas->type->id) -
                                             veh->fuel_left(gas->type->id),  200 );
                        if (gas->charges > lack) {
                            veh->refill(gas->type->id, lack);
                            gas->charges -= lack;
                            moves_left -= 100;
                            gas++;
                        } else {
                            add_msg(m_bad, _("With a clang and a shudder, the pump goes silent."));
                            veh->refill (gas->type->id, gas->charges);
                            gas = maybe_gas.erase( gas );
                            moves_left = 0;
                        }
                        i = 2;
                        j = 2;
                        break;
                    }
                }
            }
        }
    }
    if( !fuel_pumped ) {
        // Can't find any fuel, give up.
        debugmsg("Can't find any fuel, cancelling pumping.");
        g->u.cancel_activity();
        return;
    }
    g->u.pause();
}



void player_activity::reload_finish()
{
    item *reloadable = nullptr;
    {
        int reloadable_pos;
        std::stringstream ss(name);
        ss >> reloadable_pos;
        reloadable = &g->u.i_at(reloadable_pos);
    }
    if (reloadable->reload(g->u, position)) {
        if (reloadable->is_gun() && reloadable->has_flag("RELOAD_ONE")) {
            if (reloadable->ammo_type() == "bolt") {
                add_msg(_("You insert a bolt into your %s."),
                        reloadable->tname().c_str());
            } else {
                add_msg(_("You insert a cartridge into your %s."),
                        reloadable->tname().c_str());
            }
            g->u.recoil = std::max(MIN_RECOIL, (MIN_RECOIL + g->u.recoil) / 2);
        } else {
            add_msg(_("You reload your %s."), reloadable->tname().c_str());
            g->u.recoil = MIN_RECOIL;
        }
    } else {
        add_msg(m_info, _("Can't reload your %s."), reloadable->tname().c_str());
    }
    type = ACT_NULL;
}


void player_activity::start_fire_finish()
{
    item &it = g->u.i_at(position);
    iuse tmp;
    tmp.resolve_firestarter_use(&g->u, &it, placement);
    type = ACT_NULL;
}



void player_activity::start_fire_lens_do_turn()
{
    float natural_light_level = g->natural_light_level();
    // if the weather changes, we cannot start a fire with a lens. abort activity
    if (!((g->weather == WEATHER_CLEAR) || (g->weather == WEATHER_SUNNY)) ||
        !( natural_light_level >= 60 )) {
        add_msg(m_bad, _("There is not enough sunlight to start a fire now. You stop trying."));
        g->u.cancel_activity();
    } else if (natural_light_level !=
               values.back()) { // when lighting changes we recalculate the time needed
        float previous_natural_light_level = values.back();
        values.pop_back();
        values.push_back(natural_light_level); // update light level
        iuse tmp;
        float progress_left = float(moves_left) / float(tmp.calculate_time_for_lens_fire(&g->u,
                              previous_natural_light_level));
        moves_left = int(progress_left * (tmp.calculate_time_for_lens_fire(&g->u,
                                          natural_light_level))); // update moves left
    }
}


void player_activity::train_finish()
{
    const Skill *skill = Skill::skill(name);
    if (skill == NULL) {
        // Trained martial arts,
        add_msg(m_good, _("You learn %s."), martialarts[name].name.c_str());
        //~ %s is martial art
        g->u.add_memorial_log(pgettext("memorial_male", "Learned %s."),
                              pgettext("memorial_female", "Learned %s."),
                              martialarts[name].name.c_str());
        g->u.add_martialart(name);
    } else {
        int new_skill_level = g->u.skillLevel(skill) + 1;
        g->u.skillLevel(skill).level(new_skill_level);
        add_msg(m_good, _("You finish training %s to level %d."),
                skill->name().c_str(),
                new_skill_level);
        if (new_skill_level % 4 == 0) {
            //~ %d is skill level %s is skill name
            g->u.add_memorial_log(pgettext("memorial_male", "Reached skill level %1$d in %2$s."),
                                  pgettext("memorial_female", "Reached skill level %1$d in %2$s."),
                                  new_skill_level, skill->name().c_str());
        }
    }
    type = ACT_NULL;
}


void player_activity::vehicle_finish()
{
    //Grab this now, in case the vehicle gets shifted
    vehicle *veh = g->m.veh_at(values[0], values[1]);
    complete_vehicle();
    // complete_vehicle set activity type to NULL if the vehicle
    // was completely dismantled, otherwise the vehicle still exist and
    // is to be examined again.
    if (type == ACT_NULL) {
        return;
    }
    type = ACT_NULL;
    if (values.size() < 7) {
        dbg(D_ERROR) << "game:process_activity: invalid ACT_VEHICLE values: "
                     << values.size();
        debugmsg("process_activity invalid ACT_VEHICLE values:%d",
                 values.size());
    } else {
        if (veh) {
            g->refresh_all();
            g->exam_vehicle(*veh, values[0], values[1],
                            values[2], values[3]);
            return;
        } else {
            dbg(D_ERROR) << "game:process_activity: ACT_VEHICLE: vehicle not found";
            debugmsg("process_activity ACT_VEHICLE: vehicle not found");
        }
    }
}



void player_activity::vibe_do_turn()
{
    //Using a vibrator takes time, not speed
    moves_left -= 100;

    item &vibrator_item = g->u.i_at(position);

    if ((g->u.is_wearing("rebreather")) || (g->u.is_wearing("rebreather_xl")) ||
        (g->u.is_wearing("mask_h20survivor"))) {
        moves_left = 0;
        add_msg(m_bad, _("You have trouble breathing, and stop."));
    }

    //Deduct 1 battery charge for every minute using the vibrator
    if (int(calendar::turn) % 10 == 0) {
        vibrator_item.charges--;
        g->u.add_morale(MORALE_FEELING_GOOD, 4, 320); //4 points/min, one hour to fill
        // 1:1 fatigue:morale ratio, so maxing the morale is possible but will take
        // you pretty close to Dead Tired from a well-rested state.
        g->u.fatigue += 4;
    }
    if (vibrator_item.charges == 0) {
        moves_left = 0;
        add_msg(m_info, _("The %s runs out of batteries."), vibrator_item.tname().c_str());
    }
    if (g->u.fatigue >= 383) { // Dead Tired: different kind of relaxation needed
        moves_left = 0;
        add_msg(m_info, _("You're too tired to continue."));
    }

    // Vibrator requires that you be able to move around, stretch, etc, so doesn't play
    // well with roots.  Sorry.  :-(

    g->u.pause();
}
