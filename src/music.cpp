#include "music.h"
#include "debug.h"

namespace music
{

std::vector<std::pair<music_id, bool>> music_id_list { std::make_pair( mp3, false ), std::make_pair( instrument, false ), std::make_pair( sound, false ), std::make_pair( title, true ) };

bool is_active_music_id( music_id data )
{
    for( std::pair<music_id, bool> &pair : music_id_list ) {
        if( pair.first == data ) {
            return pair.second;
        }
    }

    return false;
}

std::string enum_to_string( music_id data )
{
    switch( data ) {
        case mp3:
            return "mp3";
        case instrument:
            return "instrument";
        case sound:
            return "sound";
        case title:
            return "title";
    }

    cata_fatal( "Invalid music_id in music::enum_to_string" );
}

music_id string_to_enum( std::string data )
{
    for( std::pair<music_id, bool> &pair : music_id_list ) {
        if( !data.compare( enum_to_string( pair.first ) ) ) {
            return pair.first;
        }
    }

    cata_fatal( "Invalid string in music::string_to_enum" );
}

music_id get_music_id()
{
    for( std::pair<music_id, bool> &pair : music_id_list ) {
        if( pair.second ) {
            return pair.first;
        }
    }

    cata_fatal( "At least one music_id must remain activated in music::get_music_id" );
}

std::string get_music_id_string()
{
    return enum_to_string( get_music_id() );
}

void activate_music_id( music_id data )
{
    for( std::pair<music_id, bool> &pair : music_id_list ) {
        if( pair.first == data ) {
            pair.second = true;
            return;
        }
    }

    cata_fatal( "Invalid music_id in music::activate_music_id" );
}

void activate_music_id( std::string data )
{
    activate_music_id( string_to_enum( data ) );
}

void deactivate_music_id( music_id data )
{
    for( std::pair<music_id, bool> &pair : music_id_list ) {
        if( pair.first == data ) {
            pair.second = false;
            return;
        }
    }

    cata_fatal( "Invalid music_id in music::deactivate_music_id" );
}

void deactivate_music_id( std::string data )
{
    deactivate_music_id( string_to_enum( data ) );
}

void deactivate_music_id_all()
{
    for( std::pair<music_id, bool> &pair : music_id_list ) {
        pair.second = false;
    }

    activate_music_id( music_id::title );
}

} // namespace music