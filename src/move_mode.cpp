#include "move_mode.h"

#include <algorithm>
#include <cstddef>
#include <set>
#include <string>

#include "assign.h"
#include "debug.h"
#include "game_constants.h"
#include "generic_factory.h"
#include "json.h"

static std::vector<move_mode_id> move_modes_sorted;

const std::vector<move_mode_id> &move_modes_by_speed()
{
    return move_modes_sorted;
}

namespace
{
generic_factory<move_mode> move_mode_factory( "move_mode" );
} // namespace

template<>
const move_mode &move_mode_id::obj() const
{
    return move_mode_factory.obj( *this );
}

template<>
bool move_mode_id::is_valid() const
{
    return move_mode_factory.is_valid( *this );
}

static const std::unordered_map<std::string, move_mode_type> move_types {
    { "prone",     move_mode_type::PRONE },
    { "crouching", move_mode_type::CROUCHING },
    { "walking",   move_mode_type::WALKING },
    { "running",   move_mode_type::RUNNING }
};

void move_mode::load_move_mode( const JsonObject &jo, const std::string &src )
{
    move_mode_factory.load( jo, src );
}

void move_mode::load( const JsonObject &jo, const std::string_view/*src*/ )
{
    mandatory( jo, was_loaded, "character", _letter, unicode_codepoint_from_symbol_reader );
    mandatory( jo, was_loaded, "name",  _name );

    mandatory( jo, was_loaded, "panel_char", _panel_letter, unicode_codepoint_from_symbol_reader );
    assign( jo, "panel_color", _panel_color );
    assign( jo, "symbol_color", _symbol_color );

    std::string exert = jo.get_string( "exertion_level" );
    if( !activity_levels_map.count( exert ) ) {
        jo.throw_error_at( id.str(), "Invalid activity level for move mode " + id.str() );
    }
    _exertion_level = activity_levels_map.at( exert );

    mandatory( jo, was_loaded, "change_good_none", change_messages_success[steed_type::NONE] );
    mandatory( jo, was_loaded, "change_good_animal", change_messages_success[steed_type::ANIMAL] );
    mandatory( jo, was_loaded, "change_good_mech", change_messages_success[steed_type::MECH] );

    optional( jo, was_loaded, "change_bad_none", change_messages_fail[steed_type::NONE],
              to_translation( "You feel bugs crawl over your skin." ) );
    optional( jo, was_loaded, "change_bad_animal", change_messages_fail[steed_type::ANIMAL],
              to_translation( "You feel bugs crawl over your skin." ) );
    optional( jo, was_loaded, "change_bad_mech", change_messages_fail[steed_type::MECH],
              to_translation( "You feel bugs crawl over your skin." ) );

    mandatory( jo, was_loaded, "move_type", _type, make_flag_reader( move_types, "move type" ) );

    optional( jo, was_loaded, "stamina_multiplier", _stamina_multiplier, 1.0 );
    optional( jo, was_loaded, "sound_multiplier", _sound_multiplier, 1.0 );
    optional( jo, was_loaded, "move_speed_multiplier", _move_speed_mult, 1.0 );
    optional( jo, was_loaded, "mech_power_use", _mech_power_use, 2 );
    optional( jo, was_loaded, "swim_speed_mod", _swim_speed_mod, 0 );

    optional( jo, was_loaded, "stop_hauling", _stop_hauling );
}

void move_mode::reset()
{
    move_mode_factory.reset();
    move_modes_sorted.clear();
}

void move_mode::finalize()
{
    for( const move_mode &mode : move_mode_factory.get_all() ) {
        move_modes_sorted.emplace_back( mode.ident() );
    }

    // Sort it fastest to slowest
    auto stamina_sorter = [&]( const move_mode_id & lhs, const move_mode_id & rhs ) {
        return lhs->move_speed_mult() < rhs->move_speed_mult();
    };

    std::sort( move_modes_sorted.begin(), move_modes_sorted.end(), stamina_sorter );

    // Cycle to the move mode above ours
    for( size_t i = move_modes_sorted.size(); i > 0; --i ) {
        const move_mode &curr = *move_modes_sorted[i - 1];
        if( i == move_modes_sorted.size() ) {
            curr.set_cycle( move_modes_sorted[0] );
        } else {
            curr.set_cycle( move_modes_sorted[i] );
        }
    }

    // Cycle to the move mode below ours
    for( size_t i = move_modes_sorted.size(); i > 0; --i ) {
        const move_mode &curr = *move_modes_sorted[i - 1];
        if( i == 1 ) {
            curr.set_cycle_back( move_modes_sorted.back() );
        } else {
            curr.set_cycle_back( move_modes_sorted[i - 2] );
        }
    }
}

std::string move_mode::name() const
{
    return _name.translated();
}

std::string move_mode::change_message( bool success, steed_type steed ) const
{
    if( steed == steed_type::NUM ) {
        debugmsg( "Attempted to switch to bad movement mode!" );
        //~ This should never occur - this is the message when the character switches to
        //~ an invalid move mode or there's not a message for failing to switch to a move
        //~ mode
        return _( "You feel bugs crawl over your skin." );
    }

    if( success ) {
        return change_messages_success.at( steed ).translated();
    }

    return change_messages_fail.at( steed ).translated();
}

move_mode_id move_mode::cycle() const
{
    return cycle_to;
}

move_mode_id move_mode::cycle_reverse() const
{
    return cycle_back;
}

move_mode_id move_mode::ident() const
{
    return id;
}

float move_mode::sound_mult() const
{
    return _sound_multiplier;
}

float move_mode::stamina_mult() const
{
    return _stamina_multiplier;
}

float move_mode::exertion_level() const
{
    return _exertion_level;
}

float move_mode::move_speed_mult() const
{
    return _move_speed_mult;
}

units::energy move_mode::mech_power_use() const
{
    return units::from_kilojoule( static_cast<std::int64_t>( _mech_power_use ) );
}

int move_mode::swim_speed_mod() const
{
    return _swim_speed_mod;
}

nc_color move_mode::panel_color() const
{
    return _panel_color;
}

nc_color move_mode::symbol_color() const
{
    return _symbol_color;
}

char move_mode::panel_letter() const
{
    return _panel_letter;
}

char move_mode::letter() const
{
    return _letter;
}

bool move_mode::stop_hauling() const
{
    return _stop_hauling;
}

move_mode_type move_mode::type() const
{
    return _type;
}

void move_mode::set_cycle( const move_mode_id &mode ) const
{
    cycle_to = mode;
}

void move_mode::set_cycle_back( const move_mode_id &mode ) const
{
    cycle_back = mode;
}