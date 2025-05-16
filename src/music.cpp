#include "music.h"

#include "debug.h"
#include "enum_conversions.h"

namespace io
{
template<>
std::string enum_to_string<music::music_id>( music::music_id data )
{
    switch( data ) {
        case music::music_id::mp3:
            return "mp3";
        case music::music_id::instrument:
            return "instrument";
        case music::music_id::sound:
            return "sound";
        case music::music_id::title:
            return "title";
        case music::music_id::LAST:
            break;
    }

    cata_fatal( "Invalid music_id in enum_to_string" );
}
}  // namespace io

namespace music
{
std::map<music_id, std::pair<bool, bool>> music_id_list {
    { mp3, { false, false } }, { instrument, { false, false } }, { sound, { false, false } }, {title, { true, true } }
};

bool is_active_music_id( music_id data )
{
    return music_id_list[data].first && music_id_list[data].second;
}

music_id get_music_id()
{
    for( std::pair<const music_id, std::pair<bool, bool>> &pair : music_id_list ) {
        if( pair.second.first && pair.second.second ) {
            return pair.first;
        }
    }

    cata_fatal( "At least one music_id must remain activated in music::get_music_id" );
}

std::string get_music_id_string()
{
    return io::enum_to_string<music_id>( get_music_id() );
}

void activate_music_id( music_id data )
{
    music_id_list[data].second = true;
}

void activate_music_id( const std::string &data )
{
    activate_music_id( io::string_to_enum<music_id>( data ) );
}

void deactivate_music_id( music_id data )
{
    music_id_list[data].second = false;
}

void deactivate_music_id( const std::string &data )
{
    deactivate_music_id( io::string_to_enum<music_id>( data ) );
}

void deactivate_music_id_all()
{
    for( std::pair<const music_id, std::pair<bool, bool>> &pair : music_id_list ) {
        pair.second.second = false;
    }

    activate_music_id( music_id::title );
}

void update_music_id_is_empty_flag( music_id data, bool update )
{
    music_id_list[data].first = update;
}

void update_music_id_is_empty_flag( const std::string &data, bool update )
{
    music_id_list[io::string_to_enum<music_id>( data )].first = update;
}
} // namespace music
