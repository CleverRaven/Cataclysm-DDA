#include "bionics.h"

#include <climits>
#include <cmath>
#include <cstdlib>
#include <algorithm> //std::min
#include <sstream>
#include <array>
#include <iterator>
#include <list>
#include <memory>

#include "action.h"
#include "avatar.h"
#include "avatar_action.h"
#include "ballistics.h"
#include "cata_utility.h"
#include "debug.h"
#include "effect.h"
#include "explosion.h"
#include "field.h"
#include "game.h"
#include "handle_liquid.h"
#include "input.h"
#include "item.h"
#include "itype.h"
#include "json.h"
#include "map.h"
#include "map_iterator.h"
#include "messages.h"
#include "morale_types.h"
#include "options.h"
#include "output.h"
#include "overmapbuffer.h"
#include "npc.h"
#include "player.h"
#include "projectile.h"
#include "rng.h"
#include "sounds.h"
#include "string_formatter.h"
#include "translations.h"
#include "ui.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "weather.h"
#include "weather_gen.h"
#include "calendar.h"
#include "color.h"
#include "cursesdef.h"
#include "damage.h"
#include "enums.h"
#include "line.h"
#include "optional.h"
#include "pimpl.h"
#include "pldata.h"
#include "units.h"
#include "colony.h"
#include "inventory.h"
#include "item_location.h"
#include "monster.h"
#include "point.h"

const skill_id skilll_electronics( "electronics" );
const skill_id skilll_firstaid( "firstaid" );
const skill_id skilll_mechanics( "mechanics" );
const skill_id skilll_computer( "computer" );

const efftype_id effect_adrenaline( "adrenaline" );
const efftype_id effect_adrenaline_mycus( "adrenaline_mycus" );
const efftype_id effect_asthma( "asthma" );
const efftype_id effect_assisted( "assisted" );
const efftype_id effect_bleed( "bleed" );
const efftype_id effect_bloodworms( "bloodworms" );
const efftype_id effect_brainworms( "brainworms" );
const efftype_id effect_cig( "cig" );
const efftype_id effect_datura( "datura" );
const efftype_id effect_dermatik( "dermatik" );
const efftype_id effect_drunk( "drunk" );
const efftype_id effect_fungus( "fungus" );
const efftype_id effect_hallu( "hallu" );
const efftype_id effect_high( "high" );
const efftype_id effect_iodine( "iodine" );
const efftype_id effect_narcosis( "narcosis" );
const efftype_id effect_meth( "meth" );
const efftype_id effect_operating( "operating" );
const efftype_id effect_paincysts( "paincysts" );
const efftype_id effect_pblue( "pblue" );
const efftype_id effect_pkill1( "pkill1" );
const efftype_id effect_pkill2( "pkill2" );
const efftype_id effect_pkill3( "pkill3" );
const efftype_id effect_pkill_l( "pkill_l" );
const efftype_id effect_poison( "poison" );
const efftype_id effect_sleep( "sleep" );
const efftype_id effect_stung( "stung" );
const efftype_id effect_tapeworm( "tapeworm" );
const efftype_id effect_teleglow( "teleglow" );
const efftype_id effect_tetanus( "tetanus" );
const efftype_id effect_took_flumed( "took_flumed" );
const efftype_id effect_took_prozac( "took_prozac" );
const efftype_id effect_took_prozac_bad( "took_prozac_bad" );
const efftype_id effect_took_xanax( "took_xanax" );
const efftype_id effect_under_op( "under_operation" );
const efftype_id effect_visuals( "visuals" );
const efftype_id effect_weed_high( "weed_high" );

static const trait_id trait_PROF_MED( "PROF_MED" );
static const trait_id trait_PROF_AUTODOC( "PROF_AUTODOC" );

static const trait_id trait_THRESH_MEDICAL( "THRESH_MEDICAL" );
static const trait_id trait_MASOCHIST( "MASOCHIST" );
static const trait_id trait_MASOCHIST_MED( "MASOCHIST_MED" );
static const trait_id trait_NOPAIN( "NOPAIN" );
static const trait_id trait_CENOBITE( "CENOBITE" );

static const bionic_id bionic_TOOLS_EXTEND( "bio_tools_extend" );

namespace
{
std::map<bionic_id, bionic_data> bionics;
std::vector<bionic_id> faulty_bionics;
} //namespace

/** @relates string_id */
template<>
bool string_id<bionic_data>::is_valid() const
{
    return bionics.count( *this ) > 0;
}

/** @relates string_id */
template<>
const bionic_data &string_id<bionic_data>::obj() const
{
    const auto it = bionics.find( *this );
    if( it != bionics.end() ) {
        return it->second;
    }

    debugmsg( "bad bionic id %s", c_str() );

    static const bionic_data null_value;
    return null_value;
}

bool bionic_data::is_included( const bionic_id &id ) const
{
    return std::find( included_bionics.begin(), included_bionics.end(), id ) != included_bionics.end();
}

bionic_data::bionic_data() : name( no_translation( "bad bionic" ) ),
    description( no_translation( "This bionic was not set up correctly, this is a bug" ) )
{
}

static void force_comedown( effect &eff )
{
    if( eff.is_null() || eff.get_effect_type() == nullptr || eff.get_duration() <= 1_turns ) {
        return;
    }

    eff.set_duration( std::min( eff.get_duration(), eff.get_int_dur_factor() ) );
}

void npc::discharge_cbm_weapon()
{
    if( cbm_weapon_index < 0 ) {
        return;
    }
    bionic &bio = ( *my_bionics )[cbm_weapon_index];
    charge_power( -bionics[bio.id].power_activate );
    weapon = real_weapon;
    cbm_weapon_index = -1;
}

void npc::check_or_use_weapon_cbm( const bionic_id &cbm_id )
{
    // if we're already using a bio_weapon, keep using it
    if( cbm_weapon_index >= 0 ) {
        return;
    }

    int free_power = std::max( 0, power_level - max_power_level *
                               static_cast<int>( rules.cbm_reserve ) / 100 );
    if( free_power == 0 ) {
        return;
    }

    int index = 0;
    bool found = false;
    for( auto &i : *my_bionics ) {
        if( i.id == cbm_id && !i.powered ) {
            found = true;
            break;
        }
        index += 1;
    }
    if( !found ) {
        return;
    }
    bionic &bio = ( *my_bionics )[index];

    if( bionics[bio.id].gun_bionic ) {
        item cbm_weapon = item( bionics[bio.id].fake_item );
        bool not_allowed = !rules.has_flag( ally_rule::use_guns ) ||
                           ( rules.has_flag( ally_rule::use_silent ) && !cbm_weapon.is_silent() );
        if( is_player_ally() && not_allowed ) {
            return;
        }

        int ups_charges = charges_of( "UPS" );
        int ammo_count = weapon.ammo_remaining();
        int ups_drain = weapon.get_gun_ups_drain();
        if( ups_drain > 0 ) {
            ammo_count = std::min( ammo_count, ups_charges / ups_drain );
        }
        int cbm_ammo = free_power / bionics[bio.id].power_activate;

        if( weapon_value( weapon, ammo_count ) < weapon_value( cbm_weapon, cbm_ammo ) ) {
            real_weapon = weapon;
            weapon = cbm_weapon;
            cbm_weapon_index = index;
        }
    } else if( bionics[bio.id].weapon_bionic && !weapon.has_flag( "NO_UNWIELD" ) &&
               free_power > bionics[bio.id].power_activate ) {
        if( is_armed() ) {
            stow_item( weapon );
        }
        if( g->u.sees( pos() ) ) {
            add_msg( m_info, _( "%s activates their %s." ), disp_name(), bionics[bio.id].name );
        }

        weapon = item( bionics[bio.id].fake_item );
        charge_power( -bionics[bio.id].power_activate );
        bio.powered = true;
        cbm_weapon_index = index;
    }
}

// Why put this in a Big Switch?  Why not let bionics have pointers to
// functions, much like monsters and items?
//
// Well, because like diseases, which are also in a Big Switch, bionics don't
// share functions....
bool player::activate_bionic( int b, bool eff_only )
{
    bionic &bio = ( *my_bionics )[b];
    const bool mounted = is_mounted();
    if( bio.incapacitated_time > 0_turns ) {
        add_msg( m_info, _( "Your %s is shorting out and can't be activated." ),
                 bionics[bio.id].name );
        return false;
    }

    // Preserve the fake weapon used to initiate bionic gun firing
    static item bio_gun( weapon );

    // Special compatibility code for people who updated saves with their claws out
    if( ( weapon.typeId() == "bio_claws_weapon" && bio.id == "bio_claws_weapon" ) ||
        ( weapon.typeId() == "bio_blade_weapon" && bio.id == "bio_blade_weapon" ) ) {
        return deactivate_bionic( b );
    }

    // eff_only means only do the effect without messing with stats or displaying messages
    if( !eff_only ) {
        if( bio.powered ) {
            // It's already on!
            return false;
        }
        if( power_level < bionics[bio.id].power_activate ) {
            add_msg_if_player( m_info, _( "You don't have the power to activate your %s." ),
                               bionics[bio.id].name );
            return false;
        }

        if( !burn_fuel( b, true ) ) {
            return false;
        }

        //We can actually activate now, do activation-y things
        charge_power( -bionics[bio.id].power_activate );
        if( bionics[bio.id].toggled || bionics[bio.id].charge_time > 0 ) {
            bio.powered = true;
        }
        if( bionics[bio.id].charge_time > 0 ) {
            bio.charge_timer = bionics[bio.id].charge_time;
        }
        add_msg_if_player( m_info, _( "You activate your %s." ), bionics[bio.id].name );
    }

    item tmp_item;
    const w_point weatherPoint = *g->weather.weather_precise;

    // On activation effects go here
    if( bionics[bio.id].gun_bionic ) {
        charge_power( bionics[bio.id].power_activate );
        bio_gun = item( bionics[bio.id].fake_item );
        g->refresh_all();
        avatar_action::fire( g->u, g->m, bio_gun, bionics[bio.id].power_activate );
    } else if( bionics[ bio.id ].weapon_bionic ) {
        if( weapon.has_flag( "NO_UNWIELD" ) ) {
            add_msg_if_player( m_info, _( "Deactivate your %s first!" ), weapon.tname() );
            charge_power( bionics[bio.id].power_activate );
            bio.powered = false;
            return false;
        }

        if( !weapon.is_null() ) {
            const std::string query = string_format( _( "Stop wielding %s?" ), weapon.tname() );
            if( !dispose_item( item_location( *this, &weapon ), query ) ) {
                charge_power( bionics[bio.id].power_activate );
                bio.powered = false;
                return false;
            }
        }

        weapon = item( bionics[bio.id].fake_item );
        weapon.invlet = '#';
        if( bio.ammo_count > 0 ) {
            weapon.ammo_set( bio.ammo_loaded, bio.ammo_count );
            avatar_action::fire( g->u, g->m, weapon );
            g->refresh_all();
        }
    } else if( bio.id == "bio_ears" && has_active_bionic( bionic_id( "bio_earplugs" ) ) ) {
        for( auto &i : *my_bionics ) {
            if( i.id == "bio_earplugs" ) {
                i.powered = false;
                add_msg_if_player( m_info, _( "Your %s automatically turn off." ),
                                   bionics[i.id].name );
            }
        }
    } else if( bio.id == "bio_earplugs" && has_active_bionic( bionic_id( "bio_ears" ) ) ) {
        for( auto &i : *my_bionics ) {
            if( i.id == "bio_ears" ) {
                i.powered = false;
                add_msg_if_player( m_info, _( "Your %s automatically turns off." ),
                                   bionics[i.id].name );
            }
        }
    } else if( bio.id == "bio_tools" ) {
        invalidate_crafting_inventory();
    } else if( bio.id == "bio_cqb" ) {
        if( !pick_style() ) {
            bio.powered = false;
            add_msg_if_player( m_info, _( "You change your mind and turn it off." ) );
            return false;
        }
    } else if( bio.id == "bio_resonator" ) {
        //~Sound of a bionic sonic-resonator shaking the area
        sounds::sound( pos(), 30, sounds::sound_t::combat, _( "VRRRRMP!" ), false, "bionic",
                       "bio_resonator" );
        for( const tripoint &bashpoint : g->m.points_in_radius( pos(), 1 ) ) {
            g->m.bash( bashpoint, 110 );
            g->m.bash( bashpoint, 110 ); // Multibash effect, so that doors &c will fall
            g->m.bash( bashpoint, 110 );
        }

        mod_moves( -100 );
    } else if( bio.id == "bio_time_freeze" ) {
        if( mounted ) {
            add_msg_if_player( m_info, _( "You cannot activate that while mounted." ) );
            return false;
        }
        mod_moves( power_level );
        power_level = 0;
        add_msg_if_player( m_good, _( "Your speed suddenly increases!" ) );
        if( one_in( 3 ) ) {
            add_msg_if_player( m_bad, _( "Your muscles tear with the strain." ) );
            apply_damage( nullptr, bp_arm_l, rng( 5, 10 ) );
            apply_damage( nullptr, bp_arm_r, rng( 5, 10 ) );
            apply_damage( nullptr, bp_leg_l, rng( 7, 12 ) );
            apply_damage( nullptr, bp_leg_r, rng( 7, 12 ) );
            apply_damage( nullptr, bp_torso, rng( 5, 15 ) );
        }
        if( one_in( 5 ) ) {
            add_effect( effect_teleglow, rng( 5_minutes, 40_minutes ) );
        }
    } else if( bio.id == "bio_teleport" ) {
        if( mounted ) {
            add_msg_if_player( m_info, _( "You cannot activate that while mounted." ) );
            return false;
        }
        g->teleport();
        add_effect( effect_teleglow, 30_minutes );
        mod_moves( -100 );
    } else if( bio.id == "bio_blood_anal" ) {
        static const std::map<efftype_id, std::string> bad_effects = {{
                { effect_fungus, _( "Fungal Infection" ) },
                { effect_dermatik, _( "Insect Parasite" ) },
                { effect_stung, _( "Stung" ) },
                { effect_poison, _( "Poison" ) },
                // Those may be good for the player, but the scanner doesn't like them
                { effect_drunk, _( "Alcohol" ) },
                { effect_cig, _( "Nicotine" ) },
                { effect_meth, _( "Methamphetamines" ) },
                { effect_high, _( "Intoxicant: Other" ) },
                { effect_weed_high, _( "THC Intoxication" ) },
                // This little guy is immune to the blood filter though, as he lives in your bowels.
                { effect_tapeworm, _( "Intestinal Parasite" ) },
                { effect_bloodworms, _( "Hemolytic Parasites" ) },
                // These little guys are immune to the blood filter too, as they live in your brain.
                { effect_brainworms, _( "Intracranial Parasites" ) },
                // These little guys are immune to the blood filter too, as they live in your muscles.
                { effect_paincysts, _( "Intramuscular Parasites" ) },
                // Tetanus infection.
                { effect_tetanus, _( "Clostridium Tetani Infection" ) },
                { effect_datura, _( "Anticholinergic Tropane Alkaloids" ) },
                // TODO: Hallucinations not inducted by chemistry
                { effect_hallu, _( "Hallucinations" ) },
                { effect_visuals, _( "Hallucinations" ) },
            }
        };

        static const std::map<efftype_id, std::string> good_effects = {{
                { effect_pkill1, _( "Minor Painkiller" ) },
                { effect_pkill2, _( "Moderate Painkiller" ) },
                { effect_pkill3, _( "Heavy Painkiller" ) },
                { effect_pkill_l, _( "Slow-Release Painkiller" ) },

                { effect_pblue, _( "Prussian Blue" ) },
                { effect_iodine, _( "Potassium Iodide" ) },

                { effect_took_xanax, _( "Xanax" ) },
                { effect_took_prozac, _( "Prozac" ) },
                { effect_took_flumed, _( "Antihistamines" ) },
                { effect_adrenaline, _( "Adrenaline Spike" ) },
                // Should this be described like that? Does the bionic know what is this?
                { effect_adrenaline_mycus, _( "Mycal Spike" ) },
            }
        };

        std::vector<std::string> good;
        std::vector<std::string> bad;

        if( radiation > 0 ) {
            bad.push_back( _( "Irradiated" ) );
        }

        // TODO: Expose the player's effects to check it in a cleaner way
        for( const auto &pr : bad_effects ) {
            if( has_effect( pr.first ) ) {
                bad.push_back( pr.second );
            }
        }

        for( const auto &pr : good_effects ) {
            if( has_effect( pr.first ) ) {
                good.push_back( pr.second );
            }
        }

        const size_t win_h = std::min( static_cast<size_t>( TERMY ), bad.size() + good.size() + 2 );
        const int win_w = 46;
        catacurses::window w = catacurses::newwin( win_h, win_w, point( ( TERMX - win_w ) / 2,
                               ( TERMY - win_h ) / 2 ) );
        draw_border( w, c_red, string_format( " %s ", _( "Blood Test Results" ) ) );
        if( good.empty() && bad.empty() ) {
            trim_and_print( w, point( 2, 1 ), win_w - 3, c_white, _( "No effects." ) );
        } else {
            for( size_t line = 1; line < ( win_h - 1 ) && line <= good.size() + bad.size(); ++line ) {
                if( line <= bad.size() ) {
                    trim_and_print( w, point( 2, line ), win_w - 3, c_red, bad[line - 1] );
                } else {
                    trim_and_print( w, point( 2, line ), win_w - 3, c_green,
                                    good[line - 1 - bad.size()] );
                }
            }
        }
        wrefresh( w );
        catacurses::refresh();
        inp_mngr.wait_for_any_key();
    } else if( bio.id == "bio_blood_filter" ) {
        static const std::vector<efftype_id> removable = {{
                effect_fungus, effect_dermatik, effect_bloodworms,
                effect_tetanus, effect_poison, effect_stung,
                effect_pkill1, effect_pkill2, effect_pkill3, effect_pkill_l,
                effect_drunk, effect_cig, effect_high, effect_hallu, effect_visuals,
                effect_pblue, effect_iodine, effect_datura,
                effect_took_xanax, effect_took_prozac, effect_took_prozac_bad,
                effect_took_flumed,
            }
        };

        for( const auto &eff : removable ) {
            remove_effect( eff );
        }
        // Purging the substance won't remove the fatigue it caused
        force_comedown( get_effect( effect_adrenaline ) );
        force_comedown( get_effect( effect_meth ) );
        set_painkiller( 0 );
        stim = 0;
        mod_moves( -100 );
    } else if( bio.id == "bio_evap" ) {
        item water = item( "water_clean", 0 );
        water.set_item_temperature( 283.15 );
        int humidity = weatherPoint.humidity;
        int water_charges = lround( humidity * 3.0 / 100.0 );
        // At 50% relative humidity or more, the player will draw 2 units of water
        // At 16% relative humidity or less, the player will draw 0 units of water
        water.charges = water_charges;
        if( water_charges == 0 ) {
            add_msg_if_player( m_bad,
                               _( "There was not enough moisture in the air from which to draw water!" ) );
        } else if( !liquid_handler::consume_liquid( water ) ) {
            charge_power( bionics[bionic_id( "bio_evap" )].power_activate );
        }
    } else if( bio.id == "bio_torsionratchet" ) {
        add_msg_if_player( m_info, _( "Your torsion ratchet locks onto your joints." ) );
    } else if( bio.id == "bio_jointservo" ) {
        add_msg_if_player( m_info, _( "You can now run faster, assisted by joint servomotors." ) );
    } else if( bio.id == "bio_lighter" ) {
        g->refresh_all();
        const cata::optional<tripoint> pnt = choose_adjacent( _( "Start a fire where?" ) );
        if( pnt && g->m.add_field( *pnt, fd_fire, 1 ) ) {
            mod_moves( -100 );
        } else {
            add_msg_if_player( m_info, _( "You can't light a fire there." ) );
            charge_power( bionics[bionic_id( "bio_lighter" )].power_activate );
        }
    } else if( bio.id == "bio_geiger" ) {
        add_msg_if_player( m_info, _( "Your radiation level: %d" ), radiation );
    } else if( bio.id == "bio_radscrubber" ) {
        if( radiation > 4 ) {
            radiation -= 5;
        } else {
            radiation = 0;
        }
    } else if( bio.id == "bio_adrenaline" ) {
        if( has_effect( effect_adrenaline ) ) {
            // Safety
            add_msg_if_player( m_bad, _( "The bionic refuses to activate!" ) );
            charge_power( bionics[bio.id].power_activate );
        } else {
            add_effect( effect_adrenaline, 20_minutes );
        }

    } else if( bio.id == "bio_emp" ) {
        g->refresh_all();
        if( const cata::optional<tripoint> pnt = choose_adjacent( _( "Create an EMP where?" ) ) ) {
            explosion_handler::emp_blast( *pnt );
            mod_moves( -100 );
        } else {
            charge_power( bionics[bionic_id( "bio_emp" )].power_activate );
        }
    } else if( bio.id == "bio_hydraulics" ) {
        add_msg_if_player( m_good, _( "Your muscles hiss as hydraulic strength fills them!" ) );
        //~ Sound of hissing hydraulic muscle! (not quite as loud as a car horn)
        sounds::sound( pos(), 19, sounds::sound_t::activity, _( "HISISSS!" ), false, "bionic",
                       "bio_hydraulics" );
    } else if( bio.id == "bio_water_extractor" ) {
        bool extracted = false;
        for( item &it : g->m.i_at( pos() ) ) {
            static const auto volume_per_water_charge = 500_ml;
            if( it.is_corpse() ) {
                const int avail = it.get_var( "remaining_water", it.volume() / volume_per_water_charge );
                if( avail > 0 &&
                    query_yn( _( "Extract water from the %s" ),
                              colorize( it.tname(), it.color_in_inventory() ) ) ) {
                    item water( "water_clean", calendar::turn, avail );
                    water.set_item_temperature( 0.00001 * it.temperature );
                    if( liquid_handler::consume_liquid( water ) ) {
                        extracted = true;
                        it.set_var( "remaining_water", static_cast<int>( water.charges ) );
                    }
                    break;
                }
            }
        }
        if( !extracted ) {
            charge_power( bionics[bionic_id( "bio_water_extractor" )].power_activate );
        }
    } else if( bio.id == "bio_magnet" ) {
        static const std::set<material_id> affected_materials =
        { material_id( "iron" ), material_id( "steel" ) };
        // Remember all items that will be affected, then affect them
        // Don't "snowball" by affecting some items multiple times
        std::vector<std::pair<item, tripoint>> affected;
        const auto weight_cap = weight_capacity();
        for( const tripoint &p : g->m.points_in_radius( pos(), 10 ) ) {
            if( p == pos() || !g->m.has_items( p ) || g->m.has_flag( "SEALED", p ) ) {
                continue;
            }

            auto stack = g->m.i_at( p );
            for( auto it = stack.begin(); it != stack.end(); it++ ) {
                if( it->weight() < weight_cap &&
                    it->made_of_any( affected_materials ) ) {
                    affected.emplace_back( std::make_pair( *it, p ) );
                    stack.erase( it );
                    break;
                }
            }
        }

        g->refresh_all();
        for( const auto &pr : affected ) {
            projectile proj;
            proj.speed  = 50;
            proj.impact = damage_instance::physical( pr.first.weight() / 250_gram, 0, 0, 0 );
            // make the projectile stop one tile short to prevent hitting the player
            proj.range = rl_dist( pr.second, pos() ) - 1;
            proj.proj_effects = {{ "NO_ITEM_DAMAGE", "DRAW_AS_LINE", "NO_DAMAGE_SCALING", "JET" }};

            auto dealt = projectile_attack( proj, pr.second, pos(), 0 );
            g->m.add_item_or_charges( dealt.end_point, pr.first );
        }

        mod_moves( -100 );
    } else if( bio.id == "bio_lockpick" ) {
        tmp_item = item( "pseuso_bio_picklock", 0 );
        g->refresh_all();
        if( invoke_item( &tmp_item ) == 0 ) {
            if( tmp_item.charges > 0 ) {
                // restore the energy since CBM wasn't used
                charge_power( bionics[bio.id].power_activate );
            }
            return true;
        }

        mod_moves( -100 );
    } else if( bio.id == "bio_flashbang" ) {
        explosion_handler::flashbang( pos(), true );
        mod_moves( -100 );
    } else if( bio.id == "bio_shockwave" ) {
        explosion_handler::shockwave( pos(), 3, 4, 2, 8, true );
        add_msg_if_player( m_neutral, _( "You unleash a powerful shockwave!" ) );
        mod_moves( -100 );
    } else if( bio.id == "bio_meteorologist" ) {
        // Calculate local wind power
        int vehwindspeed = 0;
        if( optional_vpart_position vp = g->m.veh_at( pos() ) ) {
            vehwindspeed = abs( vp->vehicle().velocity / 100 ); // vehicle velocity in mph
        }
        const oter_id &cur_om_ter = overmap_buffer.ter( global_omt_location() );
        /* cache g->get_temperature( player location ) since it is used twice. No reason to recalc */
        const auto player_local_temp = g->weather.get_temperature( g->u.pos() );
        /* windpower defined in internal velocity units (=.01 mph) */
        double windpower = 100.0f * get_local_windpower( g->weather.windspeed + vehwindspeed,
                           cur_om_ter, pos(), g->weather.winddirection, g->is_sheltered( pos() ) );
        add_msg_if_player( m_info, _( "Temperature: %s." ), print_temperature( player_local_temp ) );
        add_msg_if_player( m_info, _( "Relative Humidity: %s." ),
                           print_humidity(
                               get_local_humidity( weatherPoint.humidity, g->weather.weather,
                                       g->is_sheltered( g->u.pos() ) ) ) );
        add_msg_if_player( m_info, _( "Pressure: %s." ),
                           print_pressure( static_cast<int>( weatherPoint.pressure ) ) );
        add_msg_if_player( m_info, _( "Wind Speed: %.1f %s." ),
                           convert_velocity( static_cast<int>( windpower ), VU_WIND ),
                           velocity_units( VU_WIND ) );
        add_msg_if_player( m_info, _( "Feels Like: %s." ),
                           print_temperature(
                               get_local_windchill( weatherPoint.temperature, weatherPoint.humidity,
                                       windpower / 100 ) + player_local_temp ) );
        std::string dirstring = get_dirstring( g->weather.winddirection );
        add_msg_if_player( m_info, _( "Wind Direction: From the %s." ), dirstring );
    } else if( bio.id == "bio_remote" ) {
        int choice = uilist( _( "Perform which function:" ), {
            _( "Control vehicle" ), _( "RC radio" )
        } );
        if( choice >= 0 && choice <= 1 ) {
            item ctr;
            if( choice == 0 ) {
                ctr = item( "remotevehcontrol", 0 );
            } else {
                ctr = item( "radiocontrol", 0 );
            }
            ctr.charges = power_level;
            int power_use = invoke_item( &ctr );
            charge_power( -power_use );
            bio.powered = ctr.active;
        } else {
            bio.powered = g->remoteveh() != nullptr || !get_value( "remote_controlling" ).empty();
        }
    } else if( bio.id == "bio_plutdump" ) {
        if( query_yn(
                _( "WARNING: Purging all fuel is likely to result in radiation!  Purge anyway?" ) ) ) {
            slow_rad += ( tank_plut + reactor_plut );
            tank_plut = 0;
            reactor_plut = 0;
        }
    } else if( bio.id == "bio_cable" ) {
        bool has_cable = has_item_with( []( const item & it ) {
            return it.active && it.has_flag( "CABLE_SPOOL" );
        } );
        bool has_connected_cable = has_item_with( []( const item & it ) {
            return it.active && it.has_flag( "CABLE_SPOOL" ) && it.get_var( "state" ) == "solar_pack_link";
        } );

        if( !has_cable ) {
            add_msg_if_player( m_info,
                               _( "You need a jumper cable connected to a vehicle to drain power from it." ) );
        }
        if( is_wearing( "solarpack_on" ) || is_wearing( "q_solarpack_on" ) ) {
            if( has_connected_cable ) {
                add_msg_if_player( m_info, _( "Your plugged-in solar pack is now able to charge"
                                              " your system." ) );
            } else {
                add_msg_if_player( m_info, _( "You need to connect the cable to yourself and the solar pack"
                                              " before your solar pack can charge your system." ) );
            }
        } else if( is_wearing( "solarpack" ) || is_wearing( "q_solarpack" ) ) {
            add_msg_if_player( m_info, _( "You might plug in your solar pack to the cable charging"
                                          " system, if you unfold it." ) );
        }
    }

    // Recalculate stats (strength, mods from pain etc.) that could have been affected
    reset_encumbrance();
    reset();

    // Also reset crafting inventory cache if this bionic spawned a fake item
    if( !bionics[ bio.id ].fake_item.empty() ) {
        invalidate_crafting_inventory();
    }

    return true;
}

bool player::deactivate_bionic( int b, bool eff_only )
{
    bionic &bio = ( *my_bionics )[b];

    if( bio.incapacitated_time > 0_turns ) {
        add_msg( m_info, _( "Your %s is shorting out and can't be deactivated." ),
                 bionics[bio.id].name );
        return false;
    }

    // Just do the effect, no stat changing or messages
    if( !eff_only ) {
        if( !bio.powered ) {
            // It's already off!
            return false;
        }
        // Compatibility with old saves without the toolset hammerspace
        if( bio.id == "bio_tools" && !has_bionic( bionic_TOOLS_EXTEND ) ) {
            add_bionic( bionic_TOOLS_EXTEND ); // E X T E N D    T O O L S
        }
        if( !bionics[bio.id].toggled ) {
            // It's a fire-and-forget bionic, we can't turn it off but have to wait for it to run out of charge
            add_msg_if_player( m_info, _( "You can't deactivate your %s manually!" ),
                               bionics[bio.id].name );
            return false;
        }
        if( power_level < bionics[bio.id].power_deactivate ) {
            add_msg( m_info, _( "You don't have the power to deactivate your %s." ),
                     bionics[bio.id].name );
            return false;
        }

        //We can actually deactivate now, do deactivation-y things
        charge_power( -bionics[bio.id].power_deactivate );
        bio.powered = false;
        add_msg_if_player( m_neutral, _( "You deactivate your %s." ), bionics[bio.id].name );
    }

    // Deactivation effects go here
    if( bionics[ bio.id ].weapon_bionic ) {
        if( weapon.typeId() == bionics[ bio.id ].fake_item ) {
            add_msg_if_player( _( "You withdraw your %s." ), weapon.tname() );
            if( g->u.sees( pos() ) ) {
                add_msg_if_npc( m_info, _( "<npcname> withdraws %s %s." ), disp_name( true ), weapon.tname() );
            }
            bio.ammo_loaded = weapon.ammo_data() != nullptr ? weapon.ammo_data()->get_id() : "null";
            bio.ammo_count = static_cast<unsigned int>( weapon.ammo_remaining() );
            weapon = item();
            invalidate_crafting_inventory();
        }
    } else if( bio.id == "bio_cqb" ) {
        // check if player knows current style naturally, otherwise drop them back to style_none
        if( style_selected != matype_id( "style_none" ) && style_selected != matype_id( "style_kicks" ) ) {
            bool has_style = false;
            for( auto &elem : ma_styles ) {
                if( elem == style_selected ) {
                    has_style = true;
                }
            }
            if( !has_style ) {
                style_selected = matype_id( "style_none" );
            }
        }
    } else if( bio.id == "bio_remote" ) {
        if( g->remoteveh() != nullptr && !has_active_item( "remotevehcontrol" ) ) {
            g->setremoteveh( nullptr );
        } else if( !get_value( "remote_controlling" ).empty() && !has_active_item( "radiocontrol" ) ) {
            set_value( "remote_controlling", "" );
        }
    } else if( bio.id == "bio_tools" ) {
        invalidate_crafting_inventory();
    }

    // Recalculate stats (strength, mods from pain etc.) that could have been affected
    reset_encumbrance();
    reset();

    // Also reset crafting inventory cache if this bionic spawned a fake item
    if( !bionics[ bio.id ].fake_item.empty() ) {
        invalidate_crafting_inventory();
    }

    return true;
}

bool player::burn_fuel( int b, bool start )
{
    bionic &bio = ( *my_bionics )[b];
    if( bio.info().fuel_opts.empty() || bio.is_muscle_powered() ) {
        return true;
    }

    if( start && get_fuel_available( bio.id ).empty() ) {
        add_msg_player_or_npc( m_bad, _( "Your %s does not have enought fuel to start." ),
                               _( "<npcname>'s %s does not have enought fuel to start." ), bio.info().name );
        deactivate_bionic( b );
        return false;
    }
    if( !start ) {// don't produce power on start to avoid instant recharge exploit by turning bionic ON/OFF in the menu
        for( const itype_id &fuel : get_fuel_available( bio.id ) ) {
            const item tmp_fuel( fuel );
            int temp = std::stoi( get_value( fuel ) );
            if( power_level + tmp_fuel.fuel_energy() *bio.info().fuel_efficiency > max_power_level ) {

                add_msg_player_or_npc( m_info, _( "Your %s turns off to not waste fuel." ),
                                       _( "<npcname>'s %s turns off to not waste fuel." ), bio.info().name );

                bio.powered = false;
                deactivate_bionic( b, true );
                return false;

            } else {
                if( temp > 0 ) {
                    temp -= 1;
                    charge_power( tmp_fuel.fuel_energy() *bio.info().fuel_efficiency );
                    set_value( fuel, std::to_string( temp ) );
                    update_fuel_storage( fuel );
                } else {
                    remove_value( fuel );
                    add_msg_player_or_npc( m_info, _( "Your %s runs out of fuel and turn off." ),
                                           _( "<npcname>'s %s runs out of fuel and turn off." ), bio.info().name );
                    bio.powered = false;
                    deactivate_bionic( b, true );
                    return false;
                }
            }
        }
    }
    return true;
}

/**
 * @param p the player
 * @param bio the bionic that is meant to be recharged.
 * @param amount the amount of power that is to be spent recharging the bionic.
 * @param factor multiplies the power cost per turn.
 * @param rate divides the number of turns we may charge (rate of 2 discharges in half the time).
 * @return indicates whether we successfully charged the bionic.
 */
static bool attempt_recharge( player &p, bionic &bio, int &amount, int factor = 1, int rate = 1 )
{
    const bionic_data &info = bio.info();
    const int armor_power_cost = 1;
    int power_cost = info.power_over_time * factor;
    bool recharged = false;

    if( power_cost > 0 ) {
        if( info.armor_interface ) {
            // Don't spend any power on armor interfacing unless we're wearing active powered armor.
            bool powered_armor = std::any_of( p.worn.begin(), p.worn.end(),
            []( const item & w ) {
                return w.active && w.is_power_armor();
            } );
            if( !powered_armor ) {
                power_cost -= armor_power_cost * factor;
            }
        }
        if( p.power_level >= power_cost ) {
            // Set the recharging cost and charge the bionic.
            amount = power_cost;
            // This is our first turn of charging, so subtract a turn from the recharge delay.
            bio.charge_timer = info.charge_time - rate;
            recharged = true;
        }
    }

    return recharged;
}

void player::process_bionic( int b )
{
    bionic &bio = ( *my_bionics )[b];
    // Only powered bionics should be processed
    if( !bio.powered ) {
        return;
    }

    // These might be affected by environmental conditions, status effects, faulty bionics, etc.
    int discharge_factor = 1;
    int discharge_rate = 1;

    if( bio.charge_timer > 0 ) {
        bio.charge_timer -= discharge_rate;
    } else {
        if( bio.info().charge_time > 0 ) {
            if( bio.info().power_source ) {
                // Convert fuel to bionic power
                burn_fuel( b );
                // Reset timer
                bio.charge_timer = bio.info().charge_time;
            } else {
                // Try to recharge our bionic if it is made for it
                int cost = 0;
                bool recharged = attempt_recharge( *this, bio, cost, discharge_factor, discharge_rate );
                if( !recharged ) {
                    // No power to recharge, so deactivate
                    bio.powered = false;
                    add_msg_if_player( m_neutral, _( "Your %s powers down." ), bio.info().name );
                    // This purposely bypasses the deactivation cost
                    deactivate_bionic( b, true );
                    return;
                }
                if( cost ) {
                    charge_power( -cost );
                }
            }
        }
    }

    // Bionic effects on every turn they are active go here.
    if( bio.id == "bio_night" ) {
        if( calendar::once_every( 5_turns ) ) {
            add_msg_if_player( m_neutral, _( "Artificial night generator active!" ) );
        }
    } else if( bio.id == "bio_remote" ) {
        if( g->remoteveh() == nullptr && get_value( "remote_controlling" ).empty() ) {
            bio.powered = false;
            add_msg_if_player( m_warning, _( "Your %s has lost connection and is turning off." ),
                               bionics[bio.id].name );
        }
    } else if( bio.id == "bio_hydraulics" ) {
        // Sound of hissing hydraulic muscle! (not quite as loud as a car horn)
        sounds::sound( pos(), 19, sounds::sound_t::activity, _( "HISISSS!" ), false, "bionic",
                       "bio_hydraulics" );
    } else if( bio.id == "bio_nanobots" ) {
        for( int i = 0; i < num_hp_parts; i++ ) {
            if( power_level >= 5 && hp_cur[i] > 0 && hp_cur[i] < hp_max[i] ) {
                heal( static_cast<hp_part>( i ), 1 );
                charge_power( -5 );
            }
        }
        for( const body_part bp : all_body_parts ) {
            if( power_level >= 2 && remove_effect( effect_bleed, bp ) ) {
                charge_power( -2 );
            }
        }
    } else if( bio.id == "bio_painkiller" ) {
        const int pkill = get_painkiller();
        const int pain = get_pain();
        int max_pkill = std::min( 150, pain );
        if( pkill < max_pkill ) {
            mod_painkiller( 1 );
            charge_power( -2 );
        }

        // Only dull pain so extreme that we can't pkill it safely
        if( pkill >= 150 && pain > pkill && stim > -150 ) {
            mod_pain( -1 );
            // Negative side effect: negative stim
            stim--;
            charge_power( -2 );
        }
    } else if( bio.id == "bio_cable" ) {
        if( power_level >= max_power_level ) {
            return;
        }

        const std::vector<item *> cables = items_with( []( const item & it ) {
            return it.active && it.has_flag( "CABLE_SPOOL" );
        } );

        constexpr int battery_per_power = 10;
        int wants_power_amt = battery_per_power;
        for( const item *cable : cables ) {
            const cata::optional<tripoint> target = cable->get_cable_target( this, pos() );
            if( !target ) {
                continue;
            }
            const optional_vpart_position vp = g->m.veh_at( *target );
            if( !vp ) {
                continue;
            }

            wants_power_amt = vp->vehicle().discharge_battery( wants_power_amt );
            if( wants_power_amt == 0 ) {
                charge_power( 1 );
                break;
            }
        }

        if( wants_power_amt < battery_per_power &&
            wants_power_amt > 0 &&
            x_in_y( battery_per_power - wants_power_amt, battery_per_power ) ) {
            charge_power( 1 );
        }
    } else if( bio.id == "bio_gills" ) {
        if( has_effect( effect_asthma ) ) {
            add_msg_if_player( m_good,
                               _( "You feel your throat open up and air filling your lungs!" ) );
            remove_effect( effect_asthma );
        }
    } else if( bio.id == "afs_bio_dopamine_stimulators" ) { // Aftershock
        add_morale( MORALE_FEELING_GOOD, 20, 20, 30_minutes, 20_minutes, true );
    }
}

void player::bionics_uninstall_failure( int difficulty, int success, float adjusted_skill )
{
    // "success" should be passed in as a negative integer representing how far off we
    // were for a successful removal.  We use this to determine consequences for failing.
    success = abs( success );

    // failure level is decided by how far off the character was from a successful removal, and
    // this is scaled up or down by the ratio of difficulty/skill.  At high skill levels (or low
    // difficulties), only minor consequences occur.  At low skill levels, severe consequences
    // are more likely.
    const int failure_level = static_cast<int>( sqrt( success * 4.0 * difficulty / adjusted_skill ) );
    const int fail_type = std::min( 5, failure_level );

    if( fail_type <= 0 ) {
        add_msg( m_neutral, _( "The removal fails without incident." ) );
        return;
    }

    add_msg( m_neutral, _( "The removal is a failure." ) );
    switch( fail_type ) {
        case 1:
            if( !has_trait( trait_id( "NOPAIN" ) ) ) {
                add_msg_if_player( m_bad, _( "It really hurts!" ) );
                mod_pain( rng( failure_level * 3, failure_level * 6 ) );
            }
            break;

        case 2:
        case 3:
            for( const body_part &bp : all_body_parts ) {
                if( has_effect( effect_under_op, bp ) ) {
                    apply_damage( this, bp, rng( failure_level, failure_level * 2 ), true );
                    add_msg_player_or_npc( m_bad, _( "Your %s is damaged." ), _( "<npcname>'s %s is damaged." ),
                                           body_part_name_accusative( bp ) );
                }
            }
            break;

        case 4:
        case 5:
            for( const body_part &bp : all_body_parts ) {
                if( has_effect( effect_under_op, bp ) ) {
                    apply_damage( this, bp, rng( 30, 80 ), true );
                    add_msg_player_or_npc( m_bad, _( "Your %s is severely damaged." ),
                                           _( "<npcname>'s %s is severely damaged." ),
                                           body_part_name_accusative( bp ) );
                }
            }
            break;
    }

}

void player::bionics_uninstall_failure( monster &installer, player &patient, int difficulty,
                                        int success,
                                        float adjusted_skill )
{

    // "success" should be passed in as a negative integer representing how far off we
    // were for a successful removal.  We use this to determine consequences for failing.
    success = abs( success );

    // failure level is decided by how far off the monster was from a successful removal, and
    // this is scaled up or down by the ratio of difficulty/skill.  At high skill levels (or low
    // difficulties), only minor consequences occur.  At low skill levels, severe consequences
    // are more likely.
    const int failure_level = static_cast<int>( sqrt( success * 4.0 * difficulty / adjusted_skill ) );
    const int fail_type = std::min( 5, failure_level );

    bool u_see = sees( patient );

    if( u_see || patient.is_player() ) {
        if( fail_type <= 0 ) {
            add_msg( m_neutral, _( "The removal fails without incident." ) );
            return;
        }
        switch( rng( 1, 5 ) ) {
            case 1:
                add_msg( m_mixed, _( "The %s flub the operation." ), installer.name() );
                break;
            case 2:
                add_msg( m_mixed, _( "The %s messes up the operation." ), installer.name() );
                break;
            case 3:
                add_msg( m_mixed, _( "The operation fails." ) );
                break;
            case 4:
                add_msg( m_mixed, _( "The operation is a failure." ) );
                break;
            case 5:
                add_msg( m_mixed, _( "The %s screws up the operation." ), installer.name() );
                break;
        }
    }
    switch( fail_type ) {
        case 1:
            if( !has_trait( trait_id( "NOPAIN" ) ) ) {
                patient.add_msg_if_player( m_bad, _( "It really hurts!" ) );
                patient.mod_pain( rng( failure_level * 3, failure_level * 6 ) );
            }
            break;

        case 2:
        case 3:
            for( const body_part &bp : all_body_parts ) {
                if( has_effect( effect_under_op, bp ) ) {
                    patient.apply_damage( this, bp, rng( failure_level, failure_level * 2 ), true );
                    if( u_see ) {
                        patient.add_msg_player_or_npc( m_bad, _( "Your %s is damaged." ), _( "<npcname>'s %s is damaged." ),
                                                       body_part_name_accusative( bp ) );
                    }
                }
            }
            break;

        case 4:
        case 5:
            for( const body_part &bp : all_body_parts ) {
                if( has_effect( effect_under_op, bp ) ) {
                    patient.apply_damage( this, bp, rng( 30, 80 ), true );
                    if( u_see ) {
                        patient.add_msg_player_or_npc( m_bad, _( "Your %s is severely damaged." ),
                                                       _( "<npcname>'s %s is severely damaged." ),
                                                       body_part_name_accusative( bp ) );
                    }
                }
            }
            break;
    }
}

bool player::has_enough_anesth( const itype *cbm, player &patient )
{
    if( !cbm->bionic ) {
        debugmsg( "has_enough_anesth( const itype *cbm ): %s is not a bionic", cbm->get_id() );
        return false;
    }

    if( has_bionic( bionic_id( "bio_painkiller" ) ) || has_trait( trait_NOPAIN ) ||
        has_trait( trait_id( "DEBUG_BIONICS" ) ) ) {
        return true;
    }

    const int weight = units::to_kilogram( patient.bodyweight() ) / 10;
    const requirement_data req_anesth = *requirement_id( "anesthetic" ) *
                                        cbm->bionic->difficulty * 2 * weight;

    std::vector<const item *> b_filter = crafting_inventory().items_with( []( const item & it ) {
        return it.has_flag( "ANESTHESIA" ); // legacy
    } );

    return req_anesth.can_make_with_inventory( crafting_inventory(), is_crafting_component ) ||
           !b_filter.empty();
}

// bionic manipulation adjusted skill
float player::bionics_adjusted_skill( const skill_id &most_important_skill,
                                      const skill_id &important_skill,
                                      const skill_id &least_important_skill,
                                      int skill_level )
{
    int pl_skill = bionics_pl_skill( most_important_skill, important_skill, least_important_skill,
                                     skill_level );

    // for chance_of_success calculation, shift skill down to a float between ~0.4 - 30
    float adjusted_skill = static_cast<float>( pl_skill ) - std::min( static_cast<float>( 40 ),
                           static_cast<float>( pl_skill ) - static_cast<float>( pl_skill ) / static_cast<float>( 10.0 ) );
    return adjusted_skill;
}

int player::bionics_pl_skill( const skill_id &most_important_skill, const skill_id &important_skill,
                              const skill_id &least_important_skill, int skill_level )
{
    int pl_skill;
    if( skill_level == -1 ) {
        pl_skill = int_cur                                  * 4 +
                   get_skill_level( most_important_skill )  * 4 +
                   get_skill_level( important_skill )       * 3 +
                   get_skill_level( least_important_skill ) * 1;
    } else {
        // override chance as though all values were skill_level if it is provided
        pl_skill = 12 * skill_level;
    }

    // Medical residents have some idea what they're doing
    if( has_trait( trait_PROF_MED ) ) {
        pl_skill += 3;
        add_msg_player_or_npc( m_neutral, _( "You prep to begin surgery." ),
                               _( "<npcname> prepares for surgery." ) );
    }

    // People trained in bionics gain an additional advantage towards using it
    if( has_trait( trait_PROF_AUTODOC ) ) {
        pl_skill += 7;
        add_msg( m_neutral, _( "A lifetime of augmentation has taught %s a thing or two..." ),
                 disp_name() );
    }
    return pl_skill;
}

// bionic manipulation chance of success
int bionic_manip_cos( float adjusted_skill, bool autodoc, int bionic_difficulty )
{
    if( ( autodoc && get_option < bool > ( "SAFE_AUTODOC" ) ) ||
        g->u.has_trait( trait_id( "DEBUG_BIONICS" ) ) ) {
        return 100;
    }

    int chance_of_success = 0;
    // we will base chance_of_success on a ratio of skill and difficulty
    // when skill=difficulty, this gives us 1.  skill < difficulty gives a fraction.
    float skill_difficulty_parameter = static_cast<float>( adjusted_skill /
                                       ( 4.0 * bionic_difficulty ) );

    // when skill == difficulty, chance_of_success is 50%. Chance of success drops quickly below that
    // to reserve bionics for characters with the appropriate skill.  For more difficult bionics, the
    // curve flattens out just above 80%
    chance_of_success = static_cast<int>( ( 100 * skill_difficulty_parameter ) /
                                          ( skill_difficulty_parameter + sqrt( 1 / skill_difficulty_parameter ) ) );

    return chance_of_success;
}

bool player::can_uninstall_bionic( const bionic_id &b_id, player &installer, bool autodoc,
                                   int skill_level )
{
    // if malfunctioning bionics doesn't have associated item it gets a difficulty of 12
    int difficulty = 12;
    if( item::type_is_defined( b_id.c_str() ) ) {
        auto type = item::find_type( b_id.c_str() );
        if( type->bionic ) {
            difficulty = type->bionic->difficulty;
        }
    }

    if( !has_bionic( b_id ) ) {
        popup( _( "%s don't have this bionic installed." ), disp_name() );
        return false;
    }

    if( b_id == "bio_blaster" ) {
        popup( _( "Removing %s Fusion Blaster Arm would leave %s with a useless stump." ),
               disp_name( true ), disp_name() );
        return false;
    }

    if( ( b_id == "bio_reactor" ) || ( b_id == "bio_advreactor" ) ) {
        if( !g->u.query_yn(
                _( "WARNING: Removing a reactor may leave radioactive material! Remove anyway?" ) ) ) {
            return false;
        }
    }

    for( const auto &e : bionics ) {
        if( e.second.is_included( b_id ) ) {
            popup( _( "%s must remove the %s bionic to remove the %s." ), installer.disp_name(),
                   e.second.name, b_id->name );
            return false;
        }
    }

    if( b_id == "bio_eye_optic" ) {
        popup( _( "The Telescopic Lenses are part of %s eyes now.  Removing them would leave %s blind." ),
               disp_name( true ), disp_name() );
        return false;
    }

    int assist_bonus = installer.get_effect_int( effect_assisted );

    // removal of bionics adds +2 difficulty over installation
    float adjusted_skill;
    if( autodoc ) {
        adjusted_skill = installer.bionics_adjusted_skill( skilll_firstaid,
                         skilll_computer,
                         skilll_electronics,
                         skill_level );
    } else {
        adjusted_skill = installer.bionics_adjusted_skill( skilll_electronics,
                         skilll_firstaid,
                         skilll_mechanics,
                         skill_level );
    }
    int chance_of_success = bionic_manip_cos( adjusted_skill + assist_bonus, autodoc, difficulty + 2 );

    if( chance_of_success >= 100 ) {
        if( !g->u.query_yn(
                _( "Are you sure you wish to uninstall the selected bionic?" ),
                100 - chance_of_success ) ) {
            return false;
        }
    } else {
        if( !g->u.query_yn(
                _( "WARNING: %i percent chance of SEVERE damage to all body parts! Continue anyway?" ),
                ( 100 - static_cast<int>( chance_of_success ) ) ) ) {
            return false;
        }
    }

    return true;
}

bool player::uninstall_bionic( const bionic_id &b_id, player &installer, bool autodoc,
                               int skill_level )
{
    // if malfunctioning bionics doesn't have associated item it gets a difficulty of 12
    int difficulty = 12;
    if( item::type_is_defined( b_id.c_str() ) ) {
        auto type = item::find_type( b_id.c_str() );
        if( type->bionic ) {
            difficulty = type->bionic->difficulty;
        }
    }

    int assist_bonus = installer.get_effect_int( effect_assisted );

    // removal of bionics adds +2 difficulty over installation
    float adjusted_skill;
    int pl_skill;
    if( autodoc ) {
        adjusted_skill = installer.bionics_adjusted_skill( skilll_firstaid,
                         skilll_computer,
                         skilll_electronics,
                         skill_level );
        pl_skill = installer.bionics_pl_skill( skilll_firstaid,
                                               skilll_computer,
                                               skilll_electronics,
                                               skill_level );
    } else {
        adjusted_skill = installer.bionics_adjusted_skill( skilll_electronics,
                         skilll_firstaid,
                         skilll_mechanics,
                         skill_level );
        pl_skill = installer.bionics_pl_skill( skilll_electronics,
                                               skilll_firstaid,
                                               skilll_mechanics,
                                               skill_level );
    }

    int chance_of_success = bionic_manip_cos( adjusted_skill + assist_bonus, autodoc, difficulty + 2 );

    // Surgery is imminent, retract claws or blade if active
    for( size_t i = 0; i < installer.my_bionics->size(); i++ ) {
        const auto &bio = ( *installer.my_bionics )[ i ];
        if( bio.powered && bio.info().weapon_bionic ) {
            installer.deactivate_bionic( i );
        }
    }

    int success = chance_of_success - rng( 1, 100 );

    if( is_npc() ) {
        static_cast<npc *>( this )->set_attitude( NPCATT_ACTIVITY );
        assign_activity( activity_id( "ACT_OPERATION" ), to_moves<int>( difficulty * 20_minutes ) );
        static_cast<npc *>( this )->set_mission( NPC_MISSION_ACTIVITY );
    } else {
        assign_activity( activity_id( "ACT_OPERATION" ), to_moves<int>( difficulty * 20_minutes ) );
    }

    activity.values.push_back( difficulty );
    activity.values.push_back( success );
    activity.values.push_back( bionics[b_id].capacity );
    activity.values.push_back( pl_skill );
    activity.str_values.push_back( "uninstall" );
    activity.str_values.push_back( "" );
    activity.str_values.push_back( b_id.str() );
    activity.str_values.push_back( "" );
    activity.str_values.push_back( "" );
    activity.str_values.push_back( "" );
    if( autodoc ) {
        activity.str_values.push_back( "true" );
    } else {
        activity.str_values.push_back( "false" );
    }
    for( const auto &elem : bionics[b_id].occupied_bodyparts ) {
        activity.values.push_back( elem.first );
        add_effect( effect_under_op, difficulty * 20_minutes, elem.first, true, difficulty );
    }
    return true;
}

void player::perform_uninstall( bionic_id bid, int difficulty, int success, int power_lvl,
                                int pl_skill )
{
    if( success > 0 ) {

        if( is_player() ) {
            add_memorial_log( pgettext( "memorial_male", "Removed bionic: %s." ),
                              pgettext( "memorial_female", "Removed bionic: %s." ),
                              bid.obj().name );
        }

        // until bionics can be flagged as non-removable
        add_msg_player_or_npc( m_neutral, _( "Your parts are jiggled back into their familiar places." ),
                               _( "<npcname>'s parts are jiggled back into their familiar places." ) );
        add_msg( m_good, _( "Successfully removed %s." ), bid.obj().name );
        remove_bionic( bid );

        // remove power bank provided by bionic
        max_power_level -= power_lvl;

        item cbm( "burnt_out_bionic" );
        if( item::type_is_defined( bid.c_str() ) ) {
            cbm = item( bid.c_str() );
        }
        cbm.set_flag( "FILTHY" );
        cbm.set_flag( "NO_STERILE" );
        cbm.set_flag( "NO_PACKED" );
        cbm.faults.emplace( fault_id( "fault_bionic_salvaged" ) );
        g->m.add_item( pos(), cbm );
    } else {
        if( is_player() ) {
            add_memorial_log( pgettext( "memorial_male", "Failed to remove bionic: %s." ),
                              pgettext( "memorial_female", "Failed to remove bionic: %s." ),
                              bid.obj().name );
        }

        // for chance_of_success calculation, shift skill down to a float between ~0.4 - 30
        float adjusted_skill = static_cast<float>( pl_skill ) - std::min( static_cast<float>( 40 ),
                               static_cast<float>( pl_skill ) - static_cast<float>( pl_skill ) / static_cast<float>
                               ( 10.0 ) );
        bionics_uninstall_failure( difficulty, success, adjusted_skill );

    }
    g->m.invalidate_map_cache( g->get_levz() );
    g->refresh_all();
}

bool player::uninstall_bionic( const bionic &target_cbm, monster &installer, player &patient,
                               float adjusted_skill, bool autodoc )
{
    const std::string ammo_type( "anesthetic" );

    if( installer.ammo[ammo_type] <= 0 ) {
        if( g->u.sees( installer ) ) {
            add_msg( _( "The %s's anesthesia kit looks empty." ), installer.name() );
        }
        return false;
    }

    item bionic_to_uninstall = item( target_cbm.id.str(), 0 );
    const itype *itemtype = bionic_to_uninstall.type;
    int difficulty = itemtype->bionic->difficulty;
    int chance_of_success = bionic_manip_cos( adjusted_skill, autodoc, difficulty + 2 );
    int success = chance_of_success - rng( 1, 100 );

    const time_duration duration = difficulty * 20_minutes;
    if( !installer.has_effect( effect_operating ) ) { // don't stack up the effect
        installer.add_effect( effect_operating, duration + 5_turns );
    }

    if( patient.is_player() ) {
        add_msg( m_bad,
                 _( "You feel a tiny pricking sensation in your right arm, and lose all sensation before abruptly blacking out." ) );
    } else if( g->u.sees( installer ) ) {
        add_msg( m_bad,
                 _( "The %1$s gently inserts a syringe into %2$s's arm and starts injecting something while holding them down." ),
                 installer.name(), patient.disp_name() );
    }

    installer.ammo[ammo_type] -= 1 ;

    patient.add_effect( effect_narcosis, duration );
    patient.add_effect( effect_sleep, duration );

    if( patient.is_player() ) {
        add_msg( _( "You fall asleep and %1$s starts operating." ), installer.disp_name() );
    } else if( g->u.sees( patient ) ) {
        add_msg( _( "%1$s falls asleep and %2$s starts operating." ), patient.disp_name(),
                 installer.disp_name() );
    }

    if( success > 0 ) {

        if( patient.is_player() ) {
            add_msg( m_neutral, _( "Your parts are jiggled back into their familiar places." ) );
            add_msg( m_mixed, _( "Successfully removed %s." ), target_cbm.info().name );
        } else if( patient.is_npc() && g->u.sees( patient ) ) {
            add_msg( m_neutral, _( "%s's parts are jiggled back into their familiar places." ),
                     patient.disp_name() );
            add_msg( m_mixed, _( "Successfully removed %s." ), target_cbm.info().name );
        }

        // remove power bank provided by bionic
        patient.max_power_level -= target_cbm.info().capacity;
        patient.remove_bionic( target_cbm.id );
        item cbm( "burnt_out_bionic" );
        if( item::type_is_defined( target_cbm.id.c_str() ) ) {
            cbm = item( target_cbm.id.c_str() );
        }
        cbm.set_flag( "FILTHY" );
        cbm.set_flag( "NO_STERILE" );
        cbm.set_flag( "NO_PACKED" );
        cbm.faults.emplace( fault_id( "fault_bionic_salvaged" ) );
        g->m.add_item( patient.pos(), cbm );
    } else {
        bionics_uninstall_failure( installer, patient, difficulty, success, adjusted_skill );
    }
    g->refresh_all();

    return false;
}

bool player::can_install_bionics( const itype &type, player &installer, bool autodoc,
                                  int skill_level )
{
    if( !type.bionic ) {
        debugmsg( "Tried to install NULL bionic" );
        return false;
    }
    if( is_mounted() ) {
        return false;
    }
    int assist_bonus = installer.get_effect_int( effect_assisted );

    const bionic_id &bioid = type.bionic->id;
    const int difficult = type.bionic->difficulty;
    float adjusted_skill;

    if( autodoc ) {
        adjusted_skill = installer.bionics_adjusted_skill( skilll_firstaid,
                         skilll_computer,
                         skilll_electronics,
                         skill_level );
    } else {
        adjusted_skill = installer.bionics_adjusted_skill( skilll_electronics,
                         skilll_firstaid,
                         skilll_mechanics,
                         skill_level );
    }
    int chance_of_success = bionic_manip_cos( adjusted_skill + assist_bonus, autodoc, difficult );

    const std::map<body_part, int> &issues = bionic_installation_issues( bioid );
    // show all requirements which are not satisfied
    if( !issues.empty() ) {
        std::string detailed_info;
        for( auto &elem : issues ) {
            //~ <Body part name>: <number of slots> more slot(s) needed.
            detailed_info += string_format( _( "\n%s: %i more slot(s) needed." ),
                                            body_part_name_as_heading( elem.first, 1 ),
                                            elem.second );
        }
        popup( _( "Not enough space for bionic installation!%s" ), detailed_info );
        return false;
    }

    if( chance_of_success >= 100 ) {
        if( !g->u.query_yn(
                _( "Are you sure you wish to install the selected bionic?" ),
                100 - chance_of_success ) ) {
            return false;
        }
    } else {
        if( !g->u.query_yn(
                _( "WARNING: %i percent chance of failure that may result in damage, pain, or a faulty installation! Continue anyway?" ),
                ( 100 - static_cast<int>( chance_of_success ) ) ) ) {
            return false;
        }
    }

    return true;
}

bool player::install_bionics( const itype &type, player &installer, bool autodoc, int skill_level )
{
    if( !type.bionic ) {
        debugmsg( "Tried to install NULL bionic" );
        return false;
    }

    int assist_bonus = installer.get_effect_int( effect_assisted );

    const bionic_id &bioid = type.bionic->id;
    const bionic_id &upbioid = bioid->upgraded_bionic;
    const int difficulty = type.bionic->difficulty;
    float adjusted_skill;
    int pl_skill;
    if( autodoc ) {
        adjusted_skill = installer.bionics_adjusted_skill( skilll_firstaid,
                         skilll_computer,
                         skilll_electronics,
                         skill_level );
        pl_skill = installer.bionics_pl_skill( skilll_firstaid,
                                               skilll_computer,
                                               skilll_electronics,
                                               skill_level );
    } else {
        adjusted_skill = installer.bionics_adjusted_skill( skilll_electronics,
                         skilll_firstaid,
                         skilll_mechanics,
                         skill_level );
        pl_skill = installer.bionics_pl_skill( skilll_electronics,
                                               skilll_firstaid,
                                               skilll_mechanics,
                                               skill_level );
    }
    int chance_of_success = bionic_manip_cos( adjusted_skill + assist_bonus, autodoc, difficulty );

    // Practice skills only if conducting manual installation
    if( !autodoc ) {
        installer.practice( skilll_electronics, static_cast<int>( ( 100 - chance_of_success ) * 1.5 ) );
        installer.practice( skilll_firstaid, static_cast<int>( ( 100 - chance_of_success ) * 1.0 ) );
        installer.practice( skilll_mechanics, static_cast<int>( ( 100 - chance_of_success ) * 0.5 ) );
    }

    int success = chance_of_success - rng( 0, 99 );
    if( is_npc() ) {
        static_cast<npc *>( this )->set_attitude( NPCATT_ACTIVITY );
        assign_activity( activity_id( "ACT_OPERATION" ), to_moves<int>( difficulty * 20_minutes ) );
        static_cast<npc *>( this )->set_mission( NPC_MISSION_ACTIVITY );
    } else {
        assign_activity( activity_id( "ACT_OPERATION" ), to_moves<int>( difficulty * 20_minutes ) );
    }

    activity.values.push_back( difficulty );
    activity.values.push_back( success );
    activity.values.push_back( bionics[bioid].capacity );
    activity.values.push_back( pl_skill );
    activity.str_values.push_back( "install" );
    activity.str_values.push_back( "" );
    activity.str_values.push_back( bioid.str() );
    if( upbioid ) {
        activity.str_values.push_back( "" );
        activity.str_values.push_back( upbioid.str() );
    } else {
        activity.str_values.push_back( "" );
        activity.str_values.push_back( "" );
    }
    if( installer.has_trait( trait_PROF_MED ) || installer.has_trait( trait_PROF_AUTODOC ) ) {
        activity.str_values.push_back( installer.disp_name( true ) );
    } else {
        activity.str_values.push_back( "NOT_MED" );
    }
    if( autodoc ) {
        activity.str_values.push_back( "true" );
    } else {
        activity.str_values.push_back( "false" );
    }
    for( const auto &elem : bionics[bioid].occupied_bodyparts ) {
        activity.values.push_back( elem.first );
        add_effect( effect_under_op, difficulty * 20_minutes, elem.first, true, difficulty );
    }
    for( const trait_id &mid : bioid->canceled_mutations ) {
        if( has_trait( mid ) ) {
            activity.str_values.push_back( mid.c_str() );
        }
    }
    return true;
}

void player::perform_install( bionic_id bid, bionic_id upbid, int difficulty, int success,
                              int pl_skill, std::string installer_name,
                              std::vector<trait_id> trait_to_rem, tripoint patient_pos )
{
    if( success > 0 ) {

        if( is_player() ) {
            add_memorial_log( pgettext( "memorial_male", "Installed bionic: %s." ),
                              pgettext( "memorial_female", "Installed bionic: %s." ),
                              bid.obj().name );
        }
        if( upbid != bionic_id( "" ) ) {
            remove_bionic( upbid );
            //~ %1$s - name of the bionic to be upgraded (inferior), %2$s - name of the upgraded bionic (superior).
            add_msg( m_good, _( "Successfully upgraded %1$s to %2$s." ),
                     upbid.obj().name, bid.obj().name );
        } else {
            //~ %s - name of the bionic.
            add_msg( m_good, _( "Successfully installed %s." ), bid.obj().name );
        }

        add_bionic( bid );

        if( !trait_to_rem.empty() ) {
            for( trait_id tid : trait_to_rem ) {
                remove_mutation( tid );
            }
        }

    } else {
        if( is_player() ) {
            add_memorial_log( pgettext( "memorial_male", "Failed install of bionic: %s." ),
                              pgettext( "memorial_female", "Failed install of bionic: %s." ),
                              bid.obj().name );
        }

        // for chance_of_success calculation, shift skill down to a float between ~0.4 - 30
        float adjusted_skill = static_cast<float>( pl_skill ) - std::min( static_cast<float>( 40 ),
                               static_cast<float>( pl_skill ) - static_cast<float>( pl_skill ) / static_cast<float>
                               ( 10.0 ) );
        bionics_install_failure( bid, installer_name, difficulty, success, adjusted_skill, patient_pos );
    }
    g->m.invalidate_map_cache( g->get_levz() );
    g->refresh_all();
}

void player::bionics_install_failure( bionic_id bid, std::string installer, int difficulty,
                                      int success, float adjusted_skill, tripoint patient_pos )
{
    // "success" should be passed in as a negative integer representing how far off we
    // were for a successful install.  We use this to determine consequences for failing.
    success = abs( success );

    // failure level is decided by how far off the character was from a successful install, and
    // this is scaled up or down by the ratio of difficulty/skill.  At high skill levels (or low
    // difficulties), only minor consequences occur.  At low skill levels, severe consequences
    // are more likely.
    int failure_level = static_cast<int>( sqrt( success * 4.0 * difficulty / adjusted_skill ) );
    int fail_type = ( failure_level > 5 ? 5 : failure_level );
    bool drop_cbm = false;
    add_msg( m_neutral, _( "The installation is a failure." ) );



    if( installer != "NOT_MED" ) {
        //~"Complications" is USian medical-speak for "unintended damage from a medical procedure".
        add_msg( m_neutral, _( "%s training helps to minimize the complications." ),
                 installer );
        // In addition to the bonus, medical residents know enough OR protocol to avoid botching.
        // Take MD and be immune to faulty bionics.
        if( fail_type == 5 ) {
            fail_type = rng( 1, 3 );
        }
    }
    if( fail_type <= 0 ) {
        add_msg( m_neutral, _( "The installation fails without incident." ) );
        drop_cbm = true;
    } else {
        switch( fail_type ) {

            case 1:
                if( !( has_trait( trait_id( "NOPAIN" ) ) ) ) {
                    add_msg_if_player( m_bad, _( "It really hurts!" ) );
                    mod_pain( rng( failure_level * 3, failure_level * 6 ) );
                }
                drop_cbm = true;
                break;

            case 2:
            case 3:
                for( const body_part &bp : all_body_parts ) {
                    if( has_effect( effect_under_op, bp ) ) {
                        apply_damage( this, bp, rng( 30, 80 ), true );
                        add_msg_player_or_npc( m_bad, _( "Your %s is damaged." ), _( "<npcname>'s %s is damaged." ),
                                               body_part_name_accusative( bp ) );
                    }
                }
                drop_cbm = true;
                break;

            case 4:
            case 5: {
                add_msg( m_bad, _( "The installation is faulty!" ) );
                std::vector<bionic_id> valid;
                std::copy_if( begin( faulty_bionics ), end( faulty_bionics ), std::back_inserter( valid ),
                [&]( const bionic_id & id ) {
                    return !has_bionic( id );
                } );

                if( valid.empty() ) { // We've got all the bad bionics!
                    if( max_power_level > 0 ) {
                        int old_power = max_power_level;
                        add_msg( m_bad, _( "%s lose power capacity!" ), disp_name() );
                        max_power_level = rng( 0, max_power_level - 25 );
                        if( is_player() ) {
                            add_memorial_log( pgettext( "memorial_male", "Lost %d units of power capacity." ),
                                              pgettext( "memorial_female", "Lost %d units of power capacity." ),
                                              old_power - max_power_level );
                        }
                    }
                    // TODO: What if we can't lose power capacity?  No penalty?
                } else {
                    const bionic_id &id = random_entry( valid );
                    add_bionic( id );
                    if( is_player() ) {
                        add_memorial_log( pgettext( "memorial_male", "Installed bad bionic: %s." ),
                                          pgettext( "memorial_female", "Installed bad bionic: %s." ),
                                          bionics[ id ].name );
                    }
                }
            }
            break;
        }
    }
    if( drop_cbm ) {
        item cbm( bid.c_str() );
        cbm.set_flag( "NO_STERILE" );
        cbm.set_flag( "NO_PACKED" );
        cbm.faults.emplace( fault_id( "fault_bionic_salvaged" ) );
        g->m.add_item( patient_pos, cbm );
    }
}

std::string list_occupied_bps( const bionic_id &bio_id, const std::string &intro,
                               const bool each_bp_on_new_line )
{
    if( bio_id->occupied_bodyparts.empty() ) {
        return "";
    }
    std::ostringstream desc;
    desc << intro;
    for( const auto &elem : bio_id->occupied_bodyparts ) {
        desc << ( each_bp_on_new_line ? "\n" : " " );
        //~ <Bodypart name> (<number of occupied slots> slots);
        desc << string_format( _( "%s (%i slots);" ),
                               body_part_name_as_heading( elem.first, 1 ),
                               elem.second );
    }
    return desc.str();
}

int player::get_used_bionics_slots( const body_part bp ) const
{
    int used_slots = 0;
    for( auto &bio : *my_bionics ) {
        auto search = bionics[bio.id].occupied_bodyparts.find( bp );
        if( search != bionics[bio.id].occupied_bodyparts.end() ) {
            used_slots += search->second;
        }
    }

    return used_slots;
}

std::map<body_part, int> player::bionic_installation_issues( const bionic_id &bioid )
{
    std::map<body_part, int> issues;
    if( !get_option < bool >( "CBM_SLOTS_ENABLED" ) ) {
        return issues;
    }
    for( auto &elem : bioid->occupied_bodyparts ) {
        const int lacked_slots = elem.second - get_free_bionics_slots( elem.first );
        if( lacked_slots > 0 ) {
            issues.emplace( elem.first, lacked_slots );
        }
    }
    return issues;
}

int player::get_total_bionics_slots( const body_part bp ) const
{
    switch( bp ) {
        case bp_torso:
            return 80;

        case bp_head:
            return 18;

        case bp_eyes:
        case bp_mouth:
            return 4;

        case bp_arm_l:
        case bp_arm_r:
            return 20;

        case bp_hand_l:
        case bp_hand_r:
            return 5;

        case bp_leg_l:
        case bp_leg_r:
            return 30;

        case bp_foot_l:
        case bp_foot_r:
            return 7;

        case num_bp:
            debugmsg( "number of slots for incorrect bodypart is requested!" );
            return 0;
    }
    return 0;
}

int player::get_free_bionics_slots( const body_part bp ) const
{
    return get_total_bionics_slots( bp ) - get_used_bionics_slots( bp );
}

void player::add_bionic( const bionic_id &b )
{
    if( has_bionic( b ) ) {
        debugmsg( "Tried to install bionic %s that is already installed!", b.c_str() );
        return;
    }

    int pow_up = bionics[b].capacity;
    max_power_level += pow_up;
    if( b == "bio_power_storage" || b == "bio_power_storage_mkII" ) {
        add_msg_if_player( m_good, _( "Increased storage capacity by %i." ), pow_up );
        // Power Storage CBMs are not real bionic units, so return without adding it to my_bionics
        return;
    }

    my_bionics->push_back( bionic( b, get_free_invlet( *this ) ) );
    if( b == "bio_tools" || b == "bio_ears" ) {
        activate_bionic( my_bionics->size() - 1 );
    }

    for( const auto &inc_bid : bionics[b].included_bionics ) {
        add_bionic( inc_bid );
    }

    reset_encumbrance();
    recalc_sight_limits();
}

void player::remove_bionic( const bionic_id &b )
{
    bionic_collection new_my_bionics;
    for( auto &i : *my_bionics ) {
        if( b == i.id ) {
            continue;
        }

        // Linked bionics: if either is removed, the other is removed as well.
        if( b->is_included( i.id ) || i.id->is_included( b ) ) {
            continue;
        }

        new_my_bionics.push_back( bionic( i.id, i.invlet ) );
    }
    *my_bionics = new_my_bionics;
    reset_encumbrance();
    recalc_sight_limits();
}

int player::num_bionics() const
{
    return my_bionics->size();
}

std::pair<int, int> player::amount_of_storage_bionics() const
{
    int lvl = max_power_level;

    // exclude amount of power capacity obtained via non-power-storage CBMs
    for( const auto &it : *my_bionics ) {
        lvl -= bionics[it.id].capacity;
    }

    std::pair<int, int> results( 0, 0 );
    if( lvl <= 0 ) {
        return results;
    }

    int pow_mkI = bionics[bionic_id( "bio_power_storage" )].capacity;
    int pow_mkII = bionics[bionic_id( "bio_power_storage_mkII" )].capacity;

    while( lvl >= std::min( pow_mkI, pow_mkII ) ) {
        if( one_in( 2 ) ) {
            if( lvl >= pow_mkI ) {
                results.first++;
                lvl -= pow_mkI;
            }
        } else {
            if( lvl >= pow_mkII ) {
                results.second++;
                lvl -= pow_mkII;
            }
        }
    }
    return results;
}

bionic &player::bionic_at_index( int i )
{
    return ( *my_bionics )[i];
}

// Returns true if a bionic was removed.
bool player::remove_random_bionic()
{
    const int numb = num_bionics();
    if( numb ) {
        int rem = rng( 0, num_bionics() - 1 );
        const auto bionic = ( *my_bionics )[rem];
        //Todo: Currently, contained/containing bionics don't get explicitly deactivated when the removal of a linked bionic removes them too
        deactivate_bionic( rem, true );
        remove_bionic( bionic.id );
        add_msg( m_bad, _( "Your %s fails, and is destroyed!" ), bionics[ bionic.id ].name );
        recalc_sight_limits();
    }
    return numb;
}

void player::clear_bionics()
{
    my_bionics->clear();
}

void reset_bionics()
{
    bionics.clear();
    faulty_bionics.clear();
}

static bool get_bool_or_flag( JsonObject &jsobj, const std::string &name, const std::string &flag,
                              const bool fallback, const std::string &flags_node = "flags" )
{
    bool value = fallback;
    if( jsobj.has_bool( name ) ) {
        value = jsobj.get_bool( name, fallback );
        debugmsg( "JsonObject contains legacy node `" + name + "`.  Consider replacing it with `" +
                  flag + "` flag in `" + flags_node + "` node." );
    } else {
        const std::set<std::string> flags = jsobj.get_tags( flags_node );
        value = flags.count( flag );
    }
    return value;
}

void load_bionic( JsonObject &jsobj )
{

    bionic_data new_bionic;

    const bionic_id id( jsobj.get_string( "id" ) );
    jsobj.read( "name", new_bionic.name );
    jsobj.read( "description", new_bionic.description );
    new_bionic.power_activate = jsobj.get_int( "act_cost", 0 );

    new_bionic.toggled = get_bool_or_flag( jsobj, "toggled", "BIONIC_TOGGLED", false );
    // Requires ability to toggle
    new_bionic.power_deactivate = jsobj.get_int( "deact_cost", 0 );

    new_bionic.charge_time = jsobj.get_int( "time", 0 );
    // Requires a non-zero time
    new_bionic.power_over_time = jsobj.get_int( "react_cost", 0 );

    new_bionic.capacity = jsobj.get_int( "capacity", 0 );

    new_bionic.npc_usable = get_bool_or_flag( jsobj, "npc_usable", "BIONIC_NPC_USABLE", false );
    new_bionic.faulty = get_bool_or_flag( jsobj, "faulty", "BIONIC_FAULTY", false );
    new_bionic.power_source = get_bool_or_flag( jsobj, "power_source", "BIONIC_POWER_SOURCE", false );

    new_bionic.gun_bionic = get_bool_or_flag( jsobj, "gun_bionic", "BIONIC_GUN", false );
    new_bionic.weapon_bionic = get_bool_or_flag( jsobj, "weapon_bionic", "BIONIC_WEAPON", false );
    new_bionic.armor_interface = get_bool_or_flag( jsobj, "armor_interface", "BIONIC_ARMOR_INTERFACE",
                                 false );
    new_bionic.sleep_friendly = get_bool_or_flag( jsobj, "sleep_friendly", "BIONIC_SLEEP_FRIENDLY",
                                false );
    new_bionic.shockproof = get_bool_or_flag( jsobj, "shockproof", "BIONIC_SHOCKPROOF", false );

    new_bionic.fuel_efficiency = jsobj.get_float( "fuel_efficiency", 0 );

    if( new_bionic.gun_bionic && new_bionic.weapon_bionic ) {
        debugmsg( "Bionic %s specified as both gun and weapon bionic", id.c_str() );
    }

    new_bionic.fake_item = jsobj.get_string( "fake_item", "" );

    new_bionic.weight_capacity_modifier = jsobj.get_float( "weight_capacity_modifier", 1.0 );

    if( jsobj.has_string( "weight_capacity_bonus" ) ) {
        new_bionic.weight_capacity_bonus = read_from_json_string<units::mass>
                                           ( *jsobj.get_raw( "weight_capacity_bonus" ), units::mass_units );
    }


    jsobj.read( "canceled_mutations", new_bionic.canceled_mutations );
    jsobj.read( "included_bionics", new_bionic.included_bionics );
    jsobj.read( "included", new_bionic.included );
    jsobj.read( "upgraded_bionic", new_bionic.upgraded_bionic );
    jsobj.read( "fuel_options", new_bionic.fuel_opts );
    jsobj.read( "fuel_capacity", new_bionic.fuel_capacity );

    JsonArray jsar = jsobj.get_array( "encumbrance" );
    if( !jsar.empty() ) {
        while( jsar.has_more() ) {
            JsonArray ja = jsar.next_array();
            new_bionic.encumbrance.emplace( get_body_part_token( ja.get_string( 0 ) ),
                                            ja.get_int( 1 ) );
        }
    }

    JsonArray jsarr = jsobj.get_array( "occupied_bodyparts" );
    if( !jsarr.empty() ) {
        while( jsarr.has_more() ) {
            JsonArray ja = jsarr.next_array();
            new_bionic.occupied_bodyparts.emplace( get_body_part_token( ja.get_string( 0 ) ),
                                                   ja.get_int( 1 ) );
        }
    }

    JsonArray json_arr = jsobj.get_array( "env_protec" );
    if( !json_arr.empty() ) {
        while( json_arr.has_more() ) {
            JsonArray ja = json_arr.next_array();
            new_bionic.env_protec.emplace( get_body_part_token( ja.get_string( 0 ) ),
                                           ja.get_int( 1 ) );
        }
    }

    new_bionic.activated = new_bionic.toggled ||
                           new_bionic.power_activate > 0 ||
                           new_bionic.charge_time > 0;

    const auto result = bionics.insert( std::make_pair( id, new_bionic ) );

    if( !result.second ) {
        debugmsg( "duplicate bionic id" );
    } else if( new_bionic.faulty ) {
        faulty_bionics.push_back( id );
    }
}

void check_bionics()
{
    for( const auto &bio : bionics ) {
        if( !bio.second.fake_item.empty() &&
            !item::type_is_defined( bio.second.fake_item ) ) {
            debugmsg( "Bionic %s has unknown fake_item %s",
                      bio.first.c_str(), bio.second.fake_item.c_str() );
        }
        for( const auto &mid : bio.second.canceled_mutations ) {
            if( !mid.is_valid() ) {
                debugmsg( "Bionic %s cancels undefined mutation %s",
                          bio.first.c_str(), mid.c_str() );
            }
        }
        for( const auto &bid : bio.second.included_bionics ) {
            if( !bid.is_valid() ) {
                debugmsg( "Bionic %s includes undefined bionic %s",
                          bio.first.c_str(), bid.c_str() );
            }
            if( !bionics[bid].occupied_bodyparts.empty() ) {
                debugmsg( "Bionic %s (included by %s) consumes slots, those should be part of the containing bionic instead.",
                          bid.c_str(), bio.first.c_str() );
            }
        }
        if( bio.second.upgraded_bionic ) {
            if( bio.second.upgraded_bionic == bio.first ) {
                debugmsg( "Bionic %s is upgraded with itself", bio.first.c_str() );
            } else if( !bio.second.upgraded_bionic.is_valid() ) {
                debugmsg( "Bionic %s upgrades undefined bionic %s",
                          bio.first.c_str(), bio.second.upgraded_bionic.c_str() );
            }
        }
        if( !item::type_is_defined( bio.first.c_str() ) && !bio.second.included ) {
            debugmsg( "Bionic %s has no defined item version", bio.first.c_str() );
        }
    }
}

void finalize_bionics()
{
    for( const auto &bio : bionics ) {
        if( bio.second.upgraded_bionic ) {
            bionics[ bio.second.upgraded_bionic ].available_upgrades.insert( bio.first );
        }
    }
}

int bionic::get_quality( const quality_id &quality ) const
{
    const auto &i = info();
    if( i.fake_item.empty() ) {
        return INT_MIN;
    }

    return item( i.fake_item ).get_quality( quality );
}

bool bionic::is_muscle_powered() const
{
    const std::vector<itype_id> fuel_op = info().fuel_opts;
    return std::find( fuel_op.begin(), fuel_op.end(), "muscle" ) != fuel_op.end();
}

void bionic::serialize( JsonOut &json ) const
{
    json.start_object();
    json.member( "id", id );
    json.member( "invlet", static_cast<int>( invlet ) );
    json.member( "powered", powered );
    json.member( "charge", charge_timer );
    json.member( "ammo_loaded", ammo_loaded );
    json.member( "ammo_count", ammo_count );
    if( incapacitated_time > 0_turns ) {
        json.member( "incapacitated_time", incapacitated_time );
    }
    json.end_object();
}

void bionic::deserialize( JsonIn &jsin )
{
    JsonObject jo = jsin.get_object();
    id = bionic_id( jo.get_string( "id" ) );
    invlet = jo.get_int( "invlet" );
    powered = jo.get_bool( "powered" );
    charge_timer = jo.get_int( "charge" );
    if( jo.has_string( "ammo_loaded" ) ) {
        ammo_loaded = jo.get_string( "ammo_loaded" );
    }
    if( jo.has_int( "ammo_count" ) ) {
        ammo_count = jo.get_int( "ammo_count" );
    }
    if( jo.has_int( "incapacitated_time" ) ) {
        incapacitated_time = 1_turns * jo.get_int( "incapacitated_time" );
    }
}

void player::introduce_into_anesthesia( const time_duration &duration, player &installer,
                                        bool needs_anesthesia ) //used by the Autodoc
{
    installer.add_msg_player_or_npc( m_info,
                                     _( "You set up the operation step-by-step, configuring the Autodoc to manipulate a CBM." ),
                                     _( "<npcname> sets up the operation, configuring the Autodoc to manipulate a CBM." ) );

    add_msg_player_or_npc( m_info,
                           _( "You settle into position, sliding your right wrist into the couch's strap." ),
                           _( "<npcname> settles into position, sliding their wrist into the couch's strap." ) );
    if( needs_anesthesia ) {
        //post-threshold medical mutants do not fear operations.
        if( has_trait( trait_THRESH_MEDICAL ) ) {
            add_msg_if_player( m_mixed,
                               _( "You feel excited as the operation starts." ) );
        }

        add_msg_if_player( m_mixed,
                           _( "You feel a tiny pricking sensation in your right arm, and lose all sensation before abruptly blacking out." ) );

        //post-threshold medical mutants with Deadened don't need anesthesia due to their inability to feel pain
    } else {
        //post-threshold medical mutants do not fear operations.
        if( has_trait( trait_THRESH_MEDICAL ) ) {
            add_msg_if_player( m_mixed,
                               _( "You feel excited as the Autodoc slices painlessly into you.  You enjoy the sight of scalpels slicing you apart, but as operation proceeds you suddenly feel tired and pass out." ) );
        } else {
            add_msg_if_player( m_mixed,
                               _( "You stay very, very still, focusing intently on an interesting stain on the ceiling, as the Autodoc slices painlessly into you.  Mercifully, you pass out when the blades reach your line of sight." ) );
        }
    }

    //Pain junkies feel sorry about missed pain from operation.
    if( has_trait( trait_MASOCHIST ) || has_trait( trait_MASOCHIST_MED ) ||
        has_trait( trait_CENOBITE ) ) {
        add_msg_if_player( m_mixed,
                           _( "As your conciousness slips away, you feel regret that you won't be able to enjoy the operation." ) );
    }

    if( has_effect( effect_narcosis ) ) {
        const time_duration remaining_time = get_effect_dur( effect_narcosis );
        if( remaining_time <= duration ) {
            const time_duration top_off_time = duration - remaining_time;
            add_effect( effect_narcosis, top_off_time );
            fall_asleep( top_off_time );
        }
    } else {
        add_effect( effect_narcosis, duration );
        fall_asleep( duration );
    }
}
