#include "game_inventory.h"

#include "game.h"
#include "inventory_ui.h"
#include "options.h"
#include "player.h"
#include "crafting.h"
#include "output.h"
#include "recipe_dictionary.h"
#include "string_formatter.h"
#include "item.h"
#include "itype.h"
#include "iuse_actor.h"
#include "skill.h"
#include "map.h"

#include <algorithm>
#include <functional>

typedef std::function<bool( const item & )> item_filter;
typedef std::function<bool( const item_location & )> item_location_filter;

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

}

inventory_filter_preset::inventory_filter_preset( const item_location_filter &filter )
    : filter( filter )
{}

bool inventory_filter_preset::is_shown( const item_location &location ) const
{
    return filter( location );
}

item_location_filter convert_filter( const item_filter &filter )
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
    u.inv.restack( u );

    inventory_pick_selector inv_s( u, preset );

    inv_s.set_title( title );
    inv_s.set_hint( hint );
    inv_s.set_display_stats( false );

    inv_s.add_character_items( u );
    inv_s.add_nearby_items( radius );

    if( inv_s.empty() ) {
        const std::string msg = none_message.empty()
                                ? _( "You don't have the necessary item at hand." )
                                : none_message;
        popup( msg, PF_GET_KEY );
        return item_location();
    }

    return inv_s.execute();
}

void game_menus::inv::common( player &p )
{
    static const std::set<int> allowed_selections = { { ' ', '.', 'q', '=', '\n', KEY_LEFT, KEY_ESCAPE } };

    p.inv.restack( p );

    inventory_pick_selector inv_s( p );

    inv_s.add_character_items( p );
    inv_s.set_title( _( "Inventory" ) );

    int res;
    do {
        inv_s.set_hint( string_format(
                            _( "Item hotkeys assigned: <color_light_gray>%d</color>/<color_light_gray>%d</color>" ),
                            p.allocated_invlets().size(), inv_chars.size() - p.allocated_invlets().size() ) );
        const item_location &location = inv_s.execute();
        if( location == item_location::nowhere ) {
            break;
        }
        g->refresh_all();
        res = g->inventory_item_menu( p.get_item_position( location.get_item() ) );
        g->refresh_all();
    } while( allowed_selections.count( res ) != 0 );
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
    }, string_format( _( "You don't have a %s." ), item::nname( id ).c_str() ) );
}

class armor_inventory_preset: public inventory_selector_preset
{
    public:
        armor_inventory_preset( const std::string &color_in ) : color( color_in ) {
            append_cell( [ this ]( const item_location & loc ) {
                return get_number_string( loc->get_encumber() );
            }, _( "ENCUMBRANCE" ) );

            append_cell( [ this ]( const item_location & loc ) {
                return loc->get_storage() > 0 ? string_format( "<%s>%s</color>", color,
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

    private:
        std::string get_number_string( int number ) const {
            return number ? string_format( "<%s>%d</color>", color, number ) : std::string();
        }

        const std::string color;
};

class wear_inventory_preset: public armor_inventory_preset
{
    public:
        wear_inventory_preset( const player &p,
                               const std::string &color ) : armor_inventory_preset( color ), p( p ) {}

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

    private:
        const player &p;
};

item_location game_menus::inv::wear( player &p )
{
    return inv_internal( p, wear_inventory_preset( p, "color_yellow" ), _( "Wear item" ), 1,
                         _( "You have nothing to wear." ) );
}

class take_off_inventory_preset: public armor_inventory_preset
{
    public:
        take_off_inventory_preset( const player &p,
                                   const std::string &color ) : armor_inventory_preset( color ), p( p ) {}

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

    private:
        const player &p;
};

item_location game_menus::inv::take_off( player &p )
{
    return inv_internal( p, take_off_inventory_preset( p, "color_red" ), _( "Take off item" ), 1,
                         _( "You don't wear anything." ) );
}

item_location game::inv_map_splice( item_filter filter, const std::string &title, int radius,
                                    const std::string &none_message )
{
    return inv_internal( u, inventory_filter_preset( convert_filter( filter ) ),
                         title, radius, none_message );
}

item_location game_menus::inv::container_for( player &p, const item &liquid, int radius )
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

    return inv_internal( p, inventory_filter_preset( filter ),
                         string_format( _( "Container for %s" ), liquid.display_name( liquid.charges ).c_str() ), radius,
                         string_format( _( "You don't have a suitable container for carrying %s." ),
                                        liquid.tname().c_str() ) );
}

class pickup_inventory_preset : public inventory_selector_preset
{
    public:
        pickup_inventory_preset( const player &p ) : p( p ) {}

        std::string get_denial( const item_location &loc ) const override {
            if( !p.has_item( *loc ) ) {
                if( loc->made_of( LIQUID ) ) {
                    return _( "Can't pick up spilt liquids" );
                } else if( !p.can_pickVolume( *loc ) ) {
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

            append_cell( [ this ]( const item_location & loc ) {
                const auto &req = get_recipe( loc ).disassembly_requirements();
                if( req.is_empty() ) {
                    return std::string();
                }
                const auto components = req.get_components();
                return enumerate_as_string( components.begin(), components.end(),
                []( const decltype( components )::value_type & comps ) {
                    return comps.front().to_string();
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
        const inventory inv;
};

item_location game_menus::inv::disassemble( player &p )
{
    return inv_internal( p, disassemble_inventory_preset( p, p.crafting_inventory() ),
                         _( "Disassemble item" ), 1,
                         _( "You don't have any items you could disassemble." ) );
};

class comestible_inventory_preset : public inventory_selector_preset
{
    public:
        comestible_inventory_preset( const player &p ) : inventory_selector_preset(), p( p ) {

            append_cell( [ p, this ]( const item_location & loc ) {
                return good_bad_none( p.nutrition_for( get_comestible_item( loc ) ) );
            }, _( "NUTRITION" ) );

            append_cell( [ this ]( const item_location & loc ) {
                return good_bad_none( get_edible_comestible( loc ).quench );
            }, _( "QUENCH" ) );

            append_cell( [ p, this ]( const item_location & loc ) {
                return good_bad_none( p.fun_for( get_comestible_item( loc ) ).first );
            }, _( "JOY" ) );

            append_cell( [ this ]( const item_location & loc ) {
                const time_duration spoils = get_edible_comestible( loc ).spoils;
                if( spoils > 0 ) {
                    return to_string_clipped( spoils );
                }
                return std::string();
            }, _( "SPOILS IN" ) );

            append_cell( [ this, &p ]( const item_location & loc ) {
                std::string cbm_name;

                switch( p.get_cbm_rechargeable_with( get_comestible_item( loc ) ) ) {
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
                }

                if( !cbm_name.empty() ) {
                    return string_format( "<color_cyan>%s</color>", cbm_name.c_str() );
                }

                return std::string();
            }, _( "CBM" ) );

            append_cell( [ this, &p ]( const item_location & loc ) {
                return good_bad_none( p.get_acquirable_energy( get_comestible_item( loc ) ) );
            }, _( "ENERGY" ) );
        }

        bool is_shown( const item_location &loc ) const override {
            if( loc->typeId() == "1st_aid" ) {
                return false; // temporary fix for #12991
            }
            return p.can_consume( *loc );
        }

        std::string get_denial( const item_location &loc ) const override {
            if( loc->made_of( LIQUID ) && !g->m.has_flag( "LIQUIDCONT", loc.position() ) ) {
                return _( "Can't drink spilt liquids" );
            }

            const auto &it = get_comestible_item( loc );
            const auto res = p.can_eat( it );
            const auto cbm = p.get_cbm_rechargeable_with( it );

            if( !res.success() && cbm == rechargeable_cbm::none ) {
                return res.str();
            } else if( cbm == rechargeable_cbm::battery && p.power_level >= p.max_power_level ) {
                return _( "You're fully charged" );
            }

            return inventory_selector_preset::get_denial( loc );
        }

        bool sort_compare( const item_location &lhs, const item_location &rhs ) const override {
            const auto &a = get_comestible_item( lhs );
            const auto &b = get_comestible_item( rhs );

            const int freshness = rate_freshness( a, *lhs ) - rate_freshness( b, *rhs );
            if( freshness != 0 ) {
                return freshness > 0;
            }

            return inventory_selector_preset::sort_compare( lhs, rhs );
        }

    protected:
        int rate_freshness( const item &it, const item &container ) const {
            if( p.will_eat( it ).value() == edible_rating::ROTTEN ) {
                return -1;
            } else if( !container.type->container || !container.type->container->preserves ) {
                if( it.is_fresh() ) {
                    return 1;
                } else if( it.is_going_bad() ) {
                    return 3;
                } else if( it.goes_bad() ) {
                    return 2;
                }
            }

            return 0;
        }

        const item &get_comestible_item( const item_location &loc ) const {
            return p.get_comestible_from( const_cast<item &>( *loc ) );
        }

        const islot_comestible &get_edible_comestible( const item_location &loc ) const {
            return get_edible_comestible( get_comestible_item( loc ) );
        }

        const islot_comestible &get_edible_comestible( const item &it ) const {
            if( it.is_comestible() && p.can_eat( it ).success() ) {
                return *it.type->comestible;
            }
            static const islot_comestible dummy {};
            return dummy;
        }

    private:
        const player &p;
};

item_location game_menus::inv::consume( player &p )
{
    return inv_internal( p, comestible_inventory_preset( p ),
                         _( "Consume item" ), 1,
                         _( "You have nothing to consume." ) );
}

class activatable_inventory_preset : public pickup_inventory_preset
{
    public:
        activatable_inventory_preset( const player &p ) : pickup_inventory_preset( p ), p( p ) {
            if( get_option<bool>( "INV_USE_ACTION_NAMES" ) ) {
                append_cell( [ this ]( const item_location & loc ) {
                    return string_format( "<color_light_green>%s</color>", get_action_name( *loc ).c_str() );
                }, _( "ACTION" ) );
            }
        }

        bool is_shown( const item_location &loc ) const override {
            return loc->type->has_use();
        }

        std::string get_denial( const item_location &loc ) const override {
            const auto &uses = loc->type->use_methods;

            if( uses.size() == 1 ) {
                const auto ret = uses.begin()->second.can_call( p, *loc, false, p.pos() );
                if( !ret.success() ) {
                    return trim_punctuation_marks( ret.str() );
                }
            }

            if( !p.has_enough_charges( *loc, false ) ) {
                return string_format(
                           ngettext( "Needs at least %d charge",
                                     "Needs at least %d charges", loc->ammo_required() ),
                           loc->ammo_required() );
            }

            return pickup_inventory_preset::get_denial( loc );
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

item_location game_menus::inv::use( player &p )
{
    return inv_internal( p, activatable_inventory_preset( p ),
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
                                      p.enumerate_unmet_requirements( gunmod, *loc ).c_str() );
            }

            if( get_odds( loc ).first <= 0 ) {
                return _( "is too difficult for you to modify" );
            }

            return std::string();
        }

        bool sort_compare( const item_location &lhs, const item_location &rhs ) const override {
            const auto a = get_odds( lhs );
            const auto b = get_odds( rhs );

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
            static const std::string unknown( _( "<color_dark_gray>?</color>" ) );
            static const std::string martial_arts( _( "martial arts" ) );

            append_cell( [ this, &p ]( const item_location & loc ) -> std::string {
                if( loc->type->can_use( "MA_MANUAL" ) ) {
                    return martial_arts;
                }
                if( !is_known( loc ) ) {
                    return unknown;
                }
                const auto &book = get_book( loc );
                if( book.skill && p.get_skill_level_object( book.skill ).can_train() ) {
                    return string_format( _( "%s to %d" ), book.skill->name().c_str(), book.level );
                }
                return std::string();
            }, _( "TRAINS" ), unknown );

            append_cell( [ this ]( const item_location & loc ) -> std::string {
                if( !is_known( loc ) ) {
                    return unknown;
                }
                const auto &book = get_book( loc );
                const int unlearned = book.recipes.size() - get_known_recipes( book );

                return unlearned > 0 ? to_string( unlearned ) : std::string();
            }, _( "RECIPES" ), unknown );

            append_cell( [ this ]( const item_location & loc ) -> std::string {
                if( !is_known( loc ) ) {
                    return unknown;
                }
                return good_bad_none( get_book( loc ).fun );
            }, _( "FUN" ), unknown );

            append_cell( [ this, &p ]( const item_location & loc ) -> std::string {
                if( !is_known( loc ) ) {
                    return unknown;
                }
                std::vector<std::string> dummy;
                const player *reader = p.get_book_reader( *loc, dummy );
                if( reader == nullptr ) {
                    return std::string();  // Just to make sure
                }
                // Actual reading time (in turns). Can be penalized.
                const int actual_turns = p.time_to_read( *loc, *reader ) / to_moves<int>( 1_turns );
                // Theoretical reading time (in turns) based on the reader speed. Free of penalties.
                const int normal_turns = get_book( loc ).time * reader->read_speed() / to_moves<int>( 1_turns );
                const std::string duration = to_string_approx( time_duration::from_turns( actual_turns ), false );

                if( actual_turns > normal_turns ) { // Longer - complicated stuff.
                    return string_format( "<color_light_red>%s</color>", duration.c_str() );
                }

                return duration; // Normal speed.
            }, _( "CHAPTER IN" ), unknown );
        }

        bool is_shown( const item_location &loc ) const override {
            return loc->is_book();
        }

        std::string get_denial( const item_location &loc ) const override {
            std::vector<std::string> denials;
            if( p.get_book_reader( *loc, denials ) == nullptr && !denials.empty() ) {
                return denials.front();
            }
            return pickup_inventory_preset::get_denial( loc );
        }

    private:
        const islot_book &get_book( const item_location &loc ) const {
            return *loc->type->book;
        }

        bool is_known( const item_location &loc ) const {
            return p.has_identified( loc->typeId() );
        }

        int get_known_recipes( const islot_book &book ) const {
            int res = 0;
            for( auto const &elem : book.recipes ) {
                if( p.knows_recipe( elem.recipe ) ) {
                    ++res; // If the player knows it, they recognize it even if it's not clearly stated.
                }
            }
            return res;
        }

        const player &p;
};

item_location game_menus::inv::read( player &p )
{
    return inv_internal( p, read_inventory_preset( p ),
                         _( "Read" ), 1,
                         _( "You have nothing to read." ) );
}

class steal_inventory_preset : public pickup_inventory_preset
{
    public:
        steal_inventory_preset( const player &p, const player &victim ) :
            pickup_inventory_preset( p ), victim( victim ) {}

        bool is_shown( const item_location &loc ) const override {
            return !victim.is_worn( *loc ) && &victim.weapon != &( *loc );
        }

    private:
        const player &victim;
};

item_location game_menus::inv::steal( player &p, player &victim )
{
    return inv_internal( victim, steal_inventory_preset( p, victim ),
                         string_format( _( "Steal from %s" ), victim.name.c_str() ), -1,
                         string_format( _( "%s's inventory is empty." ), victim.name.c_str() ) );
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
                    const int ammo_damage = loc->ammo_data()->ammo->damage.total_damage();

                    return string_format( "%s<color_light_gray>+</color>%s <color_light_gray>=</color> %s",
                                          get_damage_string( basic_damage, true ).c_str(),
                                          get_damage_string( ammo_damage, true ).c_str(),
                                          get_damage_string( total_damage, true ).c_str()
                                        );
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

item_location game_menus::inv::wield( player &p )
{
    return inv_internal( p, weapon_inventory_preset( p ), _( "Wield item" ), 1,
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
                                               holster_name.c_str() );
        popup( msg, PF_GET_KEY );
        return item_location();
    }

    const std::string title = actor->holster_prompt.empty()
                              ? _( "Holster item" )
                              : _( actor->holster_prompt.c_str() );
    const std::string hint = string_format( _( "Choose a weapon to put into your %s" ),
                                            holster_name.c_str() );

    return inv_internal( p, holster_inventory_preset( p, *actor ), title, 1,
                         string_format( _( "You have no weapons you could put into your %s." ),
                                        holster_name.c_str() ),
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
                                        tool.tname( 1, false ).c_str()
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

void game_menus::inv::compare( player &p, const tripoint &offset )
{
    p.inv.restack( p );

    inventory_compare_selector inv_s( p );

    inv_s.add_character_items( p );
    inv_s.set_title( _( "Compare" ) );
    inv_s.set_hint( _( "Select two items to compare them." ) );

    if( offset != tripoint_min ) {
        inv_s.add_map_items( p.pos() + offset );
        inv_s.add_vehicle_items( p.pos() + offset );
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
        const long invlet = popup_getkey(
                                _( "Enter new letter (press SPACE for none, ESCAPE to cancel)." ) );

        if( invlet == KEY_ESCAPE ) {
            break;
        } else if( invlet == ' ' ) {
            p.reassign_item( it, 0 );
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
