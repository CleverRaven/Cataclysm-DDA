#ifndef CATA_SRC_MAPGEN_POST_PROCESS_GENERATORS_H
#define CATA_SRC_MAPGEN_POST_PROCESS_GENERATORS_H

#include <list>
#include <string>
#include <string_view>

#include "coords_fwd.h"
#include "dialogue_helpers.h"
#include "type_id.h"

class JsonObject;
class map;
template <typename E> struct enum_traits;

enum class map_generator : int {
    bash_damage,
    move_items,
    add_fire,
    pre_burn,
    place_blood,
    afs_ruin,
    last
};

template<>
struct enum_traits<map_generator> {
    static constexpr map_generator last = map_generator::last;
};

struct generator_instance {

    bool was_loaded = false;
    map_generator type;

    int num_attempts;
    dbl_or_var percent_chance;
    int min_intensity;
    int max_intensity;
    void load( const JsonObject &jo, std::string_view );

    void bash_damage( map &md, std::list<tripoint_bub_ms> &all_points_in_map ) const;

    void move_items( map &md, std::list<tripoint_bub_ms> &all_points_in_map ) const;

    void add_fire( map &md, std::list<tripoint_bub_ms> &all_points_in_map ) const;

    void pre_burn( map &md, std::list<tripoint_bub_ms> &all_points_in_map ) const;

    void place_blood( map &md, std::list<tripoint_bub_ms> &all_points_in_map ) const;

    static void aftershock_ruin( map &md, std::list<tripoint_bub_ms> &all_points_in_map,
                                 const tripoint_abs_omt &p );

    int chance() const;

};

class pp_generator
{
    public:
        bool was_loaded = false;
        pp_generator_id id;

        // list of all types stored with this
        std::vector<generator_instance> all_generators;

        static void load_pp_generator( const JsonObject &jo, const std::string &src );

        static void apply_pp_generators( map &md, std::vector<generator_instance> gen,
                                         tripoint_abs_omt omt_point );

        void load( const JsonObject &jo, std::string_view );

        static void reset();

        static void check();
};

#endif // CATA_SRC_MAPGEN_POST_PROCESS_GENERATORS_H
