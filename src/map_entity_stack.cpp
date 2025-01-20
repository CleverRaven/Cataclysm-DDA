#include "map_entity_stack.h"

#include "character.h"
#include "creature.h"
#include "item.h"
#include "item_category.h"
#include "item_search.h"
#include "mapdata.h"

static const trait_id trait_INATTENTIVE( "INATTENTIVE" );

template<typename T>
const T *map_entity_stack<T>::get_selected_entity() const
{
    if( !entities.empty() ) {
        // todo: check index
        return entities[selected_index].entity;
    }
    return nullptr;
}

template<typename T>
std::optional<tripoint_rel_ms> map_entity_stack<T>::get_selected_pos() const
{
    if( !entities.empty() ) {
        // todo: check index
        return entities[selected_index].pos;
    }
    return std::nullopt;
}

template<typename T>
int map_entity_stack<T>::get_selected_count() const
{
    if( !entities.empty() ) {
        // todo: check index
        return entities[selected_index].count;
    }
    return 0;
}

template<typename T>
std::string map_entity_stack<T>::get_selected_name()
{
    if( entities.empty() ) {
        return std::string();
    }

    if( !entities[selected_index].cached_name ) {
        make_name_cache();
    }

    return *entities[selected_index].cached_name;
}

template<>
void map_entity_stack<item>::make_name_cache()
{
    std::string text;
    if( entities.size() > 1 ) {
        text = string_format( "[%d/%d] (%d) ", get_selected_index() + 1, entities.size(), totalcount );
    }

    const int count = get_selected_count();
    if( count > 1 ) {
        text += string_format( "%d ", count );
    }
    text += get_selected_entity()->display_name( count );

    entities[selected_index].cached_name = text;
}

template<>
void map_entity_stack<Creature>::make_name_cache()
{
    std::string mon_name;
    if( get_selected_entity()->is_monster() ) {
        mon_name = get_selected_entity()->as_monster()->name();
    } else {
        mon_name = get_selected_entity()->disp_name();
    }

    entities[selected_index].cached_name = mon_name;
}

template<>
void map_entity_stack<map_data_common_t>::make_name_cache()
{
    std::string text;
    if( entities.size() > 1 ) {
        text = string_format( "[%d/%d] ", get_selected_index() + 1, entities.size() );
    }
    text += get_selected_entity()->name();

    entities[selected_index].cached_name = text;
}

template<typename T>
std::string map_entity_stack<T>::get_selected_distance_string()
{
    if( entities.empty() ) {
        return std::string();
    }

    if( !entities[selected_index].cached_distance_string ) {
        make_distance_string_cache();
    }

    return *entities[selected_index].cached_distance_string;
}

template<typename T>
void map_entity_stack<T>::make_distance_string_cache()
{
    const int dist = rl_dist( tripoint_rel_ms::zero, *get_selected_pos() );

    std::string text = string_format( "%*d %s", 2, dist,
                                      direction_name_short( direction_from( tripoint_rel_ms::zero, *get_selected_pos() ) ) );

    entities[selected_index].cached_distance_string = text;
}

template<typename T>
void map_entity_stack<T>::select_next()
{
    if( entities.empty() ) {
        return;
    }

    selected_index++;

    if( selected_index >= static_cast<int>( entities.size() ) ) {
        selected_index = 0;
    }
}

template<typename T>
void map_entity_stack<T>::select_prev()
{
    if( entities.empty() ) {
        return;
    }

    if( selected_index <= 0 ) {
        selected_index = entities.size();
    }

    selected_index--;
}

template<typename T>
int map_entity_stack<T>::get_selected_index() const
{
    return selected_index;
}

template<typename T>
map_entity_stack<T>::map_entity_stack() : totalcount( 0 )
{
    entities.emplace_back();
}

template<typename T>
map_entity_stack<T>::map_entity_stack( const T *const entity, const tripoint_rel_ms &pos,
                                       const int count ) : totalcount( count )
{
    entities.emplace_back( pos, 1, entity );
}

template<typename T>
void map_entity_stack<T>::add_at_pos( const T *const entity, const tripoint_rel_ms &pos,
                                      const int count )
{
    if( entities.empty() || entities.back().pos != pos ) {
        entities.emplace_back( pos, 1, entity );
    } else {
        entities.back().count++;
    }

    totalcount += count;
}

template<typename T>
const std::string map_entity_stack<T>::get_category() const
{
    return std::string();
}

template<>
const std::string map_entity_stack<item>::get_category() const
{
    const item *it = get_selected_entity();
    if( it ) {
        return it->get_category_of_contents().name_header();
    }

    return std::string();
}

template<>
const std::string map_entity_stack<Creature>::get_category() const
{
    const Creature *mon = get_selected_entity();
    if( mon ) {
        const Character &you = get_player_character();
        if( you.has_trait( trait_INATTENTIVE ) ) {
            return _( "Unknown" );
        }
        return Creature::get_attitude_ui_data( mon->attitude_to( you ) ).first.translated();
    }

    return std::string();
}

template<>
const std::string map_entity_stack<map_data_common_t>::get_category() const
{
    const map_data_common_t *tf = get_selected_entity();
    if( tf ) {
        if( tf->is_terrain() ) {
            return "TERRAIN";
        } else {
            return "FURNITURE";
        }
    }

    return std::string();
}

template<typename T>
bool map_entity_stack<T>::compare( const map_entity_stack<T> &, bool ) const
{
    return false;
}

template<>
bool map_entity_stack<item>::compare( const map_entity_stack<item> &rhs, bool use_category ) const
{
    bool compare_dist = rl_dist( tripoint_rel_ms(), entities[0].pos ) <
                        rl_dist( tripoint_rel_ms(), rhs.entities[0].pos );

    if( !use_category ) {
        return compare_dist;
    }

    const item_category &lhs_cat = get_selected_entity()->get_category_of_contents();
    const item_category &rhs_cat = rhs.get_selected_entity()->get_category_of_contents();

    if( lhs_cat == rhs_cat ) {
        return compare_dist;
    }

    return lhs_cat < rhs_cat;
}

template<>
bool map_entity_stack<Creature>::compare( const map_entity_stack<Creature> &rhs,
        bool use_category ) const
{
    bool compare_dist = rl_dist( tripoint_rel_ms(), entities[0].pos ) <
                        rl_dist( tripoint_rel_ms(), rhs.entities[0].pos );

    if( !use_category ) {
        return compare_dist;
    }

    Character &you = get_player_character();
    const Creature::Attitude att_lhs = get_selected_entity()->attitude_to( you );
    const Creature::Attitude att_rhs = rhs.get_selected_entity()->attitude_to( you );

    if( att_lhs == att_rhs ) {
        return compare_dist;
    }

    return att_lhs < att_rhs;
}

template<>
bool map_entity_stack<map_data_common_t>::compare( const map_entity_stack<map_data_common_t> &rhs,
        bool use_category ) const
{
    bool compare_dist = rl_dist( tripoint_rel_ms(), entities[0].pos ) <
                        rl_dist( tripoint_rel_ms(), rhs.entities[0].pos );

    if( !use_category ) {
        return compare_dist;
    }

    const bool &lhs_cat = get_selected_entity()->is_terrain();
    const bool &rhs_cat = rhs.get_selected_entity()->is_terrain();

    if( lhs_cat == rhs_cat ) {
        return compare_dist;
    }

    return rhs_cat;
}

//returns the last priority item index.
template<typename T>
int list_filter_high_priority( std::vector<map_entity_stack<T>*> &stack,
                               const std::string &priorities )
{
    std::vector<map_entity_stack<T>*> tempstack;
    const auto filter_fn = item_filter_from_string( priorities );
    for( auto it = stack.begin(); it != stack.end(); ) {
        if( priorities.empty() || ( ( *it )->get_selected_entity() != nullptr &&
                                    !filter_fn( *( *it )->get_selected_entity() ) ) ) {
            tempstack.push_back( *it );
            it = stack.erase( it );
        } else {
            it++;
        }
    }

    int id = stack.size();
    for( map_entity_stack<T> *elem : tempstack ) {
        stack.push_back( elem );
    }
    return id - 1;
}

template<typename T>
int list_filter_low_priority( std::vector<map_entity_stack<T>*> &stack, const int start,
                              const std::string &priorities )
{
    // todo: actually use it and specialization (only used for items)
    // TODO:optimize if necessary
    std::vector<map_entity_stack<T>*> tempstack;
    const auto filter_fn = item_filter_from_string( priorities );
    for( auto it = stack.begin() + start; it != stack.end(); ) {
        if( !priorities.empty() && ( *it )->get_selected_entity() != nullptr &&
            filter_fn( *( *it )->get_selected_entity() ) ) {
            tempstack.push_back( *it );
            it = stack.erase( it );
        } else {
            it++;
        }
    }

    int id = stack.size();
    for( map_entity_stack<T> *elem : tempstack ) {
        stack.push_back( elem );
    }
    return id;
}

// explicit template instantiation
template class map_entity_stack<item>;
template class map_entity_stack<Creature>;
template class map_entity_stack<map_data_common_t>;

template int list_filter_high_priority<item>( std::vector<map_entity_stack<item>*> &,
        const std::string & );
template int list_filter_low_priority<item>( std::vector<map_entity_stack<item>*> &, const int,
        const std::string & );
