#include "item_tname.h"

#include <algorithm>
#include <array>
#include <iomanip>
#include <iterator>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "avatar.h"
#include "cata_utility.h"
#include "color.h"
#include "coordinates.h"
#include "debug.h"
#include "enum_conversions.h"
#include "enums.h"
#include "fault.h"
#include "flag.h"
#include "flat_set.h"
#include "item.h"
#include "item_category.h"
#include "item_contents.h"
#include "item_pocket.h"
#include "itype.h"
#include "mutation.h"
#include "options.h"
#include "point.h"
#include "recipe.h"
#include "relic.h"
#include "string_formatter.h"
#include "text_snippets.h"
#include "translation.h"
#include "translation_cache.h"
#include "translations.h"
#include "type_id.h"
#include "units.h"
#include "value_ptr.h"

static const flag_id json_flag_HINT_THE_LOCATION( "HINT_THE_LOCATION" );

static const itype_id itype_barrel_small( "barrel_small" );
static const itype_id itype_disassembly( "disassembly" );

static const skill_id skill_survival( "survival" );

using segment_bitset = tname::segment_bitset;

namespace
{

std::string noop( item const & /* it */, unsigned int /* quantity */,
                  segment_bitset const & /* segments */ )
{
    return {};
}

std::string faults( item const &it, unsigned int /* quantity */,
                    segment_bitset const &/* segments */ )
{
    std::string damtext;
    // add first prefix if item has a fault that defines a prefix (prioritize?)
    for( const fault_id &f : it.faults ) {
        const std::string prefix = f->item_prefix();
        if( !prefix.empty() ) {
            damtext = prefix + " ";
            break;
        }
    }

    return damtext;
}

std::string faults_suffix( item const &it, unsigned int /* quantity */,
                           segment_bitset const &/* segments */ )
{
    std::string text;
    for( const fault_id &f : it.faults ) {
        const std::string suffix = f->item_suffix();
        if( !suffix.empty() ) {
            text = "(" + suffix + ") ";
            break;
        }
    }
    // remove excess space, add one space before the string
    if( !text.empty() ) {
        text.pop_back();
        const std::string ret = " " + text;
        return ret;
    } else {
        return "";
    }
}

std::string dirt_symbol( item const &it, unsigned int /* quantity */,
                         segment_bitset const &/* segments */ )
{
    return it.dirt_symbol();
}

std::string overheat_symbol( item const &it, unsigned int /* quantity */,
                             segment_bitset const &/* segments */ )
{
    return it.overheat_symbol();
}

std::string pre_asterisk( item const &it, unsigned int /* quantity */,
                          segment_bitset const &/* segments */ )
{
    if( it.is_favorite && get_option<std::string>( "ASTERISK_POSITION" ) == "prefix" ) {
        return _( "* " ); // Display asterisk for favorite items, before item's name
    }
    return {};
}

std::string durability( item const &it, unsigned int /* quantity */,
                        segment_bitset const &/* segments */ )
{
    const std::string item_health_option = get_option<std::string>( "ITEM_HEALTH" );
    const bool show_bars = item_health_option == "both" || item_health_option == "bars";
    if( !it.is_null() &&
        ( it.damage() != 0 || ( it.degradation() > 0 && it.degradation() >= it.max_damage() / 5 ) ||
          ( show_bars && it.is_armor() ) ) ) {
        return it.durability_indicator();
    }
    return {};
}

std::string wheel_diameter( item const &it, unsigned int /* quantity */,
                            segment_bitset const &/* segments */ )
{
    if( it.is_wheel() && it.type->wheel->diameter > 0 ) {
        return string_format( pgettext( "vehicle adjective", "%d\" " ), it.type->wheel->diameter );
    }
    return {};
}

std::string burn( item const &it, unsigned int /* quantity */,
                  segment_bitset const &/* segments */ )
{
    if( !it.made_of_from_type( phase_id::LIQUID ) ) {
        if( it.volume() >= 1_liter && it.burnt * 125_ml >= it.volume() ) {
            return pgettext( "burnt adjective", "badly burnt " );
        }
        if( it.burnt > 0 ) {
            return pgettext( "burnt adjective", "burnt " );
        }
    }
    return {};
}

std::string custom_item_prefix( item const &it, unsigned int /* quantity */,
                                segment_bitset const &/* segments */ )
{
    std::string prefix;
    for( const flag_id &f : it.get_prefix_flags() ) {
        prefix += f->item_prefix().translated();
    }
    return prefix;
}

std::string custom_item_suffix( item const &it, unsigned int /* quantity */,
                                segment_bitset const &/* segments */ )
{
    std::string suffix;
    for( const flag_id &f : it.get_suffix_flags() ) {
        suffix.insert( 0, f->item_suffix().translated() );
    }
    return suffix;
}

std::string label( item const &it, unsigned int quantity, segment_bitset const &segments )
{
    if( !it.is_craft() ) {
        return it.label( quantity, segments[tname::segments::VARIANT],
                         ( segments & tname::tname_conditional ) == tname::tname_conditional,
                         segments[tname::segments::CORPSE] );
    }
    return {};
}

std::string mods( item const &it, unsigned int /* quantity */,
                  segment_bitset const &/* segments */ )
{
    int amt = 0;
    if( it.is_armor() && it.has_clothing_mod() ) {
        amt++;
    }
    if( ( ( it.is_gun() || it.is_tool() || it.is_magazine() ) && !it.is_power_armor() ) ||
        it.get_contents().has_additional_pockets() ) {

        for( const item *mod : it.is_gun() ? it.gunmods() : it.toolmods() ) {
            if( !it.type->gun || !it.type->gun->built_in_mods.count( mod->typeId() ) ||
                !it.type->gun->default_mods.count( mod->typeId() ) ) {
                amt++;
            }
        }
        amt += it.get_contents().get_added_pockets().size();
    }
    if( amt > 0 ) {
        return string_format( "+%d", amt );
    }
    return {};
}

std::string craft( item const &it, unsigned int /* quantity */,
                   segment_bitset const &/* segments */ )
{
    if( it.is_craft() ) {
        std::string maintext;
        if( it.typeId() == itype_disassembly ) {
            maintext = string_format( _( "in progress disassembly of %s" ),
                                      it.get_making().result_name() );
        } else {
            maintext = string_format( _( "in progress %s" ), it.get_making().result_name() );
        }
        if( it.charges > 1 ) {
            maintext += string_format( " (%d)", it.charges );
        }
        const int percent_progress = it.item_counter / 100000;
        return string_format( "%s (%d%%)", maintext, percent_progress );
    }
    return {};
}

std::string wbl_mark( item const &it, unsigned int /* quantity */,
                      segment_bitset const &/* segments */ )
{
    std::vector<const item_pocket *> pkts = it.get_all_contained_pockets();
    bool wl = false;
    bool bl = false;
    bool player_wbl = false;
    for( item_pocket const *pkt : pkts ) {
        bool const wl_ = !pkt->settings.get_item_whitelist().empty() ||
                         !pkt->settings.get_category_whitelist().empty();
        bool const bl_ = !pkt->settings.get_item_blacklist().empty() ||
                         !pkt->settings.get_category_blacklist().empty();
        player_wbl |= ( wl_ || bl_ ) && pkt->settings.was_edited();
        wl |= wl_;
        bl |= bl_;
    }
    return
        wl || bl ? colorize( "⁺", player_wbl ? c_light_gray : c_dark_gray ) : std::string();
}

std::string contents( item const &it, unsigned int /* quantity */,
                      segment_bitset const &segments )
{
    item_contents const &contents = it.get_contents();
    if( item::aggregate_t aggi = it.aggregated_contents();
        segments[tname::segments::CONTENTS_FULL] && aggi ) {

        const item &contents_item = *aggi.header;
        const unsigned aggi_count = contents_item.count_by_charges() && contents_item.charges > 1
                                    ? contents_item.charges
                                    : aggi.count;
        const unsigned total_count = contents_item.count_by_charges() && contents_item.charges > 1
                                     ? contents_item.charges
                                     : contents.num_item_stacks();

        // without full contents for nested items to prevent excessively long names
        segment_bitset abrev( aggi.info.bits );
        abrev.set( tname::segments::CONTENTS_FULL, false );
        abrev.set( tname::segments::CONTENTS_ABREV, aggi_count == 1 );
        abrev.set( tname::segments::CATEGORY,
                   abrev[tname::segments::CATEGORY] && !abrev[tname::segments::TYPE] );
        std::string const contents_tname = contents_item.tname( aggi_count, abrev );
        std::string const ctnc = abrev[tname::segments::CATEGORY]
                                 ? contents_tname
                                 : colorize( contents_tname, contents_item.color_in_inventory() );

        // Don't append an item count for single items, or items that are ammo-exclusive
        // (eg: quivers), as they format their own counts.
        if( total_count > 1 && it.ammo_types().empty() ) {
            if( total_count == aggi_count ) {
                return string_format(
                           segments[tname::segments::CONTENTS_COUNT]
                           //~ [container item name] " > [count] [type]"
                           ? npgettext( "item name", " > %1$zd %2$s", " > %1$zd %2$s", total_count )
                           : " > %2$s",
                           total_count, ctnc );
            }
            return string_format(
                       segments[tname::segments::CONTENTS_COUNT]
                       //~ [container item name] " > [count] [type] / [total] items"
                       ? npgettext( "item name", " > %1$zd %2$s / %3$zd item", " > %1$zd %2$s / %3$zd items", total_count )
                       : " > %2$s",
                       aggi_count, ctnc, total_count );
        }
        return string_format( " > %1$s", ctnc );

    } else if( segments[tname::segments::CONTENTS_ABREV] && !contents.empty_container() &&
               contents.num_item_stacks() != 0 ) {
        std::string const suffix =
            segments[tname::segments::CONTENTS_COUNT]
            ? npgettext( "item name",
                         //~ [container item name] " > [count] item"
                         " > %1$zd item", " > %1$zd items", contents.num_item_stacks() )
            : _( " > items" );
        return string_format( suffix, contents.num_item_stacks() );
    }
    return {};
}

std::string food_traits( item const &it, unsigned int /* quantity */,
                         segment_bitset const &/* segments */ )
{
    if( it.is_food() ) {
        if( it.has_flag( flag_HIDDEN_POISON ) &&
            get_avatar().get_greater_skill_or_knowledge_level( skill_survival ) >= 3 ) {
            return _( " (poisonous)" );
        } else if( it.has_flag( flag_HIDDEN_HALLU ) &&
                   get_avatar().get_greater_skill_or_knowledge_level( skill_survival ) >= 5 ) {
            return _( " (hallucinogenic)" );
        }
    }
    return {};
}

std::string location_hint( item const &it, unsigned int /* quantity */,
                           segment_bitset const &/* segments */ )
{
    if( it.has_flag( json_flag_HINT_THE_LOCATION ) && it.has_var( "spawn_location" ) ) {
        tripoint_abs_omt loc( coords::project_to<coords::omt>(
                                  it.get_var( "spawn_location", tripoint_abs_ms::zero ) ) );
        tripoint_abs_omt player_loc( coords::project_to<coords::omt>(
                                         get_avatar().pos_abs() ) );
        int dist = rl_dist( player_loc, loc );
        if( dist < 1 ) {
            return _( " (from here)" );
        } else if( dist < 6 ) {
            return _( " (from nearby)" );
        } else if( dist < 30 ) {
            return _( " (from this area)" );
        }
        return _( " (from far away)" );
    }
    return {};
}

std::string ethereal( item const &it, unsigned int /* quantity */,
                      segment_bitset const &/* segments */ )
{
    if( it.ethereal ) {
        return string_format( _( " (%s turns)" ), it.get_var( "ethereal", 0 ) );
    }
    return {};
}

std::string food_status( item const &it, unsigned int /* quantity */,
                         segment_bitset const &/* segments */ )
{
    std::string tagtext;
    if( it.goes_bad() || it.is_food() ) {
        if( it.has_own_flag( flag_DIRTY ) ) {
            tagtext += _( " (dirty)" );
        } else if( it.rotten() ) {
            tagtext += _( " (rotten)" );
        } else if( it.has_flag( flag_MUSHY ) ) {
            tagtext += _( " (mushy)" );
        } else if( it.is_going_bad() ) {
            tagtext += _( " (old)" );
        } else if( it.is_fresh() ) {
            tagtext += _( " (fresh)" );
        }
    }
    return tagtext;
}

std::string food_irradiated( item const &it, unsigned int /* quantity */,
                             segment_bitset const &/* segments */ )
{
    if( it.goes_bad() ) {
        if( it.has_own_flag( flag_IRRADIATED ) ) {
            return _( " (irradiated)" );
        }
    }
    return {};
}

std::string temperature( item const &it, unsigned int /* quantity */,
                         segment_bitset const &/* segments */ )
{
    std::string ret;
    if( it.has_temperature() ) {
        if( it.has_flag( flag_HOT ) ) {
            ret = _( " (hot)" );
        }
        if( it.has_flag( flag_COLD ) ) {
            ret = _( " (cold)" );
        }
        if( it.has_flag( flag_FROZEN ) ) {
            ret += _( " (frozen)" );
        } else if( it.has_flag( flag_MELTS ) ) {
            ret += _( " (melted)" ); // he melted
        }
    }
    return ret;
}

std::string clothing_size( item const &it, unsigned int /* quantity */,
                           segment_bitset const &/* segments */ )
{
    const item::sizing sizing_level = it.get_sizing( get_avatar() );

    if( sizing_level == item::sizing::human_sized_small_char ) {
        return _( " (too big)" );
    } else if( sizing_level == item::sizing::big_sized_small_char ) {
        return _( " (huge!)" );
    } else if( sizing_level == item::sizing::human_sized_big_char ||
               sizing_level == item::sizing::small_sized_human_char ) {
        return _( " (too small)" );
    } else if( sizing_level == item::sizing::small_sized_big_char ) {
        return _( " (tiny!)" );
    } else if( !it.has_flag( flag_FIT ) && it.has_flag( flag_VARSIZE ) ) {
        return _( " (poor fit)" );
    }
    return {};
}

std::string filthy( item const &it, unsigned int /* quantity */,
                    segment_bitset const &/* segments */ )
{
    if( it.is_filthy() ) {
        return _( " (filthy)" );
    }
    return {};
}

std::string tags( item const &it, unsigned int /* quantity */,
                  segment_bitset const &/* segments */ )
{
    std::string ret;
    if( it.is_gun() && ( it.has_flag( flag_COLLAPSED_STOCK ) || it.has_flag( flag_FOLDED_STOCK ) ) ) {
        ret += _( " (folded)" );
    }
    if( it.has_flag( flag_RADIO_MOD ) ) {
        ret += _( " (radio:" );
        if( it.has_flag( flag_RADIOSIGNAL_1 ) ) {
            ret += pgettext( "The radio mod is associated with the [R]ed button.", "R)" );
        } else if( it.has_flag( flag_RADIOSIGNAL_2 ) ) {
            ret += pgettext( "The radio mod is associated with the [B]lue button.", "B)" );
        } else if( it.has_flag( flag_RADIOSIGNAL_3 ) ) {
            ret += pgettext( "The radio mod is associated with the [G]reen button.", "G)" );
        } else {
            debugmsg( "Why is the radio neither red, blue, nor green?" );
            ret += "?)";
        }
    }
    if( it.active && ( it.has_flag( flag_WATER_EXTINGUISH ) || it.has_flag( flag_LITCIG ) ) ) {
        ret += _( " (lit)" );
    }
    return ret;
}

std::string vars( item const &it, unsigned int /* quantity */,
                  segment_bitset const &/* segments */ )
{
    std::string ret;
    if( it.has_var( "NANOFAB_ITEM_ID" ) ) {
        if( it.has_flag( flag_NANOFAB_TEMPLATE_SINGLE_USE ) ) {
            //~ Single-use descriptor for nanofab templates. %s = name of resulting item. The leading space is intentional.
            ret += string_format( _( " (SINGLE USE %s)" ),
                                  item::nname( itype_id( it.get_var( "NANOFAB_ITEM_ID" ) ) ) );
        }
        ret += string_format( " (%s)", item::nname( itype_id( it.get_var( "NANOFAB_ITEM_ID" ) ) ) );
    }

    if( it.has_var( "snippet_file" ) ) {
        std::string has_snippet = it.get_var( "snippet_file" );
        if( has_snippet == "has" ) {
            std::optional<translation> snippet_name =
                SNIPPET.get_name_by_id( snippet_id( it.get_var( "local_files_simple_snippet_id" ) ) );
            if( snippet_name ) {
                ret += string_format( " (%s)", snippet_name->translated() );
            }
        } else {
            ret += _( " (uninteresting)" );
        }
    }

    if( it.has_var( "map_cache" ) ) {
        std::string has_map_cache = it.get_var( "map_cache" );
        if( has_map_cache == "read" ) {
            ret += _( " (read)" );
        }
    }

    if( it.already_used_by_player( get_avatar() ) ) {
        ret += _( " (used)" );
    }
    if( it.has_flag( flag_IS_UPS ) && it.get_var( "cable" ) == "plugged_in" ) {
        ret += _( " (plugged in)" );
    }
    return ret;
}


std::string traits( item const &it, unsigned int /* quantity */,
                    segment_bitset const &/* segments */ )
{
    std::string ret;
    if( it.has_flag( flag_GENE_TECH ) && it.template_traits.size() == 1 ) {
        if( it.has_flag( flag_NANOFAB_TEMPLATE_SINGLE_USE ) ) {
            //~ Single-use descriptor for nanofab templates. %s = name of resulting item. The leading space is intentional.
            ret += string_format( _( " (SINGLE USE %s)" ), it.template_traits.front()->name() );
        } else {
            ret += string_format( " (%s)", it.template_traits.front()->name() );
        }
    }
    return ret;

}

std::string segment_broken( item const &it, unsigned int /* quantity */,
                            segment_bitset const &/* segments */ )
{
    if( it.is_broken() ) {
        return _( " (broken)" );
    }
    return {};
}

std::string cbm_status( item const &it, unsigned int /* quantity */,
                        segment_bitset const &/* segments */ )
{
    if( it.is_bionic() && !it.has_flag( flag_NO_PACKED ) ) {
        if( !it.has_flag( flag_NO_STERILE ) ) {
            return _( " (sterile)" );
        }
        return _( " (packed)" );
    }
    return {};
}

std::string ups( item const &it, unsigned int /* quantity */,
                 segment_bitset const &/* segments */ )
{
    if( it.is_tool() && it.has_flag( flag_USE_UPS ) ) {
        return _( " (UPS)" );
    }
    return {};
}

std::string wetness( item const &it, unsigned int /* quantity */,
                     segment_bitset const &/* segments */ )
{
    if( ( it.wetness > 0 ) || it.has_flag( flag_WET ) ) {
        return _( " (wet)" );
    }
    return {};
}

std::string active( item const &it, unsigned int /* quantity */,
                    segment_bitset const &/* segments */ )
{
    if( it.active && !it.has_temperature() && !string_ends_with( it.typeId().str(), "_on" ) ) {
        // Usually the items whose ids end in "_on" have the "active" or "on" string already contained
        // in their name, also food is active while it rots.
        return _( " (active)" );
    }
    return {};
}
std::string activity_occupany( item const &it, unsigned int /* quantity */,
                               segment_bitset const &/* segments */ )
{
    if( it.has_var( "activity_var" ) ) {
        // Usually the items whose ids end in "_on" have the "active" or "on" string already contained
        // in their name, also food is active while it rots.
        return _( " (in use)" );
    }
    return {};
}


std::string sealed( item const &it, unsigned int /* quantity */,
                    segment_bitset const &/* segments */ )
{
    if( it.all_pockets_sealed() ) {
        return _( " (sealed)" );
    } else if( it.any_pockets_sealed() ) {
        return _( " (part sealed)" );
    }
    return {};
}

std::string post_asterisk( item const &it, unsigned int /* quantity */,
                           segment_bitset const &/* segments */ )
{
    if( it.is_favorite && get_option<std::string>( "ASTERISK_POSITION" ) == "suffix" ) {
        return _( " *" );
    }
    return {};
}

std::string weapon_mods( item const &it, unsigned int /* quantity */,
                         segment_bitset const &/* segments */ )
{
    std::string modtext;
    if( it.gunmod_find( itype_barrel_small ) != nullptr ) {
        modtext += _( "sawn-off " );
    }
    if( it.is_gun() && it.has_flag( flag_REMOVED_STOCK ) ) {
        modtext += _( "pistol " );
    }
    if( it.has_flag( flag_DIAMOND ) ) {
        modtext += pgettext( "Adjective, as in diamond katana", "diamond " );
    }
    return modtext;
}

std::string relic_charges( item const &it, unsigned int /* quantity */,
                           segment_bitset const &/* segments */ )
{
    if( it.is_relic() && it.relic_data->max_charges() > 0 && it.relic_data->charges_per_use() > 0 ) {
        return string_format( " (%d/%d)", it.relic_data->charges(), it.relic_data->max_charges() );
    }
    return {};
}

std::string category( item const &it, unsigned int quantity,
                      segment_bitset const &segments )
{
    nc_color const &color = segments[tname::segments::FOOD_PERISHABLE] && it.is_food()
                            ? it.color_in_inventory( &get_avatar() )
                            : c_magenta;
    return colorize( it.get_category_of_contents().name_noun( quantity ), color );
}

std::string ememory( item const &it, unsigned int /* quantity */,
                     segment_bitset const &/* segments */ )
{
    if( it.is_estorage() && !it.is_broken() ) {
        if( it.is_browsed() ) {
            units::ememory remain_mem = it.remaining_ememory();
            units::ememory total_mem = it.total_ememory();
            double ratio = static_cast<double>( remain_mem.value() ) / static_cast<double>( total_mem.value() );
            nc_color ememory_color;
            if( ratio > 0.66f ) {
                ememory_color = c_light_green;
            } else if( ratio > 0.33f ) {
                ememory_color = c_yellow;
            } else {
                ememory_color = c_light_red;
            }
            std::string out_of = remain_mem == total_mem ? units::display( remain_mem ) :
                                 string_format( "%s/%s", units::display( remain_mem ), units::display( total_mem ) );
            return string_format( _( " (%s free)" ), colorize( out_of, ememory_color ) );
        } else {
            return colorize( _( " (unbrowsed)" ), c_dark_gray );
        }
    }
    return {};
}

// function type that prints an element of tname::segments
using decl_f_print_segment = std::string( item const &it, unsigned int quantity,
                             segment_bitset const &segments );
constexpr size_t num_segments = static_cast<size_t>( tname::segments::last_segment );

constexpr std::array<decl_f_print_segment *, num_segments> get_segs_array()
{
    std::array<decl_f_print_segment *, num_segments> arr{};
    arr[static_cast<size_t>( tname::segments::FAULTS ) ] = faults;
    arr[static_cast<size_t>( tname::segments::FAULTS_SUFFIX ) ] = faults_suffix;
    arr[static_cast<size_t>( tname::segments::DIRT ) ] = dirt_symbol;
    arr[static_cast<size_t>( tname::segments::OVERHEAT ) ] = overheat_symbol;
    arr[static_cast<size_t>( tname::segments::FAVORITE_PRE ) ] = pre_asterisk;
    arr[static_cast<size_t>( tname::segments::DURABILITY ) ] = durability;
    arr[static_cast<size_t>( tname::segments::WHEEL_DIAMETER ) ] = wheel_diameter;
    arr[static_cast<size_t>( tname::segments::BURN ) ] = burn;
    arr[static_cast<size_t>( tname::segments::WEAPON_MODS ) ] = weapon_mods;
    arr[static_cast<size_t>( tname::segments::CUSTOM_ITEM_PREFIX )] = custom_item_prefix;
    arr[static_cast<size_t>( tname::segments::TYPE ) ] = label;
    arr[static_cast<size_t>( tname::segments::CATEGORY ) ] = category;
    arr[static_cast<size_t>( tname::segments::CUSTOM_ITEM_SUFFIX )] = custom_item_suffix;
    arr[static_cast<size_t>( tname::segments::MODS ) ] = mods;
    arr[static_cast<size_t>( tname::segments::CRAFT ) ] = craft;
    arr[static_cast<size_t>( tname::segments::WHITEBLACKLIST ) ] = wbl_mark;
    arr[static_cast<size_t>( tname::segments::CHARGES ) ] = noop;
    arr[static_cast<size_t>( tname::segments::FOOD_TRAITS ) ] = food_traits;
    arr[static_cast<size_t>( tname::segments::FOOD_STATUS ) ] = food_status;
    arr[static_cast<size_t>( tname::segments::FOOD_IRRADIATED ) ] = food_irradiated;
    arr[static_cast<size_t>( tname::segments::TEMPERATURE ) ] = temperature;
    arr[static_cast<size_t>( tname::segments::LOCATION_HINT ) ] = location_hint;
    arr[static_cast<size_t>( tname::segments::CLOTHING_SIZE ) ] = clothing_size;
    arr[static_cast<size_t>( tname::segments::ETHEREAL ) ] = ethereal;
    arr[static_cast<size_t>( tname::segments::FILTHY ) ] = filthy;
    arr[static_cast<size_t>( tname::segments::BROKEN ) ] = segment_broken;
    arr[static_cast<size_t>( tname::segments::CBM_STATUS ) ] = cbm_status;
    arr[static_cast<size_t>( tname::segments::UPS ) ] = ups;
    arr[static_cast<size_t>( tname::segments::TRAITS ) ] = traits;
    arr[static_cast<size_t>( tname::segments::TAGS ) ] = tags;
    arr[static_cast<size_t>( tname::segments::VARS ) ] = vars;
    arr[static_cast<size_t>( tname::segments::WETNESS ) ] = wetness;
    arr[static_cast<size_t>( tname::segments::ACTIVE ) ] = active;
    arr[static_cast<size_t>( tname::segments::ACTIVITY_OCCUPANCY ) ] = activity_occupany;
    arr[static_cast<size_t>( tname::segments::SEALED ) ] = sealed;
    arr[static_cast<size_t>( tname::segments::FAVORITE_POST ) ] = post_asterisk;
    arr[static_cast<size_t>( tname::segments::RELIC ) ] = relic_charges;
    arr[static_cast<size_t>( tname::segments::LINK ) ] = noop;
    arr[static_cast<size_t>( tname::segments::TECHNIQUES ) ] = noop;
    arr[static_cast<size_t>( tname::segments::CONTENTS ) ] = contents;
    arr[static_cast<size_t>( tname::segments::EMEMORY )] = ememory;

    return arr;
}
constexpr bool all_segments_have_printers()
{
    for( decl_f_print_segment *printer : get_segs_array() ) {
        if( printer == nullptr ) {
            return false;
        }
    }
    return true;
}
} // namespace

static_assert( all_segments_have_printers(),
               "every element of tname::segments (up to tname::segments::last_segment) "
               "must map to a printer in segs_array" );


namespace io
{
template<>
std::string enum_to_string<tname::segments>( tname::segments seg )
{
    switch( seg ) {
        // *INDENT-OFF*
        case tname::segments::FAULTS: return "FAULTS";
        case tname::segments::FAULTS_SUFFIX: return "FAULTS_SUFFIX";
        case tname::segments::DIRT: return "DIRT";
        case tname::segments::OVERHEAT: return "OVERHEAT";
        case tname::segments::FAVORITE_PRE: return "FAVORITE_PRE";
        case tname::segments::DURABILITY: return "DURABILITY";
        case tname::segments::WHEEL_DIAMETER: return "WHEEL_DIAMETER";
        case tname::segments::BURN: return "BURN";
        case tname::segments::WEAPON_MODS: return "WEAPON_MODS";
        case tname::segments::CUSTOM_ITEM_PREFIX: return "CUSTOM_ITEM_PREFIX";
        case tname::segments::TYPE: return "TYPE";
        case tname::segments::CATEGORY: return "CATEGORY";
        case tname::segments::CUSTOM_ITEM_SUFFIX: return "CUSTOM_ITEM_SUFFIX";
        case tname::segments::MODS: return "MODS";
        case tname::segments::CRAFT: return "CRAFT";
        case tname::segments::WHITEBLACKLIST: return "WHITEBLACKLIST";
        case tname::segments::CHARGES: return "CHARGES";
        case tname::segments::FOOD_TRAITS: return "FOOD_TRAITS";
        case tname::segments::FOOD_STATUS: return "FOOD_STATUS";
        case tname::segments::FOOD_IRRADIATED: return "FOOD_IRRADIATED";
        case tname::segments::TEMPERATURE: return "TEMPERATURE";
        case tname::segments::LOCATION_HINT: return "LOCATION_HINT";
        case tname::segments::CLOTHING_SIZE: return "CLOTHING_SIZE";
        case tname::segments::ETHEREAL: return "ETHEREAL";
        case tname::segments::FILTHY: return "FILTHY";
        case tname::segments::BROKEN: return "BROKEN";
        case tname::segments::CBM_STATUS: return "CBM_STATUS";
        case tname::segments::UPS: return "UPS";
        case tname::segments::TAGS: return "TAGS";
        case tname::segments::VARS: return "VARS";
        case tname::segments::WETNESS: return "WETNESS";
        case tname::segments::ACTIVE: return "ACTIVE";
        case tname::segments::ACTIVITY_OCCUPANCY: return "ACTIVITY_OCCUPANCY";
        case tname::segments::SEALED: return "SEALED";
        case tname::segments::FAVORITE_POST: return "FAVORITE_POST";
        case tname::segments::RELIC: return "RELIC";
        case tname::segments::LINK: return "LINK";
        case tname::segments::TECHNIQUES: return "TECHNIQUES";
        case tname::segments::CONTENTS: return "CONTENTS";
        case tname::segments::last_segment: return "last_segment";
        case tname::segments::VARIANT: return "VARIANT";
        case tname::segments::COMPONENTS: return "COMPONENTS";
        case tname::segments::CORPSE: return "CORPSE";
        case tname::segments::CONTENTS_FULL: return "CONTENTS_FULL";
        case tname::segments::CONTENTS_ABREV: return "CONTENTS_ABBREV";
        case tname::segments::CONTENTS_COUNT: return "CONTENTS_COUNT";
        case tname::segments::FOOD_PERISHABLE: return "FOOD_PERISHABLE";
        case tname::segments::EMEMORY: return "EMEMORY";
        case tname::segments::last: return "last";
        default:
        // *INDENT-ON*
            break;
    }
    return {};
}

} // namespace io

namespace
{

constexpr tname::segments fixed_pos_segments = tname::segments::CONTENTS;
static_assert( fixed_pos_segments <= tname::segments::last_segment );

using tname_array = std::array<int, static_cast<std::size_t>( fixed_pos_segments )>;
struct segment_order {
    constexpr explicit segment_order( tname_array const &arr_ ) : arr( &arr_ ) {};
    constexpr bool operator()( tname::segments lhs, tname::segments rhs ) const {
        return arr->at( static_cast<std::size_t>( lhs ) ) <
               arr->at( static_cast<std::size_t>( rhs ) );
    }

    tname_array const *arr;
};

std::optional<std::size_t> str_to_segment_idx( std::string const &str )
{
    if( std::optional<tname::segments> ret = io::string_to_enum_optional<tname::segments>( str );
        ret && ret < fixed_pos_segments ) {

        return static_cast<std::size_t>( *ret );
    }

    return {};
}

} // namespace
namespace tname
{
std::string print_segment( tname::segments segment, item const &it, unsigned int quantity,
                           segment_bitset const &segments )
{
    static std::array<decl_f_print_segment *, num_segments> const arr = get_segs_array();
    std::size_t const idx = static_cast<std::size_t>( segment );
    return ( *arr.at( idx ) )( it, quantity, segments );
}

tname_set const &get_tname_set()
{
    static tname_set tns;
    static int lang_ver = INVALID_LANGUAGE_VERSION;
    if( int const cur_lang_ver = detail::get_current_language_version(); lang_ver != cur_lang_ver ) {
        lang_ver = cur_lang_ver;
        tns.clear();
        for( std::size_t i = 0; i < static_cast<std::size_t>( fixed_pos_segments ); i++ ) {
            tns.emplace_back( static_cast<tname::segments>( i ) );
        }

        //~ You can use this string to change the order of item name segments. The default order is:
        //~ FAULTS DIRT OVERHEAT FAVORITE_PRE DURABILITY WHEEL_DIAMETER BURN WEAPON_MODS
        //~ CUSTOM_ITEM_PREFIX TYPE CATEGORY CUSTOM_ITEM_SUFFIX MODS CRAFT WHITEBLACKLIST CHARGES
        //~ FOOD_TRAITS FOOD_STATUS FOOD_IRRADIATED TEMPERATURE LOCATION_HINT CLOTHING_SIZE ETHEREAL
        //~ FILTHY BROKEN CBM_STATUS UPS TAGS VARS WETNESS ACTIVE SEALED FAVORITE_POST RELIC LINK
        //~ TECHNIQUES
        //~ --
        //~ refer to io::enum_to_string<tname::segments> for an updated list
        std::string order_i18n( _( "tname_segments_order" ) );
        if( order_i18n != "tname_segments_order" ) {
            std::stringstream ss( order_i18n );
            std::istream_iterator<std::string> begin( ss );
            std::istream_iterator<std::string> end;
            std::vector<std::string> tokens( begin, end );

            tname_array tna;
            tna.fill( 999 );
            int cur_order = 0;
            for( std::string const &s : tokens ) {
                if( std::optional<std::size_t> idx = str_to_segment_idx( s ); idx ) {
                    tna[*idx] = cur_order++;
                } else {
                    DebugLog( D_WARNING, D_MAIN ) << "Ignoring tname segment " << std::quoted( s ) << std::endl;
                }
            }

            std::stable_sort( tns.begin(), tns.end(), segment_order( tna ) );
        }
        for( std::size_t i = static_cast<std::size_t>( fixed_pos_segments );
             i < static_cast<std::size_t>( tname::segments::last_segment ); i++ ) {
            tns.emplace_back( static_cast<tname::segments>( i ) );
        }
    }

    return tns;
}
} // namespace tname
