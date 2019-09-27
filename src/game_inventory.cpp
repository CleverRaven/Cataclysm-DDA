#include "game_inventory.h"

#include <math.h>
#include <climits>
#include <cstddef>
#include <functional>
#include <bitset>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include <algorithm>

#include "avatar.h"
#include "game.h"
#include "bionics.h"
#include "inventory_ui.h"
#include "item.h"
#include "itype.h"
#include "iuse_actor.h"
#include "map.h"
#include "options.h"
#include "output.h"
#include "player.h"
#include "recipe_dictionary.h"
#include "skill.h"
#include "string_formatter.h"
#include "calendar.h"
#include "cata_utility.h"
#include "color.h"
#include "compatibility.h"
#include "damage.h"
#include "debug.h"
#include "enums.h"
#include "input.h"
#include "inventory.h"
#include "item_location.h"
#include "iuse.h"
#include "optional.h"
#include "player_activity.h"
#include "recipe.h"
#include "requirements.h"
#include "ret_val.h"
#include "translations.h"
#include "units.h"
#include "type_id.h"
#include "point.h"

const efftype_id effect_assisted( "assisted" );

const skill_id skill_computer( "computer" );
const skill_id skill_electronics( "electronics" );
const skill_id skill_firstaid( "firstaid" );

static const trait_id trait_NOPAIN( "NOPAIN" );
static const trait_id trait_SAPROPHAGE( "SAPROPHAGE" );
static const trait_id trait_SAPROVORE( "SAPROVORE" );

class Character;

using item_filter = std::function<bool ( const item & )>;
using item_location_filter = std::function<bool ( const item_location & )>;

namespace
{

std::string good_bad_none( int value )
{
    if( value > 0 ) {
        return string_format( "<good>+%d</good>", value );
    } else if( value < 0 ) {
        return string_format( "<bad>%d</bad>", value );
    }
    return std::string();
}

std::string highlight_good_bad_none( int value )
{
    if( value > 0 ) {
        return string_format( "<color_yellow_green>+%d</color>", value );
    } else if( value < 0 ) {
        return string_format( "<color_yellow_red>%d</color>", value );
    }
    return string_format( "<color_yellow>%d</color>", value );
}

} // namespace

inventory_filter_preset::inventory_filter_preset( const item_location_filter &filter )
    : filter( filter )
{}

bool inventory_filter_preset::is_shown( const item_location &location ) const
{
    return filter( location );
}

static item_location_filter convert_filter( const item_filter &filter )
{
    return [ &filter ]( const item_location & loc ) {
        return filter( *loc );
    };
}

static item_location inv_internal( player &u, const inventory_selector_preset &preset,
                                   const std::string &title, int radius,
                                   const std::string &none_message,
                                   const std::string &hint = std::string() )
{
    inventory_pick_selector inv_s( u, preset );

    inv_s.set_title( title );
    inv_s.set_hint( hint );
    inv_s.set_display_stats( false );

    std::pair<size_t, size_t> init_pair;
    bool init_selection = false;

    const std::vector<activity_id> consuming {
        activity_id( "ACT_EAT_MENU" ),
        activity_id( "ACT_CONSUME_FOOD_MENU" ),
        activity_id( "ACT_CONSUME_DRINK_MENU" ),
        activity_id( "ACT_CONSUME_MEDS_MENU" ) };

    if( u.has_activity( consuming ) && u.activity.values.size() >= 2 ) {
        init_pair.first = u.activity.values[0];
        init_pair.second = u.activity.values[1];
        init_selection = true;
    }

    do {
        u.inv.restack( u );

        inv_s.clear_items();
        inv_s.add_character_items( u );
        inv_s.add_nearby_items( radius );

        if( init_selection ) {
            inv_s.update();
            inv_s.select_position( init_pair );
            init_selection = false;
        }

        if( inv_s.empty() ) {
            const std::string msg = none_message.empty()
                                    ? _( "You don't have the necessary item at hand." )
                                    : none_message;
            popup( msg, PF_GET_KEY );
            return item_location();
        }

        item_location location = inv_s.execute();

        if( inv_s.keep_open ) {
            inv_s.keep_open = false;
            continue;
        }

        if( u.has_activity( consuming ) ) {
            u.activity.values.clear();
            init_pair = inv_s.get_selection_position();
            u.activity.values.push_back( init_pair.first );
            u.activity.values.push_back( init_pair.second );
        }

        return location;

    } while( true );
}

void game_menus::inv::common( avatar &you )
{
    // Return to inventory menu on those inputs
    static const std::set<int> loop_options = { { '\0', '=', 'f' } };

    inventory_pick_selector inv_s( you );

    inv_s.set_title( _( "Inventory" ) );
    inv_s.set_hint( string_format(
                        _( "Item hotkeys assigned: <color_light_gray>%d</color>/<color_light_gray>%d</color>" ),
                        you.allocated_invlets().count(), inv_chars.size() ) );

    int res = 0;

    do {
        you.inv.restack( you );
        inv_s.clear_items();
        inv_s.add_character_items( you );
        inv_s.update();

        const item_location &location = inv_s.execute();

        if( location == item_location::nowhere ) {
            if( inv_s.keep_open ) {
                inv_s.keep_open = false;
                continue;
            } else {
                break;
            }
        }

        g->refresh_all();
        res = g->inventory_item_menu( you.get_item_position( location.get_item() ) );
        g->refresh_all();

    } while( loop_options.count( res ) != 0 );
}

int game::inv_for_filter( const std::string &title, item_filter filter,
                          const std::string &none_message )
{
    return u.get_item_position( inv_map_splice( filter, title, -1, none_message ).get_item() );
}

int game::inv_for_all( const std::string &title, const std::string &none_message )
{
    const std::string msg = none_message.empty() ? _( "Your inventory is empty." ) : none_message;
    return u.get_item_position( inv_internal( u, inventory_selector_preset(), title, -1,
                                msg ).get_item() );
}

int game::inv_for_flag( const std::string &flag, const std::string &title )
{
    return inv_for_filter( title, [ &flag ]( const item & it ) {
        return it.has_flag( flag );
    } );
}

int game::inv_for_id( const itype_id &id, const std::string &title )
{
    return inv_for_filter( title, [ &id ]( const item & it ) {
        return it.typeId() == id;
    }, string_format( _( "You don't have a %s." ), item::nname( id ) ) );
}

class armor_inventory_preset: public inventory_selector_preset
{
    public:
        armor_inventory_preset( const player &pl, const std::string &color_in ) :
            p( pl ), color( color_in ) {
            append_cell( [ this ]( const item_location & loc ) {
                return get_number_string( loc->get_encumber( p ) );
            }, _( "ENCUMBRANCE" ) );

            append_cell( [ this ]( const item_location & loc ) {
                return loc->get_storage() > 0_ml ? string_format( "<%s>%s</color>", color,
                        format_volume( loc->get_storage() ) ) : std::string();
            }, _( "STORAGE" ) );

            append_cell( [ this ]( const item_location & loc ) {
                return string_format( "<%s>%d%%</color>", color, loc->get_coverage() );
            }, _( "COVERAGE" ) );

            append_cell( [ this ]( const item_location & loc ) {
                return get_number_string( loc->get_warmth() );
            }, _( "WARMTH" ) );

            append_cell( [ this ]( const item_location & loc ) {
                return get_number_string( loc->bash_resist() );
            }, _( "BASH" ) );

            append_cell( [ this ]( const item_location & loc ) {
                return get_number_string( loc->cut_resist() );
            }, _( "CUT" ) );

            append_cell( [ this ]( const item_location & loc ) {
                return get_number_string( loc->acid_resist() );
            }, _( "ACID" ) );

            append_cell( [ this ]( const item_location & loc ) {
                return get_number_string( loc->fire_resist() );
            }, _( "FIRE" ) );

            append_cell( [ this ]( const item_location & loc ) {
                return get_number_string( loc->get_env_resist() );
            }, _( "ENV" ) );
        }

    protected:
        const player &p;
    private:
        std::string get_number_string( int number ) const {
            return number ? string_format( "<%s>%d</color>", color, number ) : std::string();
        }

        const std::string color;
};

class wear_inventory_preset: public armor_inventory_preset
{
    public:
        wear_inventory_preset( const player &p, const std::string &color ) :
            armor_inventory_preset( p, color )
        {}

        bool is_shown( const item_location &loc ) const override {
            return loc->is_armor() && !p.is_worn( *loc );
        }

        std::string get_denial( const item_location &loc ) const override {
            const auto ret = p.can_wear( *loc );

            if( !ret.success() ) {
                return trim_punctuation_marks( ret.str() );
            }

            return std::string();
        }
};

item_location game_menus::inv::wear( player &p )
{
    return inv_internal( p, wear_inventory_preset( p, "color_yellow" ), _( "Wear item" ), 1,
                         _( "You have nothing to wear." ) );
}

class take_off_inventory_preset: public armor_inventory_preset
{
    public:
        take_off_inventory_preset( const player &p, const std::string &color ) :
            armor_inventory_preset( p, color )
        {}

        bool is_shown( const item_location &loc ) const override {
            return loc->is_armor() && p.is_worn( *loc );
        }

        std::string get_denial( const item_location &loc ) const override {
            const auto ret = p.can_takeoff( *loc );

            if( !ret.success() ) {
                return trim_punctuation_marks( ret.str() );
            }

            return std::string();
        }
};

item_location game_menus::inv::take_off( avatar &you )
{
    return inv_internal( you, take_off_inventory_preset( you, "color_red" ), _( "Take off item" ), 1,
                         _( "You don't wear anything." ) );
}

item_location game::inv_map_splice( item_filter filter, const std::string &title, int radius,
                                    const std::string &none_message )
{
    return inv_internal( u, inventory_filter_preset( convert_filter( filter ) ),
                         title, radius, none_message );
}

item_location game_menus::inv::container_for( avatar &you, const item &liquid, int radius )
{
    const auto filter = [ &liquid ]( const item_location & location ) {
        if( location.where() == item_location::type::character ) {
            Character *character = g->critter_at<Character>( location.position() );
            if( character == nullptr ) {
                debugmsg( "Invalid location supplied to the liquid filter: no character found." );
                return false;
            }
            return location->get_remaining_capacity_for_liquid( liquid, *character ) > 0;
        }

        const bool allow_buckets = location.where() == item_location::type::map;
        return location->get_remaining_capacity_for_liquid( liquid, allow_buckets ) > 0;
    };

    return inv_internal( you, inventory_filter_preset( filter ),
                         string_format( _( "Container for %s" ), liquid.display_name( liquid.charges ) ), radius,
                         string_format( _( "You don't have a suitable container for carrying %s." ),
                                        liquid.tname() ) );
}

class pickup_inventory_preset : public inventory_selector_preset
{
    public:
        pickup_inventory_preset( const player &p ) : p( p ) {}

        std::string get_denial( const item_location &loc ) const override {
            if( !p.has_item( *loc ) ) {
                if( loc->made_of_from_type( LIQUID ) ) {
                    return _( "Can't pick up spilt liquids" );
                } else if( !p.can_pickVolume( *loc ) && p.is_armed() ) {
                    return _( "Too big to pick up" );
                } else if( !p.can_pickWeight( *loc ) ) {
                    return _( "Too heavy to pick up" );
                }
            }

            return std::string();
        }

    private:
        const player &p;
};

class disassemble_inventory_preset : public pickup_inventory_preset
{
    public:
        disassemble_inventory_preset( const player &p, const inventory &inv ) :
            pickup_inventory_preset( p ), p( p ), inv( inv ) {

            check_components = true;

            append_cell( [ this ]( const item_location & loc ) {
                const auto &req = get_recipe( loc ).disassembly_requirements();
                if( req.is_empty() ) {
                    return std::string();
                }
                const item *i = loc.get_item();
                const auto components = i->get_uncraft_components();
                return enumerate_as_string( components.begin(), components.end(),
                []( const decltype( components )::value_type & comps ) {
                    return comps.to_string();
                } );
            }, _( "YIELD" ) );

            append_cell( [ this ]( const item_location & loc ) {
                return to_string_clipped( time_duration::from_turns( get_recipe( loc ).time / 100 ) );
            }, _( "TIME" ) );
        }

        bool is_shown( const item_location &loc ) const override {
            return get_recipe( loc );
        }

        std::string get_denial( const item_location &loc ) const override {
            const auto ret = p.can_disassemble( *loc, inv );
            if( !ret.success() ) {
                return ret.str();
            }
            return pickup_inventory_preset::get_denial( loc );
        }

    protected:
        const recipe &get_recipe( const item_location &loc ) const {
            return recipe_dictionary::get_uncraft( loc->typeId() );
        }

    private:
        const player &p;
        const inventory &inv;
};

item_location game_menus::inv::disassemble( player &p )
{
    return inv_internal( p, disassemble_inventory_preset( p, p.crafting_inventory() ),
                         _( "Disassemble item" ), 1,
                         _( "You don't have any items you could disassemble." ) );
}

class comestible_inventory_preset : public inventory_selector_preset
{
    public:
        comestible_inventory_preset( const player &p ) : p( p ) {

            append_cell( [ &p, this ]( const item_location & loc ) {
                return good_bad_none( p.kcal_for( get_consumable_item( loc ) ) );
            }, _( "CALORIES" ) );

            append_cell( [ this ]( const item_location & loc ) {
                return good_bad_none( get_edible_comestible( loc ).quench );
            }, _( "QUENCH" ) );

            append_cell( [ &p, this ]( const item_location & loc ) {
                const item &it = get_consumable_item( loc );
                if( it.has_flag( "MUSHY" ) ) {
                    return highlight_good_bad_none( p.fun_for( get_consumable_item( loc ) ).first );
                } else {
                    return good_bad_none( p.fun_for( get_consumable_item( loc ) ).first );
                }
            }, _( "JOY" ) );

            append_cell( [ this ]( const item_location & loc ) {
                const time_duration spoils = get_edible_comestible( loc ).spoils;
                if( spoils > 0_turns ) {
                    return to_string_clipped( spoils );
                }
                //~ Used for permafood shelf life in the Eat menu
                return std::string( _( "indefinite" ) );
            }, _( "SHELF LIFE" ) );

            append_cell( [this]( const item_location & loc ) {
                if( g->u.can_estimate_rot() ) {
                    const islot_comestible item = get_edible_comestible( loc );
                    if( item.spoils > 0_turns ) {
                        return get_freshness( loc );
                    }
                    return std::string( "---" );
                }
                return std::string();
            }, _( "FRESHNESS" ) );

            append_cell( [ this ]( const item_location & loc ) {
                if( g->u.can_estimate_rot() ) {
                    const islot_comestible item = get_edible_comestible( loc );
                    if( item.spoils > 0_turns ) {
                        if( !get_consumable_item( loc ).rotten() ) {
                            return get_time_left_rounded( loc );
                        }
                    }
                    return std::string( "---" );
                }
                return std::string();
            }, _( "SPOILS IN" ) );

            append_cell( [ this, &p ]( const item_location & loc ) {
                std::string cbm_name;

                switch( p.get_cbm_rechargeable_with( get_consumable_item( loc ) ) ) {
                    case rechargeable_cbm::none:
                        break;
                    case rechargeable_cbm::battery:
                        cbm_name = _( "Battery" );
                        break;
                    case rechargeable_cbm::reactor:
                        cbm_name = _( "Reactor" );
                        break;
                    case rechargeable_cbm::furnace:
                        cbm_name = _( "Furnace" );
                        break;
                    case rechargeable_cbm::other:
                        std::vector<bionic_id> bids = p.get_bionic_fueled_with( get_consumable_item( loc ) );
                        if( !bids.empty() ) {
                            bionic_id bid = p.get_most_efficient_bionic( bids );
                            cbm_name = bid->name.translated();
                        }
                        break;
                }

                if( !cbm_name.empty() ) {
                    return string_format( "<color_cyan>%s</color>", cbm_name );
                }

                return std::string();
            }, _( "CBM" ) );

            append_cell( [ this, &p ]( const item_location & loc ) {
                return good_bad_none( p.get_acquirable_energy( get_consumable_item( loc ) ) );
            }, _( "ENERGY" ) );
        }

        bool is_shown( const item_location &loc ) const override {
            return p.can_consume( *loc );
        }

        std::string get_denial( const item_location &loc ) const override {
            if( loc->made_of_from_type( LIQUID ) && !g->m.has_flag( "LIQUIDCONT", loc.position() ) ) {
                return _( "Can't drink spilt liquids" );
            }

            const auto &it = get_consumable_item( loc );
            const auto res = p.can_eat( it );
            const auto cbm = p.get_cbm_rechargeable_with( it );

            if( !res.success() && cbm == rechargeable_cbm::none ) {
                return res.str();
            } else if( cbm == rechargeable_cbm::battery && p.power_level >= p.max_power_level ) {
                return _( "You're fully charged" );
            } else if( cbm == rechargeable_cbm::other && ( p.get_fuel_capacity( it.typeId() ) <= 0 ) ) {
                return string_format( _( "No space to store more %s" ), it.tname() );
            }

            return inventory_selector_preset::get_denial( loc );
        }

        bool sort_compare( const inventory_entry &lhs, const inventory_entry &rhs ) const override {
            time_duration time_a = get_time_left( lhs.any_item() );
            time_duration time_b = get_time_left( rhs.any_item() );
            int order_a = get_order( lhs.any_item(), time_a );
            int order_b = get_order( rhs.any_item(), time_b );

            return order_a < order_b
                   || ( order_a == order_b && time_a < time_b )
                   || ( order_a == order_b && time_a == time_b &&
                        inventory_selector_preset::sort_compare( lhs, rhs ) );
        }

    protected:
        int get_order( const item_location &loc, const time_duration time ) const {
            if( time > 0_turns && !( loc->type->container && loc->type->container->preserves ) ) {
                return 0;
            } else if( get_consumable_item( loc ).rotten() ) {
                if( p.has_trait( trait_SAPROPHAGE ) || p.has_trait( trait_SAPROVORE ) ) {
                    return 1;
                } else {
                    return 4;
                }
            } else if( time == 0_turns ) {
                return 3;
            } else {
                return 2;
            }
        }

        // WARNING: this can return consumables which are not necessarily possessing
        // the comestible type. please dereference responsibly.
        const item &get_consumable_item( const item_location &loc ) const {
            return p.get_consumable_from( const_cast<item &>( *loc ) );
        }

        const islot_comestible &get_edible_comestible( const item_location &loc ) const {
            return get_edible_comestible( get_consumable_item( loc ) );
        }

        const islot_comestible &get_edible_comestible( const item &it ) const {
            if( it.is_comestible() && p.can_eat( it ).success() ) {
                // Ok since can_eat() returns false if is_craft() is true
                return *it.type->comestible;
            }
            static const islot_comestible dummy {};
            return dummy;
        }

        time_duration get_time_left( const item_location &loc ) const {
            time_duration time_left = 0_turns;
            const time_duration shelf_life = get_edible_comestible( loc ).spoils;
            if( shelf_life > 0_turns ) {
                const item &it = get_consumable_item( loc );
                const double relative_rot = it.get_relative_rot();
                time_left = shelf_life - shelf_life * relative_rot;

                // Correct for an estimate that exceeds shelf life -- this happens especially with
                // fresh items.
                if( time_left > shelf_life ) {
                    time_left = shelf_life;
                }
            }

            return time_left;
        }

        std::string get_time_left_rounded( const item_location &loc ) const {
            const item &it = get_consumable_item( loc );
            if( it.is_going_bad() ) {
                return _( "soon!" );
            }

            time_duration time_left = get_time_left( loc );
            return to_string_approx( time_left );
        }

        std::string get_freshness( const item_location &loc ) {
            const item &it = get_consumable_item( loc );
            const double rot_progress = it.get_relative_rot();
            if( it.is_fresh() ) {
                return _( "fresh" );
            } else if( rot_progress < 0.3 ) {
                return _( "quite fresh" );
            } else if( rot_progress < 0.5 ) {
                return _( "near midlife" );
            } else if( rot_progress < 0.7 ) {
                return _( "past midlife" );
            } else if( rot_progress < 0.9 ) {
                return _( "getting older" );
            } else if( !it.rotten() ) {
                return _( "old" );
            } else {
                return _( "rotten" );
            }
        }

    private:
        const player &p;
};

static std::string get_consume_needs_hint( player &p )
{
    auto hint = std::string();
    auto desc = p.get_hunger_description();
    hint.append( string_format( "%s %s", _( "Food :" ), colorize( desc.first, desc.second ) ) );
    hint.append( string_format( " %s ", LINE_XOXO_S ) );
    desc = p.get_thirst_description();
    hint.append( string_format( "%s %s", _( "Drink:" ), colorize( desc.first, desc.second ) ) );
    hint.append( string_format( " %s ", LINE_XOXO_S ) );
    desc = p.get_pain_description();
    hint.append( string_format( "%s %s", _( "Pain :" ), colorize( desc.first, desc.second ) ) );
    return hint;
}

item_location game_menus::inv::consume( player &p )
{
    if( !g->u.has_activity( activity_id( "ACT_EAT_MENU" ) ) ) {
        g->u.assign_activity( activity_id( "ACT_EAT_MENU" ) );
    }

    return inv_internal( p, comestible_inventory_preset( p ),
                         _( "Consume item" ), 1,
                         _( "You have nothing to consume." ),
                         get_consume_needs_hint( p ) );
}

class comestible_filtered_inventory_preset : public comestible_inventory_preset
{
    public:
        comestible_filtered_inventory_preset( const player &p, bool( *predicate )( const item &it ) ) :
            comestible_inventory_preset( p ), predicate( predicate ) {}

        bool is_shown( const item_location &loc ) const override {
            return comestible_inventory_preset::is_shown( loc ) &&
                   predicate( get_consumable_item( loc ) );
        }

    private:
        bool( *predicate )( const item &it );
};

item_location game_menus::inv::consume_food( player &p )
{
    if( !g->u.has_activity( activity_id( "ACT_CONSUME_FOOD_MENU" ) ) ) {
        g->u.assign_activity( activity_id( "ACT_CONSUME_FOOD_MENU" ) );
    }

    return inv_internal( p, comestible_filtered_inventory_preset( p, []( const item & it ) {
        return ( it.is_comestible() && it.get_comestible()->comesttype == "FOOD" ) ||
               it.has_flag( "USE_EAT_VERB" );
    } ),
    _( "Consume food" ), 1,
    _( "You have no food to consume." ),
    get_consume_needs_hint( p ) );
}

item_location game_menus::inv::consume_drink( player &p )
{
    if( !g->u.has_activity( activity_id( "ACT_CONSUME_DRINK_MENU" ) ) ) {
        g->u.assign_activity( activity_id( "ACT_CONSUME_DRINK_MENU" ) );
    }

    return inv_internal( p, comestible_filtered_inventory_preset( p, []( const item & it ) {
        return it.is_comestible() && it.get_comestible()->comesttype == "DRINK" &&
               !it.has_flag( "USE_EAT_VERB" );
    } ),
    _( "Consume drink" ), 1,
    _( "You have no drink to consume." ),
    get_consume_needs_hint( p ) );
}

item_location game_menus::inv::consume_meds( player &p )
{
    if( !g->u.has_activity( activity_id( "ACT_CONSUME_MEDS_MENU" ) ) ) {
        g->u.assign_activity( activity_id( "ACT_CONSUME_MEDS_MENU" ) );
    }

    return inv_internal( p, comestible_filtered_inventory_preset( p, []( const item & it ) {
        return it.is_medication();
    } ),
    _( "Consume medication" ), 1,
    _( "You have no medication to consume." ),
    get_consume_needs_hint( p ) );
}

class activatable_inventory_preset : public pickup_inventory_preset
{
    public:
        activatable_inventory_preset( const player &p ) : pickup_inventory_preset( p ), p( p ) {
            if( get_option<bool>( "INV_USE_ACTION_NAMES" ) ) {
                append_cell( [ this ]( const item_location & loc ) {
                    const item &it = !( *loc ).is_container_empty() && ( *loc ).get_contained().is_medication() &&
                                     ( *loc ).get_contained().type->has_use() ? ( *loc ).get_contained() : *loc;
                    return string_format( "<color_light_green>%s</color>", get_action_name( it ) );
                }, _( "ACTION" ) );
            }
        }

        bool is_shown( const item_location &loc ) const override {
            if( !( *loc ).is_container_empty() && ( *loc ).get_contained().is_medication() &&
                ( *loc ).get_contained().type->has_use() ) {
                return true;
            }
            return loc->type->has_use();
        }

        std::string get_denial( const item_location &loc ) const override {
            const item &it = !( *loc ).is_container_empty() && ( *loc ).get_contained().is_medication() &&
                             ( *loc ).get_contained().type->has_use() ? ( *loc ).get_contained() : *loc;
            const auto &uses = it.type->use_methods;

            if( uses.size() == 1 ) {
                const auto ret = uses.begin()->second.can_call( p, it, false, p.pos() );
                if( !ret.success() ) {
                    return trim_punctuation_marks( ret.str() );
                }
            }

            if( !p.has_enough_charges( it, false ) ) {
                return string_format(
                           ngettext( "Needs at least %d charge",
                                     "Needs at least %d charges", loc->ammo_required() ),
                           loc->ammo_required() );
            }

            if( !it.has_flag( "ALLOWS_REMOTE_USE" ) ) {
                return pickup_inventory_preset::get_denial( loc );
            }

            return std::string();
        }

    protected:
        std::string get_action_name( const item &it ) const {
            const auto &uses = it.type->use_methods;

            if( uses.size() == 1 ) {
                return uses.begin()->second.get_name();
            } else if( uses.size() > 1 ) {
                return _( "..." );
            }

            return std::string();
        }

    private:
        const player &p;
};

item_location game_menus::inv::use( avatar &you )
{
    return inv_internal( you, activatable_inventory_preset( you ),
                         _( "Use item" ), 1,
                         _( "You don't have any items you can use." ) );
}

class gunmod_inventory_preset : public inventory_selector_preset
{
    public:
        gunmod_inventory_preset( const player &p, const item &gunmod ) : p( p ), gunmod( gunmod ) {
            append_cell( [ this ]( const item_location & loc ) {
                const auto odds = get_odds( loc );

                if( odds.first >= 100 ) {
                    return string_format( "<color_light_green>%s</color>", _( "always" ) );
                }

                return string_format( "<color_light_green>%d%%</color>", odds.first );
            }, _( "SUCCESS CHANCE" ) );

            append_cell( [ this ]( const item_location & loc ) {
                return good_bad_none( get_odds( loc ).second );
            }, _( "DAMAGE RISK" ) );
        }

        bool is_shown( const item_location &loc ) const override {
            return loc->is_gun() && !loc->is_gunmod();
        }

        std::string get_denial( const item_location &loc ) const override {
            const auto ret = loc->is_gunmod_compatible( gunmod );

            if( !ret.success() ) {
                return ret.str();
            }

            if( !p.meets_requirements( gunmod, *loc ) ) {
                return string_format( _( "requires at least %s" ),
                                      p.enumerate_unmet_requirements( gunmod, *loc ) );
            }

            if( get_odds( loc ).first <= 0 ) {
                return _( "is too difficult for you to modify" );
            }

            return std::string();
        }

        bool sort_compare( const inventory_entry &lhs, const inventory_entry &rhs ) const override {
            const auto a = get_odds( lhs.any_item() );
            const auto b = get_odds( rhs.any_item() );

            if( a.first > b.first || ( a.first == b.first && a.second < b.second ) ) {
                return true;
            }

            return inventory_selector_preset::sort_compare( lhs, rhs );
        }

    protected:
        /** @return Odds for successful installation (pair.first) and gunmod damage (pair.second) */
        std::pair<int, int> get_odds( const item_location &gun ) const {
            return p.gunmod_installation_odds( *gun, gunmod );
        }

    private:
        const player &p;
        const item &gunmod;
};

item_location game_menus::inv::gun_to_modify( player &p, const item &gunmod )
{
    return inv_internal( p, gunmod_inventory_preset( p, gunmod ),
                         _( "Select gun to modify" ), -1,
                         _( "You don't have any guns to modify." ) );
}

class read_inventory_preset: public pickup_inventory_preset
{
    public:
        read_inventory_preset( const player &p ) : pickup_inventory_preset( p ), p( p ) {
            const std::string unknown = _( "<color_dark_gray>?</color>" );

            append_cell( [ this, &p, unknown ]( const item_location & loc ) -> std::string {
                if( loc->type->can_use( "MA_MANUAL" ) ) {
                    return _( "martial arts" );
                }
                if( !is_known( loc ) ) {
                    return unknown;
                }
                const auto &book = get_book( loc );
                if( book.skill ) {
                    const SkillLevel &skill = p.get_skill_level_object( book.skill );
                    if( skill.can_train() ) {
                        //~ %1$s: book skill name, %2$d: book skill level, %3$d: player skill level
                        return string_format( pgettext( "skill", "%1$s to %2$d (%3$d)" ), book.skill->name(), book.level,
                                              skill.level() );
                    }
                }
                return std::string();
            }, _( "TRAINS (CURRENT)" ), unknown );

            append_cell( [ this, unknown ]( const item_location & loc ) -> std::string {
                if( !is_known( loc ) ) {
                    return unknown;
                }
                const auto &book = get_book( loc );
                const int unlearned = book.recipes.size() - get_known_recipes( book );

                return unlearned > 0 ? to_string( unlearned ) : std::string();
            }, _( "RECIPES" ), unknown );
            append_cell( [ this, &p, unknown ]( const item_location & loc ) -> std::string {
                if( !is_known( loc ) ) {
                    return unknown;
                }
                return good_bad_none( p.book_fun_for( *loc, p ) );
            }, _( "FUN" ), unknown );

            append_cell( [ this, &p, unknown ]( const item_location & loc ) -> std::string {
                if( !is_known( loc ) ) {
                    return unknown;
                }
                std::vector<std::string> dummy;

                // This is terrible and needs to be removed asap when this entire file is refactored
                // to use the new avatar class
                const avatar *u = dynamic_cast<const avatar *>( &p );
                if( !u ) {
                    return std::string();
                }
                const player *reader = u->get_book_reader( *loc, dummy );
                if( reader == nullptr ) {
                    return std::string();  // Just to make sure
                }
                // Actual reading time (in turns). Can be penalized.
                const int actual_turns = u->time_to_read( *loc, *reader ) / to_moves<int>( 1_turns );
                // Theoretical reading time (in turns) based on the reader speed. Free of penalties.
                const int normal_turns = get_book( loc ).time * reader->read_speed() / to_moves<int>( 1_turns );
                const std::string duration = to_string_approx( time_duration::from_turns( actual_turns ), false );

                if( actual_turns > normal_turns ) { // Longer - complicated stuff.
                    return string_format( "<color_light_red>%s</color>", duration );
                }

                return duration; // Normal speed.
            }, _( "CHAPTER IN" ), unknown );
        }

        bool is_shown( const item_location &loc ) const override {
            return loc->is_book();
        }

        std::string get_denial( const item_location &loc ) const override {
            // This is terrible and needs to be removed asap when this entire file is refactored
            // to use the new avatar class
            const avatar *u = dynamic_cast<const avatar *>( &p );
            if( !u ) {
                return std::string();
            }

            std::vector<std::string> denials;
            if( u->get_book_reader( *loc, denials ) == nullptr && !denials.empty() ) {
                return denials.front();
            }
            return pickup_inventory_preset::get_denial( loc );
        }

        std::function<bool( const inventory_entry & )> get_filter( const std::string &filter ) const
        override {
            auto base_filter = pickup_inventory_preset::get_filter( filter );

            return [this, base_filter, filter]( const inventory_entry & e ) {
                if( base_filter( e ) ) {
                    return true;
                }

                if( !is_known( e.any_item() ) ) {
                    return false;
                }

                const auto &book = get_book( e.any_item() );
                if( book.skill && p.get_skill_level_object( book.skill ).can_train() ) {
                    return lcmatch( book.skill->name(), filter );
                }

                return false;
            };
        }

        bool sort_compare( const inventory_entry &lhs, const inventory_entry &rhs ) const override {
            const bool base_sort = inventory_selector_preset::sort_compare( lhs, rhs );

            const bool known_a = is_known( lhs.any_item() );
            const bool known_b = is_known( rhs.any_item() );

            if( !known_a || !known_b ) {
                return ( !known_a && !known_b ) ? base_sort : !known_a;
            }

            const auto &book_a = get_book( lhs.any_item() );
            const auto &book_b = get_book( rhs.any_item() );

            if( !book_a.skill && !book_b.skill ) {
                return ( book_a.fun == book_b.fun ) ? base_sort : book_a.fun > book_b.fun;
            } else if( !book_a.skill || !book_a.skill ) {
                return static_cast<bool>( book_a.skill );
            }

            const bool train_a = p.get_skill_level( book_a.skill ) < book_a.level;
            const bool train_b = p.get_skill_level( book_b.skill ) < book_b.level;

            if( !train_a || !train_b ) {
                return ( !train_a && !train_b ) ? base_sort : train_a;
            }

            return base_sort;
        }

    private:
        const islot_book &get_book( const item_location &loc ) const {
            return *loc->type->book;
        }

        bool is_known( const item_location &loc ) const {
            // This is terrible and needs to be removed asap when this entire file is refactored
            // to use the new avatar class
            if( const avatar *u = dynamic_cast<const avatar *>( &p ) ) {
                return u->has_identified( loc->typeId() );
            }
            return false;
        }

        int get_known_recipes( const islot_book &book ) const {
            int res = 0;
            for( const auto &elem : book.recipes ) {
                if( p.knows_recipe( elem.recipe ) ) {
                    ++res; // If the player knows it, they recognize it even if it's not clearly stated.
                }
            }
            return res;
        }

        const player &p;
};

item_location game_menus::inv::read( player &pl )
{
    const std::string msg = pl.is_player() ? _( "You have nothing to read." ) :
                            string_format( _( "%s has nothing to read." ), pl.disp_name() );
    return inv_internal( pl, read_inventory_preset( pl ), _( "Read" ), 1, msg );
}

class steal_inventory_preset : public pickup_inventory_preset
{
    public:
        steal_inventory_preset( const avatar &p, const player &victim ) :
            pickup_inventory_preset( p ), victim( victim ) {}

        bool is_shown( const item_location &loc ) const override {
            return !victim.is_worn( *loc ) && &victim.weapon != &( *loc );
        }

    private:
        const player &victim;
};

item_location game_menus::inv::steal( avatar &you, player &victim )
{
    return inv_internal( victim, steal_inventory_preset( you, victim ),
                         string_format( _( "Steal from %s" ), victim.name ), -1,
                         string_format( _( "%s's inventory is empty." ), victim.name ) );
}

class weapon_inventory_preset: public inventory_selector_preset
{
    public:
        weapon_inventory_preset( const player &p ) : p( p ) {
            append_cell( [ this ]( const item_location & loc ) {
                if( !loc->is_gun() ) {
                    return std::string();
                }

                const int total_damage = loc->gun_damage( true ).total_damage();

                if( loc->ammo_data() && loc->ammo_remaining() ) {
                    const int basic_damage = loc->gun_damage( false ).total_damage();
                    if( loc->ammo_data()->ammo->prop_damage ) {
                        const int ammo_mult = *loc->ammo_data()->ammo->prop_damage;

                        return string_format( "%s<color_light_gray>*</color>%s <color_light_gray>=</color> %s",
                                              get_damage_string( basic_damage, true ),
                                              get_damage_string( ammo_mult, true ),
                                              get_damage_string( total_damage, true )
                                            );
                    } else {
                        const int ammo_damage = loc->ammo_data()->ammo->damage.total_damage();

                        return string_format( "%s<color_light_gray>+</color>%s <color_light_gray>=</color> %s",
                                              get_damage_string( basic_damage, true ),
                                              get_damage_string( ammo_damage, true ),
                                              get_damage_string( total_damage, true )
                                            );
                    }
                } else {
                    return get_damage_string( total_damage );
                }
            }, pgettext( "Shot as damage", "SHOT" ) );

            append_cell( [ this ]( const item_location & loc ) {
                return get_damage_string( loc->damage_melee( DT_BASH ) );
            }, _( "BASH" ) );

            append_cell( [ this ]( const item_location & loc ) {
                return get_damage_string( loc->damage_melee( DT_CUT ) );
            }, _( "CUT" ) );

            append_cell( [ this ]( const item_location & loc ) {
                return get_damage_string( loc->damage_melee( DT_STAB ) );
            }, _( "STAB" ) );

            append_cell( [ this ]( const item_location & loc ) {
                if( deals_melee_damage( *loc ) ) {
                    return good_bad_none( loc->type->m_to_hit );
                }
                return std::string();
            }, _( "MELEE" ) );

            append_cell( [ this ]( const item_location & loc ) {
                if( deals_melee_damage( *loc ) ) {
                    return string_format( "<color_yellow>%d</color>", this->p.attack_speed( *loc ) );
                }
                return std::string();
            }, _( "MOVES" ) );
        }

        std::string get_denial( const item_location &loc ) const override {
            const auto ret = p.can_wield( *loc );

            if( !ret.success() ) {
                return trim_punctuation_marks( ret.str() );
            }

            return std::string();
        }

    private:
        bool deals_melee_damage( const item &it ) const {
            return it.damage_melee( DT_BASH ) || it.damage_melee( DT_CUT ) || it.damage_melee( DT_STAB );
        }

        std::string get_damage_string( int damage, bool display_zeroes = false ) const {
            return damage ||
                   display_zeroes ? string_format( "<color_yellow>%d</color>", damage ) : std::string();
        }

        const player &p;
};

item_location game_menus::inv::wield( avatar &you )
{
    return inv_internal( you, weapon_inventory_preset( you ), _( "Wield item" ), 1,
                         _( "You have nothing to wield." ) );
}

class holster_inventory_preset: public weapon_inventory_preset
{
    public:
        holster_inventory_preset( const player &p, const holster_actor &actor ) :
            weapon_inventory_preset( p ), actor( actor ) {
        }

        bool is_shown( const item_location &loc ) const override {
            return actor.can_holster( *loc );
        }

    private:
        const holster_actor &actor;
};

item_location game_menus::inv::holster( player &p, item &holster )
{
    const std::string holster_name = holster.tname( 1, false );
    const auto actor = dynamic_cast<const holster_actor *>
                       ( holster.type->get_use( "holster" )->get_actor_ptr() );

    if( !actor ) {
        const std::string msg = string_format( _( "You can't put anything into your %s." ),
                                               holster_name );
        popup( msg, PF_GET_KEY );
        return item_location();
    }

    const std::string title = actor->holster_prompt.empty()
                              ? _( "Holster item" )
                              : _( actor->holster_prompt );
    const std::string hint = string_format( _( "Choose a weapon to put into your %s" ),
                                            holster_name );

    return inv_internal( p, holster_inventory_preset( p, *actor ), title, 1,
                         string_format( _( "You have no weapons you could put into your %s." ),
                                        holster_name ),
                         hint );
}

class saw_barrel_inventory_preset: public weapon_inventory_preset
{
    public:
        saw_barrel_inventory_preset( const player &p, const item &tool, const saw_barrel_actor &actor ) :
            weapon_inventory_preset( p ), p( p ), tool( tool ), actor( actor ) {
        }

        bool is_shown( const item_location &loc ) const override {
            return loc->is_gun();
        }

        std::string get_denial( const item_location &loc ) const override {
            const auto ret = actor.can_use_on( p, tool, *loc );

            if( !ret.success() ) {
                return trim_punctuation_marks( ret.str() );
            }

            return std::string();
        }

    private:
        const player &p;
        const item &tool;
        const saw_barrel_actor &actor;
};

class salvage_inventory_preset: public inventory_selector_preset
{
    public:
        salvage_inventory_preset( const salvage_actor *actor ) :
            actor( actor ) {

            append_cell( [ actor ]( const item_location & loc ) {
                return to_string_clipped( time_duration::from_turns( actor->time_to_cut_up(
                                              *loc.get_item() ) / 100 ) );
            }, _( "TIME" ) );
        }

        bool is_shown( const item_location &loc ) const override {
            return actor->valid_to_cut_up( *loc.get_item() );
        }

    private:
        const salvage_actor *actor;
};

item_location game_menus::inv::salvage( player &p, const salvage_actor *actor )
{
    return inv_internal( p, salvage_inventory_preset( actor ),
                         _( "Cut up what?" ), 1,
                         _( "You have nothing to cut up." ) );
}

class repair_inventory_preset: public inventory_selector_preset
{
    public:
        repair_inventory_preset( const repair_item_actor *actor, const item *main_tool ) :
            actor( actor ), main_tool( main_tool ) {
        }

        bool is_shown( const item_location &loc ) const override {
            return loc->made_of_any( actor->materials ) && !loc->count_by_charges() && !loc->is_firearm() &&
                   &*loc != main_tool;
        }

    private:
        const repair_item_actor *actor;
        const item *main_tool;
};

item_location game_menus::inv::repair( player &p, const repair_item_actor *actor,
                                       const item *main_tool )
{
    return inv_internal( p, repair_inventory_preset( actor, main_tool ),
                         _( "Repair what?" ), 1,
                         string_format( _( "You have no items that could be repaired with a %s." ),
                                        main_tool->type_name( 1 ) ) );
}

item_location game_menus::inv::saw_barrel( player &p, item &tool )
{
    const auto actor = dynamic_cast<const saw_barrel_actor *>
                       ( tool.type->get_use( "saw_barrel" )->get_actor_ptr() );

    if( !actor ) {
        debugmsg( "Tried to use a wrong item." );
        return item_location();
    }

    return inv_internal( p, saw_barrel_inventory_preset( p, tool, *actor ),
                         _( "Saw barrel" ), 1,
                         _( "You don't have any guns." ),
                         string_format( _( "Choose a weapon to use your %s on" ),
                                        tool.tname( 1, false )
                                      )
                       );
}

std::list<std::pair<int, int>> game_menus::inv::multidrop( player &p )
{
    p.inv.restack( p );

    const inventory_filter_preset preset( [ &p ]( const item_location & location ) {
        return p.can_unwield( *location ).success();
    } );

    inventory_drop_selector inv_s( p, preset );

    inv_s.add_character_items( p );
    inv_s.set_title( _( "Multidrop" ) );
    inv_s.set_hint( _( "To drop x items, type a number before selecting." ) );

    if( inv_s.empty() ) {
        popup( std::string( _( "You have nothing to drop." ) ), PF_GET_KEY );
        return std::list<std::pair<int, int> >();
    }

    return inv_s.execute();
}

void game_menus::inv::compare( player &p, const cata::optional<tripoint> &offset )
{
    p.inv.restack( p );

    inventory_compare_selector inv_s( p );

    inv_s.add_character_items( p );
    inv_s.set_title( _( "Compare" ) );
    inv_s.set_hint( _( "Select two items to compare them." ) );

    if( offset ) {
        inv_s.add_map_items( p.pos() + *offset );
        inv_s.add_vehicle_items( p.pos() + *offset );
    } else {
        inv_s.add_nearby_items();
    }

    if( inv_s.empty() ) {
        popup( std::string( _( "There are no items to compare." ) ), PF_GET_KEY );
        return;
    }

    std::string action;
    input_context ctxt;
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "UP" );
    ctxt.register_action( "DOWN" );
    ctxt.register_action( "PAGE_UP" );
    ctxt.register_action( "PAGE_DOWN" );

    do {
        const auto to_compare = inv_s.execute();

        if( to_compare.first == nullptr || to_compare.second == nullptr ) {
            break;
        }

        std::vector<iteminfo> vItemLastCh;
        std::vector<iteminfo> vItemCh;
        std::string sItemLastCh;
        std::string sItemCh;
        std::string sItemLastTn;
        std::string sItemTn;

        to_compare.first->info( true, vItemCh );
        sItemCh = to_compare.first->tname();
        sItemTn = to_compare.first->type_name();

        to_compare.second->info( true, vItemLastCh );
        sItemLastCh = to_compare.second->tname();
        sItemLastTn = to_compare.second->type_name();

        int iScrollPos = 0;
        int iScrollPosLast = 0;

        do {
            draw_item_info( 0, ( TERMX - VIEW_OFFSET_X * 2 ) / 2, 0, TERMY - VIEW_OFFSET_Y * 2,
                            sItemLastCh, sItemLastTn, vItemLastCh, vItemCh, iScrollPosLast, true );
            draw_item_info( ( TERMX - VIEW_OFFSET_X * 2 ) / 2, ( TERMX - VIEW_OFFSET_X * 2 ) / 2,
                            0, TERMY - VIEW_OFFSET_Y * 2, sItemCh, sItemTn, vItemCh, vItemLastCh,
                            iScrollPos, true );

            action = ctxt.handle_input();

            if( action == "UP" || action == "PAGE_UP" ) {
                iScrollPos--;
                iScrollPosLast--;
            } else if( action == "DOWN" || action == "PAGE_DOWN" ) {
                iScrollPos++;
                iScrollPosLast++;
            }

        } while( action != "QUIT" );
        g->refresh_all();
    } while( true );
}

void game_menus::inv::reassign_letter( player &p, item &it )
{
    while( true ) {
        const int invlet = popup_getkey(
                               _( "Enter new letter. Press SPACE to clear a manually assigned letter, ESCAPE to cancel." ) );

        if( invlet == KEY_ESCAPE ) {
            break;
        } else if( invlet == ' ' ) {
            p.reassign_item( it, 0 );
            const std::string auto_setting = get_option<std::string>( "AUTO_INV_ASSIGN" );
            if( auto_setting == "enabled" || ( auto_setting == "favorites" && it.is_favorite ) ) {
                popup_getkey(
                    _( "Note: The Auto Inventory Letters setting might still reassign a letter to this item.\n"
                       "If this is undesired, you may wish to change the setting in Options." ) );
            }
            break;
        } else if( inv_chars.valid( invlet ) ) {
            p.reassign_item( it, invlet );
            break;
        }
    }
}

void game_menus::inv::swap_letters( player &p )
{
    p.inv.restack( p );

    inventory_pick_selector inv_s( p );

    inv_s.add_character_items( p );
    inv_s.set_title( _( "Swap Inventory Letters" ) );
    inv_s.set_display_stats( false );

    if( inv_s.empty() ) {
        popup( std::string( _( "Your inventory is empty." ) ), PF_GET_KEY );
        return;
    }

    while( true ) {
        const std::string invlets = colorize_symbols( inv_chars.get_allowed_chars(),
        [ &p ]( const std::string::value_type & elem ) {
            if( p.inv.assigned_invlet.count( elem ) ) {
                return c_yellow;
            } else if( p.invlet_to_position( elem ) != INT_MIN ) {
                return c_white;
            } else {
                return c_dark_gray;
            }
        } );

        inv_s.set_hint( invlets );

        auto loc = inv_s.execute();

        if( !loc ) {
            break;
        }

        reassign_letter( p, *loc );
        g->refresh_all();
    }
}

static item_location autodoc_internal( player &u, player &patient,
                                       const inventory_selector_preset &preset,
                                       int radius, bool uninstall = false, bool surgeon = false )
{
    inventory_pick_selector inv_s( u, preset );
    std::string hint;
    int drug_count = 0;

    if( !surgeon ) {//surgeon use their own anesthetic, player just need money
        if( patient.has_trait( trait_NOPAIN ) ) {
            hint = _( "<color_yellow>Patient has Deadened nerves.  Anesthesia unneeded.</color>" );
        } else if( patient.has_bionic( bionic_id( "bio_painkiller" ) ) ) {
            hint = _( "<color_yellow>Patient has Sensory Dulling CBM installed.  Anesthesia unneeded.</color>" );
        } else {
            const inventory &crafting_inv = u.crafting_inventory();
            std::vector<const item *> a_filter = crafting_inv.items_with( []( const item & it ) {
                return it.has_quality( quality_id( "ANESTHESIA" ) );
            } );
            std::vector<const item *> b_filter = crafting_inv.items_with( []( const item & it ) {
                return it.has_flag( "ANESTHESIA" ); // legacy
            } );
            for( const item *anesthesia_item : a_filter ) {
                if( anesthesia_item->ammo_remaining() >= 1 ) {
                    drug_count += anesthesia_item->ammo_remaining();
                }
            }

            if( !b_filter.empty() ) {
                hint = string_format( _( "<color_yellow>Available kit: %i</color>" ), b_filter.size() );// legacy
            } else {
                hint = string_format( _( "<color_yellow>Available anesthetic: %i mL</color>" ), drug_count );
            }

        }
    } else {
        hint = string_format( _( "<color_yellow>Money available: %s</color>" ), format_money( u.cash ) );
    }

    if( uninstall ) {
        inv_s.set_title( string_format( _( "Bionic removal patient: %s" ), patient.get_name() ) );
    } else {
        inv_s.set_title( string_format( _( "Bionic installation patient: %s" ), patient.get_name() ) );
    }

    inv_s.set_hint( hint );
    inv_s.set_display_stats( false );

    std::pair<size_t, size_t> init_pair;
    bool init_selection = false;

    do {
        u.inv.restack( u );

        inv_s.clear_items();
        inv_s.add_character_items( u );
        inv_s.add_nearby_items( radius );

        if( init_selection ) {
            inv_s.update();
            inv_s.select_position( init_pair );
            init_selection = false;
        }

        if( inv_s.empty() ) {
            popup( _( "You don't have any bionics to install." ), PF_GET_KEY );
            return item_location();
        }

        item_location location = inv_s.execute();

        if( inv_s.keep_open ) {
            inv_s.keep_open = false;
            continue;
        }

        return location;

    } while( true );
}

// Menu used by autodoc when installing a bionic
class bionic_install_preset: public inventory_selector_preset
{
    public:
        bionic_install_preset( player &pl, player &patient ) :
            p( pl ), pa( patient ) {
            append_cell( [ this ]( const item_location & loc ) {
                return get_failure_chance( loc ) ;
            }, _( "FAILURE CHANCE" ) );

            append_cell( [ this ]( const item_location & loc ) {
                return get_operation_duration( loc ) ;
            }, _( "OPERATION DURATION" ) );

            append_cell( [this]( const item_location & loc ) {
                return get_anesth_amount( loc );
            }, _( "ANESTHETIC REQUIRED" ) );
        }

        bool is_shown( const item_location &loc ) const override {
            return loc->is_bionic();
        }

        std::string get_denial( const item_location &loc ) const override {
            const item *it = loc.get_item();
            const itype *itemtype = it->type;
            const bionic_id &bid = itemtype->bionic->id;

            if( it->has_flag( "FILTHY" ) ) {
                return _( "/!\\ CBM is highly contaminated. /!\\" );
            } else if( it->has_flag( "NO_STERILE" ) ) {
                return _( "/!\\ CBM is not sterile. /!\\" ) ;
            } else if( it->has_fault( fault_id( "fault_bionic_salvaged" ) ) ) {
                return _( "CBM already deployed.  Please reset to factory state." );
            } else if( pa.has_bionic( bid ) ) {
                return _( "CBM already installed" );
            } else if( bid->upgraded_bionic &&
                       !pa.has_bionic( bid->upgraded_bionic ) &&
                       it->is_upgrade() ) {
                return _( "No base version installed" );
            } else if( std::any_of( bid->available_upgrades.begin(),
                                    bid->available_upgrades.end(),
                                    std::bind( &player::has_bionic, &pa,
                                               std::placeholders::_1 ) ) ) {
                return _( "Superior version installed" );
            } else if( pa.is_npc() && !bid->npc_usable ) {
                return _( "CBM not compatible with patient" );
            } else if( !p.has_enough_anesth( itemtype, pa ) ) {
                const int weight = units::to_kilogram( pa.bodyweight() ) / 10;
                const int duration = loc.get_item()->type->bionic->difficulty * 2;
                const requirement_data req_anesth = *requirement_id( "anesthetic" ) *
                                                    duration * weight;
                return string_format( _( "%i mL" ), req_anesth.get_tools().front().front().count );
            }

            return std::string();
        }

    protected:
        player &p;
        player &pa;

    private:
        // Returns a formatted string of how long the operation will take.
        std::string get_operation_duration( const item_location &loc ) {
            const int difficulty = loc.get_item()->type->bionic->difficulty;
            // 20 minutes per bionic difficulty.
            return to_string( time_duration::from_minutes( difficulty * 20 ) );
        }

        // Failure chance for bionic install. Combines multiple other functions together.
        std::string get_failure_chance( const item_location &loc ) {

            const int difficulty = loc.get_item()->type->bionic->difficulty;
            int chance_of_failure = 100;
            player &installer = p;
            const int assist_bonus = installer.get_effect_int( effect_assisted );

            const int adjusted_skill = installer.bionics_adjusted_skill( skill_firstaid,
                                       skill_computer,
                                       skill_electronics,
                                       -1 );

            if( ( get_option < bool > ( "SAFE_AUTODOC" ) ) ||
                g->u.has_trait( trait_id( "DEBUG_BIONICS" ) ) ) {
                chance_of_failure = 0;
            } else {
                float skill_difficulty_parameter = static_cast<float>( ( adjusted_skill + assist_bonus ) /
                                                   ( 4.0 * difficulty ) );
                chance_of_failure = 100 - static_cast<int>( ( 100 * skill_difficulty_parameter ) /
                                    ( skill_difficulty_parameter + sqrt( 1 / skill_difficulty_parameter ) ) );
            }

            return string_format( _( "%i%%" ), chance_of_failure );
        }

        std::string get_anesth_amount( const item_location &loc ) {

            const int weight = units::to_kilogram( pa.bodyweight() ) / 10;
            const int duration = loc.get_item()->type->bionic->difficulty * 2;
            const requirement_data req_anesth = *requirement_id( "anesthetic" ) *
                                                duration * weight;

            std::vector<const item *> b_filter = p.crafting_inventory().items_with( []( const item & it ) {
                return it.has_flag( "ANESTHESIA" ); // legacy
            } );

            if( !b_filter.empty() ) {
                return  _( "kit available" );// legacy
            } else {
                return string_format( _( "%i mL" ), req_anesth.get_tools().front().front().count );
            }
        }
};

// Menu used by surgeon when installing a bionic
class bionic_install_surgeon_preset : public inventory_selector_preset
{
    public:
        bionic_install_surgeon_preset( player &pl, player &patient ) :
            p( pl ), pa( patient ) {
            append_cell( [this]( const item_location & loc ) {
                return get_failure_chance( loc );
            }, _( "FAILURE CHANCE" ) );

            append_cell( [this]( const item_location & loc ) {
                return get_operation_duration( loc );
            }, _( "OPERATION DURATION" ) );

            append_cell( [this]( const item_location & loc ) {
                return get_money_amount( loc );
            }, _( "PRICE" ) );
        }

        bool is_shown( const item_location &loc ) const override {
            return loc->is_bionic();
        }

        std::string get_denial( const item_location &loc ) const override {
            const item *it = loc.get_item();
            const itype *itemtype = it->type;
            const bionic_id &bid = itemtype->bionic->id;

            if( it->has_flag( "FILTHY" ) ) {
                return _( "CBM is filthy." );
            } else if( it->has_flag( "NO_STERILE" ) ) {
                return _( "CBM is not sterile." );
            } else if( it->has_fault( fault_id( "fault_bionic_salvaged" ) ) ) {
                return _( "CBM is already deployed." );
            } else if( pa.has_bionic( bid ) ) {
                return _( "CBM is already installed." );
            } else if( bid->upgraded_bionic &&
                       !pa.has_bionic( bid->upgraded_bionic ) &&
                       it->is_upgrade() ) {
                return _( "No base version installed." );
            } else if( std::any_of( bid->available_upgrades.begin(),
                                    bid->available_upgrades.end(),
                                    std::bind( &player::has_bionic, &pa,
                                               std::placeholders::_1 ) ) ) {
                return _( "Superior version installed." );
            } else if( pa.is_npc() && !bid->npc_usable ) {
                return _( "CBM is not compatible with patient." );
            } else if( it->price( true ) * 2 > p.cash ) {
                return format_money( it->price( true ) * 2 );
            }

            return std::string();
        }

    protected:
        player &p;
        player &pa;

    private:
        // Returns a formatted string of how long the operation will take.
        std::string get_operation_duration( const item_location &loc ) {
            const int difficulty = loc.get_item()->type->bionic->difficulty;
            // 20 minutes per bionic difficulty.
            return to_string( time_duration::from_minutes( difficulty * 20 ) );
        }

        // Failure chance for bionic install. Combines multiple other functions together.
        std::string get_failure_chance( const item_location &loc ) {

            const int difficulty = loc.get_item()->type->bionic->difficulty;
            int chance_of_failure = 100;
            player &installer = p;
            const int assist_bonus = installer.get_effect_int( effect_assisted );

            const int adjusted_skill = installer.bionics_adjusted_skill( skill_firstaid,
                                       skill_computer,
                                       skill_electronics,
                                       20 );//override player's skills with surgeon skill

            if( ( get_option < bool >( "SAFE_AUTODOC" ) ) ||
                g->u.has_trait( trait_id( "DEBUG_BIONICS" ) ) ) {
                chance_of_failure = 0;
            } else {
                float skill_difficulty_parameter = static_cast<float>( ( adjusted_skill + assist_bonus ) /
                                                   ( 4.0 * difficulty ) );
                chance_of_failure = 100 - static_cast<int>( ( 100 * skill_difficulty_parameter ) /
                                    ( skill_difficulty_parameter + sqrt( 1 / skill_difficulty_parameter ) ) );
            }

            return string_format( _( "%i%%" ), chance_of_failure );
        }

        std::string get_money_amount( const item_location &loc ) {
            return format_money( loc.get_item()->price( true ) * 2 );
        }
};

item_location game_menus::inv::install_bionic( player &p, player &patient, bool surgeon )
{
    if( surgeon ) {
        return autodoc_internal( p, patient, bionic_install_surgeon_preset( p, patient ), 5, false,
                                 surgeon );
    } else {
        return autodoc_internal( p, patient, bionic_install_preset( p, patient ), 5 );
    }

}
// Menu used by autodoc when uninstalling a bionic
class bionic_uninstall_preset : public inventory_selector_preset
{
    public:
        bionic_uninstall_preset( player &pl, player &patient ) :
            p( pl ), pa( patient ) {
            append_cell( [this]( const item_location & loc ) {
                return get_failure_chance( loc );
            }, _( "FAILURE CHANCE" ) );

            append_cell( [this]( const item_location & loc ) {
                return get_operation_duration( loc );
            }, _( "OPERATION DURATION" ) );

            append_cell( [this]( const item_location & loc ) {
                return get_anesth_amount( loc );
            }, _( "ANESTHETIC REQUIRED" ) );
        }

        bool is_shown( const item_location &loc ) const override {
            return loc->has_flag( "IN_CBM" );
        }

        std::string get_denial( const item_location &loc ) const override {
            const itype *itemtype = loc.get_item()->type;

            if( !p.has_enough_anesth( itemtype, pa ) ) {
                const int weight = units::to_kilogram( pa.bodyweight() ) / 10;
                const int duration = loc.get_item()->type->bionic->difficulty * 2;
                const requirement_data req_anesth = *requirement_id( "anesthetic" ) *
                                                    duration * weight;
                return string_format( _( "%i mL" ), req_anesth.get_tools().front().front().count );
            }

            return std::string();
        }

    protected:
        player &p;
        player &pa;

    private:
        // Returns a formatted string of how long the operation will take.
        std::string get_operation_duration( const item_location &loc ) {
            const int difficulty = loc.get_item()->type->bionic->difficulty;
            // 20 minutes per bionic difficulty.
            return to_string( time_duration::from_minutes( difficulty * 20 ) );
        }

        // Failure chance for bionic uninstall. Combines multiple other functions together.
        std::string get_failure_chance( const item_location &loc ) {

            // Uninstall difficulty gets a +2
            const int difficulty = loc.get_item()->type->bionic->difficulty + 2;
            int chance_of_failure = 100;
            player &installer = p;
            const int assist_bonus = installer.get_effect_int( effect_assisted );

            const int adjusted_skill = installer.bionics_adjusted_skill( skill_firstaid,
                                       skill_computer,
                                       skill_electronics,
                                       -1 );

            if( ( get_option < bool >( "SAFE_AUTODOC" ) ) ||
                g->u.has_trait( trait_id( "DEBUG_BIONICS" ) ) ) {
                chance_of_failure = 0;
            } else {
                float skill_difficulty_parameter = static_cast<float>( ( adjusted_skill + assist_bonus ) /
                                                   ( 4.0 * difficulty ) );
                chance_of_failure = 100 - static_cast<int>( ( 100 * skill_difficulty_parameter ) /
                                    ( skill_difficulty_parameter + sqrt( 1 / skill_difficulty_parameter ) ) );
            }

            return string_format( _( "%i%%" ), chance_of_failure );
        }

        std::string get_anesth_amount( const item_location &loc ) {

            const int weight = units::to_kilogram( pa.bodyweight() ) / 10;
            const int duration = loc.get_item()->type->bionic->difficulty * 2;
            const requirement_data req_anesth = *requirement_id( "anesthetic" ) *
                                                duration * weight;

            std::vector<const item *> b_filter = p.crafting_inventory().items_with( []( const item & it ) {
                return it.has_flag( "ANESTHESIA" ); // legacy
            } );

            if( !b_filter.empty() ) {
                return  _( "kit available" ); // legacy
            } else {
                return string_format( _( "%i mL" ), req_anesth.get_tools().front().front().count );
            }
        }
};

item_location game_menus::inv::uninstall_bionic( player &p, player &patient )
{
    return autodoc_internal( p, patient, bionic_uninstall_preset( p, patient ), 0, true );
}

// Menu used by autoclave when sterilizing a bionic
class bionic_sterilize_preset : public inventory_selector_preset
{
    public:
        bionic_sterilize_preset( player &pl ) :
            p( pl ) {

            append_cell( []( const item_location & ) {
                return to_string( 90_minutes );
            }, _( "CYCLE DURATION" ) );

            append_cell( []( const item_location & ) {
                return pgettext( "volume of water", "2 L" );
            }, _( "WATER REQUIRED" ) );
        }

        bool is_shown( const item_location &loc ) const override {
            return loc->has_flag( "NO_STERILE" ) && loc->is_bionic();
        }

        std::string get_denial( const item_location &loc ) const override {
            auto reqs = *requirement_id( "autoclave_item" );

            if( !reqs.can_make_with_inventory( p.crafting_inventory(), is_crafting_component ) ) {
                return pgettext( "volume of water", "2 L" );
            }

            if( loc.get_item()->has_flag( "FILTHY" ) ) {
                return  _( "CBM is filthy.  Wash it first." );
            }
            return std::string();
        }

    protected:
        player &p;
};

static item_location autoclave_internal( player &u,
        const inventory_selector_preset &preset,
        int radius )
{
    inventory_pick_selector inv_s( u, preset );
    inv_s.set_title( _( "Sterilization" ) );
    inv_s.set_hint( _( "<color_yellow>Select one CBM to sterilize</color>" ) );
    inv_s.set_display_stats( false );

    std::pair<size_t, size_t> init_pair;
    bool init_selection = false;
    do {
        u.inv.restack( u );

        inv_s.clear_items();
        inv_s.add_character_items( u );
        inv_s.add_nearby_items( radius );

        if( init_selection ) {
            inv_s.update();
            inv_s.select_position( init_pair );
            init_selection = false;
        }

        if( inv_s.empty() ) {
            popup( _( "You don't have any CBM to sterilize." ), PF_GET_KEY );
            return item_location();
        }

        item_location location = inv_s.execute();

        if( inv_s.keep_open ) {
            inv_s.keep_open = false;
            continue;
        }

        return location;

    } while( true );

}
item_location game_menus::inv::sterilize_cbm( player &p )
{
    return autoclave_internal( p, bionic_sterilize_preset( p ), 6 );
}
