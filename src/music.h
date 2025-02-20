#pragma once
#include <map>
#include <string>
#include <utility>

template <typename E> struct enum_traits;

namespace music
{
enum music_id {
    mp3,
    instrument,
    sound,
    title,
    LAST
};

extern std::map<music_id, std::pair<bool, bool>> music_id_list;

bool is_active_music_id( music_id data );
music_id get_music_id();
std::string get_music_id_string();
void activate_music_id( music_id data );
void activate_music_id( const std::string &data );
void deactivate_music_id( music_id data );
void deactivate_music_id( const std::string &data );
void deactivate_music_id_all();
void update_music_id_is_empty_flag( music_id data, bool update );
void update_music_id_is_empty_flag( const std::string &data, bool update );
} // namespace music

// Use enum_traits for generic iteration over music_id, and string (de-)serialization.
// Use io::string_to_enum<music_id>( music_id ) to convert a string to music_id.
template<>
struct enum_traits<music::music_id> {
    static constexpr music::music_id last = music::music_id::LAST;
};
