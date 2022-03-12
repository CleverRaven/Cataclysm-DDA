#include <utility>
#include <vector>
#include <string>

namespace music {
    enum music_id {
        mp3,
        instrument,
        sound,
        title
    };

    extern std::vector<std::pair<music_id, bool>> music_id_list;

    std::string enum_to_string( music_id data );
    music_id string_to_enum( std::string data );
    music_id get_music_id();
    std::string get_music_id_string();
    void activate_music_id( music_id data );
    void activate_music_id( std::string data );
    void deactivate_music_id( music_id data );
    void deactivate_music_id( std::string data );
    void deactivate_music_id_all();
}