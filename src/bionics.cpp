#include "bionics.h"

#include <climits>
#include <cmath>
#include <cstdlib>
#include <algorithm> //std::min
#include <array>
#include <iterator>
#include <list>
#include <memory>
#include <forward_list>

#include "action.h"
#include "avatar.h"
#include "avatar_action.h"
#include "assign.h"
#include "ballistics.h"
#include "cata_utility.h"
#include "character.h"
#include "debug.h"
#include "effect.h"
#include "event_bus.h"
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
#include "memorial_logger.h"
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
#include "teleport.h"

static const skill_id skill_electronics( "electronics" );
static const skill_id skill_firstaid( "firstaid" );
static const skill_id skill_mechanics( "mechanics" );
static const skill_id skill_computer( "computer" );

static const efftype_id effect_adrenaline( "adrenaline" );
static const efftype_id effect_adrenaline_mycus( "adrenaline_mycus" );
static const efftype_id effect_asthma( "asthma" );
static const efftype_id effect_assisted( "assisted" );
static const efftype_id effect_bleed( "bleed" );
static const efftype_id effect_bloodworms( "bloodworms" );
static const efftype_id effect_brainworms( "brainworms" );
static const efftype_id effect_cig( "cig" );
static const efftype_id effect_datura( "datura" );
static const efftype_id effect_dermatik( "dermatik" );
static const efftype_id effect_drunk( "drunk" );
static const efftype_id effect_fungus( "fungus" );
static const efftype_id effect_hallu( "hallu" );
static const efftype_id effect_high( "high" );
static const efftype_id effect_iodine( "iodine" );
static const efftype_id effect_narcosis( "narcosis" );
static const efftype_id effect_meth( "meth" );
static const efftype_id effect_operating( "operating" );
static const efftype_id effect_paincysts( "paincysts" );
static const efftype_id effect_pblue( "pblue" );
static const efftype_id effect_pkill1( "pkill1" );
static const efftype_id effect_pkill2( "pkill2" );
static const efftype_id effect_pkill3( "pkill3" );
static const efftype_id effect_pkill_l( "pkill_l" );
static const efftype_id effect_poison( "poison" );
static const efftype_id effect_sleep( "sleep" );
static const efftype_id effect_stung( "stung" );
static const efftype_id effect_tapeworm( "tapeworm" );
static const efftype_id effect_teleglow( "teleglow" );
static const efftype_id effect_tetanus( "tetanus" );
static const efftype_id effect_took_flumed( "took_flumed" );
static const efftype_id effect_took_prozac( "took_prozac" );
static const efftype_id effect_took_prozac_bad( "took_prozac_bad" );
static const efftype_id effect_took_xanax( "took_xanax" );
static const efftype_id effect_under_op( "under_operation" );
static const efftype_id effect_visuals( "visuals" );
static const efftype_id effect_weed_high( "weed_high" );

static const trait_id trait_DEBUG_BIONICS( "DEBUG_BIONICS" );
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

std::vector<body_part> get_occupied_bodyparts( const bionic_id &bid )
{
    std::vector<body_part> parts;
    for( const auto &element : bid->occupied_bodyparts ) {
        if( element.second > 0 ) {
            parts.push_back( element.first );
        }
    }
    return parts;
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
    const bionic &bio = ( *my_bionics )[cbm_weapon_index];
    mod_power_level( -bio.info().power_activate );
    weapon = real_weapon;
    cbm_weapon_index = -1;
}

void npc::check_or_use_weapon_cbm( const bionic_id &cbm_id )
{
    // if we're already using a bio_weapon, keep using it
    if( cbm_weapon_index >= 0 ) {
        return;
    }
    const float allowed_ratio = static_cast<int>( rules.cbm_reserve ) / 100.0f;
    const units::energy free_power = get_power_level() - get_max_power_level() * allowed_ratio;
    if( free_power <= 0_mJ ) {
        return;
    }

    int index = 0;
    bool found = false;
    for( bionic &i : *my_bionics ) {
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

    if( bio.info().gun_bionic ) {
        const item cbm_weapon = item( bio.info().fake_item );
        bool not_allowed = !rules.has_flag( ally_rule::use_guns ) ||
                           ( rules.has_flag( ally_rule::use_silent ) && !cbm_weapon.is_silent() );
        if( is_player_ally() && not_allowed ) {
            return;
        }

        const int ups_charges = charges_of( "UPS" );
        int ammo_count = weapon.ammo_remaining();
        const int ups_drain = weapon.get_gun_ups_drain();
        if( ups_drain > 0 ) {
            ammo_count = std::min( ammo_count, ups_charges / ups_drain );
        }
        const int cbm_ammo = free_power /  bio.info().power_activate;

        if( weapon_value( weapon, ammo_count ) < weapon_value( cbm_weapon, cbm_ammo ) ) {
            real_weapon = weapon;
            weapon = cbm_weapon;
            cbm_weapon_index = index;
        }
    } else if( bio.info().weapon_bionic && !weapon.has_flag( "NO_UNWIELD" ) &&
               free_power > bio.info().power_activate ) {
        if( is_armed() ) {
            stow_item( weapon );
        }
        if( g->u.sees( pos() ) ) {
            add_msg( m_info, _( "%s activates their %s." ), disp_name(), bio.info().name );
        }

        weapon = item( bio.info().fake_item );
        mod_power_level( -bio.info().power_activate );
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
                 bio.info().name );
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
        if( !enough_power_for( bio.id ) ) {
            add_msg_if_player( m_info, _( "You don't have the power to activate your %s." ),
                               bio.info().name );
            return false;
        }

        if( !burn_fuel( b, true ) ) {
            return false;
        }

        //We can actually activate now, do activation-y things
        mod_power_level( -bio.info().power_activate );
        if( bio.info().toggled || bio.info().charge_time > 0 ) {
            bio.powered = true;
        }
        if( bio.info().charge_time > 0 ) {
            bio.charge_timer = bio.info().charge_time;
        }
        add_msg_if_player( m_info, _( "You activate your %s." ), bio.info().name );
    }

    item tmp_item;
    const w_point weatherPoint = *g->weather.weather_precise;

    // On activation effects go here
    if( bio.info().gun_bionic ) {
        mod_power_level( bio.info().power_activate );
        bio_gun = item( bio.info().fake_item );
        g->refresh_all();
        avatar_action::fire( g->u, g->m, bio_gun, units::to_kilojoule( bio.info().power_activate ) );
    } else if( bio.info().weapon_bionic ) {
        if( weapon.has_flag( "NO_UNWIELD" ) ) {
            add_msg_if_player( m_info, _( "Deactivate your %s first!" ), weapon.tname() );
            mod_power_level( bio.info().power_activate );
            bio.powered = false;
            return false;
        }

        if( !weapon.is_null() ) {
            const std::string query = string_format( _( "Stop wielding %s?" ), weapon.tname() );
            if( !dispose_item( item_location( *this, &weapon ), query ) ) {
                mod_power_level( bio.info().power_activate );
                bio.powered = false;
                return false;
            }
        }

        weapon = item( bio.info().fake_item );
        weapon.invlet = '#';
        if( bio.ammo_count > 0 ) {
            weapon.ammo_set( bio.ammo_loaded, bio.ammo_count );
            avatar_action::fire( g->u, g->m, weapon );
            g->refresh_all();
        }
    } else if( bio.id == "bio_ears" && has_active_bionic( bionic_id( "bio_earplugs" ) ) ) {
        for( bionic &bio : *my_bionics ) {
            if( bio.id == "bio_earplugs" ) {
                bio.powered = false;
                add_msg_if_player( m_info, _( "Your %s automatically turn off." ),
                                   bio.info().name );
            }
        }
    } else if( bio.id == "bio_earplugs" && has_active_bionic( bionic_id( "bio_ears" ) ) ) {
        for( bionic &bio : *my_bionics ) {
            if( bio.id == "bio_ears" ) {
                bio.powered = false;
                add_msg_if_player( m_info, _( "Your %s automatically turns off." ),
                                   bio.info().name );
            }
        }
    } else if( bio.id == "bio_evap" ) {
        const w_point weatherPoint = *g->weather.weather_precise;
        int humidity = get_local_humidity( weatherPoint.humidity, g->weather.weather,
                                           g->is_sheltered( g->u.pos() ) );
        // thirst units = 5 mL
        int water_available = lround( humidity * 3.0 / 100.0 );
        if( water_available == 0 ) {
            bio.powered = false;
            add_msg_if_player( m_bad, _( "Your %s does not have sufficient humidity to function." ),
                               bio.info().name );
            return false;
        } else if( water_available == 1 ) {
            add_msg_if_player( m_mixed,
                               _( "Your %s issues a low humidity warning.  Efficiency will be reduced." ),
                               bio.info().name );
        }
    } else if( bio.id == "bio_tools" ) {
        invalidate_crafting_inventory();
    } else if( bio.id == "bio_cqb" ) {
        const avatar *you = as_avatar();
        if( you && !martial_arts_data.pick_style( *you ) ) {
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
            // Multibash effect, so that doors &c will fall
            g->m.bash( bashpoint, 110 );
            g->m.bash( bashpoint, 110 );
        }

        mod_moves( -100 );
    } else if( bio.id == "bio_time_freeze" ) {
        if( mounted ) {
            add_msg_if_player( m_info, _( "You cannot activate that while mounted." ) );
            return false;
        }
        mod_moves( units::to_kilojoule( get_power_level() ) );
        set_power_level( 0_kJ );
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
        teleport::teleport( *this );
        add_effect( effect_teleglow, 30_minutes );
        mod_moves( -100 );
    } else if( bio.id == "bio_blood_anal" ) {
        static const std::map<efftype_id, std::string> bad_effects = {{
                { effect_fungus, translate_marker( "Fungal Infection" ) },
                { effect_dermatik, translate_marker( "Insect Parasite" ) },
                { effect_stung, translate_marker( "Stung" ) },
                { effect_poison, translate_marker( "Poison" ) },
                // Those may be good for the player, but the scanner doesn't like them
                { effect_drunk, translate_marker( "Alcohol" ) },
                { effect_cig, translate_marker( "Nicotine" ) },
                { effect_meth, translate_marker( "Methamphetamines" ) },
                { effect_high, translate_marker( "Intoxicant: Other" ) },
                { effect_weed_high, translate_marker( "THC Intoxication" ) },
                // This little guy is immune to the blood filter though, as he lives in your bowels.
                { effect_tapeworm, translate_marker( "Intestinal Parasite" ) },
                { effect_bloodworms, translate_marker( "Hemolytic Parasites" ) },
                // These little guys are immune to the blood filter too, as they live in your brain.
                { effect_brainworms, translate_marker( "Intracranial Parasites" ) },
                // These little guys are immune to the blood filter too, as they live in your muscles.
                { effect_paincysts, translate_marker( "Intramuscular Parasites" ) },
                // Tetanus infection.
                { effect_tetanus, translate_marker( "Clostridium Tetani Infection" ) },
                { effect_datura, translate_marker( "Anticholinergic Tropane Alkaloids" ) },
                // TODO: Hallucinations not inducted by chemistry
                { effect_hallu, translate_marker( "Hallucinations" ) },
                { effect_visuals, translate_marker( "Hallucinations" ) },
            }
        };

        static const std::map<efftype_id, std::string> good_effects = {{
                { effect_pkill1, translate_marker( "Minor Painkiller" ) },
                { effect_pkill2, translate_marker( "Moderate Painkiller" ) },
                { effect_pkill3, translate_marker( "Heavy Painkiller" ) },
                { effect_pkill_l, translate_marker( "Slow-Release Painkiller" ) },

                { effect_pblue, translate_marker( "Prussian Blue" ) },
                { effect_iodine, translate_marker( "Potassium Iodide" ) },

                { effect_took_xanax, translate_marker( "Xanax" ) },
                { effect_took_prozac, translate_marker( "Prozac" ) },
                { effect_took_flumed, translate_marker( "Antihistamines" ) },
                { effect_adrenaline, translate_marker( "Adrenaline Spike" ) },
                // Should this be described like that? Does the bionic know what is this?
                { effect_adrenaline_mycus, translate_marker( "Mycal Spike" ) },
            }
        };

        std::vector<std::string> good;
        std::vector<std::string> bad;

        if( get_rad() > 0 ) {
            bad.push_back( _( "Irradiated" ) );
        }

        // TODO: Expose the player's effects to check it in a cleaner way
        for( const auto &pr : bad_effects ) {
            if( has_effect( pr.first ) ) {
                bad.push_back( _( pr.second ) );
            }
        }

        for( const auto &pr : good_effects ) {
            if( has_effect( pr.first ) ) {
                good.push_back( _( pr.second ) );
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
        set_stim( 0 );
        mod_moves( -100 );
    } else if( bio.id == "bio_torsionratchet" ) {
        add_msg_if_player( m_info, _( "Your torsion ratchet locks onto your joints." ) );
    } else if( bio.id == "bio_jointservo" ) {
        add_msg_if_player( m_info, _( "You can now run faster, assisted by joint servomotors." ) );
    } else if( bio.id == "bio_lighter" ) {
        g->refresh_all();
        const cata::optional<tripoint> pnt = choose_adjacent( _( "Start a fire where?" ) );
        if( pnt && g->m.is_flammable( *pnt ) ) {
            g->m.add_field( *pnt, fd_fire, 1 );
            mod_moves( -100 );
        } else {
            add_msg_if_player( m_info, _( "There's nothing to light there." ) );
            mod_power_level( bio.info().power_activate );
        }
    } else if( bio.id == "bio_geiger" ) {
        add_msg_if_player( m_info, _( "Your radiation level: %d" ), get_rad() );
    } else if( bio.id == "bio_radscrubber" ) {
        if( get_rad() > 4 ) {
            mod_rad( -5 );
        } else {
            set_rad( 0 );
        }
    } else if( bio.id == "bio_adrenaline" ) {
        if( has_effect( effect_adrenaline ) ) {
            // Safety
            add_msg_if_player( m_bad, _( "The bionic refuses to activate!" ) );
            mod_power_level( bio.info().power_activate );
        } else {
            add_effect( effect_adrenaline, 20_minutes );
        }

    } else if( bio.id == "bio_emp" ) {
        g->refresh_all();
        if( const cata::optional<tripoint> pnt = choose_adjacent( _( "Create an EMP where?" ) ) ) {
            explosion_handler::emp_blast( *pnt );
            mod_moves( -100 );
        } else {
            mod_power_level( bio.info().power_activate );
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
            mod_power_level( bio.info().power_activate );
        }
    } else if( bio.id == "bio_magnet" ) {
        static const std::set<material_id> affected_materials =
        { material_id( "iron" ), material_id( "steel" ) };
        // Remember all items that will be affected, then affect them
        // Don't "snowball" by affecting some items multiple times
        std::vector<std::pair<item, tripoint>> affected;
        const units::mass weight_cap = weight_capacity();
        for( const tripoint &p : g->m.points_in_radius( pos(), 10 ) ) {
            if( p == pos() || !g->m.has_items( p ) || g->m.has_flag( "SEALED", p ) ) {
                continue;
            }

            map_stack stack = g->m.i_at( p );
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
        for( const std::pair<item, tripoint> &pr : affected ) {
            projectile proj;
            proj.speed  = 50;
            proj.impact = damage_instance::physical( pr.first.weight() / 250_gram, 0, 0, 0 );
            // make the projectile stop one tile short to prevent hitting the player
            proj.range = rl_dist( pr.second, pos() ) - 1;
            proj.proj_effects = {{ "NO_ITEM_DAMAGE", "DRAW_AS_LINE", "NO_DAMAGE_SCALING", "JET" }};

            dealt_projectile_attack  dealt = projectile_attack( proj, pr.second, pos(), 0, this );
            g->m.add_item_or_charges( dealt.end_point, pr.first );
        }

        mod_moves( -100 );
    } else if( bio.id == "bio_lockpick" ) {
        tmp_item = item( "pseuso_bio_picklock", 0 );
        g->refresh_all();
        if( invoke_item( &tmp_item ) == 0 ) {
            if( tmp_item.charges > 0 ) {
                // restore the energy since CBM wasn't used
                mod_power_level( bio.info().power_activate );
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
            // vehicle velocity in mph
            vehwindspeed = abs( vp->vehicle().velocity / 100 );
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
            ctr.charges = units::to_kilojoule( get_power_level() );
            int power_use = invoke_item( &ctr );
            mod_power_level( units::from_kilojoule( -power_use ) );
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
        std::vector<item *> cables = items_with( []( const item & it ) {
            return it.has_flag( "CABLE_SPOOL" );
        } );
        bool has_cable = !cables.empty();
        bool free_cable = false;
        if( !has_cable ) {
            add_msg_if_player( m_info,
                               _( "You need a jumper cable connected to a power source to drain power from it." ) );
        } else {
            for( item *cable : cables ) {
                const std::string state = cable->get_var( "state" );
                if( state == "cable_charger" ) {
                    add_msg_if_player( m_info,
                                       _( "Cable is plugged-in to the CBM but it has to be also connected to the power source." ) );
                }
                if( state == "cable_charger_link" ) {
                    add_msg_if_player( m_info,
                                       _( "You are plugged to the vehicle.  It will charge you if it has some juice in it." ) );
                }
                if( state == "solar_pack_link" ) {
                    add_msg_if_player( m_info,
                                       _( "You are plugged to a solar pack.  It will charge you if it's unfolded and in sunlight." ) );
                }
                if( state == "UPS_link" ) {
                    add_msg_if_player( m_info,
                                       _( "You are plugged to a UPS.  It will charge you if it has some juice in it." ) );
                }
                if( state == "solar_pack" || state == "UPS" ) {
                    add_msg_if_player( m_info,
                                       _( "You have a cable plugged to a portable power source, but you need to plug it in to the CBM." ) );
                }
                if( state == "pay_oyt_cable" ) {
                    add_msg_if_player( m_info,
                                       _( "You have a cable plugged to a vehicle, but you need to plug it in to the CBM." ) );
                }
                if( state == "attach_first" ) {
                    free_cable = true;
                }
            }
        }
        if( free_cable ) {
            add_msg_if_player( m_info,
                               _( "You have at least one free cable in your inventory that you could use to plug yourself in." ) );
        }
    }

    // Recalculate stats (strength, mods from pain etc.) that could have been affected
    reset_encumbrance();
    reset();

    // Also reset crafting inventory cache if this bionic spawned a fake item
    if( !bio.info().fake_item.empty() ) {
        invalidate_crafting_inventory();
    }

    return true;
}

bool Character::deactivate_bionic( int b, bool eff_only )
{
    bionic &bio = ( *my_bionics )[b];

    if( bio.incapacitated_time > 0_turns ) {
        add_msg( m_info, _( "Your %s is shorting out and can't be deactivated." ),
                 bio.info().name );
        return false;
    }

    // Just do the effect, no stat changing or messages
    if( !eff_only ) {
        if( !bio.powered ) {
            // It's already off!
            return false;
        }
        if( !bio.info().toggled ) {
            // It's a fire-and-forget bionic, we can't turn it off but have to wait for
            //it to run out of charge
            add_msg_if_player( m_info, _( "You can't deactivate your %s manually!" ),
                               bio.info().name );
            return false;
        }
        if( get_power_level() < bio.info().power_deactivate ) {
            add_msg( m_info, _( "You don't have the power to deactivate your %s." ),
                     bio.info().name );
            return false;
        }

        //We can actually deactivate now, do deactivation-y things
        mod_power_level( -bio.info().power_deactivate );
        bio.powered = false;
        add_msg_if_player( m_neutral, _( "You deactivate your %s." ), bio.info().name );
    }

    // Deactivation effects go here
    if( bio.info().weapon_bionic ) {
        if( weapon.typeId() == bio.info().fake_item ) {
            add_msg_if_player( _( "You withdraw your %s." ), weapon.tname() );
            if( g->u.sees( pos() ) ) {
                add_msg_if_npc( m_info, _( "<npcname> withdraws %s %s." ), disp_name( true ),
                                weapon.tname() );
            }
            bio.ammo_loaded = weapon.ammo_data() != nullptr ? weapon.ammo_data()->get_id() : "null";
            bio.ammo_count = static_cast<unsigned int>( weapon.ammo_remaining() );
            weapon = item();
            invalidate_crafting_inventory();
        }
    } else if( bio.id == "bio_cqb" ) {
        martial_arts_data.selected_style_check();
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
    if( !bio.info().fake_item.empty() ) {
        invalidate_crafting_inventory();
    }

    return true;
}

bool player::deactivate_bionic( int b, bool eff_only )
{
    const bionic &bio = ( *my_bionics )[b];
    bool success = Character::deactivate_bionic( b, eff_only );
    // Compatibility with old saves without the toolset hammerspace
    if( success && !eff_only && bio.id == "bio_tools" && !has_bionic( bionic_TOOLS_EXTEND ) ) {
        // E X T E N D    T O O L S
        add_bionic( bionic_TOOLS_EXTEND );
    }
    return success;
}

bool Character::burn_fuel( int b, bool start )
{
    bionic &bio = ( *my_bionics )[b];
    if( bio.info().fuel_opts.empty() || bio.is_this_fuel_powered( "muscle" ) ) {
        return true;
    }
    const bool is_metabolism_powered = bio.is_this_fuel_powered( "metabolism" );
    const std::vector<itype_id> &fuel_available = get_fuel_available( bio.id );
    const float effective_efficiency = get_effective_efficiency( b, bio.info().fuel_efficiency );

    if( start && fuel_available.empty() ) {
        add_msg_player_or_npc( m_bad, _( "Your %s does not have enough fuel to start." ),
                               _( "<npcname>'s %s does not have enough fuel to start." ),
                               bio.info().name );
        deactivate_bionic( b );
        return false;
    }
    // don't produce power on start to avoid instant recharge exploit by turning bionic ON/OFF
    //in the menu
    if( !start ) {
        for( const itype_id &fuel : fuel_available ) {
            const item &tmp_fuel = item( fuel );
            const int fuel_energy = tmp_fuel.fuel_energy();
            const bool is_perpetual_fuel = tmp_fuel.has_flag( "PERPETUAL" );

            int current_fuel_stock;
            if( is_metabolism_powered ) {
                current_fuel_stock = std::max( 0.0f, get_stored_kcal() - 0.8f *
                                               get_healthy_kcal() );
            } else if( is_perpetual_fuel ) {
                current_fuel_stock = 1;
            } else {
                current_fuel_stock = std::stoi( get_value( fuel ) );
            }

            if( !bio.has_flag( "SAFE_FUEL_OFF" ) &&
                get_power_level() + units::from_kilojoule( fuel_energy ) * effective_efficiency
                > get_max_power_level() ) {
                if( is_metabolism_powered ) {
                    add_msg_player_or_npc( m_info, _( "Your %s turns off to not waste calories." ),
                                           _( "<npcname>'s %s turns off to not waste calories." ),
                                           bio.info().name );
                } else if( is_perpetual_fuel ) {
                    add_msg_player_or_npc( m_info, _( "Your %s turns off after filling your power banks." ),
                                           _( "<npcname>'s %s turns off after filling their power banks." ),
                                           bio.info().name );
                } else {
                    add_msg_player_or_npc( m_info, _( "Your %s turns off to not waste fuel." ),
                                           _( "<npcname>'s %s turns off to not waste fuel." ),
                                           bio.info().name );
                }
                bio.powered = false;
                deactivate_bionic( b, true );
                return false;
            } else {
                if( current_fuel_stock > 0 ) {

                    if( is_metabolism_powered ) {
                        const int kcal_consumed = fuel_energy;
                        // 1kcal = 4187 J
                        const units::energy power_gain = kcal_consumed * 4184_J * effective_efficiency;
                        mod_stored_kcal( -kcal_consumed );
                        mod_power_level( power_gain );
                    } else if( is_perpetual_fuel ) {
                        if( fuel == itype_id( "sunlight" ) && g->is_in_sunlight( pos() ) ) {
                            const weather_type &wtype = current_weather( pos() );
                            const float tick_sunlight = incident_sunlight( wtype, calendar::turn );
                            const double intensity = tick_sunlight / default_daylight_level();
                            mod_power_level( units::from_kilojoule( fuel_energy ) * intensity * effective_efficiency );
                        } else if( fuel == itype_id( "wind" ) ) {
                            int vehwindspeed = 0;
                            const optional_vpart_position vp = g->m.veh_at( pos() );
                            if( vp ) {
                                // vehicle velocity in mph
                                vehwindspeed = abs( vp->vehicle().velocity / 100 );
                            }
                            const double windpower = get_local_windpower( g->weather.windspeed + vehwindspeed,
                                                     overmap_buffer.ter( global_omt_location() ), pos(), g->weather.winddirection,
                                                     g->is_sheltered( pos() ) );
                            mod_power_level( units::from_kilojoule( fuel_energy ) * windpower * effective_efficiency );
                        }
                    } else {
                        current_fuel_stock -= 1;
                        set_value( fuel, std::to_string( current_fuel_stock ) );
                        update_fuel_storage( fuel );
                        mod_power_level( units::from_kilojoule( fuel_energy ) * effective_efficiency );
                    }

                    heat_emission( b, fuel_energy );
                    g->m.emit_field( pos(), bio.info().power_gen_emission );
                } else {

                    if( is_metabolism_powered ) {
                        add_msg_player_or_npc( m_info,
                                               _( "Stored calories are below the safe threshold, your %s shuts down to preserve your health." ),
                                               _( "Stored calories are below the safe threshold, <npcname>'s %s shuts down to preserve their health." ),
                                               bio.info().name );
                    } else {
                        remove_value( fuel );
                        add_msg_player_or_npc( m_info,
                                               _( "Your %s runs out of fuel and turn off." ),
                                               _( "<npcname>'s %s runs out of fuel and turn off." ),
                                               bio.info().name );
                    }

                    bio.powered = false;
                    deactivate_bionic( b, true );
                    return false;
                }
            }
        }
    }
    return true;
}

void Character::passive_power_gen( int b )
{
    const bionic &bio = ( *my_bionics )[b];
    const float passive_fuel_efficiency = bio.info().passive_fuel_efficiency;
    if( bio.info().fuel_opts.empty() || bio.is_this_fuel_powered( "muscle" ) ||
        passive_fuel_efficiency == 0.0 ) {
        return;
    }
    const float effective_passive_efficiency = get_effective_efficiency( b, passive_fuel_efficiency );
    const std::vector<itype_id> &fuel_available = get_fuel_available( bio.id );

    for( const itype_id &fuel : fuel_available ) {
        const item &tmp_fuel = item( fuel );
        const int fuel_energy = tmp_fuel.fuel_energy();
        if( !tmp_fuel.has_flag( "PERPETUAL" ) ) {
            continue;
        }

        if( fuel == itype_id( "sunlight" ) ) {
            const double modifier = g->natural_light_level( pos().z ) / default_daylight_level();
            mod_power_level( units::from_kilojoule( fuel_energy ) * modifier * effective_passive_efficiency );
        } else if( fuel == itype_id( "wind" ) ) {
            int vehwindspeed = 0;
            const optional_vpart_position vp = g->m.veh_at( pos() );
            if( vp ) {
                // vehicle velocity in mph
                vehwindspeed = abs( vp->vehicle().velocity / 100 );
            }
            const double windpower = get_local_windpower( g->weather.windspeed + vehwindspeed,
                                     overmap_buffer.ter( global_omt_location() ), pos(), g->weather.winddirection,
                                     g->is_sheltered( pos() ) );
            mod_power_level( units::from_kilojoule( fuel_energy ) * windpower * effective_passive_efficiency );
        } else {
            mod_power_level( units::from_kilojoule( fuel_energy ) * effective_passive_efficiency );
        }

        heat_emission( b, fuel_energy );
        g->m.emit_field( pos(), bio.info().power_gen_emission );

    }
}

void Character::heat_emission( int b, int fuel_energy )
{
    const bionic &bio = ( *my_bionics )[b];
    if( !bio.info().exothermic_power_gen ) {
        return;
    }
    const float efficiency = bio.info().fuel_efficiency;

    const int &heat_prod = fuel_energy * ( 1 - efficiency );
    const int &heat_level = std::min( heat_prod / 10, 4 );
    const int &heat_spread = std::max( heat_prod / 10 - heat_level, 1 );
    const emit_id hotness = emit_id( "emit_hot_air" + to_string( heat_level ) + "_cbm" );
    g->m.emit_field( pos(), hotness, heat_spread );
    for( const auto bp : bio.info().occupied_bodyparts ) {
        add_effect( efftype_id( "heating_bionic" ), 2_seconds, bp.first, false, heat_prod );
    }
}

float Character::get_effective_efficiency( int b, float fuel_efficiency )
{
    const bionic &bio = ( *my_bionics )[b];
    const cata::optional<float> &coverage_penalty = bio.info().coverage_power_gen_penalty;
    float effective_efficiency = fuel_efficiency;
    if( coverage_penalty ) {
        int coverage = 0;
        const std::map< body_part, size_t > &occupied_bodyparts = bio.info().occupied_bodyparts;
        for( const std::pair< body_part, size_t > &elem : occupied_bodyparts ) {
            for( const item &i : worn ) {
                if( i.covers( elem.first ) && !i.has_flag( "ALLOWS_NATURAL_ATTACKS" ) &&
                    !i.has_flag( "SEMITANGIBLE" ) &&
                    !i.has_flag( "PERSONAL" ) && !i.has_flag( "AURA" ) ) {
                    coverage += i.get_coverage();
                }
            }
        }
        effective_efficiency = fuel_efficiency * ( 1.0 - ( coverage / ( 100.0 *
                               occupied_bodyparts.size() ) )
                               * coverage_penalty.value() );
    }
    return effective_efficiency;
}

/**
 * @param p the player
 * @param bio the bionic that is meant to be recharged.
 * @param amount the amount of power that is to be spent recharging the bionic.
 * @param factor multiplies the power cost per turn.
 * @param rate divides the number of turns we may charge (rate of 2 discharges in half the time).
 * @return indicates whether we successfully charged the bionic.
 */
static bool attempt_recharge( Character &p, bionic &bio, units::energy &amount, int factor = 1,
                              int rate = 1 )
{
    const bionic_data &info = bio.info();
    const units::energy armor_power_cost = 1_kJ;
    units::energy power_cost = info.power_over_time * factor;
    bool recharged = false;

    if( power_cost > 0_kJ ) {
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
        if( p.get_power_level() >= power_cost ) {
            // Set the recharging cost and charge the bionic.
            amount = power_cost;
            // This is our first turn of charging, so subtract a turn from the recharge delay.
            bio.charge_timer = info.charge_time - rate;
            recharged = true;
        }
    }

    return recharged;
}

void Character::process_bionic( int b )
{
    bionic &bio = ( *my_bionics )[b];
    if( !bio.id->fuel_opts.empty() && bio.is_auto_start_on() ) {
        const float start_threshold = bio.get_auto_start_thresh();
        const std::vector<itype_id> &fuel_available = get_fuel_available( bio.id );
        if( !fuel_available.empty() && get_power_level() <= start_threshold * get_max_power_level() ) {
            g->u.activate_bionic( b );
        } else if( get_power_level() <= start_threshold * get_max_power_level() &&
                   calendar::once_every( 1_hours ) ) {
            add_msg_player_or_npc( m_bad, _( "Your %s does not have enough fuel to use Auto Start." ),
                                   _( "<npcname>'s %s does not have enough fuel to use Auto Start." ),
                                   bio.info().name );
        }
    }

    // Only powered bionics should be processed
    if( !bio.powered ) {
        passive_power_gen( b );
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
                units::energy cost = 0_mJ;
                bool recharged = attempt_recharge( *this, bio, cost, discharge_factor, discharge_rate );
                if( !recharged ) {
                    // No power to recharge, so deactivate
                    bio.powered = false;
                    add_msg_if_player( m_neutral, _( "Your %s powers down." ), bio.info().name );
                    // This purposely bypasses the deactivation cost
                    deactivate_bionic( b, true );
                    return;
                }
                if( cost > 0_mJ ) {
                    mod_power_level( -cost );
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
                               bio.info().name );
        }
    } else if( bio.id == "bio_hydraulics" ) {
        // Sound of hissing hydraulic muscle! (not quite as loud as a car horn)
        sounds::sound( pos(), 19, sounds::sound_t::activity, _( "HISISSS!" ), false, "bionic",
                       "bio_hydraulics" );
    } else if( bio.id == "bio_nanobots" ) {
        if( get_power_level() >= 40_J ) {
            std::forward_list<int> bleeding_bp_parts;
            for( const body_part bp : all_body_parts ) {
                if( has_effect( effect_bleed, bp ) ) {
                    bleeding_bp_parts.push_front( static_cast<int>( bp ) );
                }
            }
            std::vector<int> damaged_hp_parts;
            for( int i = 0; i < num_hp_parts; i++ ) {
                if( hp_cur[i] > 0 && hp_cur[i] < hp_max[i] ) {
                    damaged_hp_parts.push_back( i );
                    // only healed and non-hp parts will have a chance of bleeding removal
                    bleeding_bp_parts.remove( static_cast<int>( hp_to_bp( static_cast<hp_part>( i ) ) ) );
                }
            }
            if( calendar::once_every( 60_turns ) ) {
                bool try_to_heal_bleeding = true;
                if( get_stored_kcal() >= 5 && !damaged_hp_parts.empty() ) {
                    const hp_part part_to_heal = static_cast<hp_part>( damaged_hp_parts[ rng( 0,
                                                      damaged_hp_parts.size() - 1 ) ] );
                    heal( part_to_heal, 1 );
                    mod_stored_kcal( -5 );
                    const body_part bp_healed = hp_to_bp( part_to_heal );
                    int hp_percent = float( hp_cur[part_to_heal] ) / hp_max[part_to_heal] * 100;
                    if( has_effect( effect_bleed, bp_healed ) && rng( 0, 100 ) < hp_percent ) {
                        remove_effect( effect_bleed, bp_healed );
                        try_to_heal_bleeding = false;
                    }
                }

                // if no bleed was removed, try to remove it on some other part
                if( try_to_heal_bleeding && !bleeding_bp_parts.empty() && rng( 0, 1 ) == 1 ) {
                    remove_effect( effect_bleed, static_cast<body_part>( bleeding_bp_parts.front() ) );
                }

            }
            if( !damaged_hp_parts.empty() || !bleeding_bp_parts.empty() ) {
                mod_power_level( -40_J );
            }
        }
    } else if( bio.id == "bio_painkiller" ) {
        const int pkill = get_painkiller();
        const int pain = get_pain();
        int max_pkill = std::min( 150, pain );
        if( pkill < max_pkill ) {
            mod_painkiller( 1 );
            mod_power_level( -2_kJ );
        }

        // Only dull pain so extreme that we can't pkill it safely
        if( pkill >= 150 && pain > pkill && get_stim() > -150 ) {
            mod_pain( -1 );
            // Negative side effect: negative stim
            mod_stim( -1 );
            mod_power_level( -2_kJ );
        }
    } else if( bio.id == "bio_cable" ) {
        if( is_max_power() ) {
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
                if( g->m.is_outside( pos() ) && !is_night( calendar::turn ) &&
                    cable->get_var( "state" ) == "solar_pack_link" ) {
                    double modifier =  g->natural_light_level( pos().z ) / default_daylight_level();
                    // basic solar panel produces 50W = 1 charge/20_seconds = 180 charges/hour(3600)
                    if( is_wearing( "solarpack_on" ) && x_in_y( 180 * modifier, 3600 ) ) {
                        mod_power_level( 1_kJ );
                    }
                    // quantum solar backpack = solar panel x6
                    if( is_wearing( "q_solarpack_on" ) && x_in_y( 6 * 180 * modifier, 3600 ) ) {
                        mod_power_level( 1_kJ );
                    }
                }
                if( cable->get_var( "state" ) == "UPS_link" ) {
                    static const item_filter used_ups = [&]( const item & itm ) {
                        return itm.get_var( "cable" ) == "plugged_in";
                    };
                    if( has_charges( "UPS_off", 1, used_ups ) ) {
                        use_charges( "UPS_off", 1, used_ups );
                        mod_power_level( 1_kJ );
                    } else if( has_charges( "adv_UPS_off", 1, used_ups ) ) {
                        use_charges( "adv_UPS_off", roll_remainder( 0.6 ), used_ups );
                        mod_power_level( 1_kJ );
                    }
                }
                continue;
            }
            const optional_vpart_position vp = g->m.veh_at( *target );
            if( !vp ) {
                continue;
            }

            wants_power_amt = vp->vehicle().discharge_battery( wants_power_amt );
            if( wants_power_amt == 0 ) {
                mod_power_level( 1_kJ );
                break;
            }
        }

        if( wants_power_amt < battery_per_power &&
            wants_power_amt > 0 &&
            x_in_y( battery_per_power - wants_power_amt, battery_per_power ) ) {
            mod_power_level( 1_kJ );
        }
    } else if( bio.id == "bio_gills" ) {
        if( has_effect( effect_asthma ) ) {
            add_msg_if_player( m_good,
                               _( "You feel your throat open up and air filling your lungs!" ) );
            remove_effect( effect_asthma );
        }
    } else if( bio.id == "bio_evap" ) {
        // Aero-Evaporator provides water at 60 watts with 2 L / kWh efficiency
        // which is 10 mL per 5 minutes.  Humidity can modify the amount gained.
        if( calendar::once_every( 5_minutes ) ) {
            const w_point weatherPoint = *g->weather.weather_precise;
            int humidity = get_local_humidity( weatherPoint.humidity, g->weather.weather,
                                               g->is_sheltered( g->u.pos() ) );
            // in thirst units = 5 mL water
            int water_available = lround( humidity * 3.0 / 100.0 );
            // At 50% relative humidity or more, the player will draw 10 mL
            // At 16% relative humidity or less, the bionic will give up
            if( water_available == 0 ) {
                add_msg_if_player( m_bad,
                                   _( "There is not enough humidity for your %s to function." ),
                                   bio.info().name );
                deactivate_bionic( b );
            } else if( water_available == 1 ) {
                add_msg_if_player( m_mixed,
                                   _( "Your %s issues a low humidity warning.  Efficiency is reduced." ),
                                   bio.info().name );
            }

            mod_thirst( -water_available );
        }

        if( get_thirst() < -40 ) {
            add_msg_if_player( m_good,
                               _( "You are properly hydrated.  Your %s chirps happily." ),
                               bio.info().name );
            deactivate_bionic( b );
        }
    } else if( bio.id == "afs_bio_dopamine_stimulators" ) {
        // Aftershock
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
    std::set<body_part> bp_hurt;
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
                    if( bp_hurt.count( mutate_to_main_part( bp ) ) > 0 ) {
                        continue;
                    }
                    bp_hurt.emplace( mutate_to_main_part( bp ) );
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
                    if( bp_hurt.count( mutate_to_main_part( bp ) ) > 0 ) {
                        continue;
                    }
                    bp_hurt.emplace( mutate_to_main_part( bp ) );
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
    std::set<body_part> bp_hurt;
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
                    if( bp_hurt.count( mutate_to_main_part( bp ) ) > 0 ) {
                        continue;
                    }
                    bp_hurt.emplace( mutate_to_main_part( bp ) );
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
                    if( bp_hurt.count( mutate_to_main_part( bp ) ) > 0 ) {
                        continue;
                    }
                    bp_hurt.emplace( mutate_to_main_part( bp ) );
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
        has_trait( trait_DEBUG_BIONICS ) ) {
        return true;
    }

    const int weight = units::to_kilogram( patient.bodyweight() ) / 10;
    const requirement_data req_anesth = *requirement_id( "anesthetic" ) *
                                        cbm->bionic->difficulty * 2 * weight;

    std::vector<const item *> b_filter = crafting_inventory().items_with( []( const item & it ) {
        // legacy
        return it.has_flag( "ANESTHESIA" );
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

int player::bionics_pl_skill( const skill_id &most_important_skill,
                              const skill_id &important_skill,
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
        add_msg( m_neutral, _( "A lifetime of augmentation has taught %s a thing or two…" ),
                 disp_name() );
    }
    return pl_skill;
}

// bionic manipulation chance of success
int bionic_manip_cos( float adjusted_skill, bool autodoc, int bionic_difficulty )
{
    if( ( autodoc && get_option < bool > ( "SAFE_AUTODOC" ) ) ||
        g->u.has_trait( trait_DEBUG_BIONICS ) ) {
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
                _( "WARNING: Removing a reactor may leave radioactive material!  Remove anyway?" ) ) ) {
            return false;
        }
    }

    for( const bionic_id bid : get_bionics() ) {
        if( bid->is_included( b_id ) ) {
            popup( _( "%s must remove the %s bionic to remove the %s." ), installer.disp_name(),
                   bid->name, b_id->name );
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
        adjusted_skill = installer.bionics_adjusted_skill( skill_firstaid,
                         skill_computer,
                         skill_electronics,
                         skill_level );
    } else {
        adjusted_skill = installer.bionics_adjusted_skill( skill_electronics,
                         skill_firstaid,
                         skill_mechanics,
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
                _( "WARNING: %i percent chance of SEVERE damage to all body parts!  Continue anyway?" ),
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
        adjusted_skill = installer.bionics_adjusted_skill( skill_firstaid,
                         skill_computer,
                         skill_electronics,
                         skill_level );
        pl_skill = installer.bionics_pl_skill( skill_firstaid,
                                               skill_computer,
                                               skill_electronics,
                                               skill_level );
    } else {
        adjusted_skill = installer.bionics_adjusted_skill( skill_electronics,
                         skill_firstaid,
                         skill_mechanics,
                         skill_level );
        pl_skill = installer.bionics_pl_skill( skill_electronics,
                                               skill_firstaid,
                                               skill_mechanics,
                                               skill_level );
    }

    int chance_of_success = bionic_manip_cos( adjusted_skill + assist_bonus, autodoc, difficulty + 2 );

    // Surgery is imminent, retract claws or blade if active
    for( size_t i = 0; i < installer.my_bionics->size(); i++ ) {
        const bionic &bio = ( *installer.my_bionics )[ i ];
        if( bio.powered && bio.info().weapon_bionic ) {
            installer.deactivate_bionic( i );
        }
    }

    int success = chance_of_success - rng( 1, 100 );
    if( installer.has_trait( trait_DEBUG_BIONICS ) ) {
        perform_uninstall( b_id, difficulty, success, bionics[b_id].capacity, pl_skill );
        return true;
    }
    assign_activity( activity_id( "ACT_OPERATION" ), to_moves<int>( difficulty * 20_minutes ) );

    activity.values.push_back( difficulty );
    activity.values.push_back( success );
    activity.values.push_back( units::to_kilojoule( b_id->capacity ) );
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
    for( const std::pair<body_part, size_t> &elem : b_id->occupied_bodyparts ) {
        activity.values.push_back( elem.first );
        add_effect( effect_under_op, difficulty * 20_minutes, elem.first, true, difficulty );
    }
    return true;
}

void player::perform_uninstall( bionic_id bid, int difficulty, int success, units::energy power_lvl,
                                int pl_skill )
{
    if( success > 0 ) {
        g->events().send<event_type::removes_cbm>( getID(), bid );

        // until bionics can be flagged as non-removable
        add_msg_player_or_npc( m_neutral, _( "Your parts are jiggled back into their familiar places." ),
                               _( "<npcname>'s parts are jiggled back into their familiar places." ) );
        add_msg( m_good, _( "Successfully removed %s." ), bid.obj().name );
        remove_bionic( bid );

        // remove power bank provided by bionic
        mod_max_power_level( -power_lvl );

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
        g->events().send<event_type::fails_to_remove_cbm>( getID(), bid );
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
    // don't stack up the effect
    if( !installer.has_effect( effect_operating ) ) {
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
        patient.mod_max_power_level( -target_cbm.info().capacity );
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
        adjusted_skill = installer.bionics_adjusted_skill( skill_firstaid,
                         skill_computer,
                         skill_electronics,
                         skill_level );
    } else {
        adjusted_skill = installer.bionics_adjusted_skill( skill_electronics,
                         skill_firstaid,
                         skill_mechanics,
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
                _( "WARNING: %i percent chance of failure that may result in damage, pain, or a faulty installation!  Continue anyway?" ),
                ( 100 - static_cast<int>( chance_of_success ) ) ) ) {
            return false;
        }
    }

    return true;
}

bool player::install_bionics( const itype &type, player &installer, bool autodoc,
                              int skill_level )
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
        adjusted_skill = installer.bionics_adjusted_skill( skill_firstaid,
                         skill_computer,
                         skill_electronics,
                         skill_level );
        pl_skill = installer.bionics_pl_skill( skill_firstaid,
                                               skill_computer,
                                               skill_electronics,
                                               skill_level );
    } else {
        adjusted_skill = installer.bionics_adjusted_skill( skill_electronics,
                         skill_firstaid,
                         skill_mechanics,
                         skill_level );
        pl_skill = installer.bionics_pl_skill( skill_electronics,
                                               skill_firstaid,
                                               skill_mechanics,
                                               skill_level );
    }
    int chance_of_success = bionic_manip_cos( adjusted_skill + assist_bonus, autodoc, difficulty );

    // Practice skills only if conducting manual installation
    if( !autodoc ) {
        installer.practice( skill_electronics, static_cast<int>( ( 100 - chance_of_success ) * 1.5 ) );
        installer.practice( skill_firstaid, static_cast<int>( ( 100 - chance_of_success ) * 1.0 ) );
        installer.practice( skill_mechanics, static_cast<int>( ( 100 - chance_of_success ) * 0.5 ) );
    }

    int success = chance_of_success - rng( 0, 99 );
    if( installer.has_trait( trait_DEBUG_BIONICS ) ) {
        perform_install( bioid, upbioid, difficulty, success, pl_skill, "NOT_MED",
                         bioid->canceled_mutations, pos() );
        return true;
    }
    assign_activity( activity_id( "ACT_OPERATION" ), to_moves<int>( difficulty * 20_minutes ) );
    activity.values.push_back( difficulty );
    activity.values.push_back( success );
    activity.values.push_back( units::to_millijoule( bioid->capacity ) );
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
    for( const std::pair<body_part, size_t> &elem : bioid->occupied_bodyparts ) {
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
        g->events().send<event_type::installs_cbm>( getID(), bid );
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
        g->events().send<event_type::fails_to_install_cbm>( getID(), bid );

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
        if( fail_type > 3 ) {
            fail_type = rng( 1, 3 );
        }
    }
    if( fail_type <= 0 ) {
        add_msg( m_neutral, _( "The installation fails without incident." ) );
        drop_cbm = true;
    } else {
        std::set<body_part> bp_hurt;
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
                        if( bp_hurt.count( mutate_to_main_part( bp ) ) > 0 ) {
                            continue;
                        }
                        bp_hurt.emplace( mutate_to_main_part( bp ) );
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

                // We've got all the bad bionics!
                if( valid.empty() ) {
                    if( has_max_power() ) {
                        units::energy old_power = get_max_power_level();
                        add_msg( m_bad, _( "%s lose power capacity!" ), disp_name() );
                        set_max_power_level( units::from_kilojoule( rng( 0,
                                             units::to_kilojoule( get_max_power_level() ) - 25 ) ) );
                        if( is_player() ) {
                            g->memorial().add(
                                pgettext( "memorial_male", "Lost %d units of power capacity." ),
                                pgettext( "memorial_female", "Lost %d units of power capacity." ),
                                units::to_kilojoule( old_power - get_max_power_level() ) );
                        }
                    }
                    // TODO: What if we can't lose power capacity?  No penalty?
                } else {
                    const bionic_id &id = random_entry( valid );
                    add_bionic( id );
                    g->events().send<event_type::installs_faulty_cbm>( getID(), id );
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
    std::string desc = intro;
    for( const auto &elem : bio_id->occupied_bodyparts ) {
        desc += ( each_bp_on_new_line ? "\n" : " " );
        //~ <Bodypart name> (<number of occupied slots> slots);
        desc += string_format( _( "%s (%i slots);" ),
                               body_part_name_as_heading( elem.first, 1 ),
                               elem.second );
    }
    return desc;
}

int player::get_used_bionics_slots( const body_part bp ) const
{
    int used_slots = 0;
    for( const bionic_id bid : get_bionics() ) {
        auto search = bid->occupied_bodyparts.find( bp );
        if( search != bid->occupied_bodyparts.end() ) {
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
    return convert_bp( bp )->bionic_slots();
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

    const units::energy pow_up = b->capacity;
    mod_max_power_level( pow_up );
    if( b == "bio_power_storage" || b == "bio_power_storage_mkII" ) {
        add_msg_if_player( m_good, _( "Increased storage capacity by %i." ),
                           units::to_kilojoule( pow_up ) );
        // Power Storage CBMs are not real bionic units, so return without adding it to my_bionics
        return;
    }

    my_bionics->push_back( bionic( b, get_free_invlet( *this ) ) );
    if( b == "bio_tools" || b == "bio_ears" ) {
        activate_bionic( my_bionics->size() - 1 );
    }

    for( const bionic_id &inc_bid : b->included_bionics ) {
        add_bionic( inc_bid );
    }

    reset_encumbrance();
    recalc_sight_limits();
}

void player::remove_bionic( const bionic_id &b )
{
    bionic_collection new_my_bionics;
    for( bionic &i : *my_bionics ) {
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
    units::energy lvl = get_max_power_level();

    // exclude amount of power capacity obtained via non-power-storage CBMs
    for( const bionic &it : *my_bionics ) {
        lvl -= it.info().capacity;
    }

    std::pair<int, int> results( 0, 0 );
    if( lvl <= 0_kJ ) {
        return results;
    }

    const units::energy pow_mkI = bionic_id( "bio_power_storage" )->capacity;
    const units::energy pow_mkII = bionic_id( "bio_power_storage_mkII" )->capacity;

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

void player::clear_bionics()
{
    my_bionics->clear();
}

void reset_bionics()
{
    bionics.clear();
    faulty_bionics.clear();
}

static bool get_bool_or_flag( const JsonObject &jsobj, const std::string &name,
                              const std::string &flag,
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

void load_bionic( const JsonObject &jsobj )
{

    bionic_data new_bionic;

    const bionic_id id( jsobj.get_string( "id" ) );
    jsobj.read( "name", new_bionic.name );
    jsobj.read( "description", new_bionic.description );
    assign( jsobj, "act_cost", new_bionic.power_activate, false, 0_kJ );

    new_bionic.toggled = get_bool_or_flag( jsobj, "toggled", "BIONIC_TOGGLED", false );
    // Requires ability to toggle
    assign( jsobj, "deact_cost", new_bionic.power_deactivate, false, 0_kJ );

    new_bionic.charge_time = jsobj.get_int( "time", 0 );
    // Requires a non-zero time
    assign( jsobj, "react_cost", new_bionic.power_over_time, false, 0_kJ );

    assign( jsobj, "capacity", new_bionic.capacity, false, 0_kJ );

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
    new_bionic.passive_fuel_efficiency = jsobj.get_float( "passive_fuel_efficiency", 0 );

    if( new_bionic.gun_bionic && new_bionic.weapon_bionic ) {
        debugmsg( "Bionic %s specified as both gun and weapon bionic", id.c_str() );
    }

    new_bionic.fake_item = jsobj.get_string( "fake_item", "" );

    new_bionic.weight_capacity_modifier = jsobj.get_float( "weight_capacity_modifier", 1.0 );

    assign( jsobj, "weight_capacity_bonus", new_bionic.weight_capacity_bonus, false, 0_gram );
    assign( jsobj, "exothermic_power_gen", new_bionic.exothermic_power_gen );
    assign( jsobj, "power_gen_emission", new_bionic.power_gen_emission );
    assign( jsobj, "coverage_power_gen_penalty", new_bionic.coverage_power_gen_penalty );

    jsobj.read( "canceled_mutations", new_bionic.canceled_mutations );
    jsobj.read( "included_bionics", new_bionic.included_bionics );
    jsobj.read( "included", new_bionic.included );
    jsobj.read( "upgraded_bionic", new_bionic.upgraded_bionic );
    jsobj.read( "fuel_options", new_bionic.fuel_opts );
    jsobj.read( "fuel_capacity", new_bionic.fuel_capacity );

    for( JsonArray ja : jsobj.get_array( "stat_bonus" ) ) {
        new_bionic.stat_bonus.emplace( io::string_to_enum<Character::stat>( ja.get_string( 0 ) ),
                                       ja.get_int( 1 ) );
    }

    for( JsonArray ja : jsobj.get_array( "encumbrance" ) ) {
        new_bionic.encumbrance.emplace( get_body_part_token( ja.get_string( 0 ) ),
                                        ja.get_int( 1 ) );
    }

    for( JsonArray ja : jsobj.get_array( "occupied_bodyparts" ) ) {
        new_bionic.occupied_bodyparts.emplace( get_body_part_token( ja.get_string( 0 ) ),
                                               ja.get_int( 1 ) );
    }

    for( JsonArray ja : jsobj.get_array( "env_protec" ) ) {
        new_bionic.env_protec.emplace( get_body_part_token( ja.get_string( 0 ) ),
                                       ja.get_int( 1 ) );
    }

    new_bionic.activated = new_bionic.toggled ||
                           new_bionic.power_activate > 0_kJ ||
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
    for( const std::pair<bionic_id, bionic_data> &bio : bionics ) {
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
        for( const bionic_id bid : bio.second.included_bionics ) {
            if( !bid.is_valid() ) {
                debugmsg( "Bionic %s includes undefined bionic %s",
                          bio.first.c_str(), bid.c_str() );
            }
            if( !bid->occupied_bodyparts.empty() ) {
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
    for( const std::pair<bionic_id, bionic_data> &bio : bionics ) {
        if( bio.second.upgraded_bionic ) {
            bionics[ bio.second.upgraded_bionic ].available_upgrades.insert( bio.first );
        }
    }
}

void bionic::set_flag( const std::string flag )
{
    bionic_tags.insert( flag );
}

void bionic::remove_flag( const std::string flag )
{
    bionic_tags.erase( flag );
}

bool bionic::has_flag( const std::string flag ) const
{
    return bionic_tags.find( flag ) != bionic_tags.end();
}

int bionic::get_quality( const quality_id &quality ) const
{
    const auto &i = info();
    if( i.fake_item.empty() ) {
        return INT_MIN;
    }

    return item( i.fake_item ).get_quality( quality );
}

bool bionic::is_this_fuel_powered( const itype_id &this_fuel ) const
{
    const std::vector<itype_id> fuel_op = info().fuel_opts;
    return std::find( fuel_op.begin(), fuel_op.end(), this_fuel ) != fuel_op.end();
}

void bionic::toggle_safe_fuel_mod()
{
    if( info().fuel_opts.empty() ) {
        return;
    }
    if( !has_flag( "SAFE_FUEL_OFF" ) ) {
        set_flag( "SAFE_FUEL_OFF" );
    } else {
        remove_flag( "SAFE_FUEL_OFF" );
    }
}

void bionic::toggle_auto_start_mod()
{
    if( info().fuel_opts.empty() ) {
        return;
    }
    if( !is_auto_start_on() ) {
        uilist tmenu;
        tmenu.text = _( "Chose Start Power Level Threshold" );
        tmenu.addentry( 1, true, 'o', _( "No Power Left" ) );
        tmenu.addentry( 2, true, 't', _( "Below 25 %%" ) );
        tmenu.addentry( 3, true, 'f', _( "Below 50 %%" ) );
        tmenu.addentry( 4, true, 's', _( "Below 75 %%" ) );
        tmenu.query();

        switch( tmenu.ret ) {
            case 1:
                set_auto_start_thresh( 0.0 );
                break;
            case 2:
                set_auto_start_thresh( 0.25 );
                break;
            case 3:
                set_auto_start_thresh( 0.5 );
                break;
            case 4:
                set_auto_start_thresh( 0.75 );
                break;
            default:
                break;
        }
    } else {
        set_auto_start_thresh( -1.0 );
    }
}

void bionic::set_auto_start_thresh( float val )
{
    auto_start_threshold = val;
}

float bionic::get_auto_start_thresh() const
{
    return auto_start_threshold;
}

bool bionic::is_auto_start_on() const
{
    return get_auto_start_thresh() > -1.0;
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
    json.member( "bionic_tags", bionic_tags );
    if( incapacitated_time > 0_turns ) {
        json.member( "incapacitated_time", incapacitated_time );
    }
    if( is_auto_start_on() ) {
        json.member( "auto_start_threshold", auto_start_threshold );
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
    if( jo.has_float( "auto_start_threshold" ) ) {
        auto_start_threshold = jo.get_float( "auto_start_threshold" );
    }
    if( jo.has_array( "bionic_tags" ) ) {
        for( const std::string &line : jo.get_array( "bionic_tags" ) ) {
            bionic_tags.insert( line );
        }
    }

}

void player::introduce_into_anesthesia( const time_duration &duration, player &installer,
                                        bool needs_anesthesia )   //used by the Autodoc
{
    if( installer.has_trait( trait_DEBUG_BIONICS ) ) {
        installer.add_msg_if_player( m_info,
                                     _( "You tell the pain to bug off and proceed with the operation." ) );
        return;
    }
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
                               _( "You feel excited as the Autodoc slices painlessly into you.  You enjoy the sight of scalpels slicing you apart." ) );
        } else {
            add_msg_if_player( m_mixed,
                               _( "You stay very, very still, focusing intently on an interesting stain on the ceiling, as the Autodoc slices painlessly into you." ) );
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
