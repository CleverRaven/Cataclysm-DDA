#include "music.h"

#include "debug.h"

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
    }

    cata_fatal( "Invalid music_id in enum_to_string" );
}
}  // namespace io

namespace music
{
std::map<music_id, bool> music_id_list {
    { mp3, false }, { instrument, false }, { sound, false }, {title, true }
};

bool is_active_music_id( music_id data )
{
    return music_id_list[data];
}

music_id get_music_id()
{
    for( std::pair<const music_id, bool> &pair : music_id_list ) {
        if( pair.second ) {
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
    music_id_list[data] = true;
}

void activate_music_id( std::string data )
{
    activate_music_id( io::string_to_enum<music_id>( data ) );
}

void deactivate_music_id( music_id data )
{
    music_id_list[data] = false;
}

void deactivate_music_id( std::string data )
{
    deactivate_music_id( io::string_to_enum<music_id>( data ) );
}

void deactivate_music_id_all()
{
    for( std::pair<const music_id, bool> &pair : music_id_list ) {
        pair.second = false;
    }

    activate_music_id( music_id::title );
}
} // namespace music
